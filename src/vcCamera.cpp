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
