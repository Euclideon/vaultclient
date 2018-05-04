#pragma once
#if !defined(UDMATH_H)
#define UDMATH_H

#include "udPlatform.h"

#if defined(_MSC_VER)
// warning C4201: nonstandard extension used : nameless struct/union
# pragma warning(disable: 4201)
#endif

#define UD_PI            3.1415926535897932384626433832795
#define UD_2PI           6.283185307179586476925286766559
#define UD_HALF_PI       1.5707963267948966192313216916398
#define UD_ROOT_2        1.4142135623730950488016887242097
#define UD_INV_ROOT_2    0.70710678118654752440084436210485
#define UD_RAD2DEGVAL    57.295779513082320876798154814105
#define UD_DEG2RADVAL    0.01745329251994329576923690768489
#define UD_RAD2DEG(rad)  ((rad)*UD_RAD2DEGVAL)
#define UD_DEG2RAD(deg)  ((deg)*UD_DEG2RADVAL)


#define UD_PIf           float(UD_PI)
#define UD_2PIf          float(UD_2PI)
#define UD_HALF_PIf      float(UD_HALF_PI)
#define UD_ROOT_2f       float(UD_ROOT_2)
#define UD_INV_ROOT_2f   float(UD_INV_ROOT_2)
#define UD_RAD2DEGVALf   float(UD_RAD2DEGVAL)
#define UD_DEG2RADVALf   float(UD_DEG2RADVAL)
#define UD_RAD2DEGf(rad) ((rad)*UD_RAD2DEGVALf)
#define UD_DEG2RADf(deg) ((deg)*UD_DEG2RADVALf)

#define UD_EPSILON       float(1.0f / 1024.0f)

#if defined(__cplusplus)

// prototypes
template <typename T> struct udVector2;
template <typename T> struct udVector3;
template <typename T> struct udVector4;
template <typename T> struct udQuaternion;
template <typename T> struct udMatrix4x4;

// math functions
float udPow(float f, float n);
double udPow(double d, double n);
float udLogN(float f);
double udLogN(double d);
float udLog2(float f);
double udLog2(double d);
float udLog10(float f);
double udLog10(double d);
float udRSqrt(float f);
double udRSqrt(double d);
float udSqrt(float f);
double udSqrt(double d);
float udSin(float f);
double udSin(double d);
float udCos(float f);
double udCos(double d);
float udTan(float f);
double udTan(double d);
float udASin(float f);
double udASin(double d);
float udACos(float f);
double udACos(double d);
float udATan(float f);
double udATan(double d);
float udATan2(float y, float x);
double udATan2(double y, double x);

template <typename T>
T udNormaliseRotation(T rad, T absRange = UD_PI);

// rounding functions
float udRound(float f);
double udRound(double d);
float udFloor(float f);
double udFloor(double d);
float udCeil(float f);
double udCeil(double d);

template <typename T> T udRoundEven(T t);
template <typename T> T udPowerOfTwoAbove(T i);
template <typename T> bool udIsPowerOfTwo(T i);
template <typename T> T udHighestBitValue(T i);

// typical linear algebra functions
template <typename T> T            udAbs(T v);
template <typename T> udVector2<T> udAbs(const udVector2<T> &v);
template <typename T> udVector3<T> udAbs(const udVector3<T> &v);
template <typename T> udVector4<T> udAbs(const udVector4<T> &v);
template <typename T> udQuaternion<T> udAbs(const udQuaternion<T> &q);

