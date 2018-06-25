#include "vcCamera.h"
#include "vcState.h"

struct vcCamera
{
  udDouble3 position;
  udDouble3 yprRotation;
};

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

void vcCamera_Apply(vcCamera *pCamera, vcCameraSettings *pCamSettings, vcCameraPivotMode pivotMode, udDouble3 rotationOffset, udDouble3 moveOffset, double deltaTime, float speedModifier /* = 1.f*/)
{
  vcCamera_Apply(pCamera, pCamSettings, pivotMode, rotationOffset, moveOffset, deltaTime, false, udDouble3::zero(), udDouble3::zero(), speedModifier);
}

void vcCamera_Apply(vcCamera *pCamera, vcCameraSettings *pCamSettings, vcCameraPivotMode pivotMode, udDouble3 rotationOffset, udDouble3 moveOffset, double deltaTime, bool orbitActive, udDouble3 orbitPosition, udDouble3 storedRotation, float speedModifier /* = 1.f*/)
{
  // Translation
  float speed = pCamSettings->moveSpeed * speedModifier;
  double distanceToPoint = udMag3(orbitPosition - pCamera->position);
  udDoubleQuat orientation;
  udDouble3 addPosOrbit = udDouble3::zero();

  udDouble3 addPos = udClamp(moveOffset, udDouble3::create(-1, -1, -1), udDouble3::create(1, 1, 1)); // clamp in case 2 similarly mapped movement buttons are pressed
  double vertPos = addPos.z;
  addPos.z = 0.0;

  if (pCamSettings->moveMode == vcCMM_Plane)
    addPos = (udDouble4x4::rotationYPR(pCamera->yprRotation) * udDouble4::create(addPos, 1)).toVector3();

  if (pCamSettings->moveMode == vcCMM_Helicopter)
  {
    addPos = (udDouble4x4::rotationYPR(udDouble3::create(pCamera->yprRotation.x, 0.0, 0.0)) * udDouble4::create(addPos, 1)).toVector3();
    addPos.z = 0.0; // might be unnecessary now
    if (addPos.x != 0.0 || addPos.y != 0.0)
      addPos = udNormalize3(addPos);
  }

  if (orbitActive && distanceToPoint != 0.0)
  {
    if (rotationOffset.x != 0.0 || rotationOffset.y != 0.0)
      orientation = udDoubleQuat::create(pCamera->yprRotation - storedRotation);
    else
      orientation = udDoubleQuat::create(pCamera->yprRotation);

    udDouble3 right = orientation.apply(udDouble3::create(1.0, 0.0, 0.0));
    udDouble3 forward = orientation.apply(udDouble3::create(0.0, 1.0, 0.0));

    addPos /= 50.0;
    addPosOrbit = distanceToPoint * udSin(rotationOffset.x + addPos.x) * (right + udTan((rotationOffset.x + addPos.x) / 2.0) * forward);
    addPosOrbit.z = 0;

    if (addPos.y == 0.0)
      addPosOrbit += distanceToPoint * rotationOffset.y * forward;
    else
      addPosOrbit += distanceToPoint * (rotationOffset.y + addPos.y) * udDoubleQuat::create(pCamera->yprRotation - storedRotation).apply(udDouble3::create(0.0, 1.0, 0.0));

    addPos = udDouble3::zero();
  }

  addPos.z += vertPos;
  addPos *= speed * deltaTime;
  addPos += addPosOrbit;

  pCamera->position += addPos;

  // Rotation
  if (!orbitActive)
  {
    if (pCamSettings->invertX)
      rotationOffset.x *= -1.0;
    if (pCamSettings->invertY)
      rotationOffset.y *= -1.0;

    pCamera->yprRotation += rotationOffset;
    pCamera->yprRotation.y = udClamp(pCamera->yprRotation.y, (double)-UD_PI / 2.0, (double)UD_PI / 2.0);
  }

  if (orbitActive && distanceToPoint != 0.0)
  {
    udDouble3 nanProtection = pCamera->yprRotation;
    pCamera->yprRotation = udDouble4x4::lookAt(pCamera->position, orbitPosition, orientation.apply(udDouble3::create(0.0, 0.0, 1.0))).extractYPR();
    pCamera->yprRotation += storedRotation;

    if (pCamera->yprRotation.y > UD_PI / 2.0)
      pCamera->yprRotation.y -= UD_2PI; // euler vs +/-

    pCamera->yprRotation.z = 0.0;

    if (isnan(pCamera->yprRotation.x) || isnan(pCamera->yprRotation.y))
      pCamera->yprRotation = nanProtection;
  }
}

void vcCamera_TravelZoomPath(vcCamera *pCamera, vcCameraSettings *pCamSettings, vcState *pProgramState, double deltaTime)
{
  if (pCamera == nullptr || pCamSettings == nullptr || pProgramState == nullptr)
    return;

  udDouble3 travelVector = pProgramState->zoomPath.endPos - pProgramState->zoomPath.startPos;
  travelVector *= 0.9; //stop short by 1/10th the distance

  double travelProgress = 0;

  pProgramState->zoomPath.progress += deltaTime/2; // 2 second travel time
  if (pProgramState->zoomPath.progress > 1.0)
  {
    pProgramState->zoomPath.progress = 1.0;
    pProgramState->zoomPath.isZooming = false;
  }

  double t = pProgramState->zoomPath.progress;
  if (t < 0.5)
    travelProgress = 4 * t * t * t; // cubic
  else
    travelProgress = (t - 1)*(2 * t - 2)*(2 * t - 2) + 1; // cubic

  pCamera->position = pProgramState->zoomPath.startPos + travelVector * travelProgress;
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


const char* const* vcCamera_GetLensNames()
{
  return lensNameArray;
}
