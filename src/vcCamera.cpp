#include "vcCamera.h"

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

void vcCamera_Orbit(vcCamera *pCamera, udDouble3 orbitPosition, udDouble3 storedRotation, udDouble3 deltaRotation)
{
  double distanceToPoint = udMag3(orbitPosition - pCamera->position);
  if (distanceToPoint == 0.0)
    return; // cant rotate around same point with same method

  udQuaternion<double> orientation = udQuaternion<double>::create(pCamera->yprRotation - storedRotation);
  udDouble3 up = orientation.apply(udDouble3::create(0.0, 0.0, 1.0));
  udDouble3 right = orientation.apply(udDouble3::create(1.0, 0.0, 0.0));
  udDouble3 forward = orientation.apply(udDouble3::create(0.0, 1.0, 0.0));

  udDouble3 addPos = distanceToPoint * udSin(deltaRotation.x) * (right + udTan(deltaRotation.x / 2.0) *  forward);
  addPos.z = 0.0;

  addPos += distanceToPoint * deltaRotation.y * forward;
  pCamera->position += addPos;

  if (deltaRotation.x != 0.0 || deltaRotation.y != 0.0)
  {
    pCamera->yprRotation = udDouble4x4::lookAt(pCamera->position, orbitPosition, up).extractYPR();
    pCamera->yprRotation += storedRotation;
    pCamera->yprRotation.z = 0.0;
  }
}

void vcCamera_Apply(vcCamera *pCamera, vcCameraSettings *pCamSettings, udDouble3 rotationOffset, udDouble3 moveOffset, double deltaTime, float speedModifier /* = 1.f*/)
{
  float speed = pCamSettings->moveSpeed * speedModifier;

  udDouble3 addPos = udClamp(moveOffset, udDouble3::create(-1, -1, -1), udDouble3::create(1, 1, 1)); // clamp in case 2 similarly mapped movement buttons are pressed
  double vertPos = addPos.z;
  addPos.z = 0.0;

  if (pCamSettings->moveMode != vcCMM_Orbit)
  {
    if (pCamSettings->invertX)
      rotationOffset.x *= -1.0;
    if (pCamSettings->invertY)
      rotationOffset.y *= -1.0;

    pCamera->yprRotation += rotationOffset;
    pCamera->yprRotation.y = udClamp(pCamera->yprRotation.y, (double)-UD_PI / 2.0, (double)UD_PI / 2.0);
  }

  if(pCamSettings->moveMode == vcCMM_Plane)
    addPos = (udDouble4x4::rotationYPR(pCamera->yprRotation) * udDouble4::create(addPos, 1)).toVector3();

  if (pCamSettings->moveMode == vcCMM_Helicopter)
  {
    addPos = (udDouble4x4::rotationYPR(udDouble3::create(pCamera->yprRotation.x, 0.0, 0.0)) * udDouble4::create(addPos, 1)).toVector3();
    addPos.z = 0.0; // might be unnecessary now
    if (addPos.x != 0.0 || addPos.y != 0.0)
      addPos = udNormalize3(addPos);
  }
  addPos.z += vertPos;
  addPos *= speed * deltaTime;

  pCamera->position += addPos;
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
