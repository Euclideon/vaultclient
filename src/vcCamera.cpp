#include "vcCamera.h"
#include "vcState.h"

#include "imgui.h"
#include "imgui_ex/ImGuizmo.h"

#define ONE_PIXEL_SQ 0.0001
#define LENSNAME(x) #x+5

const char *lensNameArray[] =
{
  LENSNAME(vcLS_Custom),
  LENSNAME(vcLS_15mm),
  LENSNAME(vcLS_24mm),
  LENSNAME(vcLS_30mm),
  LENSNAME(vcLS_50mm),
  LENSNAME(vcLS_70mm),
  LENSNAME(vcLS_100mm),
};

UDCOMPILEASSERT(UDARRAYSIZE(lensNameArray) == vcLS_TotalLenses, "Lens Name not in Strings");

static const udDouble2 sOrthoNearFarPlane = { 1.0, 1000000.0 };

const char** vcCamera_GetLensNames()
{
  return lensNameArray;
}

udDouble4x4 vcCamera_GetMatrix(vcCamera *pCamera)
{
  udQuaternion<double> orientation = udQuaternion<double>::create(pCamera->eulerRotation);
  udDouble3 lookPos = pCamera->position + orientation.apply(udDouble3::create(0.0, 1.0, 0.0));
  return udDouble4x4::lookAt(pCamera->position, lookPos, orientation.apply(udDouble3::create(0.0, 0.0, 1.0)));
}

