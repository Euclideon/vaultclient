#include "vcCamera.h"
#include "vcState.h"
#include "vcPOI.h"

#include "imgui.h"
#include "imgui_ex/ImGuizmo.h"

#define ONE_PIXEL_SQ 0.0001

// higher == quicker smoothing
static const double sCameraTranslationSmoothingSpeed = 22.0;
static const double sCameraRotationSmoothingSpeed = 40.0;

udDouble4x4 vcCamera_GetMatrix(vcCamera *pCamera)
{
  udQuaternion<double> orientation = udQuaternion<double>::create(pCamera->eulerRotation);
  udDouble3 lookPos = pCamera->position + orientation.apply(udDouble3::create(0.0, 1.0, 0.0));
  return udDouble4x4::lookAt(pCamera->position, lookPos, orientation.apply(udDouble3::create(0.0, 0.0, 1.0)));
}

void vcCamera_StopSmoothing(vcCameraInput *pCamInput)
{
  pCamInput->smoothTranslation = udDouble3::zero();
  pCamInput->smoothRotation = udDouble3::zero();
  pCamInput->smoothOrthographicChange = 0.0;
}

void vcCamera_UpdateSmoothing(vcState *pProgramState, vcCamera *pCamera, vcCameraInput *pCamInput, vcCameraSettings *pCamSettings, double deltaTime)
{
  static const double minSmoothingThreshold = 0.00001;
  static const double stepAmount = 0.001666667;

  static double stepRemaining = 0.0;
  stepRemaining += deltaTime;
  while (stepRemaining >= stepAmount)
  {
    stepRemaining -= stepAmount;

    // translation
    if (udMagSq3(pCamInput->smoothTranslation) > minSmoothingThreshold)
    {
      udDouble3 step = pCamInput->smoothTranslation * udMin(1.0, stepAmount * sCameraTranslationSmoothingSpeed);
      pCamera->position += step;
      pCamInput->smoothTranslation -= step;
    }
    else
    {
      pCamInput->smoothTranslation = udDouble3::zero();
    }

    // rotation
    if (udMagSq3(pCamInput->smoothRotation) > minSmoothingThreshold)
    {
      udDouble3 step = pCamInput->smoothRotation * udMin(1.0, stepAmount * sCameraRotationSmoothingSpeed);
      pCamera->eulerRotation += step;
      pCamInput->smoothRotation -= step;

      pCamera->eulerRotation.y = udClamp(pCamera->eulerRotation.y, -UD_HALF_PI, UD_HALF_PI);
      pCamera->eulerRotation.x = udMod(pCamera->eulerRotation.x, UD_2PI);
      pCamera->eulerRotation.z = udMod(pCamera->eulerRotation.z, UD_2PI);
    }
    else
    {
      pCamInput->smoothRotation = udDouble3::zero();
    }

    // ortho zoom
    if (udAbs(pCamInput->smoothOrthographicChange) > minSmoothingThreshold)
    {
      double previousOrthoSize = pCamSettings->orthographicSize;

      double step = pCamInput->smoothOrthographicChange * udMin(1.0, stepAmount * sCameraTranslationSmoothingSpeed);
      pCamSettings->orthographicSize = udClamp(pCamSettings->orthographicSize * (1.0 - step), vcSL_CameraOrthoNearFarPlane.x, vcSL_CameraOrthoNearFarPlane.y);
      pCamInput->smoothOrthographicChange -= step;

      udDouble2 towards = pProgramState->worldAnchorPoint.toVector2() - pCamera->position.toVector2();
      if (udMagSq2(towards) > 0)
      {
        towards = (pProgramState->worldAnchorPoint.toVector2() - pCamera->position.toVector2()) / previousOrthoSize;
        pCamera->position += udDouble3::create(towards * -(pCamSettings->orthographicSize - previousOrthoSize), 0.0);
      }
    }
    else
    {
      pCamInput->smoothOrthographicChange = 0.0;
    }
  }
}

void vcCamera_BeginCameraPivotModeMouseBinding(vcState *pProgramState, int bindingIndex)
{
  switch (pProgramState->settings.camera.cameraMouseBindings[bindingIndex])
  {
  case vcCPM_Orbit:
    if (pProgramState->pickingSuccess)
    {
      pProgramState->isUsingAnchorPoint = true;
      pProgramState->worldAnchorPoint = pProgramState->worldMousePosCartesian;
      pProgramState->cameraInput.inputState = vcCIS_Orbiting;
      vcCamera_StopSmoothing(&pProgramState->cameraInput);
    }
    break;
  case vcCPM_Tumble:
    pProgramState->cameraInput.inputState = vcCIS_None;
    break;
  case vcCPM_Pan:
    if (pProgramState->pickingSuccess)
    {
      pProgramState->isUsingAnchorPoint = true;
      pProgramState->worldAnchorPoint = pProgramState->worldMousePosCartesian;
    }
    pProgramState->anchorMouseRay = pProgramState->pCamera->worldMouseRay;
    pProgramState->cameraInput.inputState = vcCIS_Panning;
    break;
  case vcCPM_Forward:
    pProgramState->cameraInput.inputState = vcCIS_MovingForward;
    break;
  default:
    // Do nothing
    break;
  };
}

