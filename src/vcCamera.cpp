#include "vcCamera.h"
#include "vcState.h"

#include "imgui.h"

struct vcCamera
{
  udDouble3 position;
  udDouble3 yprRotation;
};

#define LENSNAME(x) #x+5

const char *lensNameArray[] =
{
  LENSNAME(vcLS_Custom),
  LENSNAME(vcLS_7mm),
  LENSNAME(vcLS_11mm),
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
  pCamera->yprRotation = udDouble3::zero();

  *ppCamera = pCamera;
}

void vcCamera_Destroy(vcCamera **ppCamera)
{
  if(ppCamera != nullptr)
    udFree(*ppCamera);
}

udDouble4x4 vcCamera_GetMatrix(vcCamera *pCamera)
{
  udQuaternion<double> orientation = udQuaternion<double>::create(pCamera->yprRotation);
  udDouble3 lookPos = pCamera->position + orientation.apply(udDouble3::create(0.0, 1.0, 0.0));
  return udDouble4x4::lookAt(pCamera->position, lookPos, orientation.apply(udDouble3::create(0.0, 0.0, 1.0)));
}

udDouble3 vcCamera_CreateStoredRotation(vcCamera *pCamera, udDouble3 orbitPosition)
{
  udQuaternion<double> orientation = udQuaternion<double>::create(pCamera->yprRotation);
  udDouble3 orbitToCamera = orientation.inverse().apply(pCamera->position - orbitPosition); // convert the point into camera space

  double pitch = -udATan(orbitToCamera.z / orbitToCamera.y);
  double yaw = udATan(orbitToCamera.x / orbitToCamera.y);

  return udDouble3::create(yaw, pitch, 0.0);
}



udDouble3 vcCamera_GetPosition(vcCamera *pCamera)
{
  return pCamera->position;
}

void vcCamera_SetPosition(vcCamera *pCamera, udDouble3 position)
{
  if(pCamera != nullptr)
    pCamera->position = position;
}

void vcCamera_SetRotation(vcCamera *pCamera, udDouble3 yprRotation)
{
  if (pCamera != nullptr)
    pCamera->yprRotation = yprRotation;
}

