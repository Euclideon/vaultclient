#include "vcCamera.h"
#include "vcState.h"

#include "imgui.h"

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

const char** vcCamera_GetLensNames()
{
  return lensNameArray;
}

void vcCamera_Create(vcCamera **ppCamera)
{
  if (ppCamera == nullptr)
    return;

  vcCamera *pCamera = udAllocType(vcCamera, 1, udAF_None);
  pCamera->position = udDouble3::zero();
  pCamera->eulerRotation = udDouble3::zero();

  *ppCamera = pCamera;
}

void vcCamera_Destroy(vcCamera **ppCamera)
{
  if(ppCamera != nullptr)
    udFree(*ppCamera);
}

udDouble4x4 vcCamera_GetMatrix(vcCamera *pCamera)
{
  udQuaternion<double> orientation = udQuaternion<double>::create(pCamera->eulerRotation);
  udDouble3 lookPos = pCamera->position + orientation.apply(udDouble3::create(0.0, 1.0, 0.0));
  return udDouble4x4::lookAt(pCamera->position, lookPos, orientation.apply(udDouble3::create(0.0, 0.0, 1.0)));
}

udDouble3 vcCamera_CreateStoredRotation(vcCamera *pCamera, udDouble3 orbitPosition)
{
  udQuaternion<double> orientation = udQuaternion<double>::create(pCamera->eulerRotation);
  udDouble3 orbitToCamera = orientation.inverse().apply(pCamera->position - orbitPosition); // convert the point into camera space

  double pitch = -udATan(orbitToCamera.z / orbitToCamera.y);
  double yaw = udATan(orbitToCamera.x / orbitToCamera.y);

  return udDouble3::create(yaw, pitch, 0.0);
}