void vcCamera_UpdateMatrices(vcCamera *pCamera, const vcCameraSettings &settings, vcCameraInput *pCamInput, const udFloat2 &windowSize, const udFloat2 *pMousePos /*= nullptr*/)
{
  // Update matrices
  double fov = settings.fieldOfView;
  double aspect = windowSize.x / windowSize.y;
  double zNear = settings.nearPlane;
  double zFar = settings.farPlane;

  pCamera->matrices.camera = vcCamera_GetMatrix(pCamera);


#if GRAPHICS_API_OPENGL
  pCamera->matrices.projectionNear = udDouble4x4::perspectiveNO(fov, aspect, 0.5f, 10000.f);
#else
  pCamera->matrices.projectionNear = udDouble4x4::perspectiveZO(fov, aspect, 0.5f, 10000.f);
#endif

  udDouble4x4 projectionPerspUD = udDouble4x4::perspectiveZO(fov, aspect, zNear, zFar);
  udDouble4x4 projectionOrthoUD = udDouble4x4::orthoZO(-settings.orthographicSize * aspect, settings.orthographicSize * aspect, -settings.orthographicSize, settings.orthographicSize, vcSL_CameraOrthoNearFarPlane.x, vcSL_CameraOrthoNearFarPlane.y);
#if GRAPHICS_API_OPENGL
  udDouble4x4 projectionPersp = udDouble4x4::perspectiveNO(fov, aspect, zNear, zFar);
  udDouble4x4 projectionOrtho = udDouble4x4::orthoNO(-settings.orthographicSize * aspect, settings.orthographicSize * aspect, -settings.orthographicSize, settings.orthographicSize, vcSL_CameraOrthoNearFarPlane.x, vcSL_CameraOrthoNearFarPlane.y);
#endif

  switch (settings.cameraMode)
  {
  case vcCM_OrthoMap:
    pCamera->matrices.projectionUD = projectionOrthoUD;
#if GRAPHICS_API_OPENGL
    pCamera->matrices.projection = projectionOrtho;
#endif
    break;
  case vcCM_FreeRoam:
    if (pCamInput != nullptr && pCamInput->transitioningToMapMode && pCamInput->progress > 0.15)
    {
      //Switch to ortho projection soon after camera starts rotating downward, hence the progress threshold 0.15.
      pCamera->matrices.projectionUD = projectionOrthoUD;
#if GRAPHICS_API_OPENGL
      pCamera->matrices.projection = projectionOrtho;
#endif
    }
    else
    {
      pCamera->matrices.projectionUD = projectionPerspUD;
#if GRAPHICS_API_OPENGL
      pCamera->matrices.projection = projectionPersp;
#endif
    }
    break;
  default:
    pCamera->matrices.projectionUD = projectionPerspUD;
#if GRAPHICS_API_OPENGL
    pCamera->matrices.projection = projectionPersp;
#endif
  }

#if !GRAPHICS_API_OPENGL
  pCamera->matrices.projection = pCamera->matrices.projectionUD;
#endif

  pCamera->matrices.view = pCamera->matrices.camera;
  pCamera->matrices.view.inverse();

  pCamera->matrices.viewProjection = pCamera->matrices.projection * pCamera->matrices.view;
  pCamera->matrices.inverseViewProjection = udInverse(pCamera->matrices.viewProjection);

  // Calculate the mouse ray
  if (pMousePos != nullptr)
  {
    udDouble2 mousePosClip = udDouble2::create((pMousePos->x / windowSize.x) * 2.0 - 1.0, 1.0 - (pMousePos->y / windowSize.y) * 2.0);

    double nearClipZ = 0.0;
#if GRAPHICS_API_OPENGL
    nearClipZ = -1.0;
#endif
    udDouble4 mouseNear = (pCamera->matrices.inverseViewProjection * udDouble4::create(mousePosClip, nearClipZ, 1.0));
    udDouble4 mouseFar = (pCamera->matrices.inverseViewProjection * udDouble4::create(mousePosClip, 1.0, 1.0));

    mouseNear /= mouseNear.w;
    mouseFar /= mouseFar.w;

    pCamera->worldMouseRay.position = mouseNear.toVector3();
    pCamera->worldMouseRay.direction = udNormalize3(mouseFar - mouseNear).toVector3();
  }
}

void vcCamera_Create(vcCamera **ppCamera)
{
  if (ppCamera == nullptr)
    return;

  vcCamera *pCamera = udAllocType(vcCamera, 1, udAF_Zero);

  *ppCamera = pCamera;
}

void vcCamera_Destroy(vcCamera **ppCamera)
{
  if (ppCamera != nullptr)
    udFree(*ppCamera);
}

