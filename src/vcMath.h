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
static T udDistanceToTriangle(udVector3<T> p0, udVector3<T> p1, udVector3<T> p2, udVector3<T> point, udVector3<T> *pClosestPoint = nullptr)
{
  // Solution taken from 'Real Time Collision Detection' by Christer Ericson,
  // p.141: 'Closest Point on Triangle to Point
  
  udVector3<T> p0_p1 = p1 - p0;
  udVector3<T> p0_p2 = p2 - p0;
  udVector3<T> p1_p2 = p2 - p1;

  T snom = udDot(point - p0, p0_p1);
  T sdenom = udDot(point - p1, p0 - p1);

  T tnom = udDot(point - p0, p0_p2);
  T tdenom = udDot(point - p2, p0 - p2);

  udVector3<T> closestPoint = {};

  do
  {
    if (snom <= T(0) && tnom <= T(0))
    {
      closestPoint = p0;
      break;
    }

    T unom = udDot(point - p1, p1_p2);
    T udenom = udDot(point - p2, p1 - p2);

    if (sdenom <= T(0) && unom <= T(0))
    {
      closestPoint = p1;
      break;
    }

    if (tdenom <= T(0) && udenom <= T(0))
    {
      closestPoint = p2;
      break;
    }

    udVector3<T> n = udCross3(p1 - p0, p2 - p0);
    T v_p2 = udDot(n, udCross3(p0 - point, p1 - point));
    if (v_p2 <= T(0) && snom >= T(0) && sdenom >= T(0))
    {
      closestPoint = p0 + p0_p1 * (snom / (snom + sdenom));
      break;
    }

    T v_p0 = udDot(n, udCross(p1 - point, p2 - point));
    if (v_p0 <= T(0) && unom >= T(0) && udenom >= T(0))
    {
      closestPoint = p1 + p1_p2 * (unom / (unom + udenom));
      break;
    }

    T v_p1 = udDot(n, udCross(p2 - point, p0 - point));
    if (v_p1 <= T(0) && tnom >= T(0) && tdenom >= T(0))
    {
      closestPoint = p0 + p0_p2 * (tnom / (tnom + tdenom));
      break;
    }

    T u = v_p0 / (v_p0 + v_p1 + v_p2);
    T v = v_p1 / (v_p0 + v_p1 + v_p2);
    T w = T(1) - u - v;
    closestPoint = u * p0 + v * p1 + w * p2;
  } while (false);

  if (pClosestPoint != nullptr)
    *pClosestPoint = closestPoint;

  return udMag3(closestPoint - point);
}

//A simple polygon does not have holes and does not self-intersect.
//If the area > 0, the points are ordered clockwise.
//If the area < 0, the points are ordered counterclockwise.
template<typename T>
T udSignedSimplePolygonArea2(udVector2<T> const * pPoints, size_t nPoints)
{
  if (pPoints == nullptr || nPoints < 3)
    return T(0);

  T area = T(0);
  for (size_t i = 0; i < nPoints; ++i)
  {
    size_t j = (i + 1) % nPoints;
    area += (pPoints[i].x * pPoints[j].y) - (pPoints[i].y * pPoints[j].x);
  }

  return area / T(2);
}

template<typename T>
T udSignedSimplePolygonArea3(udVector3<T> const * pPoints, size_t nPoints)
{
  if (pPoints == nullptr || nPoints < 3)
    return T(0);

  T area = T(0);
  for (size_t i = 0; i < nPoints; ++i)
  {
    size_t j = (i + 1) % nPoints;
    area += (pPoints[i].x * pPoints[j].y) - (pPoints[i].y * pPoints[j].x);
  }

  return area / T(2);
}

//plane is of the format [{x, y, z}, offset]
template<typename T>
udVector3<T> udProjectPointToPlane(const udVector3<T> &point, udVector4<T> const &plane)
{
  udVector3<T> planeNorm = udVector3<T>::create(plane.x, plane.y, plane.z);
  T val = udDot(planeNorm, point);
  return point - planeNorm * (val + plane[3]);
}

