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

template <typename T>
static bool udPointInAABB(udVector3<T> point, udVector3<T> minPos, udVector3<T> maxPos)
{
  return (point.x >= minPos.x && point.y >= minPos.y && point.z >= minPos.z &&
    point.x <= maxPos.x && point.y <= maxPos.y && point.z <= maxPos.z);
}

template <typename T>
static udVector3<T> udGetSphereCenterFromPoints(T ballRadius, udVector3<T> p0, udVector3<T> p1, udVector3<T> p2)
{
  // See https://en.wikipedia.org/wiki/Circumscribed_circle, specifically regarding "higher dimensions"

  // Get circumscribed circle from triangle
  udVector3<T> a = p1 - p0;
  udVector3<T> b = p2 - p0;
  udVector3<T> triangleNormal = udNormalize(udCross(a, b));

  T aMagSq = udMagSq3(a);
  T aMag = udSqrt(aMagSq);
  T bMagSq = udMagSq3(b);
  T bMag = udSqrt(bMagSq);

  // Get circumradius
  T circumradius = (aMag * bMag * udMag3(a - b)) / (T(2) * udMag3(udCross(a, b)));

  // Get circumcenter
  udVector3<T> circumcenter = (udCross(aMagSq * b - bMagSq * a, udCross(a, b)) / (T(2) * udMagSq3(udCross(a, b)))) + p0;

  T distanceSq = (ballRadius * ballRadius) - (circumradius * circumradius);

  // Return sphere center
  if (distanceSq < T(0))
    return udVector3<T>::zero();
  else
    return circumcenter + triangleNormal * udSqrt(distanceSq);
}

template <typename T>
static T udDistanceToTriangle(udVector3<T> p0, udVector3<T> p1, udVector3<T> p2, udVector3<T> point, udVector3<T> *pclosestPoint = nullptr)
{
  // See https://math.stackexchange.com/q/544946 for more details
  udVector3<T> u = p1 - p0;
  udVector3<T> v = p2 - p0;
  udVector3<T> n = udCross3(u, v);
  udVector3<T> w = point - p0;

  T gamma = udDot(udCross3(u, w), n) / udDot(n, n);
  T beta = udDot(udCross3(w, v), n) / udDot(n, n);
  T alpha = 1 - gamma - beta;

  if (T(0) <= alpha && alpha <= T(1) && T(0) <= beta && beta <= T(1) && T(0) <= gamma && gamma <= T(1))
  {
    if (pclosestPoint)
      *pclosestPoint = point;
    return T(0);
  }
  else
  {
    udVector3<T> triangle[3] = { p0, p1, p2 };
    udVector3<T> bestcp = {};
    T bestDistSq = T(0);
    for (int i = 0; i < 3; i++)
    {
      udVector3<T> const& origin = triangle[i];
      udVector3<T> const& direction = triangle[(i + 1) % 3] - origin;
      udVector3<T> w0 = point - origin;
      udVector3<T> cp = {};
      T u0 = T(0);

      T proj = udDot3(w0, direction);
      if (proj < T(0))
      {
        u0 = T(0);
      }
      else
      {
        T vsq = udDot(direction, direction);
        if (proj >= vsq)
          u0 = T(1);
        else
          u0 = (proj / vsq);
      }
      cp = origin + u0 * direction;
      T distSq = udMagSq3(cp - point);

      if (i == 0)
      {
        bestcp = cp;
        bestDistSq = distSq;
      }
      else if(distSq < bestDistSq)
      {
        bestDistSq = distSq;
        bestcp = cp;
      }
    }

    if (pclosestPoint)
      *pclosestPoint = bestcp;
    return udSqrt(bestDistSq);
  }
}

#endif //vcMath_h__