void vcCamera_UpdateMatrices(vcCamera *pCamera, const vcCameraSettings &settings, const udFloat2 &windowSize, const udFloat2 *pMousePos = nullptr)
{
  // Update matrices
  double fov = settings.fieldOfView;
  double aspect = windowSize.x / windowSize.y;
  double zNear = settings.nearPlane;
  double zFar = settings.farPlane;

  pCamera->matrices.camera = vcCamera_GetMatrix(pCamera);

#if defined(GRAPHICS_API_D3D11)
  pCamera->matrices.projectionNear = udDouble4x4::perspectiveZO(fov, aspect, 0.5f, 10000.f);
#elif defined(GRAPHICS_API_OPENGL)
  pCamera->matrices.projectionNear = udDouble4x4::perspectiveNO(fov, aspect, 0.5f, 10000.f);
#endif

  switch (settings.cameraMode)
  {
  case vcCM_OrthoMap:
    pCamera->matrices.projectionUD = udDouble4x4::orthoZO(-settings.orthographicSize * aspect, settings.orthographicSize * aspect, -settings.orthographicSize, settings.orthographicSize, sOrthoNearFarPlane.x, sOrthoNearFarPlane.y);
#if defined(GRAPHICS_API_OPENGL)
    pCamera->matrices.projection = udDouble4x4::orthoNO(-settings.orthographicSize * aspect, settings.orthographicSize * aspect, -settings.orthographicSize, settings.orthographicSize, sOrthoNearFarPlane.x, sOrthoNearFarPlane.y);
#endif
    break;
  case vcCM_FreeRoam: // fall through
  default:
    pCamera->matrices.projectionUD = udDouble4x4::perspectiveZO(fov, aspect, zNear, zFar);
#if defined(GRAPHICS_API_OPENGL)
    pCamera->matrices.projection = udDouble4x4::perspectiveNO(fov, aspect, zNear, zFar);
#endif
  }

#if defined(GRAPHICS_API_D3D11)
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
#if defined(GRAPHICS_API_OPENGL)
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

void vcCamera_Apply(vcCamera *pCamera, vcCameraSettings *pCamSettings, vcCameraInput *pCamInput, double deltaTime, float speedModifier /* = 1.f*/)
{
  switch (pCamInput->inputState)
  {
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

    addPos.z += vertPos;
    addPos *= pCamSettings->moveSpeed * speedModifier * deltaTime;

    pCamera->position += addPos;

    // Check for a nan camera position and reset to zero, this allows the UI to be usable in the event of error
    if (isnan(pCamera->position.x) || isnan(pCamera->position.y) || isnan(pCamera->position.z))
      pCamera->position = udDouble3::zero();

    // Rotation
    if (pCamSettings->invertX)
      pCamInput->mouseInput.x *= -1.0;
    if (pCamSettings->invertY)
      pCamInput->mouseInput.y *= -1.0;

    pCamera->eulerRotation += pCamInput->mouseInput;
    pCamera->eulerRotation.y = udClamp(pCamera->eulerRotation.y, -UD_HALF_PI, UD_HALF_PI);
    pCamera->eulerRotation.x = udMod(pCamera->eulerRotation.x, UD_2PI);
    pCamera->eulerRotation.z = udMod(pCamera->eulerRotation.z, UD_2PI);
  }
  break;

  case vcCIS_Orbiting:
  {
    double distanceToPoint = udMag3(pCamInput->worldAnchorPoint - pCamera->position);

    if (distanceToPoint != 0.0)
    {
      udRay<double> transform;
      transform.position = pCamera->position;
      transform.direction = udMath_DirFromYPR(pCamera->eulerRotation);

      // Rotation
      if (pCamSettings->invertX)
        pCamInput->mouseInput.x *= -1.0;
      if (pCamSettings->invertY)
        pCamInput->mouseInput.y *= -1.0;

      transform = udRotateAround(transform, pCamInput->worldAnchorPoint, { 0, 0, 1 }, pCamInput->mouseInput.x);
      udRay<double> temp = transform;

      transform = udRotateAround(transform, pCamInput->worldAnchorPoint, udDoubleQuat::create(udMath_DirToYPR(transform.direction)).apply({ 1, 0, 0 }), pCamInput->mouseInput.y);
      if ((transform.direction.y > 0 && temp.direction.y < 0) || (transform.direction.y < 0 && temp.direction.y > 0) || (transform.direction.x > 0 && temp.direction.x < 0) || (transform.direction.x < 0 && temp.direction.x > 0))
        transform = temp;

      udDouble3 euler = udMath_DirToYPR(transform.direction);

      // Only apply if not flipped over the top
      // eulerAngles() returns values between 0 and UD_2PI, a value of UD_PI will indicate a flip
      if (euler.y < UD_HALF_PI || euler.y >(UD_HALF_PI + UD_PI))
      {
        pCamera->position = transform.position;
        pCamera->eulerRotation = euler;
        pCamera->eulerRotation.z = 0;

        if (pCamera->eulerRotation.y > UD_PI)
          pCamera->eulerRotation.y -= UD_2PI;
      }
    }
  }
  break;

  case vcCIS_LookingAtPoint:
  {
    udDouble3 lookVector = pCamInput->lookAtPosition - pCamInput->startPosition;

    pCamInput->progress += deltaTime * 1.0;
    if (pCamInput->progress >= 1.0)
    {
      pCamInput->progress = 1.0;
      pCamInput->inputState = vcCIS_None;
    }

    double progress = udEase(pCamInput->progress, udET_QuadraticOut);

    udDouble3 targetEuler = udMath_DirToYPR(lookVector);
    pCamera->eulerRotation = udSlerp(pCamInput->startAngle, udDoubleQuat::create(targetEuler), progress).eulerAngles();

    if (pCamera->eulerRotation.y > UD_PI)
      pCamera->eulerRotation.y -= UD_2PI;
    break;
  }

  case vcCIS_MovingToPoint:
  {
    udDouble3 moveVector = pCamInput->worldAnchorPoint - pCamInput->startPosition;

    if (moveVector == udDouble3::zero())
      break;

    if (pCamSettings->moveMode == vcCMM_Helicopter)
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

    udDouble3 targetEuler = udMath_DirToYPR(pCamInput->worldAnchorPoint - (pCamInput->startPosition + moveVector * closest));
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
      double distanceToPoint = udMag3(pCamInput->worldAnchorPoint - pCamera->position);
      udDouble3 towards = udNormalize3(pCamInput->worldAnchorPoint - pCamera->position);
      addPos = distanceToPoint * pCamInput->mouseInput.y * towards;
    }
    else // map mode
    {
      double previousOrthoSize = pCamSettings->orthographicSize;

      static const double gMouseWheelToMapZoomRatio = 25.0;
      double orthoRelativeChange = 1.0 - (gMouseWheelToMapZoomRatio * pCamInput->mouseInput.y * deltaTime);
      pCamSettings->orthographicSize = udClamp(pCamSettings->orthographicSize * orthoRelativeChange, sOrthoNearFarPlane.x, sOrthoNearFarPlane.y);

      double orthoSizeChange = pCamSettings->orthographicSize - previousOrthoSize;
      udDouble2 towards = (pCamInput->worldAnchorPoint.toVector2() - pCamera->position.toVector2()) / previousOrthoSize;
      addPos = udDouble3::create(towards * -orthoSizeChange, 0.0);
    }
    pCamera->position += addPos;
  }
  break;

  case vcCIS_PinchZooming:
  {
    // TODO:
  }
  break;

  case vcCIS_Panning:
  {
    if ((pCamInput->mouseInput.x * pCamInput->mouseInput.x) < ONE_PIXEL_SQ && (pCamInput->mouseInput.y * pCamInput->mouseInput.y) < ONE_PIXEL_SQ)
      break;

    udPlane<double> plane = udPlane<double>::create(pCamInput->worldAnchorPoint, { 0, 0, 1 });

    if (pCamSettings->cameraMode != vcCM_OrthoMap && pCamSettings->moveMode == vcCMM_Plane)
      plane.normal = udDoubleQuat::create(pCamera->eulerRotation).apply({ 0, 1, 0 });

    udDouble3 offset, anchorOffset;
    if (udIntersect(plane, pCamera->worldMouseRay, &offset) == udR_Success && udIntersect(plane, pCamInput->anchorMouseRay, &anchorOffset) == udR_Success)
      pCamera->position += (anchorOffset - offset);
  }
  break;

  case vcCIS_Count:
    break; // to cover all implemented cases
  }

  if (pCamInput->inputState == vcCIS_None && pCamInput->transitioningToMapMode)
  {
    pCamInput->transitioningToMapMode = false;
    pCamSettings->cameraMode = vcCM_OrthoMap; // actually swap now
  }

  // in orthographic mode, force camera straight down
  if (pCamSettings->cameraMode == vcCM_OrthoMap)
  {
    pCamera->position.z = sOrthoNearFarPlane.y * 0.5;
    pCamera->eulerRotation = udDouble3::create(0.0, -UD_HALF_PI, 0.0); // down orientation
  }
}