template <typename T> T            udMax(T a, T b);
template <typename T> udVector2<T> udMax(const udVector2<T> &v1, const udVector2<T> &v2);
template <typename T> udVector3<T> udMax(const udVector3<T> &v1, const udVector3<T> &v2);
template <typename T> udVector4<T> udMax(const udVector4<T> &v1, const udVector4<T> &v2);
template <typename T> T            udMin(T a, T b);
template <typename T> udVector2<T> udMin(const udVector2<T> &v1, const udVector2<T> &v2);
template <typename T> udVector3<T> udMin(const udVector3<T> &v1, const udVector3<T> &v2);
template <typename T> udVector4<T> udMin(const udVector4<T> &v1, const udVector4<T> &v2);
template <typename T> T            udClamp(T v, T _min, T _max);
template <typename T> udVector2<T> udClamp(const udVector2<T> &v, const udVector2<T> &_min, const udVector2<T> &_max);
template <typename T> udVector3<T> udClamp(const udVector3<T> &v, const udVector3<T> &_min, const udVector3<T> &_max);
template <typename T> udVector4<T> udClamp(const udVector4<T> &v, const udVector4<T> &_min, const udVector4<T> &_max);
template <typename T> T            udSaturate(const T &v);
template <typename T> udVector2<T> udSaturate(const udVector2<T> &v);
template <typename T> udVector3<T> udSaturate(const udVector3<T> &v);
template <typename T> udVector4<T> udSaturate(const udVector4<T> &v);

template <typename V, typename T> bool udIsUnitLength(const V &v, T epsilon = UD_EPSILON);

template <typename T> T udDot(const udVector2<T> &v1, const udVector2<T> &v2);
template <typename T> T udDot(const udVector3<T> &v1, const udVector3<T> &v2);
template <typename T> T udDot(const udVector4<T> &v1, const udVector4<T> &v2);
template <typename T> T udDot(const udQuaternion<T> &q1, const udQuaternion<T> &q2);

template <typename T> T udDot2(const udVector2<T> &v1, const udVector2<T> &v2);
template <typename T> T udDot2(const udVector3<T> &v1, const udVector3<T> &v2);
template <typename T> T udDot2(const udVector4<T> &v1, const udVector4<T> &v2);
template <typename T> T udDot3(const udVector3<T> &v1, const udVector3<T> &v2);
template <typename T> T udDot3(const udVector4<T> &v1, const udVector4<T> &v2);
template <typename T> T udDot4(const udVector4<T> &v1, const udVector4<T> &v2);
template <typename T> T udDoth(const udVector3<T> &v3, const udVector4<T> &v4);
template <typename T> T udDotQ(const udQuaternion<T> &q1, const udQuaternion<T> &q2);

template <typename T> T udMagSq(const udVector2<T> &v);
template <typename T> T udMagSq(const udVector3<T> &v);
template <typename T> T udMagSq(const udVector4<T> &v);
template <typename T> T udMagSq(const udQuaternion<T> &q);

template <typename T> T udMagSq2(const udVector2<T> &v);
template <typename T> T udMagSq2(const udVector3<T> &v);
template <typename T> T udMagSq2(const udVector4<T> &v);
template <typename T> T udMagSq3(const udVector3<T> &v);
template <typename T> T udMagSq3(const udVector4<T> &v);
template <typename T> T udMagSq4(const udVector4<T> &v);
template <typename T> T udMagSqQ(const udQuaternion<T> &q);

template <typename T> T udMag(const udVector2<T> &v);
template <typename T> T udMag(const udVector3<T> &v);
template <typename T> T udMag(const udVector4<T> &v);
template <typename T> T udMag(const udQuaternion<T> &v);

template <typename T> T udMag2(const udVector2<T> &v);
template <typename T> T udMag2(const udVector3<T> &v);
template <typename T> T udMag2(const udVector4<T> &v);
template <typename T> T udMag3(const udVector3<T> &v);
template <typename T> T udMag3(const udVector4<T> &v);
template <typename T> T udMag4(const udVector4<T> &v);
template <typename T> T udMagQ(const udQuaternion<T> &v);

template <typename T> T udCross(const udVector2<T> &v1, const udVector2<T> &v2);
template <typename T> udVector3<T> udCross(const udVector3<T> &v1, const udVector3<T> &v2);

