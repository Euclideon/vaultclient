#include "vcCamera.h"
#include "udPlatform/udMath.h"

void vcCamera_Create(vcCamera * pCamera)
{
  pCamera->position = udDouble3::zero();
  pCamera->yprRotation = udDouble3::zero();
  pCamera->orientation = udQuaternion<double>::identity();

  pCamera->moveDirection.forward = 0.f;
  pCamera->moveDirection.right = 0.f;
  pCamera->moveDirection.vertical = 0.f;
}

udDouble4x4 vcCamera_GetMatrix(vcCamera * pCamera)
{
  pCamera->orientation = udQuaternion<double>::create(pCamera->yprRotation);
  udDouble3 lookPos = pCamera->position + pCamera->orientation.apply(udDouble3::create(0, 1, 0));
  return udDouble4x4::lookAt(pCamera->position, lookPos, pCamera->orientation.apply(udDouble3::create(0, 0, 1)));
}

void vcCamera_Apply(vcCamera *pCamera, vcCameraSettings *pCamSettings, udDouble3 yprRotation, udDouble3 frvVector)
{
  pCamera->moveDirection.forward += frvVector.x;
  pCamera->moveDirection.right += frvVector.y;
  pCamera->moveDirection.vertical += frvVector.z;

  if (pCamSettings->invertX)
    yprRotation.x *= -1;
  if (pCamSettings->invertY)
    yprRotation.y *= -1;

  pCamera->yprDirection.yaw += yprRotation.x;
  pCamera->yprDirection.pitch += yprRotation.y;
  pCamera->yprDirection.roll += yprRotation.z;
}

void vcCamera_Update(vcCamera *pCamera, vcCameraSettings *pCamSettings, double deltaTime, float speedModifier /* = 1.f*/)
{
  pCamera->yprRotation += udDouble3::create(pCamera->yprDirection.yaw, pCamera->yprDirection.pitch, pCamera->yprDirection.roll);
  pCamera->yprRotation.y = udClamp(pCamera->yprRotation.y, (double)-UD_PI / 2, (double)UD_PI / 2);

  float speed = pCamSettings->moveSpeed * speedModifier;

  udDouble3 addPos = udDouble3::create(pCamera->moveDirection.right, pCamera->moveDirection.forward, 0);

  addPos = (udDouble4x4::rotationYPR(pCamera->yprRotation) * udDouble4::create(addPos, 1)).toVector3();
  if (pCamSettings->moveMode == vcCMM_Helicopter)
  {
    addPos.z = 0;
    if (addPos.x != 0 || addPos.y != 0)
      addPos = udNormalize3(addPos);
  }
  addPos.z += pCamera->moveDirection.vertical;
  addPos *= speed * deltaTime;

  pCamera->position += addPos;


  pCamera->moveDirection.vertical = 0;
  pCamera->moveDirection.forward = 0;
  pCamera->moveDirection.right = 0;

  pCamera->yprDirection.yaw = 0;
  pCamera->yprDirection.pitch = 0;
  pCamera->yprDirection.roll = 0;
}
