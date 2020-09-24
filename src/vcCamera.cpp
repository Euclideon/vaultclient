#include "vcCamera.h"
#include "vcState.h"
#include "vcHotkey.h"
#include "vcRender.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_ex/ImGuizmo.h"
#include "udStringUtil.h"

#define ONE_PIXEL_SQ 0.0001

// higher == quicker smoothing
static const double sCameraTranslationSmoothingSpeed = 14.0;
static const double sCameraRotationSmoothingSpeed = 30.0;
static const double sCameraOrbitSmoothingSpeed = 10.0;

udDouble4x4 vcCamera_GetMatrix(const udGeoZone &zone, vcCamera *pCamera)
{
  udDoubleQuat orientation = vcGIS_HeadingPitchToQuaternion(zone, pCamera->position, pCamera->headingPitch);
  udDouble3 lookPos = pCamera->position + orientation.apply(udDouble3::create(0.0, 1.0, 0.0));
  return udDouble4x4::lookAt(pCamera->position, lookPos, orientation.apply(udDouble3::create(0.0, 0.0, 1.0)));
}

void vcCamera_StopSmoothing(vcCameraInput *pCamInput)
{
  pCamInput->smoothTranslation = udDouble3::zero();
  pCamInput->smoothRotation = udDouble2::zero();
}

void vcCamera_UpdateCameraMap(vcState* pProgramState, vcCamera* pCamera)
{
  if (pProgramState->settings.camera.mapMode[pProgramState->activeViewportIndex])
  {
    for (int v = 0; v < vcMaxViewportCount; v++)
    {
      if (!pProgramState->settings.camera.mapMode[v])
      {
        if (pProgramState->geozone.projection != udGZPT_ECEF)
        {
          pProgramState->pViewports[v].camera.position.x = pCamera->position.x;
          pProgramState->pViewports[v].camera.position.y = pCamera->position.y;
        }
        else
        {
          udDouble3 camMapPosLatLong = udGeoZone_CartesianToLatLong(pProgramState->geozone, pCamera->position);
          udDouble3 camPosLatLong = udGeoZone_CartesianToLatLong(pProgramState->geozone, pProgramState->pViewports[v].camera.position);
          camMapPosLatLong.z = camPosLatLong.z;
          pProgramState->pViewports[v].camera.position = udGeoZone_LatLongToCartesian(pProgramState->geozone, camMapPosLatLong);
        }
      }
    }
  }
  else
  {
    for (int v = 0; v < vcMaxViewportCount; v++)
    {
      if (pProgramState->settings.camera.mapMode[v])
      {
        if (pProgramState->geozone.projection != udGZPT_ECEF)
          pProgramState->pViewports[v].camera.position = udDouble3::create(pCamera->position.x, pCamera->position.y, pProgramState->pViewports[v].camera.position.z);
        else
        {
          udDouble3 camPosLatLong = udGeoZone_CartesianToLatLong(pProgramState->geozone, pCamera->position);
          udDouble3 camMapPosLatLong = udGeoZone_CartesianToLatLong(pProgramState->geozone, pProgramState->pViewports[v].camera.position);
          camPosLatLong.z = camMapPosLatLong.z;
          pProgramState->pViewports[v].camera.position = udGeoZone_LatLongToCartesian(pProgramState->geozone, camPosLatLong);
        }
        pProgramState->pViewports[v].camera.headingPitch = udDouble2::create(0.0f, -UD_HALF_PI);
      }
    }
  }
}

