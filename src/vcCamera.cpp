#include "vcCamera.h"
#include "vcState.h"
#include "vcPOI.h"
#include "vcHotkey.h"
#include "vcRender.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_ex/ImGuizmo.h"
#include "udStringUtil.h"

#define ONE_PIXEL_SQ 0.0001

// higher == quicker smoothing
static const double sCameraTranslationSmoothingSpeed = 22.0;
static const double sCameraRotationSmoothingSpeed = 40.0;

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

void vcCamera_UpdateSmoothing(vcCamera *pCamera, vcCameraInput *pCamInput, double deltaTime)
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
      pProgramState->anchorMouseRay = pProgramState->camera.worldMouseRay;
      pProgramState->cameraInput.inputState = vcCIS_Panning;
    }
    break;
  case vcCPM_Forward:
    pProgramState->cameraInput.inputState = vcCIS_MovingForward;
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

void vcCamera_UpdateMatrices(const udGeoZone &zone, vcCamera *pCamera, const vcCameraSettings &settings, const udFloat2 &windowSize, const udFloat2 *pMousePos /*= nullptr*/)
{
  // Update matrices
  double fov = settings.fieldOfView;
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

void vcCamera_Apply(vcState *pProgramState, vcCamera *pCamera, vcCameraSettings *pCamSettings, vcCameraInput *pCamInput, double deltaTime)
{
  udDouble3 worldAnchorNormal = vcGIS_GetWorldLocalUp(pProgramState->geozone, pProgramState->worldAnchorPoint);
  udDoubleQuat orientation = vcGIS_HeadingPitchToQuaternion(pProgramState->geozone, pCamera->position, pCamera->headingPitch);

  pCamera->cameraUp = vcGIS_GetWorldLocalUp(pProgramState->geozone, pProgramState->camera.position);
  pCamera->cameraNorth = vcGIS_GetWorldLocalNorth(pProgramState->geozone, pProgramState->camera.position);

  switch (pCamInput->inputState)
  {
  case vcCIS_MovingForward:
    pCamInput->keyboardInput.y += 1;
    // fall through
  case vcCIS_None:
  {
    udDouble3 addPos = (pCamInput->keyboardInput + pCamInput->controllerDPADInput);

    double vertPos = addPos.z;

    addPos.z = 0.0;
    addPos = orientation.apply(addPos);

    // Translation
    if (pCamSettings->lockAltitude && addPos != udDouble3::zero())
      addPos -= udDot(pCamera->cameraUp, addPos) * pCamera->cameraUp;

    if (addPos != udDouble3::zero())
      addPos = udNormalize3(addPos);

    addPos += vertPos * pCamera->cameraUp;
    addPos *= pCamSettings->moveSpeed * deltaTime;

    pCamInput->smoothTranslation += addPos;

    // Check for a nan camera position and reset to zero, this allows the UI to be usable in the event of error
    if (isnan(pCamera->position.x) || isnan(pCamera->position.y) || isnan(pCamera->position.z))
      pCamera->position = udDouble3::zero();

    udDouble2 rotation = udDouble2::create(-pCamInput->mouseInput.x, pCamInput->mouseInput.y) * 0.5; // Because this messes with HEADING, has to be reversed
    udDouble2 result = pCamera->headingPitch + rotation;
    if (result.y > UD_HALF_PI)
      rotation.y = UD_HALF_PI - pCamera->headingPitch.y;
    else if (result.y < -UD_HALF_PI)
      rotation.y = -UD_HALF_PI - pCamera->headingPitch.y;

    pCamInput->smoothRotation += rotation;
  }
  break;

  case vcCIS_Orbiting:
  {
    double distanceToPointSqr = udMagSq3(pProgramState->worldAnchorPoint - pCamera->position);
    if (distanceToPointSqr != 0.0 && (pCamInput->mouseInput.x != 0 || pCamInput->mouseInput.y != 0))
    {
      // Orbit Left/Right
      if (pCamInput->mouseInput.x != 0)
      {
      udDoubleQuat rotation = udDoubleQuat::create(worldAnchorNormal, pCamInput->mouseInput.x);
      udDouble3 direction = pCamera->position - pProgramState->worldAnchorPoint; // find current direction relative to center

      orientation = (rotation * orientation);

      pCamera->position = pProgramState->worldAnchorPoint + rotation.apply(direction); // define new position
      pCamera->headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->geozone, pProgramState->camera.position, orientation);
      }

      //
      if (pCamera->headingPitch.y < UD_DEG2RADf(-89.0) && pCamInput->mouseInput.y <= 0)
        break;
      if (pCamera->headingPitch.y > UD_DEG2RADf(89.0) && pCamInput->mouseInput.y >= 0)
        break;

      // Orbit Up/Down
      udDoubleQuat rotation = udDoubleQuat::create(orientation.apply({ 1, 0, 0 }), pCamInput->mouseInput.y);
      udDouble3 direction = pCamera->position - pProgramState->worldAnchorPoint; // find current direction relative to center

      orientation = (rotation * orientation);

      // Save it back to the camera
      pCamera->position = pProgramState->worldAnchorPoint + rotation.apply(direction); // define new position
      pCamera->headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->geozone, pProgramState->camera.position, orientation);

    }
  }
  break;

  case vcCIS_MovingToPoint:
  {
    udDouble3 moveVector = pProgramState->worldAnchorPoint - pCamInput->startPosition;

    if (moveVector == udDouble3::zero())
      break;

    if (pCamSettings->lockAltitude)
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

    if (pCamera->headingPitch.y > UD_PI)
      pCamera->headingPitch.y -= UD_2PI;
  }
  break;

  case vcCIS_MoveToViewpoint:
  {
    udDouble3 moveVector = pProgramState->worldAnchorPoint - pCamInput->startPosition;
    pCamInput->progress += deltaTime / 2; // 2 second travel time

    if (pCamInput->progress > 1.0)
    {
      pCamInput->targetAngle = udDoubleQuat::identity();
      pCamInput->progress = 1.0;

      pCamInput->inputState = vcCIS_None;
      pProgramState->camera.headingPitch = pCamInput->headingPitch;
      pCamera->position = pProgramState->worldAnchorPoint;
      
      break;
    }

    double travelProgress = udEase(pCamInput->progress, udET_CubicInOut);
    pCamera->position = pCamInput->startPosition + moveVector * travelProgress;
    pCamera->headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->geozone, pCamera->position, udSlerp(pCamInput->startAngle, pCamInput->targetAngle, travelProgress));
    if (pCamera->headingPitch.y > UD_PI)
      pCamera->headingPitch.y -= UD_2PI;
  }
  break;

  case vcCIS_ZoomTo:
  {
    udDouble3 addPos = udDouble3::zero();
    udDouble3 towards = pProgramState->worldAnchorPoint - pCamera->position;
    if (udMagSq3(towards) > 0)
    {
      double maxDistance = 0.9 * pCamSettings->farPlane; // limit to 90% of visible distance
      double distanceToPoint = udMin(udMag3(towards), maxDistance);

      addPos = distanceToPoint * pCamInput->mouseInput.y * udNormalize3(towards);
    }

    pCamInput->smoothTranslation += addPos;
  }
  break;

  case vcCIS_Rotate:
  {
    // Not good to use udSlerp because the camera may keep unexpected rolling angle if smooth interrupted by mouse button clicked.
    pCamInput->progress += deltaTime;
    if (pCamInput->progress > 1.0)
    {
      pProgramState->camera.headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->geozone, pCamera->position, pCamInput->targetAngle);
      pCamInput->targetAngle = udDoubleQuat::identity();
      pCamInput->inputState = vcCIS_None;
      pCamInput->progress = 1.0;
    }
    else
    {
      pCamera->headingPitch = vcGIS_QuaternionToHeadingPitch(pProgramState->geozone, pCamera->position, udSlerp(pCamInput->startAngle, pCamInput->targetAngle, pCamInput->progress));
    }

    if (pCamera->headingPitch.y > UD_PI)
      pCamera->headingPitch.y -= UD_2PI;
  }
  break;

  case vcCIS_PinchZooming:
  {
    // TODO
  }
  break;

  case vcCIS_Panning:
  {
    udPlane<double> plane = udPlane<double>::create(pProgramState->worldAnchorPoint, worldAnchorNormal);

    if (!pCamSettings->lockAltitude) // generate a plane facing the camera
      plane.normal = orientation.apply({ 0, 1, 0 });

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
    pCamInput->progress += deltaTime * 2; // .5 second stabilize time
    if (pCamInput->progress >= 1.0)
    {
      pCamInput->progress = 1.0;
      pCamInput->stabilize = false;
    }

    if (pCamera->headingPitch.y > UD_PI)
      pCamera->headingPitch.y -= UD_2PI;
  }

  vcCamera_UpdateSmoothing(pCamera, pCamInput, deltaTime);
}