template <typename T> T udCross2(const udVector2<T> &v1, const udVector2<T> &v2);
template <typename T> T udCross2(const udVector3<T> &v1, const udVector3<T> &v2);
template <typename T> T udCross2(const udVector4<T> &v1, const udVector4<T> &v2);
template <typename T> udVector3<T> udCross3(const udVector3<T> &v1, const udVector3<T> &v2);
template <typename T> udVector3<T> udCross3(const udVector4<T> &v1, const udVector4<T> &v2);

template <typename T> udVector2<T> udNormalize(const udVector2<T> &v);
template <typename T> udVector3<T> udNormalize(const udVector3<T> &v);
template <typename T> udVector4<T> udNormalize(const udVector4<T> &v);
template <typename T> udQuaternion<T> udNormalize(const udQuaternion<T> &q);

template <typename T> udVector2<T> udNormalize2(const udVector2<T> &v);
template <typename T> udVector3<T> udNormalize2(const udVector3<T> &v);
template <typename T> udVector4<T> udNormalize2(const udVector4<T> &v);
template <typename T> udVector3<T> udNormalize3(const udVector3<T> &v);
template <typename T> udVector4<T> udNormalize3(const udVector4<T> &v);
template <typename T> udVector4<T> udNormalize4(const udVector4<T> &v);
template <typename T> udQuaternion<T> udNormalizeQ(const udQuaternion<T> &q);

// matrix and quat functions
template <typename T> udVector2<T> udMul(const udMatrix4x4<T> &m, const udVector2<T> &v);
template <typename T> udVector3<T> udMul(const udMatrix4x4<T> &m, const udVector3<T> &v);
template <typename T> udVector4<T> udMul(const udMatrix4x4<T> &m, const udVector4<T> &v);
template <typename T, typename U> udMatrix4x4<T> udMul(const udMatrix4x4<T> &m, U f);
template <typename T> udMatrix4x4<T> udMul(const udMatrix4x4<T> &m1, const udMatrix4x4<T> &m2);
template <typename T> udQuaternion<T> udMul(const udQuaternion<T> &q1, const udQuaternion<T> &q2);

template <typename T> T udLerp(T a, T b, double t);
template <typename T> udVector2<T> udLerp(const udVector2<T> &v1, const udVector2<T> &v2, double t);
template <typename T> udVector3<T> udLerp(const udVector3<T> &v1, const udVector3<T> &v2, double t);
template <typename T> udVector4<T> udLerp(const udVector4<T> &v1, const udVector4<T> &v2, double t);
template <typename T> udQuaternion<T> udLerp(const udQuaternion<T> &q1, const udQuaternion<T> &q2, double t);
template <typename T> udMatrix4x4<T> udLerp(const udMatrix4x4<T> &m1, const udMatrix4x4<T> &m2, double t);

template <typename T> udMatrix4x4<T> udTranspose(const udMatrix4x4<T> &m);

template <typename T> T udDeterminant(const udMatrix4x4<T> &m);

template <typename T> udQuaternion<T> udInverse(const udQuaternion<T> &q);
template <typename T> udMatrix4x4<T>  udInverse(const udMatrix4x4<T> &m);

template <typename T> udQuaternion<T> udConjugate(const udQuaternion<T> &q);
template <typename T> udQuaternion<T> udSlerp(const udQuaternion<T> &q1, const udQuaternion<T> &q2, double t);

template <typename V> bool udEqualApprox(const V &a, const V &b, typename V::ElementType epsilon = UD_EPSILON);

// types
template <typename T>
struct udVector2
{
  T x, y;
  typedef T ElementType;
  enum { ElementCount = 2 } ;

  udVector2<T> operator -() const                      { udVector2<T> r = { -x, -y }; return r; }
  bool         operator ==(const udVector2<T> &v) const { return x == v.x && y == v.y; }
  bool         operator !=(const udVector2<T> &v) const { return !(*this == v); }

