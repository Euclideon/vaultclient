#ifndef vcMath_h__
#define vcMath_h__

#include "udMath.h"

enum udEaseType
{
  udET_Linear,

  udET_QuadraticIn,
  udET_QuadraticOut,
  udET_QuadraticInOut,

  udET_CubicIn,
  udET_CubicOut,
  udET_CubicInOut,
};

template <typename T>
T udEase(const T t, udEaseType easeType = udET_Linear)
{
  T result = t;
  switch (easeType)
  {
  case udET_QuadraticIn:
    result = t * t;
    break;
  case udET_QuadraticOut:
    result = t * (T(2) - t);
    break;
  case udET_QuadraticInOut:
    result = (t < T(0.5)) ? (T(2) * t * t) : (T(-1) + (T(4) - T(2) * t) * t);
    break;
  case udET_CubicIn:
    result = t * t * t;
    break;
  case udET_CubicOut:
    result = (t - T(1)) * (t - 1) * (t - 1) + T(1);
    break;
  case udET_CubicInOut:
    result = (t < T(0.5)) ? (T(4) * t * t * t) : ((t - T(1))*(T(2) * t - T(2))*(T(2) * t - T(2)) + T(1));
    break;
  case udET_Linear: // fall through
  default:
    break;
  }

  return result;
}

//Clamp with wrap
template<typename T>
inline T udClampWrap(T val, T min, T max)
{
  if (max <= min)
    return min;

  if (val < min)
    val += ((min - val + (max - min - T(1))) / (max - min)) * (max - min); // Clamp above min

  return (val - min) % (max - min) + min;
}

template<>
inline float udClampWrap(float val, float min, float max)
{
  if (max <= min)
    return min;

  if (val < min)
    val += udCeil((min - val) / (max - min)) * (max - min); // Clamp above min

  return fmodf(val - min, max - min) + min;
}

template<>
inline double udClampWrap(double val, double min, double max)
{
  if (max <= min)
    return min;

  if (val < min)
    val += udCeil((min - val) / (max - min)) * (max - min); // Clamp above min

  return fmod(val - min, max - min) + min;
}

template <typename T>
static udVector3<T> udMath_DirFromYPR(const udVector3<T> &ypr)
{
  udVector3<T> r;

  r.x = -udSin(ypr.x) * udCos(ypr.y);
  r.y = udCos(ypr.x) * udCos(ypr.y);
  r.z = udSin(ypr.y);

  return r;
}

template <typename T>
static udVector3<T> udMath_DirToYPR(const udVector3<T> &direction)
{
  udVector3<T> r;

  udVector3<T> dir = udNormalize(direction);

  r.x = -udATan2(dir.x, dir.y);
  r.y = udASin(dir.z);
  r.z = 0; // No Roll

  return r;
}

template <typename T>
static udVector3<T> udClosestPointOnOOBB(const udVector3<T> &point, const udMatrix4x4<T> &oobbMatrix)
{
  udVector3<T> origin = udVector3<T>::zero();
  udVector3<T> scale = udVector3<T>::zero();
  udQuaternion<T> rotation = udQuaternion<T>::identity();
  udExtractTransform(oobbMatrix, origin, scale, rotation);
  udVector3<T> localPoint = udInverse(rotation).apply(point - origin);
  return origin + rotation.apply(udClamp(localPoint, udDouble3::zero(), scale));
}

// Returns -1=outside, 0=inside, >0=partial (bits of planes crossed)
template <typename T>
static int udFrustumTest(const udVector4<T> frustumPlanes[6], const udVector3<T> &boundCenter, const udVector3<T> &boundExtents)
{
  int partial = 0;

  for (int i = 0; i < 6; ++i)
  {
    double distToCenter = udDot4(udVector4<T>::create(boundCenter, 1.0), frustumPlanes[i]);
    //optimized for case where boxExtents are all same: udFloat radiusBoxAtPlane = udDot3(boxExtents, udAbs(udVec3(curPlane)));
    double radiusBoxAtPlane = udDot3(boundExtents, udAbs(frustumPlanes[i].toVector3()));
    if (distToCenter < -radiusBoxAtPlane)
      return -1; // Box is entirely behind at least one plane
    else if (distToCenter <= radiusBoxAtPlane) // If spanned (not entirely infront)
      partial |= (1 << i);
  }

  return partial;
}

#endif //vcMath_h__
