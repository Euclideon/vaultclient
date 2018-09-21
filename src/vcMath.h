#include "udPlatform/udMath.h"

template <typename T>
struct udRay
{
  udVector3<T> position;
  udQuaternion<T> orientation;
};

template <typename T>
udRay<T> udRotateAround(udRay<T> ray, udVector3<T> center, udVector3<T> axis, T angle)
{
  udRay<T> r;

  udVector3<T> pos = ray.position;
  udQuaternion<T> rotation = udQuaternion<T>::create(axis, angle);

  udVector3<T> direction = ray.position - center; // find current direction relative to center
  r.position = center + rotation.apply(direction); // define new position
  r.orientation = rotation * ray.orientation; // rotate object to keep looking at the center

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