void vcCamera_HandleSceneInput(vcState *pProgramState, udDouble3 oscMove, udFloat2 windowSize, udFloat2 mousePos)
{
  ImGuiIO &io = ImGui::GetIO();

  udDouble3 keyboardInput = udDouble3::zero();
  udDouble3 mouseInput = udDouble3::zero();

  // bring in values
  keyboardInput += oscMove;

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
  bool isFocused = (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || isMouseBtnBeingHeld) && !vcGizmo_IsActive() && !pProgramState->modalOpen;

  int totalButtonsHeld = 0;
  for (size_t i = 0; i < udLengthOf(isBtnHeld); ++i)
    totalButtonsHeld += isBtnHeld[i] ? 1 : 0;

  // Start hold time
  if (isFocused && (isBtnClicked[0] || isBtnClicked[1] || isBtnClicked[2]))
  {
    isMouseBtnBeingHeld = true;
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
      if (pProgramState->gizmo.operation == vcGO_Scale || pProgramState->gizmo.coordinateSystem == vcGCS_Local)
      {
        pProgramState->gizmo.direction[0] = udDouble3::create(1, 0, 0);
        pProgramState->gizmo.direction[1] = udDouble3::create(0, 1, 0);
        pProgramState->gizmo.direction[2] = udDouble3::create(0, 0, 1);
      }
      else
      {
        vcGIS_GetOrthonormalBasis(pProgramState->geozone, pItem->GetWorldSpacePivot(), &pProgramState->gizmo.direction[2], &pProgramState->gizmo.direction[1], &pProgramState->gizmo.direction[0]);
      }
    }
  }
  gizmoCapturedMouse = gizmoCapturedMouse || (pProgramState->gizmo.operation != 0 && vcGizmo_IsHovered(pProgramState->gizmo.direction) && (isBtnClicked[0] || isBtnClicked[1] || isBtnClicked[2]));
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
    mouseInput.x = (pProgramState->settings.camera.invertMouseX ? mouseDelta.x : -mouseDelta.x) / 100.0;
    mouseInput.y = (pProgramState->settings.camera.invertMouseY ? mouseDelta.y : -mouseDelta.y) / 100.0;
    mouseInput.z = 0.0;
  }

  // Controller Input
  if (io.NavActive)
  {
    keyboardInput.x += io.NavInputs[ImGuiNavInput_LStickLeft]; // Left Stick Horizontal
    keyboardInput.y += -io.NavInputs[ImGuiNavInput_LStickUp]; // Left Stick Vertical

    mouseInput.x += (pProgramState->settings.camera.invertControllerX ? io.NavInputs[ImGuiNavInput_LStickRight] : -io.NavInputs[ImGuiNavInput_LStickRight]) / 15.0f; // Right Stick Horizontal
    mouseInput.y += (pProgramState->settings.camera.invertControllerY ? io.NavInputs[ImGuiNavInput_LStickDown] : -io.NavInputs[ImGuiNavInput_LStickDown]) / 25.0f; // Right Stick Vertical

    // In Imgui the DPAD is bound to navigation, so disable DPAD panning until the issue is resolved
    //pProgramState->cameraInput.controllerDPADInput = udDouble3::create(io.NavInputs[ImGuiNavInput_DpadRight] - io.NavInputs[ImGuiNavInput_DpadLeft], 0, io.NavInputs[ImGuiNavInput_DpadUp] - io.NavInputs[ImGuiNavInput_DpadDown]);

    if (isRightTriggerHeld)
    {
      if (pProgramState->pickingSuccess && pProgramState->cameraInput.inputState == vcCIS_None)
      {
        pProgramState->isUsingAnchorPoint = true;
        pProgramState->worldAnchorPoint = pProgramState->worldMousePosCartesian;
        pProgramState->cameraInput.inputState = vcCIS_Orbiting;
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

    if (io.NavInputs[ImGuiNavInput_Activate] && !io.NavInputsDownDuration[ImGuiNavInput_Activate]) // A Button
      pProgramState->settings.camera.lockAltitude = !pProgramState->settings.camera.lockAltitude;
  }

  if (vcHotkey::IsPressed(vcB_DecreaseCameraSpeed))
  {
    pProgramState->settings.camera.moveSpeed *= 0.1f;
    pProgramState->settings.camera.moveSpeed = udMax(pProgramState->settings.camera.moveSpeed, vcSL_CameraMinMoveSpeed);
  }
  else if (vcHotkey::IsPressed(vcB_IncreaseCameraSpeed))
  {
    pProgramState->settings.camera.moveSpeed *= 10.f;
    pProgramState->settings.camera.moveSpeed = udMin(pProgramState->settings.camera.moveSpeed, vcSL_CameraMaxMoveSpeed);
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
      pProgramState->gizmo.operation = ((pProgramState->gizmo.operation == vcGO_Translate) ? vcGO_NoGizmo : vcGO_Translate);
    if (vcHotkey::IsPressed(vcB_GizmoRotate))
      pProgramState->gizmo.operation = ((pProgramState->gizmo.operation == vcGO_Rotate) ? vcGO_NoGizmo : vcGO_Rotate);
    if (vcHotkey::IsPressed(vcB_GizmoScale))
      pProgramState->gizmo.operation = ((pProgramState->gizmo.operation == vcGO_Scale) ? vcGO_NoGizmo : vcGO_Scale);
    if (vcHotkey::IsPressed(vcB_GizmoLocalSpace))
      pProgramState->gizmo.coordinateSystem = ((pProgramState->gizmo.coordinateSystem == vcGCS_Scene) ? vcGCS_Local : vcGCS_Scene);
  }

  if (isFocused)
  {
    if (keyboardInput != udDouble3::zero() || isBtnClicked[0] || isBtnClicked[1] || isBtnClicked[2]) // if input is detected, TODO: add proper any input detection
    {
      if (pProgramState->cameraInput.inputState == vcCIS_MovingToPoint)
      {
        pProgramState->cameraInput.stabilize = true;
        pProgramState->cameraInput.progress = 0;
        pProgramState->cameraInput.inputState = vcCIS_None;
      }
    }
  }

  for (int i = 0; i < 3; ++i)
  {
    // Single Clicking
    if (isBtnClicked[i] && (pProgramState->cameraInput.inputState == vcCIS_None || totalButtonsHeld == 1)) // immediately override current input if this is a new button down
      vcCamera_BeginCameraPivotModeMouseBinding(pProgramState, i);

    if (isBtnReleased[i])
    {
      if ((pProgramState->settings.camera.cameraMouseBindings[i] == vcCPM_Orbit && pProgramState->cameraInput.inputState == vcCIS_Orbiting) ||
          (pProgramState->settings.camera.cameraMouseBindings[i] == vcCPM_Pan && pProgramState->cameraInput.inputState == vcCIS_Panning) ||
          (pProgramState->settings.camera.cameraMouseBindings[i] == vcCPM_Forward && pProgramState->cameraInput.inputState == vcCIS_MovingForward) ||
            pProgramState->cameraInput.inputState == vcCIS_ZoomTo)
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
  }

  // Double Clicking left mouse
  if (isBtnDoubleClicked[0] && pProgramState->pickingSuccess)
  {
    pProgramState->cameraInput.inputState = vcCIS_MovingToPoint;
    pProgramState->cameraInput.startPosition = pProgramState->camera.position;
    pProgramState->cameraInput.startAngle = vcGIS_HeadingPitchToQuaternion(pProgramState->geozone, pProgramState->camera.position, pProgramState->camera.headingPitch);
    pProgramState->cameraInput.progress = 0.0;

    pProgramState->isUsingAnchorPoint = true;
    pProgramState->worldAnchorPoint = pProgramState->worldMousePosCartesian;
  }

  // Mouse Wheel
  const double defaultTimeouts = 0.25;
  double timeout = defaultTimeouts; // How long you have to stop scrolling the scroll wheel before the point unlocks
  static double previousLockTime = 0.0;
  double currentTime = ImGui::GetTime();
  bool zooming = false;

  if (mouseWheel != 0)
  {
    zooming = true;
    if (pProgramState->settings.camera.scrollWheelMode == vcCSWM_Dolly)
    {
      if (previousLockTime < currentTime - timeout && (pProgramState->pickingSuccess) && pProgramState->cameraInput.inputState == vcCIS_None)
      {
        pProgramState->isUsingAnchorPoint = true;
        pProgramState->worldAnchorPoint = pProgramState->worldMousePosCartesian;
        pProgramState->cameraInput.inputState = vcCIS_ZoomTo;
      }

      if (pProgramState->cameraInput.inputState == vcCIS_ZoomTo)
      {
        mouseInput.x = 0.0;
        mouseInput.y = mouseWheel / 5.0;
        mouseInput.z = 0.0;
        previousLockTime = currentTime;

        pProgramState->cameraInput.startPosition = pProgramState->camera.position;
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

  if (!zooming && pProgramState->cameraInput.inputState == vcCIS_ZoomTo && previousLockTime < currentTime - timeout)
  {
    pProgramState->cameraInput.inputState = vcCIS_None;
  }

  if (pProgramState->cameraInput.inputState == vcCIS_ZoomTo && pProgramState->cameraInput.smoothTranslation != udDouble3::zero())
    pProgramState->worldAnchorPoint = pProgramState->worldMousePosCartesian;

  // Apply movement and rotation
  pProgramState->cameraInput.keyboardInput = keyboardInput;
  pProgramState->cameraInput.mouseInput = mouseInput;

  vcCamera_Apply(pProgramState, &pProgramState->camera, &pProgramState->settings.camera, &pProgramState->cameraInput, pProgramState->deltaTime);

  // Calculate camera surface position
  bool keepCameraAboveSurface = pProgramState->settings.maptiles.mapEnabled && pProgramState->settings.camera.keepAboveSurface;
  float cameraGroundBufferDistanceMeters = keepCameraAboveSurface ? 5.0f : 0.0f;
  udDouble3 cameraSurfacePosition = vcRender_QueryMapAtCartesian(pProgramState->pRenderContext, pProgramState->camera.position);
  cameraSurfacePosition += pProgramState->camera.cameraUp * cameraGroundBufferDistanceMeters;

  // Calculate if camera is underneath earth surface
  pProgramState->camera.cameraIsUnderSurface = (pProgramState->geozone.projection != udGZPT_Unknown) && (udDot3(pProgramState->camera.cameraUp, udNormalize3(pProgramState->camera.position - cameraSurfacePosition)) < 0);
  if (keepCameraAboveSurface && pProgramState->camera.cameraIsUnderSurface)
  {
    pProgramState->camera.cameraIsUnderSurface = false;
    pProgramState->camera.position = cameraSurfacePosition;

    // TODO: re-orient camera during orbit control to correctly focus on worldAnchorPoint
  }

  if (pProgramState->cameraInput.inputState == vcCIS_None)
    pProgramState->isUsingAnchorPoint = false;

  vcCamera_UpdateMatrices(pProgramState->geozone, &pProgramState->camera, pProgramState->settings.camera, windowSize, &mousePos);
}