void vcCamera_SwapMapMode(vcState *pProgramState)
{
  udDouble3 lookAtPosition = pProgramState->pCamera->position;
  double cameraHeight = pProgramState->pCamera->position.z;
  if (pProgramState->settings.camera.cameraMode == vcCM_FreeRoam)
  {
    pProgramState->settings.camera.orthographicSize = udMax(1.0, pProgramState->pCamera->position.z / vcCamera_HeightToOrthoRatio);

    // defer actually swapping projection mode
    pProgramState->cameraInput.transitioningToMapMode = true;

    lookAtPosition += udDouble3::create(0, 0, -1); // up
  }
  else
  {
    cameraHeight = pProgramState->settings.camera.orthographicSize * vcCamera_HeightToOrthoRatio;
    pProgramState->settings.camera.cameraMode = vcCM_FreeRoam;
    pProgramState->cameraInput.transitioningToMapMode = false;

    lookAtPosition += udDouble3::create(0, 1, 0); // forward
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

  static bool isFocused = false;
  static bool isMouseBtnBeingHeld = false;

  bool isBtnClicked[3] = { ImGui::IsMouseClicked(0, false), ImGui::IsMouseClicked(1, false), ImGui::IsMouseClicked(2, false) };
  bool isBtnDoubleClicked[3] = { ImGui::IsMouseDoubleClicked(0), ImGui::IsMouseDoubleClicked(1), ImGui::IsMouseDoubleClicked(2) };
  bool isBtnHeld[3] = { ImGui::IsMouseDown(0), ImGui::IsMouseDown(1), ImGui::IsMouseDown(2) };
  bool isBtnReleased[3] = { ImGui::IsMouseReleased(0), ImGui::IsMouseReleased(1), ImGui::IsMouseReleased(2) };

  isMouseBtnBeingHeld &= (isBtnHeld[0] || isBtnHeld[1] || isBtnHeld[2]);
  isFocused = (ImGui::IsItemHovered() || isMouseBtnBeingHeld) && !vcGizmo_IsActive() && !pProgramState->modalOpen;

  if (io.KeyCtrl)
    speedModifier *= 0.1f;

  if (io.KeyShift)
    speedModifier *= 10.f;

  if ((!ImGui::GetIO().WantCaptureKeyboard || isFocused) && !pProgramState->modalOpen)
  {
    keyboardInput.y += io.KeysDown[SDL_SCANCODE_W] - io.KeysDown[SDL_SCANCODE_S];
    keyboardInput.x += io.KeysDown[SDL_SCANCODE_D] - io.KeysDown[SDL_SCANCODE_A];
    keyboardInput.z += io.KeysDown[SDL_SCANCODE_R] - io.KeysDown[SDL_SCANCODE_F];

    if (io.KeysDown[SDL_SCANCODE_SPACE] && io.KeysDownDuration[SDL_SCANCODE_SPACE] == 0.0)
      pProgramState->settings.camera.moveMode = ((pProgramState->settings.camera.moveMode == vcCMM_Helicopter) ? vcCMM_Plane : vcCMM_Helicopter);
  }

  if (isFocused)
  {
    ImVec2 mouseDelta = io.MouseDelta;

    if (keyboardInput != udDouble3::zero() || isBtnClicked[0] || isBtnClicked[1] || isBtnClicked[2]) // if input is detected, TODO: add proper any input detection
    {
      if (pProgramState->cameraInput.inputState == vcCIS_MovingToPoint || pProgramState->cameraInput.inputState == vcCIS_LookingAtPoint)
      {
        pProgramState->pCamera->eulerRotation.z = 0.0;
        pProgramState->cameraInput.inputState = vcCIS_None;

        if (pProgramState->cameraInput.transitioningToMapMode)
        {
          pProgramState->cameraInput.transitioningToMapMode = false;
          pProgramState->settings.camera.cameraMode = vcCM_OrthoMap; // swap immediately
        }
      }
    }

    for (int i = 0; i < 3; ++i)
    {
      // Single Clicking
      if (isBtnClicked[i])
      {
        if (pProgramState->settings.camera.cameraMode == vcCM_FreeRoam)
        {
          switch (pProgramState->settings.camera.cameraMouseBindings[i])
          {
          case vcCPM_Orbit:
            if (pProgramState->pickingSuccess)
            {
              pProgramState->cameraInput.isUsingAnchorPoint = true;
              pProgramState->cameraInput.worldAnchorPoint = pProgramState->worldMousePos;
              pProgramState->cameraInput.inputState = vcCIS_Orbiting;
            }
            break;
          case vcCPM_Tumble:
            pProgramState->cameraInput.inputState = vcCIS_None;
            break;
          case vcCPM_Pan:
            if (pProgramState->pickingSuccess)
            {
              pProgramState->cameraInput.isUsingAnchorPoint = true;
              pProgramState->cameraInput.worldAnchorPoint = pProgramState->worldMousePos;
            }
            pProgramState->cameraInput.anchorMouseRay = pProgramState->pCamera->worldMouseRay;
            pProgramState->cameraInput.inputState = vcCIS_Panning;
            break;
          }
        }
        else
        {
          // orthographic always only pans
          pProgramState->cameraInput.isUsingAnchorPoint = true;
          pProgramState->cameraInput.worldAnchorPoint = pProgramState->pCamera->worldMouseRay.position;
          pProgramState->cameraInput.anchorMouseRay = pProgramState->pCamera->worldMouseRay;
          pProgramState->cameraInput.inputState = vcCIS_Panning;
        }
      }

      // Click and Hold

      if (isBtnHeld[i] && !isBtnClicked[i])
      {
        isMouseBtnBeingHeld = true;
        mouseInput.x = -mouseDelta.x / 100.0;
        mouseInput.y = -mouseDelta.y / 100.0;
        mouseInput.z = 0.0;
      }

      if (isBtnReleased[i])
      {
        if (pProgramState->settings.camera.cameraMouseBindings[i] == vcCPM_Orbit && pProgramState->cameraInput.inputState == vcCIS_Orbiting)
          pProgramState->cameraInput.inputState = vcCIS_None;

        if (pProgramState->settings.camera.cameraMouseBindings[i] == vcCPM_Pan && pProgramState->cameraInput.inputState == vcCIS_Panning)
          pProgramState->cameraInput.inputState = vcCIS_None;
      }

      // Double Clicking
      if (i == 0 && isBtnDoubleClicked[i]) // if left double clicked
      {
        if (pProgramState->pickingSuccess || pProgramState->settings.camera.cameraMode == vcCM_OrthoMap)
        {
          pProgramState->cameraInput.inputState = vcCIS_MovingToPoint;
          pProgramState->cameraInput.startPosition = pProgramState->pCamera->position;
          pProgramState->cameraInput.startAngle = udDoubleQuat::create(pProgramState->pCamera->eulerRotation);
          pProgramState->cameraInput.worldAnchorPoint = pProgramState->worldMousePos;
          pProgramState->cameraInput.progress = 0.0;

          if (pProgramState->settings.camera.cameraMode == vcCM_OrthoMap)
          {
            pProgramState->cameraInput.startAngle = udDoubleQuat::identity();
            pProgramState->cameraInput.worldAnchorPoint = pProgramState->pCamera->worldMouseRay.position;
          }
        }
      }
    }

    // Mouse Wheel
    const double defaultTimeouts[vcCM_Count] = { 0.25, 0.0 };
    double timeout = defaultTimeouts[pProgramState->settings.camera.cameraMode]; // How long you have to stop scrolling the scroll wheel before the point unlocks
    static double previousLockTime = 0.0;
    double currentTime = ImGui::GetTime();

    if (io.MouseWheel != 0)
    {
      if (pProgramState->settings.camera.scrollWheelMode == vcCSWM_Dolly)
      {
        if (previousLockTime < currentTime - timeout && (pProgramState->pickingSuccess || pProgramState->settings.camera.cameraMode == vcCM_OrthoMap) && pProgramState->cameraInput.inputState == vcCIS_None)
        {
          pProgramState->cameraInput.isUsingAnchorPoint = true;
          pProgramState->cameraInput.worldAnchorPoint = pProgramState->worldMousePos;
          pProgramState->cameraInput.inputState = vcCIS_CommandZooming;

          if (pProgramState->settings.camera.cameraMode == vcCM_OrthoMap)
            pProgramState->cameraInput.worldAnchorPoint = pProgramState->pCamera->worldMouseRay.position;
        }

        if (pProgramState->cameraInput.inputState == vcCIS_CommandZooming)
        {
          mouseInput.x = 0.0;
          mouseInput.y = io.MouseWheel / 10.f;
          mouseInput.z = 0.0;
          previousLockTime = currentTime;
        }
      }
      else
      {
        if (io.MouseWheel > 0)
          pProgramState->settings.camera.moveSpeed *= (1.f + io.MouseWheel / 10.f);
        else
          pProgramState->settings.camera.moveSpeed /= (1.f - io.MouseWheel / 10.f);

        pProgramState->settings.camera.moveSpeed = udClamp(pProgramState->settings.camera.moveSpeed, vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed);
      }
    }
    else if (pProgramState->cameraInput.inputState == vcCIS_CommandZooming && previousLockTime < currentTime - timeout)
    {
      pProgramState->cameraInput.inputState = vcCIS_None;
    }
  }

  // set pivot to send to apply function
  pProgramState->cameraInput.currentPivotMode = vcCPM_Tumble;
  if (pProgramState->settings.camera.cameraMode == vcCM_OrthoMap)
  {
    if (io.MouseDown[0] || io.MouseDown[1] || io.MouseDown[2])
      pProgramState->cameraInput.currentPivotMode = vcCPM_Pan;
  }
  else
  {
    if (io.MouseDown[0] && !io.MouseDown[1] && !io.MouseDown[2])
      pProgramState->cameraInput.currentPivotMode = pProgramState->settings.camera.cameraMouseBindings[0];

    if (!io.MouseDown[0] && io.MouseDown[1] && !io.MouseDown[2])
      pProgramState->cameraInput.currentPivotMode = pProgramState->settings.camera.cameraMouseBindings[1];

    if (!io.MouseDown[0] && !io.MouseDown[1] && io.MouseDown[2])
      pProgramState->cameraInput.currentPivotMode = pProgramState->settings.camera.cameraMouseBindings[2];
  }

  if (pProgramState->cameraInput.currentPivotMode != vcCPM_Orbit && pProgramState->cameraInput.inputState == vcCIS_Orbiting)
    pProgramState->cameraInput.inputState = vcCIS_None;

  if (pProgramState->cameraInput.currentPivotMode != vcCPM_Pan && pProgramState->cameraInput.inputState == vcCIS_Panning)
    pProgramState->cameraInput.inputState = vcCIS_None;


  // Apply movement and rotation
  pProgramState->cameraInput.keyboardInput = keyboardInput;
  pProgramState->cameraInput.mouseInput = mouseInput;

  vcCamera_Apply(pProgramState->pCamera, &pProgramState->settings.camera, &pProgramState->cameraInput, pProgramState->deltaTime, speedModifier);

  if (pProgramState->cameraInput.inputState == vcCIS_None)
    pProgramState->cameraInput.isUsingAnchorPoint = false;

  vcCamera_UpdateMatrices(pProgramState->pCamera, pProgramState->settings.camera, windowSize, &mousePos);
}