  udVector2<T> operator +(const udVector2<T> &v) const { udVector2<T> r = { x+v.x, y+v.y }; return r; }
  udVector2<T> operator -(const udVector2<T> &v) const { udVector2<T> r = { x-v.x, y-v.y }; return r; }
  udVector2<T> operator *(const udVector2<T> &v) const { udVector2<T> r = { x*v.x, y*v.y }; return r; }
  template <typename U>
  udVector2<T> operator *(U f) const                   { udVector2<T> r = { T(x*f), T(y*f) }; return r; }
  udVector2<T> operator /(const udVector2<T> &v) const { udVector2<T> r = { x/v.x, y/v.y }; return r; }
  template <typename U>
  udVector2<T> operator /(U f) const                   { udVector2<T> r = { T(x/f), T(y/f) }; return r; }
  T            operator [](size_t index) const         { return ((T*)this)[index]; }

  udVector2<T>& operator +=(const udVector2<T> &v)     { x+=v.x; y+=v.y; return *this; }
  udVector2<T>& operator -=(const udVector2<T> &v)     { x-=v.x; y-=v.y; return *this; }
  udVector2<T>& operator *=(const udVector2<T> &v)     { x*=v.x; y*=v.y; return *this; }
  template <typename U>
  udVector2<T>& operator *=(U f)                       { x=T(x*f); y=T(y*f); return *this; }
  udVector2<T>& operator /=(const udVector2<T> &v)     { x/=v.x; y/=v.y; return *this; }
  template <typename U>
  udVector2<T>& operator /=(U f)                       { x=T(x/f); y=T(y/f); return *this; }
  T&            operator [](size_t index)              { return ((T*)this)[index]; }

  // static members
  static udVector2<T> zero()      { udVector2<T> r = { T(0), T(0) }; return r; }
  static udVector2<T> one()       { udVector2<T> r = { T(1), T(1) }; return r; }

  static udVector2<T> xAxis()     { udVector2<T> r = { T(1), T(0) }; return r; }
  static udVector2<T> yAxis()     { udVector2<T> r = { T(0), T(1) }; return r; }

  static udVector2<T> create(T _f)       { udVector2<T> r = { _f, _f }; return r; }
  static udVector2<T> create(T _x, T _y) { udVector2<T> r = { _x, _y }; return r; }
  template <typename U>
  static udVector2<T> create(const udVector2<U> &_v) { udVector2<T> r = { T(_v.x), T(_v.y) }; return r; }
};
template <typename T>
udVector2<T> operator *(T f, const udVector2<T> &v) { return v * f; }

template <typename T>
struct udVector3
{
  T x, y, z;
  typedef T ElementType;
  enum { ElementCount = 3 };

  udVector2<T>& toVector2()               { return *(udVector2<T>*)this; }
  const udVector2<T>& toVector2() const   { return *(udVector2<T>*)this; }

  udVector3<T> operator -() const                       { udVector3<T> r = { -x, -y, -z }; return r; }
  bool         operator ==(const udVector3<T> &v) const { return x == v.x && y == v.y && z == v.z; }
  bool         operator !=(const udVector3<T> &v) const { return !(*this == v); }

  udVector3<T> operator +(const udVector3<T> &v) const  { udVector3<T> r = { x+v.x, y+v.y, z+v.z }; return r; }
  udVector3<T> operator -(const udVector3<T> &v) const  { udVector3<T> r = { x-v.x, y-v.y, z-v.z }; return r; }
  udVector3<T> operator *(const udVector3<T> &v) const  { udVector3<T> r = { x*v.x, y*v.y, z*v.z }; return r; }
  template <typename U>
  udVector3<T> operator *(U f) const                    { udVector3<T> r = { T(x*f), T(y*f), T(z*f) }; return r; }
  udVector3<T> operator /(const udVector3<T> &v) const  { udVector3<T> r = { x/v.x, y/v.y, z/v.z }; return r; }
  template <typename U>
  udVector3<T> operator /(U f) const                    { udVector3<T> r = { T(x/f), T(y/f), T(z/f) }; return r; }
  T            operator [](size_t index) const          { return ((T*)this)[index]; }