//TODO don't need the plane, just the plane normal
template<typename T>
T udProjectedArea(udVector4<T> const &plane, udVector3<T> const *pPoints, size_t nPoints)
{
  if (pPoints == nullptr || nPoints < 3)
    return T(0);

  udVector3<T> centroid = {};
  for (size_t i = 0; i < nPoints; ++i)
    centroid += pPoints[i];
  centroid /= T(nPoints);

  centroid = udProjectPointToPlane<T>(centroid, plane);

  T area = T(0);

  udVector3<T> v0 = udProjectPointToPlane<T>(pPoints[0], plane) - centroid;
  udVector3<T> v1 = udProjectPointToPlane<T>(pPoints[1], plane) - centroid;

  for (size_t i = 0; i < nPoints; ++i)
  {
    area += udMag3(udCross3(v0, v1));

    v0 = v1;
    size_t ind = (i == nPoints - 2) ? 0 : i + 2;
    v1 = udProjectPointToPlane<T>(pPoints[ind], plane) - centroid;
  }

  return area / T(2);
}

//Obtain a normalised perpendicular vector to axis, in no particular direction.
//Assumes axis is a non-zero vector.
template<typename T>
udVector3<T> udPerpendicular3(udVector3<T> const & axis)
{
  udVector3<T> result;

  if (axis[0] != 0 || axis[1] != 0)
  {
    result[0] = -axis[1];
    result[1] = axis[0];
    result[2] = T(0);
  }
  else
  {
    result[0] = -axis[2];
    result[1] = T(0);
    result[2] = axis[0];
  }
  return udNormalize3(result);
}

template<typename T>
class udLineSegment
{
public:

  udLineSegment(udVector3<T> const &p0, udVector3<T> const &p1)
    : m_origin(p0)
    , m_direction(p1 - p0)
  {}

  udVector3<T> origin() const { return m_origin; }
  udVector3<T> direction() const { return m_direction; }

  udVector3<T> P0() const { return m_origin; }
  udVector3<T> P1() const { return m_origin + m_direction; }

  udVector3<T> center() const { return m_origin + static_cast<T>(0.5) * m_direction; }

  void setEndPoints(udVector3<T> const &p0, udVector3<T> const &p1) { m_origin = p0; m_direction = p1 - p0; }
  void setOriginDirection(udVector3<T> const &originIn, udVector3<T> const &directionIn) { m_origin = originIn; m_direction = directionIn; }

  T length() const { return udMag3(m_direction); }
  T lengthSquared() const { return udMagSq3(m_direction); }

private:

  udVector3<T> m_origin;
  udVector3<T> m_direction;
};

template <typename T>
static T udDistanceSqLineSegmentPoint(udLineSegment<T> const & segment, udVector3<T> const & point, T * pU = nullptr)
{
  T u;
  udVector3<T> w = point - segment.origin();
  T proj = udDot3(w, segment.direction());

  //Should we check agains some epsilon?
  if (proj <= T(0))
  {
    u = T(0);
  }
  else
  {
    T vsq = segment.lengthSquared();

    if (proj >= vsq)
      u = T(1);
    else
      u = (proj / vsq);
  }

  if (pU != nullptr)
    *pU = u;

  return udMagSq3(point - (segment.origin() + u * segment.direction()));
}

template <typename T>
struct udOrientedPoint
{
  udVector3<T> position;
  udQuaternion<T> orientation;

  // static members
  static udOrientedPoint<T> create(const udVector3<T> &position, const udQuaternion<T> &orientation) { udOrientedPoint<T> r = { position, orientation }; return r; }

  template <typename U>
  static udOrientedPoint<T> create(const udOrientedPoint<U> &_v) { udOrientedPoint<T> r = { udVector3<T>::create(_v.position), udQuaternion<T>::create(_v.orientation) }; return r; }

  static udOrientedPoint<T> rotationAround(const udOrientedPoint<T> &ray, const udVector3<T> &center, const udVector3<T> &axis, const T &angle)
  {
    udOrientedPoint<T> r;

    udQuaternion<T> rotation = udQuaternion<T>::create(axis, angle);

    udVector3<T> direction = ray.position - center; // find current direction relative to center
    r.position = center + rotation.apply(direction); // define new position
    r.orientation = (rotation * ray.orientation); // rotate object to keep looking at the center

    return r;
  }
};

#endif //vcMath_h__