void vcCamera_HandleSceneInput(vcState *pProgramState, udDouble3 *pMoveOffset, udDouble3 *pRotationOffset)
{
  ImGuiIO &io = ImGui::GetIO();

  udDouble3 moveOffset = udDouble3::zero();
  udDouble3 rotationOffset = udDouble3::zero();

  udDouble3 addPos = udDouble3::zero();

  // bring in values
  moveOffset += *pMoveOffset;

  rotationOffset += *pRotationOffset;

  float speedModifier = 1.f;

  bool isHovered = ImGui::IsItemHovered();
  static bool isFocused = false;

  bool isBtnClicked[3] = { ImGui::IsMouseClicked(0, false), ImGui::IsMouseClicked(1, false), ImGui::IsMouseClicked(2, false) };
  bool isBtnDoubleClicked[3] = { ImGui::IsMouseDoubleClicked(0), ImGui::IsMouseDoubleClicked(1), ImGui::IsMouseDoubleClicked(2) };
  static bool clickedBtnWhileHovered[3] = { false, false, false };

  for (int i = 0; i < 3; ++i)
  {
    if (isHovered && isBtnClicked[i])
    {
      clickedBtnWhileHovered[i] = true;
      isFocused = true;
    }
  }

  if (!isHovered && (isBtnClicked[0] || isBtnClicked[1] || isBtnClicked[2]))
    isFocused = false;

  if (io.KeyCtrl)
    speedModifier *= 0.1f;

  if (io.KeyShift)
    speedModifier *= 10.f;

  if (isFocused)
  {
    ImVec2 mouseDelta = io.MouseDelta;

    addPos.y += io.KeysDown[SDL_SCANCODE_W] - io.KeysDown[SDL_SCANCODE_S];
    addPos.x += io.KeysDown[SDL_SCANCODE_D] - io.KeysDown[SDL_SCANCODE_A];
    addPos.z += io.KeysDown[SDL_SCANCODE_R] - io.KeysDown[SDL_SCANCODE_F];

    // keyboard controls for switching control modes
    switch (pProgramState->cameraInput.controlMode)
    {
    case vcCM_Normal:
      if (ImGui::IsKeyPressed(SDL_SCANCODE_Z, false))
      {
        pProgramState->cameraInput.controlMode = vcCM_Zoom;
        pProgramState->cameraInput.previousFoV = pProgramState->settings.camera.fieldOfView;
      }

      if (ImGui::IsKeyPressed(SDL_SCANCODE_M, false))
        pProgramState->cameraInput.controlMode = vcCM_Measure;

      break;
    case vcCM_Zoom:
      if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE, false))
      {
        pProgramState->cameraInput.controlMode = vcCM_Normal;
        pProgramState->cameraInput.inputState = vcCIS_None;
      }
      if (ImGui::IsKeyPressed(SDL_SCANCODE_LCTRL, false) || ImGui::IsKeyPressed(SDL_SCANCODE_RCTRL, false))
        pProgramState->settings.camera.fieldOfView = pProgramState->cameraInput.previousFoV;

      break;
    case vcCM_Measure:
      if (ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE, false))
      {
        pProgramState->cameraInput.controlMode = vcCM_Normal;
        pProgramState->cameraInput.inputState = vcCIS_None;
      }

      break;
    }


    if (moveOffset != udDouble3::zero() || addPos != udDouble3::zero() || isBtnClicked[0] || isBtnClicked[1] || isBtnClicked[2]) // if input is detected, TODO: add proper any input detection
    {
      if(pProgramState->cameraInput.inputState == vcCIS_MovingToPoint)
      pProgramState->cameraInput.inputState = vcCIS_None;
    }

    switch(pProgramState->cameraInput.controlMode)
    {
    case vcCM_Normal:
      for (int i = 0; i < 3; ++i)
      {
        // Single Clicking
        if (isBtnClicked[i])
        {
          switch (pProgramState->settings.camera.pivotBind[i])
          {
          case vcCPM_Orbit:
            if (pProgramState->pickingSuccess)
            {
              pProgramState->cameraInput.focusPoint = pProgramState->worldMousePos;
              pProgramState->cameraInput.storedRotation = vcCamera_CreateStoredRotation(pProgramState->pCamera, pProgramState->cameraInput.focusPoint);
              pProgramState->cameraInput.inputState = vcCIS_Orbiting;
            }
            break;
          case vcCPM_Tumble: // fall through
          case vcCPM_Pan:
            break;
          }
        }

        // Click and Hold

        if (clickedBtnWhileHovered[i] && !isBtnClicked[i])
        {
          clickedBtnWhileHovered[i] = io.MouseDown[i];
          if (io.MouseDown[i])
          {
            switch (pProgramState->settings.camera.pivotBind[i])
            {
            case vcCPM_Pan:
              moveOffset.x += -mouseDelta.x / 10.0;
              moveOffset.z += -mouseDelta.y / 10.0; // TODO: fix Pan
              break;
            default:
              rotationOffset.x = -mouseDelta.x / 100.0;
              rotationOffset.y = -mouseDelta.y / 100.0;
              rotationOffset.z = 0.0;
              break;
            }
          }
          else
          {
            if (pProgramState->cameraInput.inputState == vcCIS_Orbiting)
              pProgramState->cameraInput.inputState = vcCIS_None;
          }
        }

        // Double Clicking
        if (i == 0 && isBtnDoubleClicked[i]) // if left double clicked
        {
          if (pProgramState->pickingSuccess)
          {
            pProgramState->cameraInput.inputState = vcCIS_MovingToPoint;
            pProgramState->cameraInput.startPosition = vcCamera_GetMatrix(pProgramState->pCamera).axis.t.toVector3();
            pProgramState->cameraInput.focusPoint = pProgramState->worldMousePos;
            pProgramState->cameraInput.progress = 0.0;
          }
        }
      }
      break;
    case vcCM_Zoom:
    {
      if (isBtnClicked[0])
      {
        if (pProgramState->pickingSuccess)
        {
          pProgramState->cameraInput.focusPoint = pProgramState->worldMousePos;
          pProgramState->cameraInput.storedRotation = vcCamera_CreateStoredRotation(pProgramState->pCamera, pProgramState->cameraInput.focusPoint);
          pProgramState->cameraInput.inputState = vcCIS_CommandZooming;
        }
      }

      if (clickedBtnWhileHovered[0] && !isBtnClicked[0])
      {
        clickedBtnWhileHovered[0] = io.MouseDown[0];
        if (io.MouseDown[0])
        {
          if (!io.KeyShift)
          {
            rotationOffset.x = 0.0;
            rotationOffset.y = -mouseDelta.y / 100.0;
            rotationOffset.z = 0.0;
          }
          else
          {
            pProgramState->settings.camera.fieldOfView = (float)udClamp(pProgramState->settings.camera.fieldOfView + mouseDelta.y / 100.0, vcSL_CameraFieldOfViewMin / 180 * UD_PI, vcSL_CameraFieldOfViewMax / 180 * UD_PI);
          }
        }
        else
        {
          pProgramState->cameraInput.inputState = vcCIS_None;
        }
      }
    }
    break;
    case vcCM_Measure:
    {

    }
    break;
    }

    // Mouse Wheel
    if (io.MouseWheel > 0)
    {
      // TODO: Move closer to point
    }
    else if (io.MouseWheel < 0)
    {
      // TODO: Move away from point
    }
  }

  moveOffset += addPos;

  // set pivot to send to apply function
  vcCameraPivotMode applicationPivotMode = vcCPM_Tumble; // this is broken atm, future problem
  if (io.MouseDown[0] && !io.MouseDown[1] && !io.MouseDown[2])
    applicationPivotMode = pProgramState->settings.camera.pivotBind[0];

  if (!io.MouseDown[0] && io.MouseDown[1] && !io.MouseDown[2])
    applicationPivotMode = pProgramState->settings.camera.pivotBind[1];

  if (!io.MouseDown[0] && !io.MouseDown[1] && io.MouseDown[2])
    applicationPivotMode = pProgramState->settings.camera.pivotBind[2];

  // Apply movement and rotation

  vcCamera_Apply(pProgramState->pCamera, &pProgramState->settings.camera, &pProgramState->cameraInput, rotationOffset, moveOffset, pProgramState->deltaTime, speedModifier);

  // Set outputs
  *pMoveOffset = moveOffset;

  *pRotationOffset = rotationOffset;

  pProgramState->camMatrix = vcCamera_GetMatrix(pProgramState->pCamera);
}