  udVector3<T>& operator +=(const udVector3<T> &v)      { x+=v.x; y+=v.y; z+=v.z; return *this; }
  udVector3<T>& operator -=(const udVector3<T> &v)      { x-=v.x; y-=v.y; z-=v.z; return *this; }
  udVector3<T>& operator *=(const udVector3<T> &v)      { x*=v.x; y*=v.y; z*=v.z; return *this; }
  template <typename U>
  udVector3<T>& operator *=(U f)                        { x=T(x*f); y=T(y*f); z=T(z*f); return *this; }
  udVector3<T>& operator /=(const udVector3<T> &v)      { x/=v.x; y/=v.y; z/=v.z; return *this; }
  template <typename U>
  udVector3<T>& operator /=(U f)                        { x=T(x/f); y=T(y/f); z=T(z/f); return *this; }
  T&            operator [](size_t index)               { return ((T*)this)[index]; }

  // static members
  static udVector3<T> zero()  { udVector3<T> r = { T(0), T(0), T(0) }; return r; }
  static udVector3<T> one()   { udVector3<T> r = { T(1), T(1), T(1) }; return r; }

  static udVector3<T> create(T _f)                        { udVector3<T> r = { _f, _f, _f };     return r; }
  static udVector3<T> create(T _x, T _y, T _z)            { udVector3<T> r = { _x, _y, _z };     return r; }
  static udVector3<T> create(const udVector2<T> _v, T _z) { udVector3<T> r = { _v.x, _v.y, _z }; return r; }
  template <typename U>
  static udVector3<T> create(const udVector3<U> &_v) { udVector3<T> r = { T(_v.x), T(_v.y), T(_v.z) }; return r; }
};
template <typename T>
udVector3<T> operator *(T f, const udVector3<T> &v) { return v * f; }

template <typename T>
struct udVector4
{
  T x, y, z, w;
  typedef T ElementType;
  enum { ElementCount = 4 };

  udVector3<T>& toVector3()               { return *(udVector3<T>*)this; }
  const udVector3<T>& toVector3() const   { return *(udVector3<T>*)this; }
  udVector2<T>& toVector2()               { return *(udVector2<T>*)this; }
  const udVector2<T>& toVector2() const   { return *(udVector2<T>*)this; }