void vcCamera_Apply(vcState *pProgramState, vcCamera *pCamera, vcCameraSettings *pCamSettings, vcCameraInput *pCamInput, double deltaTime, float speedModifier /* = 1.f*/)
{
  switch (pCamInput->inputState)
  {
  case vcCIS_MovingForward:
    pCamInput->keyboardInput.y += 1;
    // fall through
  case vcCIS_None:
  {
    udDouble3 addPos = udClamp(pCamInput->keyboardInput, udDouble3::create(-1, -1, -1), udDouble3::create(1, 1, 1)); // clamp in case 2 similarly mapped movement buttons are pressed
    double vertPos = addPos.z;
    addPos.z = 0.0;

    // Translation
    if (pCamSettings->cameraMode == vcCM_FreeRoam)
    {
      if (pCamSettings->moveMode == vcCMM_Plane)
      {
        addPos = (udDouble4x4::rotationYPR(pCamera->eulerRotation) * udDouble4::create(addPos, 1)).toVector3();
      }
      else if (pCamSettings->moveMode == vcCMM_Helicopter)
      {
        addPos = (udDouble4x4::rotationYPR(udDouble3::create(pCamera->eulerRotation.x, 0.0, 0.0)) * udDouble4::create(addPos, 1)).toVector3();
        addPos.z = 0.0; // might be unnecessary now
        if (addPos.x != 0.0 || addPos.y != 0.0)
          addPos = udNormalize3(addPos);
      }
    }
    else // map mode
    {
      if (vertPos != 0)
      {
        pCamInput->smoothOrthographicChange -= 0.005 * vertPos * pCamSettings->moveSpeed * speedModifier * deltaTime;
        pProgramState->worldAnchorPoint = pCamera->position; // stops translation occuring
      }
    }

    // Panning - DPAD
    pCamInput->controllerDPADInput = (udDouble4x4::rotationYPR(pCamera->eulerRotation) * udDouble4::create(pCamInput->controllerDPADInput, 1)).toVector3();

    if (pCamSettings->cameraMode == vcCM_OrthoMap || pCamSettings->moveMode == vcCMM_Helicopter)
      pCamInput->controllerDPADInput.z = 0.0;

    addPos += pCamInput->controllerDPADInput;

    addPos.z += vertPos;
    addPos *= pCamSettings->moveSpeed * speedModifier * deltaTime;
    pCamInput->smoothTranslation += addPos;

    // Check for a nan camera position and reset to zero, this allows the UI to be usable in the event of error
    if (isnan(pCamera->position.x) || isnan(pCamera->position.y) || isnan(pCamera->position.z))
      pCamera->position = udDouble3::zero();

    pCamInput->smoothRotation += pCamInput->mouseInput * 0.5;
  }
  break;

  case vcCIS_Orbiting:
  {
    double distanceToPointSqr = udMagSq3(pProgramState->worldAnchorPoint - pCamera->position);
    if (distanceToPointSqr != 0.0 && (pCamInput->mouseInput.x != 0 || pCamInput->mouseInput.y != 0))
    {
      udRay<double> transform, tempTransform;
      transform.position = pCamera->position;
      transform.direction = udDirectionFromYPR(pCamera->eulerRotation);

      // Apply input
      tempTransform = udRay<double>::rotationAround(transform, pProgramState->worldAnchorPoint, { 0, 0, 1 }, pCamInput->mouseInput.x);
      transform = udRay<double>::rotationAround(tempTransform, pProgramState->worldAnchorPoint, udDoubleQuat::create(udDirectionToYPR(tempTransform.direction)).apply({ 1, 0, 0 }), pCamInput->mouseInput.y);

      // Prevent flipping
      if ((transform.direction.x > 0 && tempTransform.direction.x < 0) || (transform.direction.x < 0 && tempTransform.direction.x > 0))
        transform = tempTransform;

      udDouble3 euler = udDirectionToYPR(transform.direction);

      // Handle special case where ATan2 is ambiguous
      if (pCamera->eulerRotation.y == -UD_HALF_PI)
        euler.x += UD_PI;

      // Apply transform
      pCamera->position = transform.position;
      pCamera->eulerRotation = euler;
      pCamera->eulerRotation.z = 0;
    }
  }
  break;

  case vcCIS_LookingAtPoint:
  {
    udDouble3 targetEuler = udDirectionToYPR(pCamInput->lookAtPosition - pCamInput->startPosition);
    if (isnan(targetEuler.x) || isnan(targetEuler.y) || isnan(targetEuler.z))
      targetEuler = udDouble3::zero();

    pCamInput->progress += deltaTime; // 1 second
    if (pCamInput->progress >= 1.0)
    {
      pCamInput->progress = 1.0;
      pCamInput->inputState = vcCIS_None;
    }

    double progress = udEase(pCamInput->progress, udET_QuadraticOut);
    pCamera->eulerRotation = udSlerp(pCamInput->startAngle, udDoubleQuat::create(targetEuler), progress).eulerAngles();

    if (pCamera->eulerRotation.y > UD_PI)
      pCamera->eulerRotation.y -= UD_2PI;
  }
  break;

  case vcCIS_FlyThrough:
  {
    vcLineInfo *pLine = (vcLineInfo*)pCamInput->pObjectInfo;

    // If important things have been deleted cancel the flythrough
    if (pLine == nullptr || pLine->pPoints == nullptr || pLine->numPoints <= 1 || pCamInput->flyThroughPoint >= pLine->numPoints)
    {
      pCamInput->flyThroughPoint = 0;
      pCamInput->inputState = vcCIS_None;
      pCamInput->pObjectInfo = nullptr;
      break;
    }

    // Move to first point on first loop
    if (pCamInput->flyThroughPoint == -1)
    {
      pCamera->position = pLine->pPoints[0];
      pCamera->eulerRotation = udDirectionToYPR(pLine->pPoints[1] - pLine->pPoints[0]);
      pCamInput->startPosition = pCamera->position;
      pCamInput->progress = 1.0;
    }

    double remainingMovementThisFrame = pCamSettings->moveSpeed * speedModifier * deltaTime;
    udDoubleQuat startQuat = udDoubleQuat::create(pCamera->eulerRotation);

    while (remainingMovementThisFrame > 0.01)
    {
      // If target point is reached
      if (pCamInput->progress == 1.0)
      {
        pCamInput->progress = 0.0;
        pCamInput->flyThroughPoint++;
        pCamInput->startPosition = pCamera->position;

        if (pCamInput->flyThroughPoint >= pLine->numPoints)
        {
          if (pLine->closed)
          {
            pCamInput->flyThroughPoint = 0;
          }
          else
          {
            pCamInput->flyThroughPoint = -1;
            break;
          }
        }
      }

      udDouble3 moveVector = pLine->pPoints[pCamInput->flyThroughPoint] - pCamInput->startPosition;

      // If consecutive points are in the same position (avoids divide by zero)
      if (moveVector == udDouble3::zero())
      {
        pCamInput->progress = 1.0;
      }
      else
      {
        pCamInput->progress = udMin(pCamInput->progress + remainingMovementThisFrame / udMag3(moveVector), 1.0);
        udDouble3 leadingPoint = pCamInput->startPosition + moveVector * pCamInput->progress;
        udDouble3 cam2Point = leadingPoint - pCamera->position;
        double distCam2Point = udMag3(cam2Point);

        if (distCam2Point != 0)
        {
          // avoids divide by zero
          cam2Point = udNormalize3(cam2Point);

          // Smoothly rotate camera to face the leading point at all times
          udDouble3 targetEuler = udDirectionToYPR(cam2Point);
          pCamera->eulerRotation = udSlerp(startQuat, udDoubleQuat::create(targetEuler), 0.2).eulerAngles();
          if (pCamera->eulerRotation.y > UD_PI)
            pCamera->eulerRotation.y -= UD_2PI;

          if (pCamSettings->moveMode == vcCMM_Helicopter)
            cam2Point.z = 0;

          pCamera->position += cam2Point * remainingMovementThisFrame;
          remainingMovementThisFrame -= distCam2Point; // This should be calculated
        }
      }
    }
  }
  break;

  case vcCIS_MovingToPoint:
  {
    udDouble3 moveVector = pProgramState->worldAnchorPoint - pCamInput->startPosition;

    if (moveVector == udDouble3::zero())
      break;

    if (pCamSettings->moveMode == vcCMM_Helicopter || pCamSettings->cameraMode == vcCM_OrthoMap)
      moveVector.z = 0;

    double length = udMag3(moveVector);
    double closest = udMax(0.9, (length - 100.0) / length); // gets to either 90% or within 100m

    moveVector *= closest;

    pCamInput->progress += deltaTime / 2; // 2 second travel time
    if (pCamInput->progress > 1.0)
    {
      pCamInput->progress = 1.0;
      pCamInput->inputState = vcCIS_None;
    }

    double travelProgress = udEase(pCamInput->progress, udET_CubicInOut);
    pCamera->position = pCamInput->startPosition + moveVector * travelProgress;

    udDouble3 targetEuler = udDirectionToYPR(pProgramState->worldAnchorPoint - (pCamInput->startPosition + moveVector * closest));
    pCamera->eulerRotation = udSlerp(pCamInput->startAngle, udDoubleQuat::create(targetEuler), travelProgress).eulerAngles();

    if (pCamera->eulerRotation.y > UD_PI)
      pCamera->eulerRotation.y -= UD_2PI;
  }
  break;

  case vcCIS_CommandZooming:
  {
    udDouble3 addPos = udDouble3::zero();
    if (pCamSettings->cameraMode == vcCM_FreeRoam)
    {
      udDouble3 towards = pProgramState->worldAnchorPoint - pCamera->position;
      if (udMagSq3(towards) > 0)
      {
        double maxDistance = 0.9 * pCamSettings->farPlane; // limit to 90% of visible distance
        double distanceToPoint = udMin(udMag3(towards), maxDistance);
        
        addPos = distanceToPoint * pCamInput->mouseInput.y * udNormalize3(towards);
      }
    }
    else // map mode
    {
      pCamInput->smoothOrthographicChange += pCamInput->mouseInput.y;
    }

    pCamInput->smoothTranslation += addPos;
  }
  break;

  case vcCIS_PinchZooming:
  {
    // TODO
  }
  break;

  case vcCIS_Panning:
  {
    udPlane<double> plane = udPlane<double>::create(pProgramState->worldAnchorPoint, { 0, 0, 1 });

    if (pCamSettings->cameraMode == vcCM_OrthoMap)
      plane.point.z = 0;
    else if (pCamSettings->moveMode == vcCMM_Plane)
      plane.normal = udDoubleQuat::create(pCamera->eulerRotation).apply({ 0, 1, 0 });

    udDouble3 offset = udDouble3::create(0, 0, 0);
    udDouble3 anchorOffset = udDouble3::create(0, 0, 0);
    if (plane.intersects(pCamera->worldMouseRay, &offset, nullptr) && plane.intersects(pProgramState->anchorMouseRay, &anchorOffset, nullptr))
      pCamInput->smoothTranslation = (anchorOffset - offset);
  }
  break;

  case vcCIS_Count:
    break; // to cover all implemented cases
  }

  if (pCamInput->stabilize)
  {
    if (pCamera->eulerRotation.z > UD_PI)
      pCamera->eulerRotation.z -= UD_2PI;

    pCamInput->progress += deltaTime * 2; // .5 second stabilize time
    if (pCamInput->progress >= 1.0)
    {
      pCamInput->progress = 1.0;
      pCamInput->stabilize = false;
    }

    double travelProgress = udEase(pCamInput->progress, udET_CubicOut);

    pCamera->eulerRotation.z = udLerp(pCamera->eulerRotation.z, 0.0, travelProgress);

    if (pCamera->eulerRotation.y > UD_PI)
      pCamera->eulerRotation.y -= UD_2PI;
  }

  vcCamera_UpdateSmoothing(pProgramState, pCamera, pCamInput, pCamSettings, deltaTime);

  if (pCamInput->inputState == vcCIS_None && pCamInput->transitioningToMapMode)
  {
    pCamInput->transitioningToMapMode = false;
    pCamSettings->cameraMode = vcCM_OrthoMap; // actually swap now
  }

  // in orthographic mode, force camera straight down
  if (pCamSettings->cameraMode == vcCM_OrthoMap)
    pCamera->eulerRotation = udDouble3::create(0.0, -UD_HALF_PI, 0.0); // down orientation
}

