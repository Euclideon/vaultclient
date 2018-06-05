#include "vcCamera.h"

struct vcCamera
{
  udDouble3 position;
  udDouble3 yprRotation;
  udQuaternion<double> orientation;

  udDouble3 moveOffset; // right, forward, vertical
  udDouble3 rotationOffset; // yaw, pitch, roll
};

void vcCamera_Init(vcCamera **ppCamera)
{
  if (ppCamera == nullptr)
    return;

  vcCamera *pCamera = udAllocType(vcCamera, 1, udAF_None);
  pCamera->position = udDouble3::zero();
  pCamera->yprRotation = udDouble3::zero();
  pCamera->orientation = udQuaternion<double>::identity();

  pCamera->moveOffset = udDouble3::zero();
  pCamera->rotationOffset = udDouble3::zero();

  *ppCamera = pCamera;
}

void vcCamera_DeInit(vcCamera **ppCamera)
{
  if(ppCamera != nullptr)
    udFree(*ppCamera);
}

udDouble4x4 vcCamera_GetMatrix(vcCamera *pCamera)
{
  pCamera->orientation = udQuaternion<double>::create(pCamera->yprRotation);
  udDouble3 lookPos = pCamera->position + pCamera->orientation.apply(udDouble3::create(0, 1, 0));
  return udDouble4x4::lookAt(pCamera->position, lookPos, pCamera->orientation.apply(udDouble3::create(0, 0, 1)));
}

void vcCamera_Apply(vcCamera *pCamera, vcCameraSettings *pCamSettings, udDouble3 rotationOffset, udDouble3 moveOffset)
{
  pCamera->moveOffset += moveOffset;

  pCamera->moveOffset = udClamp(pCamera->moveOffset, udDouble3::create(-1, -1, -1), udDouble3::create(1, 1, 1)); // clamp in case 2 similarly mapped movement buttons are pressed

  if (pCamSettings->invertX)
    rotationOffset.x *= -1;
  if (pCamSettings->invertY)
    rotationOffset.y *= -1;

  pCamera->rotationOffset += rotationOffset;
}

void vcCamera_Update(vcCamera *pCamera, vcCameraSettings *pCamSettings, double deltaTime, float speedModifier /* = 1.f*/)
{
  pCamera->yprRotation += pCamera->rotationOffset;
  pCamera->yprRotation.y = udClamp(pCamera->yprRotation.y, (double)-UD_PI / 2, (double)UD_PI / 2);

  float speed = pCamSettings->moveSpeed * speedModifier;

  udDouble3 addPos = pCamera->moveOffset;
  addPos.z = 0;

  addPos = (udDouble4x4::rotationYPR(pCamera->yprRotation) * udDouble4::create(addPos, 1)).toVector3();
  if (pCamSettings->moveMode == vcCMM_Helicopter)
  {
    addPos.z = 0;
    if (addPos.x != 0 || addPos.y != 0)
      addPos = udNormalize3(addPos);
  }
  addPos.z += pCamera->moveOffset.z;
  addPos *= speed * deltaTime;

  pCamera->position += addPos;

  pCamera->moveOffset = udDouble3::zero();
  pCamera->rotationOffset = udDouble3::zero();
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