  udVector4<T> operator -() const                      { udVector4<T> r = { -x, -y, -z, -w }; return r; }
  bool         operator ==(const udVector4<T> &v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
  bool         operator !=(const udVector4<T> &v) const { return !(*this == v); }

  udVector4<T> operator +(const udVector4<T> &v) const { udVector4<T> r = { x+v.x, y+v.y, z+v.z, w+v.w }; return r; }
  udVector4<T> operator -(const udVector4<T> &v) const { udVector4<T> r = { x-v.x, y-v.y, z-v.z, w-v.w }; return r; }
  udVector4<T> operator *(const udVector4<T> &v) const { udVector4<T> r = { x*v.x, y*v.y, z*v.z, w*v.w }; return r; }
  template <typename U>
  udVector4<T> operator *(U f) const                   { udVector4<T> r = { T(x*f), T(y*f), T(z*f), T(w*f) }; return r; }
  udVector4<T> operator /(const udVector4<T> &v) const { udVector4<T> r = { x/v.x, y/v.y, z/v.z, w/v.w }; return r; }
  template <typename U>
  udVector4<T> operator /(U f) const                   { udVector4<T> r = { T(x/f), T(y/f), T(z/f), T(w/f) }; return r; }
  T            operator [](size_t index) const         { return ((T*)this)[index]; }

  udVector4<T>& operator +=(const udVector4<T> &v)     { x+=v.x; y+=v.y; z+=v.z; w+=v.w; return *this; }
  udVector4<T>& operator -=(const udVector4<T> &v)     { x-=v.x; y-=v.y; z-=v.z; w-=v.w; return *this; }
  udVector4<T>& operator *=(const udVector4<T> &v)     { x*=v.x; y*=v.y; z*=v.z; w*=v.w; return *this; }
  template <typename U>
  udVector4<T>& operator *=(U f)                       { x=T(x*f); y=T(y*f); z=T(z*f); w=T(w*f); return *this; }
  udVector4<T>& operator /=(const udVector4<T> &v)     { x/=v.x; y/=v.y; z/=v.z; w/=v.w; return *this; }
  template <typename U>
  udVector4<T>& operator /=(U f)                       { x=T(x/f); y=T(y/f); z=T(z/f); w=T(w/f); return *this; }
  T&            operator [](size_t index)              { return ((T*)this)[index]; }

  // static members
  static udVector4<T> zero()      { udVector4<T> r = { T(0), T(0), T(0), T(0) }; return r; }
  static udVector4<T> one()       { udVector4<T> r = { T(1), T(1), T(1), T(1) }; return r; }
  static udVector4<T> identity()  { udVector4<T> r = { T(0), T(0), T(0), T(1) }; return r; }

  static udVector4<T> create(T _f)                               { udVector4<T> r = {   _f,   _f,   _f, _f }; return r; }
  static udVector4<T> create(T _x, T _y, T _z, T _w)             { udVector4<T> r = {   _x,   _y,   _z, _w }; return r; }
  static udVector4<T> create(const udVector3<T> &_v, T _w)       { udVector4<T> r = { _v.x, _v.y, _v.z, _w }; return r; }
  static udVector4<T> create(const udVector2<T> &_v, T _z, T _w) { udVector4<T> r = { _v.x, _v.y,   _z, _w }; return r; }
  template <typename U>
  static udVector4<T> create(const udVector4<U> &_v) { udVector4<T> r = { T(_v.x), T(_v.y), T(_v.z), T(_v.w) }; return r; }
};
template <typename T, typename U>
udVector4<T> operator *(U f, const udVector4<T> &v) { return v * f; }

template <typename T>
struct udQuaternion
{
  T x, y, z, w;
  typedef T ElementType;
  enum { ElementCount = 4 };

  const udVector4<T>& toVector4() const { return *(udVector4<T>*)this; }

  udQuaternion<T> operator -() const { udQuaternion<T> r = { -x, -y, -z, -w }; return r; }
  udQuaternion<T> operator +(const udQuaternion<T> &q) const { udQuaternion<T> r = { x + q.x, y + q.y, z + q.z, w + q.w }; return r; }
  udQuaternion<T> operator -(const udQuaternion<T> &q) const { udQuaternion<T> r = { x - q.x, y - q.y, z - q.z, w - q.w }; return r; }
  udQuaternion<T> operator *(const udQuaternion<T> &q) const { return udMul(*this, q); }
  template <typename U>
  udQuaternion<T> operator *(U f) const                      { udQuaternion<T> r = { T(x*f), T(y*f), T(z*f), T(w*f) }; return r; }
  T            operator [](size_t index) const               { return ((T*)this)[index]; }

  udQuaternion<T>& operator *=(const udQuaternion<T> &q)     { *this = udMul(*this, q); return *this; }
  template <typename U>
  udQuaternion<T>& operator *=(U f)                          { x=T(x*f); y=T(y*f); z=T(z*f); w=T(w*f); return *this; }
  udQuaternion<T>& operator +=(const udQuaternion<T> &q)     { x += q.x; y += q.y; z += q.z; w += q.w; return *this; }
  udQuaternion<T>& operator -=(const udQuaternion<T> &q)     { x -= q.x; y -= q.y; z -= q.z; w -= q.w; return *this; }

