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

template <typename T>
struct udRay
{
  udVector3<T> position;
  udVector3<T> direction;

  // static members
  static udRay<T> create(const udVector3<T> &position, const udVector3<T> &direction) { udRay<T> r = { position, direction }; return r; }

  template <typename U>
  static udRay<T> create(const udRay<U> &_v) { udRay<T> r = { udVector3<T>::create(_v.position), udVector3<T>::create(_v.direction) }; return r; }
};

template <typename T>
struct udPlane
{
  udVector3<T> point;
  udVector3<T> normal;

  // static members
  static udPlane<T> create(const udVector3<T> &position, const udVector3<T> &normal) { udPlane<T> r = { position, normal }; return r; }
  template <typename U>
  static udPlane<T> create(const udPlane<U> &_v) { udPlane<T> r = { udVector3<T>::create(_v.point), udVector3<T>::create(_v.normal) }; return r; }
};

template <typename T>
udRay<T> udRotateAround(udRay<T> ray, udVector3<T> center, udVector3<T> axis, T angle)
{
  udRay<T> r;

  udQuaternion<T> rotation = udQuaternion<T>::create(axis, angle);

  udVector3<T> direction = ray.position - center; // find current direction relative to center
  r.position = center + rotation.apply(direction); // define new position
  r.direction = udMath_DirFromYPR((rotation * udQuaternion<T>::create(udMath_DirToYPR(ray.direction))).eulerAngles()); // rotate object to keep looking at the center

  return r;
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
static udQuaternion<T> udQuaternionFromMatrix(const udMatrix4x4<T> &rotMat)
{
  udQuaternion<T> retVal;

  // Matrix to Quat code from http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
  T tr = rotMat.m._00 + rotMat.m._11 + rotMat.m._22;

  if (tr > T(0))
  {
    T S = (T)(udSqrt(tr + T(1)) * T(2)); // S=4*qw
    retVal.w = T(0.25) * S;
    retVal.x = (rotMat.m._21 - rotMat.m._12) / S;
    retVal.y = (rotMat.m._02 - rotMat.m._20) / S;
    retVal.z = (rotMat.m._10 - rotMat.m._01) / S;
  }
  else if ((rotMat.m._00 > rotMat.m._11) & (rotMat.m._00 > rotMat.m._22))
  {
    T S = udSqrt(T(1) + rotMat.m._00 - rotMat.m._11 - rotMat.m._22) * T(2); // S=4*qx
    retVal.w = (rotMat.m._21 - rotMat.m._12) / S;
    retVal.x = T(0.25) * S;
    retVal.y = (rotMat.m._01 + rotMat.m._10) / S;
    retVal.z = (rotMat.m._02 + rotMat.m._20) / S;
  }
  else if (rotMat.m._11 > rotMat.m._22)
  {
    T S = udSqrt(T(1) + rotMat.m._11 - rotMat.m._00 - rotMat.m._22) * T(2); // S=4*qy
    retVal.w = (rotMat.m._02 - rotMat.m._20) / S;
    retVal.x = (rotMat.m._01 + rotMat.m._10) / S;
    retVal.y = T(0.25) * S;
    retVal.z = (rotMat.m._12 + rotMat.m._21) / S;
  }
  else
  {
    T S = udSqrt(T(1) + rotMat.m._22 - rotMat.m._00 - rotMat.m._11) * T(2); // S=4*qz
    retVal.w = (rotMat.m._10 - rotMat.m._01) / S;
    retVal.x = (rotMat.m._02 + rotMat.m._20) / S;
    retVal.y = (rotMat.m._12 + rotMat.m._21) / S;
    retVal.z = T(0.25) * S;
  }
  return udNormalize(retVal);
}

template<typename T>
static void udExtractTransform(const udMatrix4x4<T> &matrix, udVector3<T> &position, udVector3<T> &scale, udQuaternion<T> &rotation)
{
  udMatrix4x4<T> mat = matrix;

  //Extract position
  position = mat.axis.t.toVector3();

  //Extract scales
  scale = udVector3<T>::create(udMag3(mat.axis.x), udMag3(mat.axis.y), udMag3(mat.axis.z));
  mat.axis.x /= scale.x;
  mat.axis.y /= scale.y;
  mat.axis.z /= scale.z;

  //Extract rotation
  rotation = udQuaternionFromMatrix(mat);
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

template <typename T>
static udResult udIntersect(const udPlane<T> &plane, const udRay<T> &ray, udVector3<T> *pPointInterception = nullptr, T *pIntersectionDistance = nullptr)
{
  udResult result = udR_Success;

  udVector3<T> rayDir = ray.direction;
  udVector3<T> planeRay = plane.point - ray.position;

  T denom = udDot(plane.normal, rayDir);
  T distance = 0;

  UD_ERROR_IF(denom == T(0), udR_Failure_); // Parallel to the ray

  distance = udDot(planeRay, plane.normal) / denom;

  UD_ERROR_IF(distance < 0, udR_Failure_); // Behind the ray

  if (pPointInterception)
    *pPointInterception = ray.position + rayDir * distance;

  if (pIntersectionDistance)
    *pIntersectionDistance = distance;

epilogue:
  return result;
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