void vcCamera_UpdateSmoothing(vcState * pProgramState, vcCamera *pCamera, vcCameraInput *pCamInput, double deltaTime, const udDouble3 &worldAnchorNormal, const udDouble3 &worldAnchorPoint, const udGeoZone &geozone)
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
      vcCamera_UpdateCameraMap(pProgramState, pCamera);
    }
    else
    {
      pCamInput->smoothTranslation = udDouble3::zero();
    }

    // rotation
    if (udMagSq2(pCamInput->smoothRotation) > minSmoothingThreshold)
    {
      udDouble2 step = pCamInput->smoothRotation * udMin(1.0, stepAmount * sCameraRotationSmoothingSpeed);
      pCamera->headingPitch += step;
      pCamInput->smoothRotation -= step;

      pCamera->headingPitch.x = udMod(pCamera->headingPitch.x, UD_2PI);
      pCamera->headingPitch.y = udMod(pCamera->headingPitch.y, UD_2PI);
    }
    else
    {
      pCamInput->smoothRotation = udDouble2::zero();
    }

    if (udMagSq2(pCamInput->smoothOrbit) >= 0.00001)
    {
      udDouble2 step = pCamInput->smoothOrbit * udMin(1.0, stepAmount * sCameraOrbitSmoothingSpeed);
      pCamInput->smoothOrbit -= step;

      udDoubleQuat orientation = vcGIS_HeadingPitchToQuaternion(geozone, pCamera->position, pCamera->headingPitch);

      // Orbit Left/Right
      if (step.x != 0)
      {
        udDoubleQuat rotation = udDoubleQuat::create(worldAnchorNormal, step.x);
        udDouble3 direction = pCamera->position - worldAnchorPoint; // find current direction relative to center

        orientation = (rotation * orientation);

        pCamera->position = worldAnchorPoint + rotation.apply(direction); // define new position
        pCamera->headingPitch = vcGIS_QuaternionToHeadingPitch(geozone, pCamera->position, orientation);
      }

      if (pCamera->headingPitch.y < UD_DEG2RADf(-89.0) && pCamInput->mouseInput.y <= 0)
        break;
      if (pCamera->headingPitch.y > UD_DEG2RADf(89.0) && pCamInput->mouseInput.y >= 0)
        break;

      // Orbit Up/Down     
      udDoubleQuat rotation = udDoubleQuat::create(orientation.apply({ 1, 0, 0 }), step.y);
      udDouble3 direction = pCamera->position - worldAnchorPoint; // find current direction relative to center

      orientation = (rotation * orientation);

      // Save it back to the camera
      pCamera->position = worldAnchorPoint + rotation.apply(direction); // define new position
      pCamera->headingPitch = vcGIS_QuaternionToHeadingPitch(geozone, pCamera->position, orientation);

      vcCamera_UpdateCameraMap(pProgramState, pCamera);
    }
    else
    {
      pCamInput->smoothOrbit = udDouble2::zero();
    }
  }
}

void vcCamera_BeginCameraPivotModeMouseBinding(vcState *pProgramState, vcViewport *pViewport, int bindingIndex)
{
  switch (pProgramState->settings.camera.cameraMouseBindings[bindingIndex])
  {
  case vcCPM_Orbit:
    if (pViewport->pickingSuccess)
    {
      pViewport->isUsingAnchorPoint = true;
      pViewport->worldAnchorPoint = pViewport->worldMousePosCartesian;
      pViewport->cameraInput.inputState = vcCIS_Orbiting;
      vcCamera_StopSmoothing(&pViewport->cameraInput);
    }
    break;
  case vcCPM_Tumble:
    pViewport->cameraInput.inputState = vcCIS_None;
    break;
  case vcCPM_Pan:
    if (pViewport->pickingSuccess)
    {
      pViewport->isUsingAnchorPoint = true;
      pViewport->worldAnchorPoint = pViewport->worldMousePosCartesian;
      pViewport->anchorMouseRay = pViewport->camera.worldMouseRay;
      pViewport->cameraInput.inputState = vcCIS_Panning;
    }
    break;
  case vcCPM_Forward:
    pViewport->cameraInput.inputState = vcCIS_MovingForward;
    break;
  default:
    // Do nothing
    break;
  };
}

//The plane will be packed into a 4-vec: [normal[3], offset]
udDouble4 vcCamera_GetNearPlane(const vcCamera &camera, const vcCameraSettings &settings)
{
  udDouble4 normal = camera.matrices.camera * udDouble4::create(0.0, 1.0, 0.0, 0.0);
  udDouble4 point = udDouble4::create(camera.position, 1.0) + settings.nearPlane * normal;
  double offset = udDot4(normal, point);
  normal.w = offset;
  return normal;
}

void vcCamera_UpdateMatrices(const udGeoZone &zone, vcCamera *pCamera, const vcCameraSettings &settings, const int activeViewportIndex, const udFloat2 &windowSize, const udFloat2 *pMousePos /*= nullptr*/)
{
  // Update matrices
  double fov = !settings.mapMode[activeViewportIndex] ? settings.fieldOfView[activeViewportIndex] : UD_PIf * 10.0f / 18.f; // 10 degrees;
  double aspect = windowSize.x / windowSize.y;
  double zNear = settings.nearPlane;
  double zFar = settings.farPlane;

  pCamera->matrices.camera = vcCamera_GetMatrix(zone, pCamera);

  pCamera->matrices.projectionNear = vcGLState_ProjectionMatrix<double>(fov, aspect, 0.5f, 10000.f);
  pCamera->matrices.projection = vcGLState_ProjectionMatrix<double>(fov, aspect, zNear, zFar);
  pCamera->matrices.projectionUD = udDouble4x4::perspectiveZO(fov, aspect, zNear, zFar);

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
    mousePosClip.y = -mousePosClip.y;
#endif
    udDouble4 mouseNear = (pCamera->matrices.inverseViewProjection * udDouble4::create(mousePosClip, nearClipZ, 1.0));
    udDouble4 mouseFar = (pCamera->matrices.inverseViewProjection * udDouble4::create(mousePosClip, 1.0, 1.0));

    mouseNear /= mouseNear.w;
    mouseFar /= mouseFar.w;

    pCamera->worldMouseRay.position = mouseNear.toVector3();
    pCamera->worldMouseRay.direction = udNormalize3(mouseFar - mouseNear).toVector3();
  }
}