  T&            operator [](size_t index)                    { return ((T*)this)[index]; }

  udQuaternion<T>& inverse();
  udQuaternion<T>& conjugate();

  udVector3<T> apply(const udVector3<T> &v) const;

  udVector3<T> eulerAngles() const;

  // static members
  static udQuaternion<T> zero() { udQuaternion<T> q = { T(0), T(0), T(0), T(0) }; return q; }
  static udQuaternion<T> identity()  { udQuaternion<T> q = { T(0), T(0), T(0), T(1) }; return q; }

  static udQuaternion<T> create(const udVector3<T> &axis, T rad);
  static udQuaternion<T> create(const T _y, const T _p, const T _r);
  static udQuaternion<T> create(const udVector3<T> &ypr) { return create(ypr.x, ypr.y, ypr.z); }

  template <typename U>
  static udQuaternion<T> create(const udQuaternion<U> &_q) { udQuaternion<T> r = { T(_q.x), T(_q.y), T(_q.z), T(_q.w) }; return r; }
};
template <typename T, typename U>
udQuaternion<T> operator *(U f, const udQuaternion<T> &q) { return q * f; }

template <typename T>
struct udMatrix4x4
{
  union
  {
    T a[16];
    udVector4<T> c[4];
    struct
    {
      udVector4<T> x;
      udVector4<T> y;
      udVector4<T> z;
      udVector4<T> t;
    } axis;
    struct
    {
      T // remember, we store columns (axis) sequentially! (so appears transposed here)
        _00, _10, _20, _30,
        _01, _11, _21, _31,
        _02, _12, _22, _32,
        _03, _13, _23, _33;
    } m;
  };

  udMatrix4x4<T> operator *(const udMatrix4x4<T> &m2) const { return udMul(*this, m2); }
  template <typename U>
  udMatrix4x4<T> operator *(U f)                      const { return udMul(*this, f); }
  udMatrix4x4<T> operator +(const udMatrix4x4<T> &m2) const { return udAdd(*this, m2); }
  udMatrix4x4<T> operator -(const udMatrix4x4<T> &m2) const { return udSub(*this, m2); }
  udVector4<T> operator *(const udVector4<T> &v) const     { return udMul(*this, v); }

  udMatrix4x4<T>& operator +=(const udMatrix4x4<T> &m2)    { *this = udAdd(*this, m2); return *this; }
  udMatrix4x4<T>& operator -=(const udMatrix4x4<T> &m2)    { *this = udSub(*this, m2); return *this; }
  udMatrix4x4<T>& operator *=(const udMatrix4x4<T> &m2)    { *this = udMul(*this, m2); return *this; }
  udMatrix4x4<T>& operator *=(T f)                         { *this = udMul(*this, f); return *this; }

  bool operator ==(const udMatrix4x4& rh) const            { return memcmp(this, &rh, sizeof(*this)) == 0; }

  udMatrix4x4<T>& transpose();
  T determinant();
  udMatrix4x4<T>& inverse();

  udVector3<T> extractYPR() const;

  // static members
  static udMatrix4x4<T> identity();

  static udMatrix4x4<T> create(const T m[16]);
  static udMatrix4x4<T> create(T _00, T _10, T _20, T _30, T _01, T _11, T _21, T _31, T _02, T _12, T _22, T _32, T _03, T _13, T _23, T _33);
  static udMatrix4x4<T> create(const udVector4<T> &xColumn, const udVector4<T> &yColumn, const udVector4<T> &zColumn, const udVector4<T> &wColumn);
  template <typename U>
  static udMatrix4x4<T> create(const udMatrix4x4<U> &_m);