void vcCamera_HandleSceneInput(vcState *pProgramState, udDouble3 oscMove)
{
  ImGuiIO &io = ImGui::GetIO();

  udDouble3 keyboardInput = udDouble3::zero();
  udDouble3 mouseInput = udDouble3::zero();

  // bring in values
  keyboardInput += oscMove;

  float speedModifier = 1.f;

  bool isHovered = ImGui::IsItemHovered();
  static bool isFocused = false;

  bool isBtnClicked[3] = { ImGui::IsMouseClicked(0, false), ImGui::IsMouseClicked(1, false), ImGui::IsMouseClicked(2, false) };
  bool isBtnDoubleClicked[3] = { ImGui::IsMouseDoubleClicked(0), ImGui::IsMouseDoubleClicked(1), ImGui::IsMouseDoubleClicked(2) };
  bool isBtnHeld[3] = { ImGui::IsMouseDown(0), ImGui::IsMouseDown(1), ImGui::IsMouseDown(2) };
  bool isBtnReleased[3] = { ImGui::IsMouseReleased(0), ImGui::IsMouseReleased(1), ImGui::IsMouseReleased(2) };

  if (isHovered && (isBtnClicked[0] || isBtnClicked[1] || isBtnClicked[2]))
    isFocused = true;

  if (!isHovered && (isBtnClicked[0] || isBtnClicked[1] || isBtnClicked[2]))
    isFocused = false;

  if (io.KeyCtrl)
    speedModifier *= 0.1f;

  if (io.KeyShift)
    speedModifier *= 10.f;

  if (!ImGui::GetIO().WantCaptureKeyboard || isFocused)
  {
    keyboardInput.y += io.KeysDown[SDL_SCANCODE_W] - io.KeysDown[SDL_SCANCODE_S];
    keyboardInput.x += io.KeysDown[SDL_SCANCODE_D] - io.KeysDown[SDL_SCANCODE_A];
    keyboardInput.z += io.KeysDown[SDL_SCANCODE_R] - io.KeysDown[SDL_SCANCODE_F];
  }

  if (isFocused)
  {
    ImVec2 mouseDelta = io.MouseDelta;

    if (keyboardInput != udDouble3::zero() || keyboardInput != udDouble3::zero() || isBtnClicked[0] || isBtnClicked[1] || isBtnClicked[2]) // if input is detected, TODO: add proper any input detection
    {
      if (pProgramState->cameraInput.inputState == vcCIS_MovingToPoint)
        pProgramState->cameraInput.inputState = vcCIS_None;
    }

    for (int i = 0; i < 3; ++i)
    {
      // Single Clicking
      if (isBtnClicked[i])
      {
        switch (pProgramState->settings.camera.cameraMouseBindings[i])
        {
        case vcCPM_Orbit:
          if (pProgramState->pickingSuccess)
          {
            pProgramState->cameraInput.isUsingAnchorPoint = true;
            pProgramState->cameraInput.worldAnchorPoint = pProgramState->worldMousePos;
            pProgramState->cameraInput.storedRotation = vcCamera_CreateStoredRotation(pProgramState->pCamera, pProgramState->cameraInput.worldAnchorPoint);
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
          pProgramState->cameraInput.inputState = vcCIS_Panning;
          break;
        }
      }

      // Click and Hold

      if (isBtnHeld[i] && !isBtnClicked[i])
      {
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
        if (pProgramState->pickingSuccess)
        {
          pProgramState->cameraInput.inputState = vcCIS_MovingToPoint;
          pProgramState->cameraInput.startPosition = vcCamera_GetMatrix(pProgramState->pCamera).axis.t.toVector3();
          pProgramState->cameraInput.worldAnchorPoint = pProgramState->worldMousePos;
          pProgramState->cameraInput.progress = 0.0;
        }
      }
    }

    // Mouse Wheel
    const double timeout = 0.25; // How long you have to stop scrolling the scroll wheel before the point unlocks
    static double previousLockTime = 0.0;
    double currentTime = ImGui::GetTime();

    if (io.MouseWheel != 0)
    {
      if (previousLockTime < currentTime - timeout && pProgramState->pickingSuccess && pProgramState->cameraInput.inputState == vcCIS_None)
      {
        pProgramState->cameraInput.isUsingAnchorPoint = true;
        pProgramState->cameraInput.worldAnchorPoint = pProgramState->worldMousePos;
        pProgramState->cameraInput.inputState = vcCIS_CommandZooming;
      }

      if (pProgramState->cameraInput.inputState == vcCIS_CommandZooming)
      {
        pProgramState->cameraInput.storedRotation = vcCamera_CreateStoredRotation(pProgramState->pCamera, pProgramState->cameraInput.worldAnchorPoint);

        mouseInput.x = 0.0;
        mouseInput.y = io.MouseWheel / 10.f;
        mouseInput.z = 0.0;
        previousLockTime = currentTime;
      }
    }
    else if (pProgramState->cameraInput.inputState == vcCIS_CommandZooming && previousLockTime < currentTime - timeout)
    {
      pProgramState->cameraInput.inputState = vcCIS_None;
    }
  }

  // set pivot to send to apply function
  pProgramState->cameraInput.currentPivotMode = vcCPM_Tumble;
  if (io.MouseDown[0] && !io.MouseDown[1] && !io.MouseDown[2])
    pProgramState->cameraInput.currentPivotMode = pProgramState->settings.camera.cameraMouseBindings[0];

  if (!io.MouseDown[0] && io.MouseDown[1] && !io.MouseDown[2])
    pProgramState->cameraInput.currentPivotMode = pProgramState->settings.camera.cameraMouseBindings[1];

  if (!io.MouseDown[0] && !io.MouseDown[1] && io.MouseDown[2])
    pProgramState->cameraInput.currentPivotMode = pProgramState->settings.camera.cameraMouseBindings[2];

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

  // Set outputs
  pProgramState->camMatrix = vcCamera_GetMatrix(pProgramState->pCamera);
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

    if (pCamSettings->moveMode == vcCMM_Plane)
      addPos = (udDouble4x4::rotationYPR(pCamera->eulerRotation) * udDouble4::create(addPos, 1)).toVector3();

    if (pCamSettings->moveMode == vcCMM_Helicopter)
    {
      addPos = (udDouble4x4::rotationYPR(udDouble3::create(pCamera->eulerRotation.x, 0.0, 0.0)) * udDouble4::create(addPos, 1)).toVector3();
      addPos.z = 0.0; // might be unnecessary now
      if (addPos.x != 0.0 || addPos.y != 0.0)
        addPos = udNormalize3(addPos);
    }

    addPos.z += vertPos;
    addPos *= pCamSettings->moveSpeed * speedModifier * deltaTime;

    pCamera->position += addPos;

    // Rotation
    if (pCamSettings->invertX)
      pCamInput->mouseInput.x *= -1.0;
    if (pCamSettings->invertY)
      pCamInput->mouseInput.y *= -1.0;

    pCamera->eulerRotation += pCamInput->mouseInput;
    pCamera->eulerRotation.y = udClamp(pCamera->eulerRotation.y, (double)-UD_PI / 2.0, (double)UD_PI / 2.0);
  }
  break;

  case vcCIS_Orbiting:
  {
    double distanceToPoint = udMag3(pCamInput->worldAnchorPoint - pCamera->position);

    if (distanceToPoint != 0.0)
    {
      udRay<double> transform;
      transform.position = pCamera->position;
      transform.orientation = udDoubleQuat::create(pCamera->eulerRotation);

      transform = udRotateAround(transform, pCamInput->worldAnchorPoint, { 0, 0, 1 }, pCamInput->mouseInput.x);
      transform = udRotateAround(transform, pCamInput->worldAnchorPoint, transform.orientation.apply({ 1, 0, 0 }), -pCamInput->mouseInput.y);

      udDouble3 euler = transform.orientation.eulerAngles();

      // Only apply if not flipped over the top
      if (euler.z < UD_HALF_PI && euler.z > -UD_HALF_PI)
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

  case vcCIS_MovingToPoint:
  {
    udDouble3 travelVector = pCamInput->worldAnchorPoint - pCamInput->startPosition;

    double length = udMag3(travelVector);

    travelVector *= udMax(0.9, (length-100.0) / length); // gets to either 90% or within 100m

    double travelProgress = 0;

    pCamInput->progress += deltaTime / 2; // 2 second travel time
    if (pCamInput->progress > 1.0)
    {
      pCamInput->progress = 1.0;
      pCamInput->inputState = vcCIS_None;
    }

    double t = pCamInput->progress;
    if (t < 0.5)
      travelProgress = 4 * t * t * t; // cubic
    else
      travelProgress = (t - 1)*(2 * t - 2)*(2 * t - 2) + 1; // cubic

    pCamera->position = pCamInput->startPosition + travelVector * travelProgress;
  }
  break;

  case vcCIS_CommandZooming:
  {
    double distanceToPoint = udMag3(pCamInput->worldAnchorPoint - pCamera->position);
    udDouble3 towards = udNormalize3(pCamInput->worldAnchorPoint - pCamera->position);
    udDouble3 addPos = distanceToPoint * pCamInput->mouseInput.y * towards;
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
    // Regular keyboard input
    udDouble3 addPos = udClamp(pCamInput->keyboardInput, udDouble3::create(-1, -1, -1), udDouble3::create(1, 1, 1)); // clamp in case 2 similarly mapped movement buttons are pressed
    double vertPos = addPos.z;
    addPos.z = 0.0;

    if (pCamSettings->moveMode == vcCMM_Plane)
      addPos = (udDouble4x4::rotationYPR(pCamera->eulerRotation) * udDouble4::create(addPos, 1)).toVector3();

    if (pCamSettings->moveMode == vcCMM_Helicopter)
    {
      addPos = (udDouble4x4::rotationYPR(udDouble3::create(pCamera->eulerRotation.x, 0.0, 0.0)) * udDouble4::create(addPos, 1)).toVector3();
      addPos.z = 0.0; // might be unnecessary now
      if (addPos.x != 0.0 || addPos.y != 0.0)
        addPos = udNormalize3(addPos);
    }

    addPos.z += vertPos;
    addPos *= pCamSettings->moveSpeed * speedModifier * deltaTime;

    pCamera->position += addPos;

    // Mouse input
    if (pCamSettings->invertX)
      pCamInput->mouseInput.x *= -1.0;
    if (pCamSettings->invertY)
      pCamInput->mouseInput.y *= -1.0;

    addPos = pCamInput->mouseInput;

    addPos *= pCamSettings->moveSpeed * speedModifier * deltaTime;

    // Swap y and z - input is in screen space (XY), position is in UD space (XZ)
    double temp = -addPos.y; // Invert to match x
    addPos.y = addPos.z;
    addPos.z = temp;

    pCamera->position += udDoubleQuat::create(pCamera->eulerRotation).apply(addPos);
  }
  break;

  case vcCIS_Count:
    break; // to cover all implemented cases
  }
}