void vcCamera_SwapMapMode(vcState *pProgramState)
{
  udDouble3 lookAtPosition = pProgramState->pCamera->position;
  double cameraHeight = pProgramState->pCamera->position.z;
  if (pProgramState->settings.camera.cameraMode == vcCM_FreeRoam)
  {
    pProgramState->settings.camera.orthographicSize = udMax(1.0, pProgramState->pCamera->position.z / vcCamera_HeightToOrthoFOVRatios[pProgramState->settings.camera.lensIndex]);

    // defer actually swapping projection mode
    pProgramState->cameraInput.transitioningToMapMode = true;
    lookAtPosition += udDouble3::create(0, 0, -1); // up
  }
  else
  {
    cameraHeight = pProgramState->settings.camera.orthographicSize * vcCamera_HeightToOrthoFOVRatios[pProgramState->settings.camera.lensIndex];
    pProgramState->settings.camera.cameraMode = vcCM_FreeRoam;
    pProgramState->cameraInput.transitioningToMapMode = false;

    lookAtPosition += udDouble3::create(0, 1, 0); // forward

    // also adjust the far plane (so things won't disappear if the view plane isn't configured correctly)
    pProgramState->settings.camera.farPlane = udMax(pProgramState->settings.camera.farPlane, float(pProgramState->settings.camera.orthographicSize * 2.0));
    pProgramState->settings.camera.nearPlane = pProgramState->settings.camera.farPlane * vcSL_CameraFarToNearPlaneRatio;
  }

  vcCamera_LookAt(pProgramState, lookAtPosition);

  pProgramState->pCamera->position.z = cameraHeight;
}