  static udMatrix4x4<T> rotationX(T rad, const udVector3<T> &t = udVector3<T>::zero());
  static udMatrix4x4<T> rotationY(T rad, const udVector3<T> &t = udVector3<T>::zero());
  static udMatrix4x4<T> rotationZ(T rad, const udVector3<T> &t = udVector3<T>::zero());
  static udMatrix4x4<T> rotationAxis(const udVector3<T> &axis, T rad, const udVector3<T> &t = udVector3<T>::zero());
  static udMatrix4x4<T> rotationYPR(T y, T p, T r, const udVector3<T> &t = udVector3<T>::zero());
  static udMatrix4x4<T> rotationYPR(const udVector3<T> &ypr, const udVector3<T> &t = udVector3<T>::zero()) { return rotationYPR(ypr.x, ypr.y, ypr.z, t); }

  //Creates a rotation matrix with post translation. Assumes the quaternion is normalized
  static udMatrix4x4<T> rotationQuat(const udQuaternion<T> &q, const udVector3<T> &t = udVector3<T>::zero());

  static udMatrix4x4<T> translation(T x, T y, T z);
  static udMatrix4x4<T> translation(const udVector3<T> &t) { return translation(t.x, t.y, t.z); }

  static udMatrix4x4<T> scaleUniform(T s, const udVector3<T> &t = udVector3<T>::zero()) { return scaleNonUniform(s, s, s, t); }
  static udMatrix4x4<T> scaleNonUniform(T x, T y, T z, const udVector3<T> &t = udVector3<T>::zero());
  static udMatrix4x4<T> scaleNonUniform(const udVector3<T> &s, const udVector3<T> &t = udVector3<T>::zero()) { return scaleNonUniform(s.x, s.y, s.z, t); }

  static udMatrix4x4<T> perspective(T fovY, T aspectRatio, T znear, T zfar);
  static udMatrix4x4<T> ortho(T left, T right, T bottom, T top, T znear = T(0), T zfar = T(1));
  static udMatrix4x4<T> orthoForScreen(T width, T height, T znear = T(0), T zfar = T(1));

  static udMatrix4x4<T> lookAt(const udVector3<T> &from, const udVector3<T> &at, const udVector3<T> &up = udVector3<T>::create(T(0), T(0), T(1)));
};
template <typename T, typename U>
udMatrix4x4<T> operator *(U f, const udMatrix4x4<T> &m) { return m*f; }


// typedef's for typed vectors/matices
typedef udVector2<int16_t>  udShort2;
typedef udVector3<int16_t>  udShort3;
typedef udVector4<int16_t>  udShort4;
typedef udVector2<int32_t>  udInt2;
typedef udVector3<int32_t>  udInt3;
typedef udVector4<int32_t>  udInt4;
typedef udVector2<int64_t>  udLong2;
typedef udVector3<int64_t>  udLong3;
typedef udVector4<int64_t>  udLong4;
typedef udVector2<uint16_t> udUShort2;
typedef udVector3<uint16_t> udUShort3;
typedef udVector4<uint16_t> udUShort4;
typedef udVector2<uint32_t> udUInt2;
typedef udVector3<uint32_t> udUInt3;
typedef udVector4<uint32_t> udUInt4;
typedef udVector2<uint64_t> udULong2;
typedef udVector3<uint64_t> udULong3;
typedef udVector4<uint64_t> udULong4;
typedef udVector4<float>  udFloat4;
typedef udVector3<float>  udFloat3;
typedef udVector2<float>  udFloat2;
typedef udVector4<double> udDouble4;
typedef udVector3<double> udDouble3;
typedef udVector2<double> udDouble2;

typedef udMatrix4x4<float>  udFloat4x4;
typedef udMatrix4x4<double> udDouble4x4;
//typedef udMatrix3x4<float>  udFloat3x4;
//typedef udMatrix3x4<double> udDouble3x4;
//typedef udMatrix3x3<float>  udFloat3x3;
//typedef udMatrix3x3<double> udDouble3x3;

typedef udQuaternion<float>  udFloatQuat;
typedef udQuaternion<double>  udDoubleQuat;

#include "udMath_Inl.h"

#endif // __cplusplus

#endif // UDMATH_H
