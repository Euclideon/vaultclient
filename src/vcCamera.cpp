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
  udDouble3 lookPos = pCamera->position + orientation.apply(udDouble3::create(0, 1, 0));
  return udDouble4x4::lookAt(pCamera->position, lookPos, orientation.apply(udDouble3::create(0, 0, 1)));
}

void vcCamera_Apply(vcCamera *pCamera, vcCameraSettings *pCamSettings, udDouble3 rotationOffset, udDouble3 moveOffset, double deltaTime, float speedModifier /* = 1.f*/)
{
  float speed = pCamSettings->moveSpeed * speedModifier;

  udDouble3 addPos = udClamp(moveOffset, udDouble3::create(-1, -1, -1), udDouble3::create(1, 1, 1)); // clamp in case 2 similarly mapped movement buttons are pressed
  double vertPos = addPos.z;addPos.z = 0;
  addPos.z = 0;

  if (pCamSettings->invertX)
    rotationOffset.x *= -1;
  if (pCamSettings->invertY)
    rotationOffset.y *= -1;

  pCamera->yprRotation += rotationOffset;
  pCamera->yprRotation.y = udClamp(pCamera->yprRotation.y, (double)-UD_PI / 2, (double)UD_PI / 2);

  addPos = (udDouble4x4::rotationYPR(pCamera->yprRotation) * udDouble4::create(addPos, 1)).toVector3();
  if (pCamSettings->moveMode == vcCMM_Helicopter)
  {
    addPos.z = 0;
    if (addPos.x != 0 || addPos.y != 0)
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