void vcCamera_Apply(vcCamera *pCamera, vcCameraSettings *pCamSettings, vcCameraInput *pCamInput, udDouble3 rotationOffset, udDouble3 moveOffset, double deltaTime, float speedModifier /* = 1.f*/)
{
  switch (pCamInput->controlMode)
  {
  case vcCM_Normal:
    switch (pCamInput->inputState)
    {
    case vcCIS_None:
    {
      udDouble3 addPos = udClamp(moveOffset, udDouble3::create(-1, -1, -1), udDouble3::create(1, 1, 1)); // clamp in case 2 similarly mapped movement buttons are pressed
      double vertPos = addPos.z;
      addPos.z = 0.0;

      // Translation

      if (pCamSettings->moveMode == vcCMM_Plane)
        addPos = (udDouble4x4::rotationYPR(pCamera->yprRotation) * udDouble4::create(addPos, 1)).toVector3();

      if (pCamSettings->moveMode == vcCMM_Helicopter)
      {
        addPos = (udDouble4x4::rotationYPR(udDouble3::create(pCamera->yprRotation.x, 0.0, 0.0)) * udDouble4::create(addPos, 1)).toVector3();
        addPos.z = 0.0; // might be unnecessary now
        if (addPos.x != 0.0 || addPos.y != 0.0)
          addPos = udNormalize3(addPos);
      }

      addPos.z += vertPos;
      addPos *= pCamSettings->moveSpeed * speedModifier * deltaTime;

      pCamera->position += addPos;

      // Rotation

      if (pCamSettings->invertX)
        rotationOffset.x *= -1.0;
      if (pCamSettings->invertY)
        rotationOffset.y *= -1.0;

      pCamera->yprRotation += rotationOffset;
      pCamera->yprRotation.y = udClamp(pCamera->yprRotation.y, (double)-UD_PI / 2.0, (double)UD_PI / 2.0);
    }
    break;

    case vcCIS_Orbiting:
    {
      double distanceToPoint = udMag3(pCamInput->focusPoint - pCamera->position);
      udDoubleQuat orientation;
      udDouble3 addPosOrbit = udDouble3::zero();

      if (distanceToPoint != 0.0)
      {
        if (rotationOffset.x != 0.0 || rotationOffset.y != 0.0)
          orientation = udDoubleQuat::create(pCamera->yprRotation - pCamInput->storedRotation);
        else
          orientation = udDoubleQuat::create(pCamera->yprRotation);

        udDouble3 right = orientation.apply(udDouble3::create(1.0, 0.0, 0.0));
        udDouble3 forward = orientation.apply(udDouble3::create(0.0, 1.0, 0.0));

        udDouble3 addPos = udClamp(moveOffset, udDouble3::create(-1, -1, -1), udDouble3::create(1, 1, 1)); // clamp in case 2 similarly mapped movement buttons are pressed

        addPos /= 50.0;
        addPosOrbit = distanceToPoint * udSin(rotationOffset.x + addPos.x) * (right + udTan((rotationOffset.x + addPos.x) / 2.0) * forward);
        addPosOrbit.z = 0;

        if (addPos.y == 0.0)
          addPosOrbit += distanceToPoint * rotationOffset.y * forward;
        else
          addPosOrbit += distanceToPoint * (rotationOffset.y + addPos.y) * udDoubleQuat::create(pCamera->yprRotation - pCamInput->storedRotation).apply(udDouble3::create(0.0, 1.0, 0.0));

        pCamera->position += addPosOrbit;

        // Rotation

        udDouble3 nanProtection = pCamera->yprRotation;
        pCamera->yprRotation = udDouble4x4::lookAt(pCamera->position, pCamInput->focusPoint, orientation.apply(udDouble3::create(0.0, 0.0, 1.0))).extractYPR();
        pCamera->yprRotation += pCamInput->storedRotation;

        if (pCamera->yprRotation.y > UD_PI / 2.0)
          pCamera->yprRotation.y -= UD_2PI; // euler vs +/-

        pCamera->yprRotation.z = 0.0;

        if (isnan(pCamera->yprRotation.x) || isnan(pCamera->yprRotation.y))
          pCamera->yprRotation = nanProtection;
      }
    }
    break;

    case vcCIS_MovingToPoint:
    {
      udDouble3 travelVector = pCamInput->focusPoint - pCamInput->startPosition;
      travelVector *= 0.9; //stop short by 1/10th the distance

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

    case vcCIS_PinchZooming:
    {
      // WIP
    }
    break;

    case vcCIS_Count:
      break; // to cover all implemented cases
    }
    break;

  case vcCM_Zoom:
  {
    double distanceToPoint = udMag3(pCamInput->focusPoint - pCamera->position);

    udDouble3 towards = udNormalize3(pCamInput->focusPoint - pCamera->position);

    udDouble3 addPos = distanceToPoint * rotationOffset.y * towards;

    pCamera->position += addPos;
  }
  break;

  case vcCM_Measure:
  {
    // WIP
  }
  break;
  }
}