void vcCamera_Apply(vcState *pProgramState, vcViewport *pViewport, vcCameraSettings *pCamSettings, double deltaTime)
{
  udDouble3 worldAnchorNormal = vcGIS_GetWorldLocalUp(pProgramState->geozone, pViewport->worldAnchorPoint);
  udDoubleQuat orientation = vcGIS_HeadingPitchToQuaternion(pProgramState->geozone, pViewport->camera.position, pViewport->camera.headingPitch);

  pViewport->camera.cameraUp = vcGIS_GetWorldLocalUp(pProgramState->geozone, pViewport->camera.position);
  pViewport->camera.cameraNorth = vcGIS_GetWorldLocalNorth(pProgramState->geozone, pViewport->camera.position);

  switch (pViewport->cameraInput.inputState)
  {
  case vcCIS_MovingForward:
    pViewport->cameraInput.keyboardInput.y += 1;
    // fall through
  case vcCIS_None:
  {
    udDouble3 addPos = (pViewport->cameraInput.keyboardInput + pViewport->cameraInput.controllerDPADInput);

    double vertPos = addPos.z;

    addPos.z = 0.0;

    // Translation
    if (pCamSettings->mapMode[pProgramState->activeViewportIndex])
    {
      udDouble3 addPosNS = pViewport->camera.cameraNorth * addPos.y;
      udDouble3 addPosEW = udCross(pViewport->camera.cameraNorth, pViewport->camera.cameraUp) * addPos.x;

      addPos = addPosNS + addPosEW;
    }
    else
    {
      addPos = orientation.apply(addPos);

      if (pCamSettings->lockAltitude && addPos != udDouble3::zero())
        addPos -= udDot(pViewport->camera.cameraUp, addPos) * pViewport->camera.cameraUp;
    }

    if (addPos != udDouble3::zero())
      addPos = udNormalize3(addPos);

    addPos += vertPos * pViewport->camera.cameraUp;
    addPos *= pCamSettings->moveSpeed[pProgramState->activeViewportIndex] * deltaTime;

    pViewport->cameraInput.smoothTranslation += addPos;

    // Check for a nan camera position and reset to zero, this allows the UI to be usable in the event of error
    if (isnan(pViewport->camera.position.x) || isnan(pViewport->camera.position.y) || isnan(pViewport->camera.position.z))
      pViewport->camera.position = udDouble3::zero();

    udDouble2 rotation = udDouble2::create(-pViewport->cameraInput.mouseInput.x, pViewport->cameraInput.mouseInput.y) * 0.5; // Because this messes with HEADING, has to be reversed
    if (pCamSettings->mapMode[pProgramState->activeViewportIndex]) rotation = udDouble2::zero();
    udDouble2 result = pViewport->camera.headingPitch + rotation;
    if (result.y > UD_HALF_PI)
      rotation.y = UD_HALF_PI - pViewport->camera.headingPitch.y;
    else if (result.y < -UD_HALF_PI)
      rotation.y = -UD_HALF_PI - pViewport->camera.headingPitch.y;

    pViewport->cameraInput.smoothRotation += rotation;
  }
  break;

  case vcCIS_Orbiting:
  {
    if (pCamSettings->mapMode[pProgramState->activeViewportIndex])
      break;

    double distanceToPointSqr = udMagSq3(pViewport->worldAnchorPoint - pViewport->camera.position);
    if (distanceToPointSqr != 0.0 && (pViewport->cameraInput.mouseInput.x != 0 || pViewport->cameraInput.mouseInput.y != 0))
    {
      pViewport->cameraInput.smoothOrbit.x += pViewport->cameraInput.mouseInput.x;
      pViewport->cameraInput.smoothOrbit.y += pViewport->cameraInput.mouseInput.y;
    }
  }
  break;

  case vcCIS_MovingToPoint:
  {
    udDouble3 moveVector = pViewport->worldAnchorPoint - pViewport->cameraInput.startPosition;

    if (moveVector == udDouble3::zero())
      break;

    if (pCamSettings->lockAltitude)
      moveVector.z = 0;

    double length = udMag3(moveVector);
    double closest = udMax(0.9, (length - 100.0) / length); // gets to either 90% or within 100m

    moveVector *= closest;

    pViewport->cameraInput.progress += deltaTime / 2; // 2 second travel time
    if (pViewport->cameraInput.progress > 1.0)
    {
      pViewport->cameraInput.progress = 1.0;
      pViewport->cameraInput.inputState = vcCIS_None;
    }

    double travelProgress = udEase(pViewport->cameraInput.progress, udET_CubicInOut);
    pViewport->camera.position = pViewport->cameraInput.startPosition + moveVector * travelProgress;
    if (pViewport->camera.headingPitch.y > UD_PI)
      pViewport->camera.headingPitch.y -= UD_2PI;

    vcCamera_UpdateCameraMap(pProgramState, &pViewport->camera);
  }
  break;

  case vcCIS_MoveToViewpoint:
  {
    udDouble3 moveVector = pViewport->worldAnchorPoint - pViewport->cameraInput.startPosition;
    pViewport->cameraInput.progress += deltaTime / 2; // 2 second travel time

    if (pViewport->cameraInput.progress > 1.0)
    {
      pViewport->cameraInput.targetAngle = udDoubleQuat::identity();
      pViewport->cameraInput.progress = 1.0;

      pViewport->cameraInput.inputState = vcCIS_None;
      pViewport->camera.headingPitch = pViewport->cameraInput.headingPitch;
      pViewport->camera.position = pViewport->worldAnchorPoint;
      
      break;
    }

    double travelProgress = udEase(pViewport->cameraInput.progress, udET_CubicInOut);
    pViewport->camera.position = pViewport->cameraInput.startPosition + moveVector * travelProgress;
    pViewport->camera.headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->geozone, pViewport->camera.position, udSlerp(pViewport->cameraInput.startAngle, pViewport->cameraInput.targetAngle, travelProgress));
    if (pViewport->camera.headingPitch.y > UD_PI)
      pViewport->camera.headingPitch.y -= UD_2PI;

    vcCamera_UpdateCameraMap(pProgramState, &pViewport->camera);

  }
  break;

  case vcCIS_ZoomTo:
  {
    udDouble3 addPos = udDouble3::zero();
    udDouble3 towards = pViewport->worldAnchorPoint - pViewport->camera.position;
    if (udMagSq3(towards) > 0)
    {
      double minDistance = 10.0;
      double maxDistance = 0.9 * pCamSettings->farPlane; // limit to 90% of visible distance
      double distanceToPoint = udMin(udMag3(towards), maxDistance);

      if (pViewport->cameraInput.mouseInput.y > 0)
        distanceToPoint = udMax(distanceToPoint - minDistance, 0.0);

      addPos = distanceToPoint * pViewport->cameraInput.mouseInput.y * udNormalize3(towards);
      if (pCamSettings->mapMode[pProgramState->activeViewportIndex])
      {
        addPos = -1 * distanceToPoint * pViewport->cameraInput.mouseInput.y * pViewport->camera.cameraUp;
      }
    }
    pViewport->cameraInput.smoothTranslation += addPos;
  }
  break;

  case vcCIS_Rotate:
  {
    // Not good to use udSlerp because the camera may keep unexpected rolling angle if smooth interrupted by mouse button clicked.
    pViewport->cameraInput.progress += deltaTime;
    if (pViewport->cameraInput.progress > 1.0)
    {
      pViewport->camera.headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->geozone, pViewport->camera.position, pViewport->cameraInput.targetAngle);
      pViewport->cameraInput.targetAngle = udDoubleQuat::identity();
      pViewport->cameraInput.inputState = vcCIS_None;
      pViewport->cameraInput.progress = 1.0;
    }
    else
    {
      pViewport->camera.headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->geozone, pViewport->camera.position, udSlerp(pViewport->cameraInput.startAngle, pViewport->cameraInput.targetAngle, pViewport->cameraInput.progress));
    }

    if (pViewport->camera.headingPitch.y > UD_PI)
      pViewport->camera.headingPitch.y -= UD_2PI;
  }
  break;

  case vcCIS_PinchZooming:
  {
    // TODO
  }
  break;

  case vcCIS_Panning:
  {
    udPlane<double> plane = udPlane<double>::create(pViewport->worldAnchorPoint, worldAnchorNormal);

    if (!pCamSettings->lockAltitude) // generate a plane facing the camera
      plane.normal = orientation.apply({ 0, 1, 0 });

    udDouble3 offset = udDouble3::create(0, 0, 0);
    udDouble3 anchorOffset = udDouble3::create(0, 0, 0);
    if (plane.intersects(pViewport->camera.worldMouseRay, &offset, nullptr) && plane.intersects(pViewport->anchorMouseRay, &anchorOffset, nullptr))
      pViewport->cameraInput.smoothTranslation = (anchorOffset - offset);
  }
  break;

  case vcCIS_Count:
    break; // to cover all implemented cases
  }

  if (pViewport->cameraInput.stabilize)
  {
    pViewport->cameraInput.progress += deltaTime * 2; // .5 second stabilize time
    if (pViewport->cameraInput.progress >= 1.0)
    {
      pViewport->cameraInput.progress = 1.0;
      pViewport->cameraInput.stabilize = false;
    }

    if (pViewport->camera.headingPitch.y > UD_PI)
      pViewport->camera.headingPitch.y -= UD_2PI;
  }

  vcCamera_UpdateSmoothing(pProgramState, &pViewport->camera, &pViewport->cameraInput, deltaTime, worldAnchorNormal, pViewport->worldAnchorPoint, pProgramState->geozone);
}