void vcCamera_LookAt(vcState *pProgramState, const udDouble3 &targetPosition)
{
  if (udMagSq3(targetPosition - pProgramState->pCamera->position) == 0.0)
    return;

  pProgramState->cameraInput.inputState = vcCIS_LookingAtPoint;
  pProgramState->cameraInput.startPosition = pProgramState->pCamera->position;
  pProgramState->cameraInput.startAngle = udDoubleQuat::create(pProgramState->pCamera->eulerRotation);
  pProgramState->cameraInput.lookAtPosition = targetPosition;
  pProgramState->cameraInput.progress = 0.0;
}

void vcCamera_HandleSceneInput(vcState *pProgramState, udDouble3 oscMove, udFloat2 windowSize, udFloat2 mousePos)
{
  ImGuiIO &io = ImGui::GetIO();

  udDouble3 keyboardInput = udDouble3::zero();
  udDouble3 mouseInput = udDouble3::zero();

  // bring in values
  keyboardInput += oscMove;

  float speedModifier = 1.f;

  ImVec2 mouseDelta = io.MouseDelta;
  float mouseWheel = io.MouseWheel;

  static bool isMouseBtnBeingHeld = false;
  static bool isRightTriggerHeld = false;
  static bool gizmoCapturedMouse = false;

  bool isBtnClicked[3] = { ImGui::IsMouseClicked(0, false), ImGui::IsMouseClicked(1, false), ImGui::IsMouseClicked(2, false) };
  bool isBtnDoubleClicked[3] = { ImGui::IsMouseDoubleClicked(0), ImGui::IsMouseDoubleClicked(1), ImGui::IsMouseDoubleClicked(2) };
  bool isBtnHeld[3] = { ImGui::IsMouseDown(0), ImGui::IsMouseDown(1), ImGui::IsMouseDown(2) };
  bool isBtnReleased[3] = { ImGui::IsMouseReleased(0), ImGui::IsMouseReleased(1), ImGui::IsMouseReleased(2) };

  isMouseBtnBeingHeld &= (isBtnHeld[0] || isBtnHeld[1] || isBtnHeld[2]);
  bool isFocused = (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_ChildWindows) || isMouseBtnBeingHeld) && !vcGizmo_IsActive() && !pProgramState->modalOpen;

  int totalButtonsHeld = 0;
  for (size_t i = 0; i < udLengthOf(isBtnHeld); ++i)
    totalButtonsHeld += isBtnHeld[i] ? 1 : 0;

  // Start hold time
  if (isFocused && (isBtnClicked[0] || isBtnClicked[1] || isBtnClicked[2]))
    isMouseBtnBeingHeld = true;

  bool forceClearMouseState = !isFocused;

  // Was the gizmo just clicked on?
  gizmoCapturedMouse = gizmoCapturedMouse || (pProgramState->gizmo.operation != 0 && vcGizmo_IsHovered() && (isBtnClicked[0] || isBtnClicked[1] || isBtnClicked[2]));
  if (gizmoCapturedMouse)
  {
    // was the gizmo just released?
    gizmoCapturedMouse = isBtnHeld[0] || isBtnHeld[1] || isBtnHeld[2];
    forceClearMouseState = (forceClearMouseState || gizmoCapturedMouse);
  }

  if (forceClearMouseState)
  {
    memset(isBtnClicked, 0, sizeof(isBtnClicked));
    memset(isBtnDoubleClicked, 0, sizeof(isBtnDoubleClicked));
    memset(isBtnHeld, 0, sizeof(isBtnHeld));
    mouseDelta = ImVec2();
    mouseWheel = 0.0f;
    isMouseBtnBeingHeld = false;
    // Leaving isBtnReleased unchanged as there should be no reason to ignore a mouse release while the window has mouse focus
  }

  // Accept mouse input
  if (isMouseBtnBeingHeld)
  {
    mouseInput.x = -mouseDelta.x / 100.0;
    mouseInput.y = -mouseDelta.y / 100.0;
    mouseInput.z = 0.0;
  }

  if (pProgramState->settings.camera.invertMouseX)
    mouseInput.x *= -1.0;
  if (pProgramState->settings.camera.invertMouseY)
    mouseInput.y *= -1.0;

  // Controller Input
  if (io.NavActive)
  {
    keyboardInput.x += io.NavInputs[ImGuiNavInput_LStickLeft]; // Left Stick Horizontal
    keyboardInput.y += -io.NavInputs[ImGuiNavInput_LStickUp]; // Left Stick Vertical
    mouseInput.x += -io.NavInputs[ImGuiNavInput_LStickRight] / 15.0f; // Right Stick Horizontal
    mouseInput.y += -io.NavInputs[ImGuiNavInput_LStickDown] / 25.0f; // Right Stick Vertical

    if (pProgramState->settings.camera.invertControllerX)
      mouseInput.x = -mouseInput.x;
    if (pProgramState->settings.camera.invertControllerY)
      mouseInput.y = -mouseInput.y;

    // In Imgui the DPAD is bound to navigation, so disable DPAD panning until the issue is resolved
    //pProgramState->cameraInput.controllerDPADInput = udDouble3::create(io.NavInputs[ImGuiNavInput_DpadRight] - io.NavInputs[ImGuiNavInput_DpadLeft], 0, io.NavInputs[ImGuiNavInput_DpadUp] - io.NavInputs[ImGuiNavInput_DpadDown]);

    if (isRightTriggerHeld)
    {
      if (pProgramState->pickingSuccess && pProgramState->cameraInput.inputState == vcCIS_None)
      {
        pProgramState->isUsingAnchorPoint = true;
        pProgramState->worldAnchorPoint = pProgramState->worldMousePosCartesian;
        pProgramState->cameraInput.inputState = pProgramState->settings.camera.cameraMode == vcCM_OrthoMap ? vcCIS_Panning : vcCIS_Orbiting;
        vcCamera_StopSmoothing(&pProgramState->cameraInput);
      }
      if (io.NavInputs[ImGuiNavInput_FocusNext] < 0.85f) // Right Trigger
      {
        pProgramState->cameraInput.inputState = vcCIS_None;
        isRightTriggerHeld = false;
      }
    }
    else if (io.NavInputs[ImGuiNavInput_FocusNext] > 0.85f) // Right Trigger
    {
      isRightTriggerHeld = true;
    }
    if (io.NavInputs[ImGuiNavInput_Input] && !io.NavInputsDownDuration[ImGuiNavInput_Input]) // Y Button
      vcCamera_SwapMapMode(pProgramState);
    if (io.NavInputs[ImGuiNavInput_Activate] && !io.NavInputsDownDuration[ImGuiNavInput_Activate]) // A Button
      pProgramState->settings.camera.moveMode = ((pProgramState->settings.camera.moveMode == vcCMM_Helicopter) ? vcCMM_Plane : vcCMM_Helicopter);
  }

  if (io.KeyCtrl)
    speedModifier *= 0.1f;

  if (io.KeyShift || io.NavInputs[ImGuiNavInput_FocusPrev] > 0.15f) // Left Trigger
    speedModifier *= 10.f;

  if ((!ImGui::GetIO().WantCaptureKeyboard || isFocused) && !pProgramState->modalOpen)
  {
    keyboardInput.y += io.KeysDown[SDL_SCANCODE_W] - io.KeysDown[SDL_SCANCODE_S];
    keyboardInput.x += io.KeysDown[SDL_SCANCODE_D] - io.KeysDown[SDL_SCANCODE_A];
    keyboardInput.z += io.KeysDown[SDL_SCANCODE_R] - io.KeysDown[SDL_SCANCODE_F];

    if (ImGui::IsKeyPressed(SDL_SCANCODE_SPACE, false))
      pProgramState->settings.camera.moveMode = ((pProgramState->settings.camera.moveMode == vcCMM_Helicopter) ? vcCMM_Plane : vcCMM_Helicopter);
    if (ImGui::IsKeyPressed(SDL_SCANCODE_B, false))
      pProgramState->gizmo.operation = pProgramState->gizmo.operation == vcGO_Translate ? vcGO_NoGizmo : vcGO_Translate;
    if (ImGui::IsKeyPressed(SDL_SCANCODE_N, false))
      pProgramState->gizmo.operation = pProgramState->gizmo.operation == vcGO_Rotate ? vcGO_NoGizmo : vcGO_Rotate;
    if (!io.KeyCtrl && ImGui::IsKeyPressed(SDL_SCANCODE_M, false))
      pProgramState->gizmo.operation = pProgramState->gizmo.operation == vcGO_Scale ? vcGO_NoGizmo : vcGO_Scale;
    if (ImGui::IsKeyPressed(SDL_SCANCODE_L, false))
      pProgramState->gizmo.coordinateSystem = (pProgramState->gizmo.coordinateSystem == vcGCS_Scene) ? vcGCS_Local : vcGCS_Scene;

  }

  if (isFocused)
  {
    if (keyboardInput != udDouble3::zero() || isBtnClicked[0] || isBtnClicked[1] || isBtnClicked[2]) // if input is detected, TODO: add proper any input detection
    {
      if (pProgramState->cameraInput.inputState == vcCIS_MovingToPoint || pProgramState->cameraInput.inputState == vcCIS_LookingAtPoint || pProgramState->cameraInput.inputState == vcCIS_FlyThrough)
      {
        pProgramState->cameraInput.stabilize = true;
        pProgramState->cameraInput.progress = 0;
        pProgramState->cameraInput.inputState = vcCIS_None;

        if (pProgramState->cameraInput.transitioningToMapMode)
        {
          pProgramState->cameraInput.transitioningToMapMode = false;
          pProgramState->settings.camera.cameraMode = vcCM_OrthoMap; // swap immediately
        }
      }
    }
  }

  for (int i = 0; i < 3; ++i)
  {
    // Single Clicking
    if (isBtnClicked[i] && (pProgramState->cameraInput.inputState == vcCIS_None || totalButtonsHeld == 1)) // immediately override current input if this is a new button down
    {
      if (pProgramState->settings.camera.cameraMode == vcCM_FreeRoam)
      {
        vcCamera_BeginCameraPivotModeMouseBinding(pProgramState, i);
      }
      else
      {
        // orthographic always only pans
        pProgramState->isUsingAnchorPoint = true;
        pProgramState->worldAnchorPoint = pProgramState->pCamera->worldMouseRay.position;
        pProgramState->anchorMouseRay = pProgramState->pCamera->worldMouseRay;

        pProgramState->cameraInput.inputState = vcCIS_Panning;
      }
    }

    if (isBtnReleased[i])
    {
      if (pProgramState->settings.camera.cameraMode == vcCM_FreeRoam)
      {
        if ((pProgramState->settings.camera.cameraMouseBindings[i] == vcCPM_Orbit && pProgramState->cameraInput.inputState == vcCIS_Orbiting) ||
            (pProgramState->settings.camera.cameraMouseBindings[i] == vcCPM_Pan && pProgramState->cameraInput.inputState == vcCIS_Panning) ||
            (pProgramState->settings.camera.cameraMouseBindings[i] == vcCPM_Forward && pProgramState->cameraInput.inputState == vcCIS_MovingForward) ||
             pProgramState->cameraInput.inputState == vcCIS_CommandZooming)
        {
          pProgramState->cameraInput.inputState = vcCIS_None;

          // Should another mouse action take over? (it's being held down)
          for (int j = 0; j < 3; ++j)
          {
            if (isBtnHeld[j])
            {
              vcCamera_BeginCameraPivotModeMouseBinding(pProgramState, j);
              break;
            }
          }
        }
      }
      else if (pProgramState->cameraInput.progress > 0 || (pProgramState->cameraInput.inputState != vcCIS_CommandZooming && pProgramState->cameraInput.inputState != vcCIS_MovingToPoint && pProgramState->cameraInput.inputState != vcCIS_FlyThrough)) // map mode
      {
        if (!isBtnHeld[0] && !isBtnHeld[1] && !isBtnHeld[2]) // nothing is pressed (remember, they're all mapped to panning)
        {
          pProgramState->cameraInput.inputState = vcCIS_None;
        }
        else if (pProgramState->cameraInput.inputState != vcCIS_Panning) // if not panning, begin (e.g. was zooming with double mouse)
        {
          // theres still a button being held, start panning
          pProgramState->isUsingAnchorPoint = true;
          pProgramState->worldAnchorPoint = pProgramState->pCamera->worldMouseRay.position;
          pProgramState->anchorMouseRay = pProgramState->pCamera->worldMouseRay;
          pProgramState->cameraInput.inputState = vcCIS_Panning;
        }
      }
    }
  }

  // Double Clicking left mouse
  if (isBtnDoubleClicked[0] && (pProgramState->pickingSuccess || pProgramState->settings.camera.cameraMode == vcCM_OrthoMap))
  {
    pProgramState->cameraInput.inputState = vcCIS_MovingToPoint;
    pProgramState->cameraInput.startPosition = pProgramState->pCamera->position;
    pProgramState->cameraInput.startAngle = udDoubleQuat::create(pProgramState->pCamera->eulerRotation);
    pProgramState->cameraInput.progress = 0.0;

    pProgramState->isUsingAnchorPoint = true;
    pProgramState->worldAnchorPoint = pProgramState->worldMousePosCartesian;

    if (pProgramState->settings.camera.cameraMode == vcCM_OrthoMap)
    {
      pProgramState->cameraInput.startAngle = udDoubleQuat::identity();
      pProgramState->worldAnchorPoint = pProgramState->pCamera->worldMouseRay.position;
    }
  }

  // Mouse Wheel
  const double defaultTimeouts[vcCM_Count] = { 0.25, 0.0 };
  double timeout = defaultTimeouts[pProgramState->settings.camera.cameraMode]; // How long you have to stop scrolling the scroll wheel before the point unlocks
  static double previousLockTime = 0.0;
  double currentTime = ImGui::GetTime();
  bool zooming = false;

  if (mouseWheel != 0)
  {
    zooming = true;
    if (pProgramState->settings.camera.scrollWheelMode == vcCSWM_Dolly)
    {
      if (previousLockTime < currentTime - timeout && (pProgramState->pickingSuccess || pProgramState->settings.camera.cameraMode == vcCM_OrthoMap) && pProgramState->cameraInput.inputState == vcCIS_None)
      {
        pProgramState->isUsingAnchorPoint = true;
        pProgramState->worldAnchorPoint = pProgramState->worldMousePosCartesian;
        pProgramState->cameraInput.inputState = vcCIS_CommandZooming;

        if (pProgramState->settings.camera.cameraMode == vcCM_OrthoMap)
          pProgramState->worldAnchorPoint = pProgramState->pCamera->worldMouseRay.position;
      }

      if (pProgramState->cameraInput.inputState == vcCIS_CommandZooming)
      {
        mouseInput.x = 0.0;
        mouseInput.y = mouseWheel / 10.f;
        mouseInput.z = 0.0;
        previousLockTime = currentTime;

        pProgramState->cameraInput.startPosition = pProgramState->pCamera->position;
      }
    }
    else
    {
      if (mouseWheel > 0)
        pProgramState->settings.camera.moveSpeed *= (1.f + mouseWheel / 10.f);
      else
        pProgramState->settings.camera.moveSpeed /= (1.f - mouseWheel / 10.f);

      pProgramState->settings.camera.moveSpeed = udClamp(pProgramState->settings.camera.moveSpeed, vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed);
    }
  }

  if (!zooming && pProgramState->cameraInput.inputState == vcCIS_CommandZooming && previousLockTime < currentTime - timeout)
  {
    pProgramState->cameraInput.inputState = vcCIS_None;
  }

  // Apply movement and rotation
  pProgramState->cameraInput.keyboardInput = keyboardInput;
  pProgramState->cameraInput.mouseInput = mouseInput;

  vcCamera_Apply(pProgramState, pProgramState->pCamera, &pProgramState->settings.camera, &pProgramState->cameraInput, pProgramState->deltaTime, speedModifier);

  if (pProgramState->cameraInput.inputState == vcCIS_None && pProgramState->cameraInput.smoothOrthographicChange == 0.0)
    pProgramState->isUsingAnchorPoint = false;

  vcCamera_UpdateMatrices(pProgramState->pCamera, pProgramState->settings.camera, &pProgramState->cameraInput, windowSize, &mousePos);
}