void vcCamera_HandleSceneInput(vcState *pProgramState, vcViewport *pViewport, int viewportIndex, udDouble3 oscMove, udFloat2 windowSize, udFloat2 mousePos)
{
  ImGuiIO &io = ImGui::GetIO();

  udDouble3 keyboardInput = udDouble3::zero();
  udDouble3 mouseInput = udDouble3::zero();

  // bring in values
  keyboardInput += oscMove;

  ImVec2 mouseDelta = io.MouseDelta;
  float mouseWheel = io.MouseWheel;

  bool isBtnClicked[3] = { ImGui::IsMouseClicked(0, false), ImGui::IsMouseClicked(1, false), ImGui::IsMouseClicked(2, false) };
  bool isBtnDoubleClicked[3] = { ImGui::IsMouseDoubleClicked(0), ImGui::IsMouseDoubleClicked(1), ImGui::IsMouseDoubleClicked(2) };
  bool isBtnHeld[3] = { ImGui::IsMouseDown(0), ImGui::IsMouseDown(1), ImGui::IsMouseDown(2) };
  bool isBtnReleased[3] = { ImGui::IsMouseReleased(0), ImGui::IsMouseReleased(1), ImGui::IsMouseReleased(2) };

  pViewport->cameraInput.isMouseBtnBeingHeld &= (isBtnHeld[0] || isBtnHeld[1] || isBtnHeld[2]);
  bool isFocused = (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || pViewport->cameraInput.isMouseBtnBeingHeld) && !vcGizmo_IsActive(pViewport->gizmo.pContext) && !pProgramState->modalOpen;

  if (isFocused)
    pProgramState->focusedViewportIndex = viewportIndex;

  bool captureInput = (pProgramState->focusedViewportIndex == viewportIndex);

  int totalButtonsHeld = 0;
  for (size_t i = 0; i < udLengthOf(isBtnHeld); ++i)
    totalButtonsHeld += isBtnHeld[i] ? 1 : 0;

  // Start hold time
  if (isFocused && (isBtnClicked[0] || isBtnClicked[1] || isBtnClicked[2]))
  {
    pViewport->cameraInput.isMouseBtnBeingHeld = true;
    mouseDelta = { 0, 0 };
  }

  bool forceClearMouseState = !isFocused;

  // Was the gizmo just clicked on?
  vcSceneItemRef clickedItemRef = pProgramState->sceneExplorer.clickedItem;
  if (clickedItemRef.pItem != nullptr)
  {
    vcSceneItem *pItem = (vcSceneItem *)clickedItemRef.pItem->pUserData;
    if (pItem != nullptr)
    {
      if (pViewport->gizmo.operation == vcGO_Scale || pViewport->gizmo.coordinateSystem == vcGCS_Local)
      {
        pViewport->gizmo.direction[0] = udDouble3::create(1, 0, 0);
        pViewport->gizmo.direction[1] = udDouble3::create(0, 1, 0);
        pViewport->gizmo.direction[2] = udDouble3::create(0, 0, 1);
      }
      else
      {
        vcGIS_GetOrthonormalBasis(pProgramState->geozone, pItem->GetWorldSpacePivot(), &pViewport->gizmo.direction[2], &pViewport->gizmo.direction[1], &pViewport->gizmo.direction[0]);
      }
    }
  }
  pViewport->cameraInput.gizmoCapturedMouse = pViewport->cameraInput.gizmoCapturedMouse || (pViewport->gizmo.operation != 0 && vcGizmo_IsHovered(pViewport->gizmo.pContext, pViewport->gizmo.direction) && (isBtnClicked[0] || isBtnClicked[1] || isBtnClicked[2]));
  if (pViewport->cameraInput.gizmoCapturedMouse)
  {
    // was the gizmo just released?
    pViewport->cameraInput.gizmoCapturedMouse = isBtnHeld[0] || isBtnHeld[1] || isBtnHeld[2];
    forceClearMouseState = (forceClearMouseState || pViewport->cameraInput.gizmoCapturedMouse);
  }

  if (forceClearMouseState)
  {
    memset(isBtnClicked, 0, sizeof(isBtnClicked));
    memset(isBtnDoubleClicked, 0, sizeof(isBtnDoubleClicked));
    memset(isBtnHeld, 0, sizeof(isBtnHeld));
    mouseDelta = ImVec2();
    mouseWheel = 0.0f;
    pViewport->cameraInput.isMouseBtnBeingHeld = false;
    // Leaving isBtnReleased unchanged as there should be no reason to ignore a mouse release while the window has mouse focus
  }

  // Accept mouse input
  if (pViewport->cameraInput.isMouseBtnBeingHeld)
  {
    mouseInput.x = (pProgramState->settings.camera.invertMouseX ? mouseDelta.x : -mouseDelta.x) / 100.0;
    mouseInput.y = (pProgramState->settings.camera.invertMouseY ? mouseDelta.y : -mouseDelta.y) / 100.0;
    mouseInput.z = 0.0;
  }

  // Controller Input
  if (captureInput)
  {
    if (io.NavActive)
    {
      keyboardInput.x += io.NavInputs[ImGuiNavInput_LStickLeft]; // Left Stick Horizontal
      keyboardInput.y += -io.NavInputs[ImGuiNavInput_LStickUp]; // Left Stick Vertical

      mouseInput.x += (pProgramState->settings.camera.invertControllerX ? io.NavInputs[ImGuiNavInput_LStickRight] : -io.NavInputs[ImGuiNavInput_LStickRight]) / 15.0f; // Right Stick Horizontal
      mouseInput.y += (pProgramState->settings.camera.invertControllerY ? io.NavInputs[ImGuiNavInput_LStickDown] : -io.NavInputs[ImGuiNavInput_LStickDown]) / 25.0f; // Right Stick Vertical

      // In Imgui the DPAD is bound to navigation, so disable DPAD panning until the issue is resolved
      //pCameraInput->controllerDPADInput = udDouble3::create(io.NavInputs[ImGuiNavInput_DpadRight] - io.NavInputs[ImGuiNavInput_DpadLeft], 0, io.NavInputs[ImGuiNavInput_DpadUp] - io.NavInputs[ImGuiNavInput_DpadDown]);

      if (pViewport->cameraInput.isRightTriggerHeld)
      {
        if (pViewport->pickingSuccess && pViewport->cameraInput.inputState == vcCIS_None)
        {
          pViewport->isUsingAnchorPoint = true;
          pViewport->worldAnchorPoint = pViewport->worldMousePosCartesian;
          pViewport->cameraInput.inputState = vcCIS_Orbiting;
          vcCamera_StopSmoothing(&pViewport->cameraInput);
        }
        if (io.NavInputs[ImGuiNavInput_FocusNext] < 0.85f) // Right Trigger
        {
          pViewport->cameraInput.inputState = vcCIS_None;
          pViewport->cameraInput.isRightTriggerHeld = false;
        }
      }
      else if (io.NavInputs[ImGuiNavInput_FocusNext] > 0.85f) // Right Trigger
      {
        pViewport->cameraInput.isRightTriggerHeld = true;
      }

      if (io.NavInputs[ImGuiNavInput_Activate] && !io.NavInputsDownDuration[ImGuiNavInput_Activate]) // A Button
        pProgramState->settings.camera.lockAltitude = !pProgramState->settings.camera.lockAltitude;
    }

    // Allow camera movement when left mouse button is holding down.
    if (!pProgramState->modalOpen)
    {
      bool enableMove = true;
      ImGuiContext *p = ImGui::GetCurrentContext();
      if (p && p->ActiveIdWindow)
      {
        const char *activeIdName = p->ActiveIdWindow->Name;
        size_t len = udStrlen(activeIdName);
        if (len > 0 && udStrstr(activeIdName, len, "###sceneDock") == nullptr)
          enableMove = false;
      }

      if(enableMove)
      {
        keyboardInput.y += vcHotkey::IsDown(vcB_Forward) - vcHotkey::IsDown(vcB_Backward);
        keyboardInput.x += vcHotkey::IsDown(vcB_Right) - vcHotkey::IsDown(vcB_Left);
        keyboardInput.z += vcHotkey::IsDown(vcB_Up) - vcHotkey::IsDown(vcB_Down);
      }
    
    }

    if ((!ImGui::GetIO().WantCaptureKeyboard || isFocused) && !pProgramState->modalOpen && !ImGui::IsAnyItemActive())
    {
      if (vcHotkey::IsPressed(vcB_LockAltitude, false))
        pProgramState->settings.camera.lockAltitude = !pProgramState->settings.camera.lockAltitude;
      if (vcHotkey::IsPressed(vcB_GizmoTranslate))
        pViewport->gizmo.operation = ((pViewport->gizmo.operation == vcGO_Translate) ? vcGO_NoGizmo : vcGO_Translate);
      if (vcHotkey::IsPressed(vcB_GizmoRotate))
        pViewport->gizmo.operation = ((pViewport->gizmo.operation == vcGO_Rotate) ? vcGO_NoGizmo : vcGO_Rotate);
      if (vcHotkey::IsPressed(vcB_GizmoScale))
        pViewport->gizmo.operation = ((pViewport->gizmo.operation == vcGO_Scale) ? vcGO_NoGizmo : vcGO_Scale);
      if (vcHotkey::IsPressed(vcB_GizmoLocalSpace))
        pViewport->gizmo.coordinateSystem = ((pViewport->gizmo.coordinateSystem == vcGCS_Scene) ? vcGCS_Local : vcGCS_Scene);
      if (vcHotkey::IsPressed(vcB_DecreaseCameraSpeed))
      {
        pProgramState->settings.camera.moveSpeed[pProgramState->activeViewportIndex] *= 0.1f;
        pProgramState->settings.camera.moveSpeed[pProgramState->activeViewportIndex] = udMax(pProgramState->settings.camera.moveSpeed[pProgramState->activeViewportIndex], vcSL_CameraMinMoveSpeed);
      }
      else if (vcHotkey::IsPressed(vcB_IncreaseCameraSpeed))
      {
        pProgramState->settings.camera.moveSpeed[pProgramState->activeViewportIndex] *= 10.f;
        pProgramState->settings.camera.moveSpeed[pProgramState->activeViewportIndex] = udMin(pProgramState->settings.camera.moveSpeed[pProgramState->activeViewportIndex], vcSL_CameraMaxMoveSpeed);
      }
    }

    if (keyboardInput != udDouble3::zero() || isBtnClicked[0] || isBtnClicked[1] || isBtnClicked[2]) // if input is detected, TODO: add proper any input detection
    {
      if (pViewport->cameraInput.inputState == vcCIS_MovingToPoint)
      {
        pViewport->cameraInput.stabilize = true;
        pViewport->cameraInput.progress = 0;
        pViewport->cameraInput.inputState = vcCIS_None;
      }
    }
  }

  for (int i = 0; i < 3; ++i)
  {
    // Single Clicking
    if (isBtnClicked[i] && (pViewport->cameraInput.inputState == vcCIS_None || totalButtonsHeld == 1)) // immediately override current input if this is a new button down
      vcCamera_BeginCameraPivotModeMouseBinding(pProgramState, pViewport, i);

    if (isBtnReleased[i])
    {
      if ((pProgramState->settings.camera.cameraMouseBindings[i] == vcCPM_Orbit && pViewport->cameraInput.inputState == vcCIS_Orbiting) ||
        (pProgramState->settings.camera.cameraMouseBindings[i] == vcCPM_Pan && pViewport->cameraInput.inputState == vcCIS_Panning) ||
        (pProgramState->settings.camera.cameraMouseBindings[i] == vcCPM_Forward && pViewport->cameraInput.inputState == vcCIS_MovingForward) ||
        pViewport->cameraInput.inputState == vcCIS_ZoomTo)
      {
        pViewport->cameraInput.inputState = vcCIS_None;

        // Should another mouse action take over? (it's being held down)
        for (int j = 0; j < 3; ++j)
        {
          if (isBtnHeld[j])
          {
            vcCamera_BeginCameraPivotModeMouseBinding(pProgramState, pViewport, j);
            break;
          }
        }
      }
    }
  }

  // Double Clicking left mouse
  if (isBtnDoubleClicked[0] && pViewport->pickingSuccess)
  {
    pViewport->cameraInput.inputState = vcCIS_MovingToPoint;
    pViewport->cameraInput.startPosition = pViewport->camera.position;
    pViewport->cameraInput.startAngle = vcGIS_HeadingPitchToQuaternion(pProgramState->geozone, pViewport->camera.position, pViewport->camera.headingPitch);
    pViewport->cameraInput.progress = 0.0;

    pViewport->isUsingAnchorPoint = true;
    pViewport->worldAnchorPoint = pViewport->worldMousePosCartesian;
  }

  // Mouse Wheel
  const double defaultTimeouts = 0.25;
  double timeout = defaultTimeouts; // How long you have to stop scrolling the scroll wheel before the point unlocks
  double currentTime = ImGui::GetTime();
  bool zooming = false;

  if (mouseWheel != 0)
  {
    zooming = true;
    if (pProgramState->settings.camera.scrollWheelMode == vcCSWM_Dolly)
    {
      if (pViewport->cameraInput.previousLockTime < currentTime - timeout && (pViewport->pickingSuccess) && pViewport->cameraInput.inputState == vcCIS_None)
      {
        pViewport->isUsingAnchorPoint = true;
        pViewport->worldAnchorPoint = pViewport->worldMousePosCartesian;
        pViewport->cameraInput.inputState = vcCIS_ZoomTo;
      }

      if (pViewport->cameraInput.inputState == vcCIS_ZoomTo)
      {
        mouseInput.x = 0.0;
        mouseInput.y = mouseWheel / 5.0;
        mouseInput.z = 0.0;
        pViewport->cameraInput.previousLockTime = currentTime;

        pViewport->cameraInput.startPosition = pViewport->camera.position;
      }
    }
    else
    {
      if (mouseWheel > 0)
        pProgramState->settings.camera.moveSpeed[pProgramState->activeViewportIndex] *= (1.f + mouseWheel / 10.f);
      else
        pProgramState->settings.camera.moveSpeed[pProgramState->activeViewportIndex] /= (1.f - mouseWheel / 10.f);

      pProgramState->settings.camera.moveSpeed[pProgramState->activeViewportIndex] = udClamp(pProgramState->settings.camera.moveSpeed[pProgramState->activeViewportIndex], vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed);
    }
  }

  if (!zooming && pViewport->cameraInput.inputState == vcCIS_ZoomTo && pViewport->cameraInput.previousLockTime < currentTime - timeout)
  {
    pViewport->cameraInput.inputState = vcCIS_None;
  }

  if (pViewport->cameraInput.inputState == vcCIS_ZoomTo && pViewport->cameraInput.smoothTranslation != udDouble3::zero())
    pViewport->worldAnchorPoint = pViewport->worldMousePosCartesian;

  // Apply movement and rotation
  pViewport->cameraInput.keyboardInput = keyboardInput;
  pViewport->cameraInput.mouseInput = mouseInput;

  vcCamera_Apply(pProgramState, pViewport, &pProgramState->settings.camera, pProgramState->deltaTime);

  // Calculate camera surface position
  bool keepCameraAboveSurface = pProgramState->settings.maptiles.mapEnabled && pProgramState->settings.camera.keepAboveSurface;
  float cameraGroundBufferDistanceMeters = keepCameraAboveSurface ? 5.0f : 0.0f;
  udDouble3 cameraSurfacePosition = vcRender_QueryMapAtCartesian(pViewport->pRenderContext, pViewport->camera.position);
  cameraSurfacePosition += pViewport->camera.cameraUp * cameraGroundBufferDistanceMeters;

  // Calculate if camera is underneath earth surface
  pViewport->camera.cameraIsUnderSurface = (pProgramState->geozone.projection != udGZPT_Unknown) && (udDot3(pViewport->camera.cameraUp, udNormalize3(pViewport->camera.position - cameraSurfacePosition)) < 0);
  if (keepCameraAboveSurface && pViewport->camera.cameraIsUnderSurface)
  {
    pViewport->camera.cameraIsUnderSurface = false;
    pViewport->camera.position = cameraSurfacePosition;

    // TODO: re-orient camera during orbit control to correctly focus on worldAnchorPoint
  }

  if (isFocused && pViewport->cameraInput.inputState == vcCIS_None)
    pViewport->isUsingAnchorPoint = false;

  vcCamera_UpdateMatrices(pProgramState->geozone, &pViewport->camera, pProgramState->settings.camera, pProgramState->activeViewportIndex, windowSize, &mousePos);
}
