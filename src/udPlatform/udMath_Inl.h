#include <float.h>
#include <math.h>

UDFORCE_INLINE float udPow(float f, float n) { return powf(f, n); }
UDFORCE_INLINE double udPow(double d, double n) { return pow(d, n); }
UDFORCE_INLINE float udLogN(float f) { return logf(f); }
UDFORCE_INLINE double udLogN(double d) { return log(d); }
UDFORCE_INLINE float udLog10(float f) { return log10f(f); }
UDFORCE_INLINE double udLog10(double d) { return log10(d); }
UDFORCE_INLINE float udLog(float f, float base) { return udLogN(f) / udLogN(base); }
UDFORCE_INLINE double udLog(double f, double base) { return udLogN(f) / udLogN(base); }

#if defined(_MSC_VER) && _MSC_VER >= 1800
UDFORCE_INLINE float udLog2(float f) { return log2f(f); }
UDFORCE_INLINE double udLog2(double d) { return log2(d); }
#else
UDFORCE_INLINE float udLog2(float f) { return udLog(f, 2.f); }
UDFORCE_INLINE double udLog2(double d) { return udLog(d, 2.0); }
#endif

UDFORCE_INLINE float udRSqrt(float f) { return 1.f/sqrtf(f); }
UDFORCE_INLINE double udRSqrt(double d) { return 1.0/sqrt(d); }
UDFORCE_INLINE float udSqrt(float f) { return sqrtf(f); }
UDFORCE_INLINE double udSqrt(double d) { return sqrt(d); }
UDFORCE_INLINE float udSin(float f) { return sinf(f); }
UDFORCE_INLINE double udSin(double d) { return sin(d); }
UDFORCE_INLINE float udCos(float f) { return cosf(f); }
UDFORCE_INLINE double udCos(double d) { return cos(d); }
UDFORCE_INLINE float udSinh(float f) { return sinhf(f); }
UDFORCE_INLINE double udSinh(double d) { return sinh(d); }
UDFORCE_INLINE float udCosh(float f) { return coshf(f); }
UDFORCE_INLINE double udCosh(double d) { return cosh(d); }
UDFORCE_INLINE float udTan(float f) { return tanf(f); }
UDFORCE_INLINE double udTan(double d) { return tan(d); }
UDFORCE_INLINE float udASin(float f) { return asinf(f); }
UDFORCE_INLINE double udASin(double d) { return asin(d); }
UDFORCE_INLINE float udACos(float f) { return acosf(f); }
UDFORCE_INLINE double udACos(double d) { return acos(d); }
UDFORCE_INLINE float udASinh(float f) { return asinhf(f); }
UDFORCE_INLINE double udASinh(double d) { return asinh(d); }
UDFORCE_INLINE float udACosh(float f) { return acosf(f); }
UDFORCE_INLINE double udACosh(double d) { return acosh(d); }
UDFORCE_INLINE float udATan(float f) { return atanf(f); }
UDFORCE_INLINE double udATan(double d) { return atan(d); }
UDFORCE_INLINE float udATanh(float f) { return atanhf(f); }
UDFORCE_INLINE double udATanh(double d) { return atanh(d); }
UDFORCE_INLINE float udATan2(float y, float x) { return atan2f(y, x); }
UDFORCE_INLINE double udATan2(double y, double x) { return atan2(y, x); }

UDFORCE_INLINE float udRound(float f) { return f >= 0.0f ? floorf(f + 0.5f) : ceilf(f - 0.5f); }
UDFORCE_INLINE double udRound(double d) { return d >= 0.0 ? floor(d + 0.5) : ceil(d - 0.5); }
UDFORCE_INLINE float udFloor(float f) { return floorf(f); }
UDFORCE_INLINE double udFloor(double d) { return floor(d); }
UDFORCE_INLINE float udCeil(float f) { return ceilf(f); }
UDFORCE_INLINE double udCeil(double d) { return ceil(d); }
UDFORCE_INLINE float udMod(float f, float den) { return fmodf(f, den); }
UDFORCE_INLINE double udMod(double d, double den) { return fmod(d, den); }

template <typename T>
T udHighestBitValue(T i)
{
  T result = 0;
  while (i)
  {
    result = (i & (~i + 1)); // grab lowest bit
    i &= ~result; // clear lowest bit
  }
  return result;
}

template <typename T> bool udIsPowerOfTwo(T i) { return !(i & (i - 1)); }
template <typename T> T udPowerOfTwoAbove(T v) { return udIsPowerOfTwo(v) ? v : udHighestBitValue(v) << 1; }

template <typename T> T udRoundEven(T t)
{
  int integer = (int)t;
  T integerPart = (integer < 0 ? udCeil(t) : udFloor(t));
  T fractionalPart = udAbs(t - integer);

  if (fractionalPart > T(0.5) || fractionalPart < T(0.5))
    return udRound(t);
  else if ((integer % 2) == 0)
    return integerPart;
  else if (integer < 0)
    return integerPart - T(1.0); // Negative values should be +1 negative
  else
    return integerPart + T(1.0);
}

template <typename T> T            udAbs(T v) { return v < T(0) ? -v : v; }
template <typename T> udVector2<T> udAbs(const udVector2<T> &v) { udVector2<T> r = { v.x<T(0)?-v.x:v.x, v.y<T(0)?-v.y:v.y }; return r; }
template <typename T> udVector3<T> udAbs(const udVector3<T> &v) { udVector3<T> r = { v.x<T(0)?-v.x:v.x, v.y<T(0)?-v.y:v.y, v.z<T(0)?-v.z:v.z }; return r; }
template <typename T> udVector4<T> udAbs(const udVector4<T> &v) { udVector4<T> r = { v.x<T(0)?-v.x:v.x, v.y<T(0)?-v.y:v.y, v.z<T(0)?-v.z:v.z, v.w<T(0)?-v.w:v.w }; return r; }
template <typename T> udQuaternion<T> udAbs(const udQuaternion<T> &q) { udQuaternion<T> r = { q.x<T(0) ? -q.x : q.x, q.y<T(0) ? -q.y : q.y, q.z<T(0) ? -q.z : q.z, q.w<T(0) ? -q.w : q.w }; return r; }

template <typename T> udVector2<T> udMin(const udVector2<T> &v1, const udVector2<T> &v2) { udVector2<T> r = { v1.x<v2.x?v1.x:v2.x, v1.y<v2.y?v1.y:v2.y }; return r; }
template <typename T> udVector3<T> udMin(const udVector3<T> &v1, const udVector3<T> &v2) { udVector3<T> r = { v1.x<v2.x?v1.x:v2.x, v1.y<v2.y?v1.y:v2.y, v1.z<v2.z?v1.z:v2.z }; return r; }
template <typename T> udVector4<T> udMin(const udVector4<T> &v1, const udVector4<T> &v2) { udVector4<T> r = { v1.x<v2.x?v1.x:v2.x, v1.y<v2.y?v1.y:v2.y, v1.z<v2.z?v1.z:v2.z, v1.w<v2.w?v1.w:v2.w }; return r; }
template <typename T> T            udMinElement(const udVector2<T> &v) { return udMin(v.x, v.y); }
template <typename T> T            udMinElement(const udVector3<T> &v) { return udMin(udMin(v.x, v.y), v.z); }
template <typename T> T            udMinElement(const udVector4<T> &v) { return udMin(udMin(v.x, v.y), udMin(v.z, v.w)); }
template <typename T> udVector2<T> udMax(const udVector2<T> &v1, const udVector2<T> &v2) { udVector2<T> r = { v1.x>v2.x?v1.x:v2.x, v1.y>v2.y?v1.y:v2.y }; return r; }
template <typename T> udVector3<T> udMax(const udVector3<T> &v1, const udVector3<T> &v2) { udVector3<T> r = { v1.x>v2.x?v1.x:v2.x, v1.y>v2.y?v1.y:v2.y, v1.z>v2.z?v1.z:v2.z }; return r; }
template <typename T> udVector4<T> udMax(const udVector4<T> &v1, const udVector4<T> &v2) { udVector4<T> r = { v1.x>v2.x?v1.x:v2.x, v1.y>v2.y?v1.y:v2.y, v1.z>v2.z?v1.z:v2.z, v1.w>v2.w?v1.w:v2.w }; return r; }
template <typename T> T            udMaxElement(const udVector2<T> &v) { return udMax(v.x, v.y); }
template <typename T> T            udMaxElement(const udVector3<T> &v) { return udMax(udMax(v.x, v.y), v.z); }
template <typename T> T            udMaxElement(const udVector4<T> &v) { return udMax(udMax(v.x, v.y), udMax(v.z, v.w)); }
template <typename T> T            udClamp(T v, T _min, T _max) { return v<_min?_min:(v>_max?_max:v); }
template <typename T> udVector2<T> udClamp(const udVector2<T> &v, const udVector2<T> &_min, const udVector2<T> &_max) { udVector2<T> r = { v.x<_min.x?_min.x:(v.x>_max.x?_max.x:v.x), v.y<_min.y?_min.y:(v.y>_max.y?_max.y:v.y) }; return r; }
template <typename T> udVector3<T> udClamp(const udVector3<T> &v, const udVector3<T> &_min, const udVector3<T> &_max) { udVector3<T> r = { v.x<_min.x?_min.x:(v.x>_max.x?_max.x:v.x), v.y<_min.y?_min.y:(v.y>_max.y?_max.y:v.y), v.z<_min.z?_min.z:(v.z>_max.z?_max.z:v.z) }; return r; }
template <typename T> udVector4<T> udClamp(const udVector4<T> &v, const udVector4<T> &_min, const udVector4<T> &_max) { udVector4<T> r = { v.x<_min.x?_min.x:(v.x>_max.x?_max.x:v.x), v.y<_min.y?_min.y:(v.y>_max.y?_max.y:v.y), v.z<_min.z?_min.z:(v.z>_max.z?_max.z:v.z), v.w<_min.w?_min.w:(v.w>_max.w?_max.w:v.w) }; return r; }

template <typename T> T            udSaturate(const T &v) { return udClamp(v, (T)0, (T)1); }
template <typename T> udVector2<T> udSaturate(const udVector2<T> &v) { return udClamp(v, udVector2<T>::zero(), udVector2<T>::one()); }
template <typename T> udVector3<T> udSaturate(const udVector3<T> &v) { return udClamp(v, udVector3<T>::zero(), udVector3<T>::one()); }
template <typename T> udVector4<T> udSaturate(const udVector4<T> &v) { return udClamp(v, udVector4<T>::zero(), udVector4<T>::one()); }

template <typename V, typename T> bool udIsUnitLength(const V &v, T epsilon) { return udAbs(typename V::ElementType(1) - udMag(v)) < typename V::ElementType(epsilon); }

template <typename T> UDFORCE_INLINE T udDot(const udVector2<T> &v1, const udVector2<T> &v2) { return udDot2(v1, v2); }
template <typename T> UDFORCE_INLINE T udDot(const udVector3<T> &v1, const udVector3<T> &v2) { return udDot3(v1, v2); }
template <typename T> UDFORCE_INLINE T udDot(const udVector4<T> &v1, const udVector4<T> &v2) { return udDot4(v1, v2); }
template <typename T> UDFORCE_INLINE T udDot(const udQuaternion<T> &q1, const udQuaternion<T> &q2) { return udDotQ(q1, q2); }

template <typename T> T udDot2(const udVector2<T> &v1, const udVector2<T> &v2) { return v1.x*v2.x + v1.y*v2.y; }
template <typename T> T udDot2(const udVector3<T> &v1, const udVector3<T> &v2) { return v1.x*v2.x + v1.y*v2.y; }
template <typename T> T udDot2(const udVector4<T> &v1, const udVector4<T> &v2) { return v1.x*v2.x + v1.y*v2.y; }
template <typename T> T udDot3(const udVector3<T> &v1, const udVector3<T> &v2) { return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z; }
template <typename T> T udDot3(const udVector4<T> &v1, const udVector4<T> &v2) { return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z; }
template <typename T> T udDot4(const udVector4<T> &v1, const udVector4<T> &v2) { return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w; }
template <typename T> T udDoth(const udVector3<T> &v3, const udVector4<T> &v4) { return v3.x*v4.x + v3.y*v4.y + v3.z*v4.z + v4.w; }
template <typename T> T udDotQ(const udQuaternion<T> &q1, const udQuaternion<T> &q2) { return q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w; }


template <typename T> T UDFORCE_INLINE udMagSq(const udVector2<T> &v) { return udMagSq2(v); }
template <typename T> T UDFORCE_INLINE udMagSq(const udVector3<T> &v) { return udMagSq3(v); }
template <typename T> T UDFORCE_INLINE udMagSq(const udVector4<T> &v) { return udMagSq4(v); }
template <typename T> T UDFORCE_INLINE udMagSq(const udQuaternion<T> &q) { return udMagSqQ(q); }

template <typename T> T udMagSq2(const udVector2<T> &v) { return v.x*v.x + v.y*v.y; }
template <typename T> T udMagSq2(const udVector3<T> &v) { return v.x*v.x + v.y*v.y; }
template <typename T> T udMagSq2(const udVector4<T> &v) { return v.x*v.x + v.y*v.y; }
template <typename T> T udMagSq3(const udVector3<T> &v) { return v.x*v.x + v.y*v.y + v.z*v.z; }
template <typename T> T udMagSq3(const udVector4<T> &v) { return v.x*v.x + v.y*v.y + v.z*v.z; }
template <typename T> T udMagSq4(const udVector4<T> &v) { return v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w; }
template <typename T> T udMagSqQ(const udQuaternion<T> &q) { return q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w; }


template <typename T> T UDFORCE_INLINE udMag(const udVector2<T> &v) { return udMag2(v); }
template <typename T> T UDFORCE_INLINE udMag(const udVector3<T> &v) { return udMag3(v); }
template <typename T> T UDFORCE_INLINE udMag(const udVector4<T> &v) { return udMag4(v); }
template <typename T> T UDFORCE_INLINE udMag(const udQuaternion<T> &v) { return udMagQ(v); }

template <typename T> T udMag2(const udVector2<T> &v) { return udSqrt(v.x*v.x + v.y*v.y); }
template <typename T> T udMag2(const udVector3<T> &v) { return udSqrt(v.x*v.x + v.y*v.y); }
template <typename T> T udMag2(const udVector4<T> &v) { return udSqrt(v.x*v.x + v.y*v.y); }
template <typename T> T udMag3(const udVector3<T> &v) { return udSqrt(v.x*v.x + v.y*v.y + v.z*v.z); }
template <typename T> T udMag3(const udVector4<T> &v) { return udSqrt(v.x*v.x + v.y*v.y + v.z*v.z); }
template <typename T> T udMag4(const udVector4<T> &v) { return udSqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w); }
template <typename T> T udMagQ(const udQuaternion<T> &q) { return udSqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w); }

template <typename T> UDFORCE_INLINE T udCross(const udVector2<T> &v1, const udVector2<T> &v2) { return udCross2(v1, v2); }
template <typename T> UDFORCE_INLINE udVector3<T> udCross(const udVector3<T> &v1, const udVector3<T> &v2) { return udCross3(v1, v2); }

template <typename T> T udCross2(const udVector2<T> &v1, const udVector2<T> &v2) { return v1.x*v2.y - v1.y*v2.x; }
template <typename T> T udCross2(const udVector3<T> &v1, const udVector3<T> &v2) { return v1.x*v2.y - v1.y*v2.x; }
template <typename T> T udCross2(const udVector4<T> &v1, const udVector4<T> &v2) { return v1.x*v2.y - v1.y*v2.x; }

template <typename T> udVector3<T> udCross3(const udVector3<T> &v1, const udVector3<T> &v2) { udVector3<T> r = { v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x }; return r; }
template <typename T> udVector3<T> udCross3(const udVector4<T> &v1, const udVector4<T> &v2) { udVector3<T> r = { v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x }; return r; }


template <typename T> UDFORCE_INLINE udVector2<T> udNormalize(const udVector2<T> &v) { return udNormalize2(v); }
template <typename T> UDFORCE_INLINE udVector3<T> udNormalize(const udVector3<T> &v) { return udNormalize3(v); }
template <typename T> UDFORCE_INLINE udVector4<T> udNormalize(const udVector4<T> &v) { return udNormalize4(v); }
template <typename T> UDFORCE_INLINE udQuaternion<T> udNormalize(const udQuaternion<T> &q) { return udNormalizeQ(q); }

template <typename T> udVector2<T> udNormalize2(const udVector2<T> &v) { T s = udRSqrt(v.x*v.x + v.y*v.y); udVector2<T> r = { v.x*s, v.y*s }; return r; }
template <typename T> udVector3<T> udNormalize2(const udVector3<T> &v) { T s = udRSqrt(v.x*v.x + v.y*v.y); udVector3<T> r = { v.x*s, v.y*s, v.z }; return r; }
template <typename T> udVector4<T> udNormalize2(const udVector4<T> &v) { T s = udRSqrt(v.x*v.x + v.y*v.y); udVector4<T> r = { v.x*s, v.y*s, v.z, v.w }; return r; }
template <typename T> udVector3<T> udNormalize3(const udVector3<T> &v) { T s = udRSqrt(v.x*v.x + v.y*v.y + v.z*v.z); udVector3<T> r = { v.x*s, v.y*s, v.z*s }; return r; }
template <typename T> udVector4<T> udNormalize3(const udVector4<T> &v) { T s = udRSqrt(v.x*v.x + v.y*v.y + v.z*v.z); udVector4<T> r = { v.x*s, v.y*s, v.z*s, v.w }; return r; }
template <typename T> udVector4<T> udNormalize4(const udVector4<T> &v) { T s = udRSqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w); udVector4<T> r = { v.x*s, v.y*s, v.z*s, v.w*s }; return r; }
template <typename T> udQuaternion<T> udNormalizeQ(const udQuaternion<T> &v) { T s = udRSqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w); udQuaternion<T> r = { v.x*s, v.y*s, v.z*s, v.w*s }; return r; }

template <typename V>
inline bool udEqualApprox(const V &a, const V &b, typename V::ElementType epsilon)
{
  typedef typename V::ElementType ET;
  V d = udAbs(a - b);

  ET error = ET(0);
  for (int i = 0; i < V::ElementCount; ++i)
    error += d[i];

  if (error > ET(epsilon))
    return false;

  return true;
}

template <typename T>
inline T udNormaliseRotation(T rad, T absRange)
{
  absRange = udAbs(absRange);
  if (rad > absRange || rad < -absRange)
  {
    T c = T(int64_t(rad / absRange));
    return rad - (c * absRange);
  }
  return rad;
}

// many kinds of mul...
template <typename T, typename U>
udMatrix4x4<T> udMul(const udMatrix4x4<T> &m, U f)
{
  udMatrix4x4<T> r = {{{ T(m.a[0] * f), T(m.a[1] * f), T(m.a[2] * f), T(m.a[3] * f),
                         T(m.a[4] * f), T(m.a[5] * f), T(m.a[6] * f), T(m.a[7] * f),
                         T(m.a[8] * f), T(m.a[9] * f), T(m.a[10] * f),T(m.a[11] * f),
                         T(m.a[12] * f),T(m.a[13] * f),T(m.a[14] * f),T(m.a[15] * f) }}};
  return r;
}
template <typename T>
udVector2<T> udMul(const udMatrix4x4<T> &m, const udVector2<T> &v)
{
  udVector2<T> r;
  r.x = m.m._00*v.x + m.m._01*v.y + m.m._03;
  r.y = m.m._10*v.x + m.m._11*v.y + m.m._13;
  return r;
}
template <typename T>
udVector3<T> udMul(const udMatrix4x4<T> &m, const udVector3<T> &v)
{
  udVector3<T> r;
  r.x = m.m._00*v.x + m.m._01*v.y + m.m._02*v.z + m.m._03;
  r.y = m.m._10*v.x + m.m._11*v.y + m.m._12*v.z + m.m._13;
  r.z = m.m._20*v.x + m.m._21*v.y + m.m._22*v.z + m.m._23;
  return r;
}
template <typename T>
udVector4<T> udMul(const udMatrix4x4<T> &m, const udVector4<T> &v)
{
  udVector4<T> r;
  r.x = m.m._00*v.x + m.m._01*v.y + m.m._02*v.z + m.m._03*v.w;
  r.y = m.m._10*v.x + m.m._11*v.y + m.m._12*v.z + m.m._13*v.w;
  r.z = m.m._20*v.x + m.m._21*v.y + m.m._22*v.z + m.m._23*v.w;
  r.w = m.m._30*v.x + m.m._31*v.y + m.m._32*v.z + m.m._33*v.w;
  return r;
}
template <typename T>
udMatrix4x4<T> udMul(const udMatrix4x4<T> &m1, const udMatrix4x4<T> &m2)
{
  udMatrix4x4<T> r;
  r.m._00 = m1.m._00*m2.m._00 + m1.m._01*m2.m._10 + m1.m._02*m2.m._20 + m1.m._03*m2.m._30;
  r.m._01 = m1.m._00*m2.m._01 + m1.m._01*m2.m._11 + m1.m._02*m2.m._21 + m1.m._03*m2.m._31;
  r.m._02 = m1.m._00*m2.m._02 + m1.m._01*m2.m._12 + m1.m._02*m2.m._22 + m1.m._03*m2.m._32;
  r.m._03 = m1.m._00*m2.m._03 + m1.m._01*m2.m._13 + m1.m._02*m2.m._23 + m1.m._03*m2.m._33;
  r.m._10 = m1.m._10*m2.m._00 + m1.m._11*m2.m._10 + m1.m._12*m2.m._20 + m1.m._13*m2.m._30;
  r.m._11 = m1.m._10*m2.m._01 + m1.m._11*m2.m._11 + m1.m._12*m2.m._21 + m1.m._13*m2.m._31;
  r.m._12 = m1.m._10*m2.m._02 + m1.m._11*m2.m._12 + m1.m._12*m2.m._22 + m1.m._13*m2.m._32;
  r.m._13 = m1.m._10*m2.m._03 + m1.m._11*m2.m._13 + m1.m._12*m2.m._23 + m1.m._13*m2.m._33;
  r.m._20 = m1.m._20*m2.m._00 + m1.m._21*m2.m._10 + m1.m._22*m2.m._20 + m1.m._23*m2.m._30;
  r.m._21 = m1.m._20*m2.m._01 + m1.m._21*m2.m._11 + m1.m._22*m2.m._21 + m1.m._23*m2.m._31;
  r.m._22 = m1.m._20*m2.m._02 + m1.m._21*m2.m._12 + m1.m._22*m2.m._22 + m1.m._23*m2.m._32;
  r.m._23 = m1.m._20*m2.m._03 + m1.m._21*m2.m._13 + m1.m._22*m2.m._23 + m1.m._23*m2.m._33;
  r.m._30 = m1.m._30*m2.m._00 + m1.m._31*m2.m._10 + m1.m._32*m2.m._20 + m1.m._33*m2.m._30;
  r.m._31 = m1.m._30*m2.m._01 + m1.m._31*m2.m._11 + m1.m._32*m2.m._21 + m1.m._33*m2.m._31;
  r.m._32 = m1.m._30*m2.m._02 + m1.m._31*m2.m._12 + m1.m._32*m2.m._22 + m1.m._33*m2.m._32;
  r.m._33 = m1.m._30*m2.m._03 + m1.m._31*m2.m._13 + m1.m._32*m2.m._23 + m1.m._33*m2.m._33;
  return r;
}

template <typename T>
udMatrix4x4<T> udAdd(const udMatrix4x4<T> &m1, const udMatrix4x4<T> &m2)
{
  udMatrix4x4<T> r;
  r.m._00 = m1.m._00+m2.m._00;
  r.m._01 = m1.m._01+m2.m._01;
  r.m._02 = m1.m._02+m2.m._02;
  r.m._03 = m1.m._03+m2.m._03;
  r.m._10 = m1.m._10+m2.m._10;
  r.m._11 = m1.m._11+m2.m._11;
  r.m._12 = m1.m._12+m2.m._12;
  r.m._13 = m1.m._13+m2.m._13;
  r.m._20 = m1.m._20+m2.m._20;
  r.m._21 = m1.m._21+m2.m._21;
  r.m._22 = m1.m._22+m2.m._22;
  r.m._23 = m1.m._23+m2.m._23;
  r.m._30 = m1.m._30+m2.m._30;
  r.m._31 = m1.m._31+m2.m._31;
  r.m._32 = m1.m._32+m2.m._32;
  r.m._33 = m1.m._33+m2.m._33;
  return r;
}

template <typename T>
udMatrix4x4<T> udSub(const udMatrix4x4<T> &m1, const udMatrix4x4<T> &m2)
{
  udMatrix4x4<T> r;
  r.m._00 = m1.m._00 - m2.m._00;
  r.m._01 = m1.m._01 - m2.m._01;
  r.m._02 = m1.m._02 - m2.m._02;
  r.m._03 = m1.m._03 - m2.m._03;
  r.m._10 = m1.m._10 - m2.m._10;
  r.m._11 = m1.m._11 - m2.m._11;
  r.m._12 = m1.m._12 - m2.m._12;
  r.m._13 = m1.m._13 - m2.m._13;
  r.m._20 = m1.m._20 - m2.m._20;
  r.m._21 = m1.m._21 - m2.m._21;
  r.m._22 = m1.m._22 - m2.m._22;
  r.m._23 = m1.m._23 - m2.m._23;
  r.m._30 = m1.m._30 - m2.m._30;
  r.m._31 = m1.m._31 - m2.m._31;
  r.m._32 = m1.m._32 - m2.m._32;
  r.m._33 = m1.m._33 - m2.m._33;
  return r;
}

template <typename T>
udQuaternion<T> udMul(const udQuaternion<T> &q1, const udQuaternion<T> &q2)
{
  udQuaternion<T> r;
  r.x =  q1.x * q2.w + q1.y * q2.z - q1.z * q2.y + q1.w * q2.x;
  r.y = -q1.x * q2.z + q1.y * q2.w + q1.z * q2.x + q1.w * q2.y;
  r.z =  q1.x * q2.y - q1.y * q2.x + q1.z * q2.w + q1.w * q2.z;
  r.w = -q1.x * q2.x - q1.y * q2.y - q1.z * q2.z + q1.w * q2.w;
  return r;
}

template <typename T>
T udLerp(T a, T b, double t)
{
  return T(a + (b-a)*t);
}

template <typename T>
T udBiLerp(T a, T b, T c, T d, double t1, double t2)
{
  return udLerp(udLerp(a, b, t1), udLerp(c, d, t1),t2);
}

template <typename T>
udVector2<T> udLerp(const udVector2<T> &v1, const udVector2<T> &v2, double t)
{
  udVector2<T> r;
  double invT = 1.0 - t;
  r.x = T(v1.x*invT + v2.x*t);
  r.y = T(v1.y*invT + v2.y*t);
  return r;
}
template <typename T>
udVector3<T> udLerp(const udVector3<T> &v1, const udVector3<T> &v2, double t)
{
  udVector3<T> r;
  double invT = 1.0 - t;
  r.x = T(v1.x*invT + v2.x*t);
  r.y = T(v1.y*invT + v2.y*t);
  r.z = T(v1.z*invT + v2.z*t);
  return r;
}
template <typename T>
udVector4<T> udLerp(const udVector4<T> &v1, const udVector4<T> &v2, double t)
{
  udVector4<T> r;
  double invT = 1.0 - t;
  r.x = T(v1.x*invT + v2.x*t);
  r.y = T(v1.y*invT + v2.y*t);
  r.z = T(v1.z*invT + v2.z*t);
  r.w = T(v1.w*invT + v2.w*t);
  return r;
}
template <typename T>
udMatrix4x4<T> udLerp(const udMatrix4x4<T> &m1, const udMatrix4x4<T> &m2, double t)
{
  return (1.0-t)*m1 + t*m2;
}
template <typename T>
udQuaternion<T> udLerp(const udQuaternion<T> &q1, const udQuaternion<T> &q2, double t)
{
  udQuaternion<T> r;
  double invT = 1.0 - t;
  r.x = T(q1.x*invT + q2.x*t);
  r.y = T(q1.y*invT + q2.y*t);
  r.z = T(q1.z*invT + q2.z*t);
  r.w = T(q1.w*invT + q2.w*t);
  return r;
}

template <typename T>
udQuaternion<T> udSlerp(const udQuaternion<T> &q1, const udQuaternion<T> &_q2, double t)
{
#if UDASSERT_ON
  const double epsilon = 1.0 / 4096;
#endif
  const double thetaEpsilon = UD_PI / (180.0 * 100.0); // 1/100 of a degree

  udQuaternion<T> q2 = _q2;

  UDASSERT(udIsUnitLength(q1, epsilon), "q1 is not normalized, magnitude %f\n", udMagQ(q1));
  UDASSERT(udIsUnitLength(q2, epsilon), "q2 is not normalized, magnitude %f\n", udMagQ(q2));

  double cosHalfTheta = udDotQ(q1, q2); // Dot product of 2 quaterions results in cos(theta/2)

  if (cosHalfTheta < 0.0) // Rotation is greater than PI
  {
    q2 = -_q2;
    cosHalfTheta = -cosHalfTheta;
  }

  if ((1.0 - cosHalfTheta) < thetaEpsilon)
    return udNormalize(udLerp(q1, q2, t));

  double halfTheta = udACos(cosHalfTheta);

  double sinHalfTheta = udSqrt(1.0 - cosHalfTheta * cosHalfTheta);
  double ratioA = udSin(halfTheta * (1.0 - t)) / sinHalfTheta;
  double ratioB = udSin(halfTheta * t) / sinHalfTheta;

  udQuaternion<T> r;
  r.w = T(q1.w * ratioA + q2.w * ratioB);
  r.x = T(q1.x * ratioA + q2.x * ratioB);
  r.y = T(q1.y * ratioA + q2.y * ratioB);
  r.z = T(q1.z * ratioA + q2.z * ratioB);

  return r;
}

template <typename T>
udMatrix4x4<T> udTranspose(const udMatrix4x4<T> &m)
{
  udMatrix4x4<T> r = {{{ m.a[0], m.a[4], m.a[8], m.a[12],
                         m.a[1], m.a[5], m.a[9], m.a[13],
                         m.a[2], m.a[6], m.a[10], m.a[14],
                         m.a[3], m.a[7], m.a[11], m.a[15] }}};
  return r;
}

template <typename T>
T udDeterminant(const udMatrix4x4<T> &m)
{
  return m.m._03*m.m._12*m.m._21*m.m._30 - m.m._02*m.m._13*m.m._21*m.m._30 - m.m._03*m.m._11*m.m._22*m.m._30 + m.m._01*m.m._13*m.m._22*m.m._30 +
         m.m._02*m.m._11*m.m._23*m.m._30 - m.m._01*m.m._12*m.m._23*m.m._30 - m.m._03*m.m._12*m.m._20*m.m._31 + m.m._02*m.m._13*m.m._20*m.m._31 +
         m.m._03*m.m._10*m.m._22*m.m._31 - m.m._00*m.m._13*m.m._22*m.m._31 - m.m._02*m.m._10*m.m._23*m.m._31 + m.m._00*m.m._12*m.m._23*m.m._31 +
         m.m._03*m.m._11*m.m._20*m.m._32 - m.m._01*m.m._13*m.m._20*m.m._32 - m.m._03*m.m._10*m.m._21*m.m._32 + m.m._00*m.m._13*m.m._21*m.m._32 +
         m.m._01*m.m._10*m.m._23*m.m._32 - m.m._00*m.m._11*m.m._23*m.m._32 - m.m._02*m.m._11*m.m._20*m.m._33 + m.m._01*m.m._12*m.m._20*m.m._33 +
         m.m._02*m.m._10*m.m._21*m.m._33 - m.m._00*m.m._12*m.m._21*m.m._33 - m.m._01*m.m._10*m.m._22*m.m._33 + m.m._00*m.m._11*m.m._22*m.m._33;
}

template <typename T>
udMatrix4x4<T> udInverse(const udMatrix4x4<T> &m)
{
  udMatrix4x4<T> r;
  r.m._00 = m.m._12*m.m._23*m.m._31 - m.m._13*m.m._22*m.m._31 + m.m._13*m.m._21*m.m._32 - m.m._11*m.m._23*m.m._32 - m.m._12*m.m._21*m.m._33 + m.m._11*m.m._22*m.m._33;
  r.m._01 = m.m._03*m.m._22*m.m._31 - m.m._02*m.m._23*m.m._31 - m.m._03*m.m._21*m.m._32 + m.m._01*m.m._23*m.m._32 + m.m._02*m.m._21*m.m._33 - m.m._01*m.m._22*m.m._33;
  r.m._02 = m.m._02*m.m._13*m.m._31 - m.m._03*m.m._12*m.m._31 + m.m._03*m.m._11*m.m._32 - m.m._01*m.m._13*m.m._32 - m.m._02*m.m._11*m.m._33 + m.m._01*m.m._12*m.m._33;
  r.m._03 = m.m._03*m.m._12*m.m._21 - m.m._02*m.m._13*m.m._21 - m.m._03*m.m._11*m.m._22 + m.m._01*m.m._13*m.m._22 + m.m._02*m.m._11*m.m._23 - m.m._01*m.m._12*m.m._23;
  r.m._10 = m.m._13*m.m._22*m.m._30 - m.m._12*m.m._23*m.m._30 - m.m._13*m.m._20*m.m._32 + m.m._10*m.m._23*m.m._32 + m.m._12*m.m._20*m.m._33 - m.m._10*m.m._22*m.m._33;
  r.m._11 = m.m._02*m.m._23*m.m._30 - m.m._03*m.m._22*m.m._30 + m.m._03*m.m._20*m.m._32 - m.m._00*m.m._23*m.m._32 - m.m._02*m.m._20*m.m._33 + m.m._00*m.m._22*m.m._33;
  r.m._12 = m.m._03*m.m._12*m.m._30 - m.m._02*m.m._13*m.m._30 - m.m._03*m.m._10*m.m._32 + m.m._00*m.m._13*m.m._32 + m.m._02*m.m._10*m.m._33 - m.m._00*m.m._12*m.m._33;
  r.m._13 = m.m._02*m.m._13*m.m._20 - m.m._03*m.m._12*m.m._20 + m.m._03*m.m._10*m.m._22 - m.m._00*m.m._13*m.m._22 - m.m._02*m.m._10*m.m._23 + m.m._00*m.m._12*m.m._23;
  r.m._20 = m.m._11*m.m._23*m.m._30 - m.m._13*m.m._21*m.m._30 + m.m._13*m.m._20*m.m._31 - m.m._10*m.m._23*m.m._31 - m.m._11*m.m._20*m.m._33 + m.m._10*m.m._21*m.m._33;
  r.m._21 = m.m._03*m.m._21*m.m._30 - m.m._01*m.m._23*m.m._30 - m.m._03*m.m._20*m.m._31 + m.m._00*m.m._23*m.m._31 + m.m._01*m.m._20*m.m._33 - m.m._00*m.m._21*m.m._33;
  r.m._22 = m.m._01*m.m._13*m.m._30 - m.m._03*m.m._11*m.m._30 + m.m._03*m.m._10*m.m._31 - m.m._00*m.m._13*m.m._31 - m.m._01*m.m._10*m.m._33 + m.m._00*m.m._11*m.m._33;
  r.m._23 = m.m._03*m.m._11*m.m._20 - m.m._01*m.m._13*m.m._20 - m.m._03*m.m._10*m.m._21 + m.m._00*m.m._13*m.m._21 + m.m._01*m.m._10*m.m._23 - m.m._00*m.m._11*m.m._23;
  r.m._30 = m.m._12*m.m._21*m.m._30 - m.m._11*m.m._22*m.m._30 - m.m._12*m.m._20*m.m._31 + m.m._10*m.m._22*m.m._31 + m.m._11*m.m._20*m.m._32 - m.m._10*m.m._21*m.m._32;
  r.m._31 = m.m._01*m.m._22*m.m._30 - m.m._02*m.m._21*m.m._30 + m.m._02*m.m._20*m.m._31 - m.m._00*m.m._22*m.m._31 - m.m._01*m.m._20*m.m._32 + m.m._00*m.m._21*m.m._32;
  r.m._32 = m.m._02*m.m._11*m.m._30 - m.m._01*m.m._12*m.m._30 - m.m._02*m.m._10*m.m._31 + m.m._00*m.m._12*m.m._31 + m.m._01*m.m._10*m.m._32 - m.m._00*m.m._11*m.m._32;
  r.m._33 = m.m._01*m.m._12*m.m._20 - m.m._02*m.m._11*m.m._20 + m.m._02*m.m._10*m.m._21 - m.m._00*m.m._12*m.m._21 - m.m._01*m.m._10*m.m._22 + m.m._00*m.m._11*m.m._22;
  return r*(T(1)/udDeterminant(m));
}
template <typename T>
udQuaternion<T> udInverse(const udQuaternion<T> &q)
{
  T s = T(1)/(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
  udQuaternion<T> r = { -q.x*s, -q.y*s, -q.z*s, q.w*s };
  return r;
}

template <typename T>
udQuaternion<T> udConjugate(const udQuaternion<T> &q)
{
  udQuaternion<T> r = { -q.x, -q.y, -q.z, q.w };
  return r;
}

// udMatrix4x4 members
template <typename T>
udMatrix4x4<T>& udMatrix4x4<T>::transpose()
{
  *this = udTranspose(*this);
  return *this;
}

template <typename T>
T udMatrix4x4<T>::determinant()
{
  return udDeterminant(*this);
}

template <typename T>
udMatrix4x4<T>& udMatrix4x4<T>::inverse()
{
  *this = udInverse(*this);
  return *this;
}

template <typename T>
udVector3<T> udMatrix4x4<T>::extractYPR() const
{
  udVector3<T> mx = udNormalize3(axis.x.toVector3());
  udVector3<T> my = udNormalize3(axis.y.toVector3());
  udVector3<T> mz = udNormalize3(axis.z.toVector3());

  T Cy, Sy; // yaw
  T Cp, Sp; // pitch
  T Cr, Sr; // roll

  // our YPR matrix looks like this:
  // [ CyCr-SySpSr, -SyCp, CySr+CrSySp ] ** Remember, axiis are columns!
  // [ CrSy+CySpSr,  CyCp, SySr-CyCrSp ]
  // [    -SrCp,      Sp,     CrCp     ]

  // we know sin(pitch) without doing any work!
  Sp = my.z;

  // use trig identity (cos(p))^2 + (sin(p))^2 = 1, to find cos(p)
  Cp = udSqrt(1.0f - Sp*Sp);

  // There's an edge case when
  if (Cp >= 0.00001f)
  {
    // find the rest from other cells
    T factor = T(1)/Cp;
    Sy = -my.x*factor;
    Cy = my.y*factor;
    Sr = -mx.z*factor;
    Cr = mz.z*factor;
  }
  else
  {
    // if Cp = 0 and Sp = ±1 (pitch straight up or down), the above matrix becomes:
    // [ CyCr-SySr,  0,  CySr+CrSySp ] ** Remember, axiis are columns!
    // [ CrSy+CySr,  0,  SySr-CyCrSp ]
    // [     0,      Sp,     0       ]
    // we can't distinguish yaw from roll at this point,
    // we'll assume zero roll and take the rotation as yaw:
    Sr = 0.0f; //sin(0) = 0
    Cr = 1.0f; //cos(0) = 1

    // so we can now get the yaw:
    // [ Cy, 0,  SySp ] ** Remember, axiis are columns!
    // [ Sy, 0, -CySp ]
    // [ 0,  Sp,  0   ]
    Sy = mx.y;
    Cy = mx.x;
  }

  // use tan(y) = sin(y) / cos(y)
  // -> y = atan(sin(y) / cos(y))
  // -> y = atan2(sin(y) , cos(y))
  T y = udATan2(Sy, Cy);
  T p = udATan2(Sp, Cp);
  T r = udATan2(Sr, Cr);
  return udVector3<T>::create(y < T(0) ? y + T(UD_2PI) : y, p < T(0) ? p + T(UD_2PI) : p, r < T(0) ? r + T(UD_2PI) : r);
}


// udQuaternion members
template <typename T>
udQuaternion<T>& udQuaternion<T>::inverse()
{
  *this = udInverse(*this);
  return *this;
}

template <typename T>
udQuaternion<T>& udQuaternion<T>::conjugate()
{
  *this = udConjugate(*this);
  return *this;
}

template <typename T>
udVector3<T> udQuaternion<T>::apply(const udVector3<T> &v) const
{
  udVector3<T> r;

  T a = w*w - (x*x + y*y + z*z);
  T b = 2.0f*w;
  T c = 2.0f*(x*v.x + y*v.y + z*v.z);

  r.x = a*v.x + b*(y*v.z - z*v.y) + c*x;
  r.y = a*v.y + b*(z*v.x - x*v.z) + c*y;
  r.z = a*v.z + b*(x*v.y - y*v.x) + c*z;

  return r;
}

template <typename T>
udVector3<T> udQuaternion<T>::eulerAngles() const
{
  // Quaternion to Rotation Matrix
  //      1 - 2*y2 - 2*z2   2*x*y - 2*z*w 	  2*x*z + 2*y*w
  //      2*x*y + 2*z*w 	  1 - 2*x2 - 2*z2 	2*y*z - 2*x*w
  //      2*x*z - 2*y*w 	  2*y*z + 2*x*w 	  1 - 2*x2 - 2*y2

  udVector4<T> mx = udVector4<T>::create(T(1.0f) - T(2.0f) * (y * y + z * z), T(2.0f) * (x * y + z * w), T(2.0f) * (x * z - y * w), T(0.0f));
  udVector4<T> my = udVector4<T>::create(T(2.0f) * (x * y - z * w), T(1.0f) - T(2.0f) * (x * x + z * z), T(2.0f) * (y * z + x * w), T(0.0f));
  udVector4<T> mz = udVector4<T>::create(T(2.0f) * (x * z + y * w), T(2.0f) * (y * z - x * w), T(1.0f) - T(2.0f) * (x * x + y * y), T(0.0f));
  udVector4<T> mw = udVector4<T>::create(T(0.0f), T(0.0f), T(0.0f), T(0.0f));

  udMatrix4x4<T> rotationMatrix = udMatrix4x4<T>::create(mx, my, mz, mw);
  return rotationMatrix.extractYPR();
}

// udMatrix4x4 initialisers
template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::identity()
{
  udMatrix4x4<T> r = {{{ T(1),T(0),T(0),T(0),
                         T(0),T(1),T(0),T(0),
                         T(0),T(0),T(1),T(0),
                         T(0),T(0),T(0),T(1) }}};
  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::create(const T m[16])
{
  udMatrix4x4<T> r = {{{ m[0], m[1], m[2], m[3],
                         m[4], m[5], m[6], m[7],
                         m[8], m[9], m[10],m[11],
                         m[12],m[13],m[14],m[15] }}};
  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::create(T _00, T _10, T _20, T _30, T _01, T _11, T _21, T _31, T _02, T _12, T _22, T _32, T _03, T _13, T _23, T _33)
{
  udMatrix4x4<T> r = {{{ _00, _10, _20, _30,  // NOTE: remember, this looks a bit funny because we store columns (axiis) contiguous!
                         _01, _11, _21, _31,
                         _02, _12, _22, _32,
                         _03, _13, _23, _33 }}};
  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::create(const udVector4<T> &xColumn, const udVector4<T> &yColumn, const udVector4<T> &zColumn, const udVector4<T> &wColumn)
{
  udMatrix4x4<T> r = {{{ xColumn.x, xColumn.y, xColumn.z, xColumn.w,
                         yColumn.x, yColumn.y, yColumn.z, yColumn.w,
                         zColumn.x, zColumn.y, zColumn.z, zColumn.w,
                         wColumn.x, wColumn.y, wColumn.z, wColumn.w }}};
  return r;
}

template <typename T>
template <typename U> // OMG, nested templates... I didn't even know this was a thing!
udMatrix4x4<T> udMatrix4x4<T>::create(const udMatrix4x4<U> &_m)
{
  udMatrix4x4<T> r = {{{ T(_m.m._00), T(_m.m._10), T(_m.m._20), T(_m.m._30),
                         T(_m.m._01), T(_m.m._11), T(_m.m._21), T(_m.m._31),
                         T(_m.m._02), T(_m.m._12), T(_m.m._22), T(_m.m._32),
                         T(_m.m._03), T(_m.m._13), T(_m.m._23), T(_m.m._33) }}};
  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::rotationX(T rad, const udVector3<T> &t)
{
  T c = udCos(rad);
  T s = udSin(rad);
  udMatrix4x4<T> r = {{{ T(1),T(0),T(0),T(0),
                         T(0),  c ,  s ,T(0),
                         T(0), -s ,  c ,T(0),
                         t.x, t.y, t.z, T(1) }}};
  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::rotationY(T rad, const udVector3<T> &t)
{
  T c = udCos(rad);
  T s = udSin(rad);
  udMatrix4x4<T> r = {{{   c ,T(0), -s, T(0),
                         T(0),T(1),T(0),T(0),
                           s ,T(0),  c ,T(0),
                         t.x, t.y, t.z, T(1) }}};
  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::rotationZ(T rad, const udVector3<T> &t)
{
  T c = udCos(rad);
  T s = udSin(rad);
  udMatrix4x4<T> r = {{{   c ,  s ,T(0),T(0),
                          -s ,  c ,T(0),T(0),
                         T(0),T(0),T(1),T(0),
                         t.x, t.y, t.z, T(1) }}};
  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::rotationAxis(const udVector3<T> &axis, T rad, const udVector3<T> &t)
{
  T c = udCos(rad);
  T s = udSin(rad);
  udVector3<T> n = axis;
  udVector3<T> a = (T(1) - c) * axis;
  udMatrix4x4<T> r = {{{ a.x*n.x + c,     a.x*n.y + s*n.z, a.x*n.z - s*n.y, T(0),
                         a.y*n.x - s*n.z, a.y*n.y + c,     a.y*n.z + s*n.x, T(0),
                         a.z*n.x + s*n.y, a.z*n.y - s*n.x, a.z*n.z + c,     T(0),
                         t.x,             t.y,             t.z,             T(1) }}};
  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::rotationYPR(T y, T p, T r, const udVector3<T> &t)
{
  T c1 = udCos(y), c2 = udCos(p), c3 = udCos(r);
  T s1 = udSin(y), s2 = udSin(p), s3 = udSin(r);
  udMatrix4x4<T> result = {{{  c1*c3-s1*s2*s3, c3*s1+c1*s2*s3, -c2*s3, T(0),
                              -c2*s1,          c1*c2,           s2,    T(0),
                               c1*s3+c3*s1*s2, s1*s3-c1*c3*s2,  c2*c3, T(0),
                               t.x,            t.y,             t.z,   T(1) }}};
  return result;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::rotationQuat(const udQuaternion<T> &q, const udVector3<T> &t)
{
  UDASSERT(udIsUnitLength(q, T(1.0 / 4096)), "q is not normalized, magnitude %f\n", udMagQ(q));
  //This function makes the assumption the quaternion is normalized
  T qx2 = q.x * q.x;
  T qy2 = q.y * q.y;
  T qz2 = q.z * q.z;

  udMatrix4x4<T> r = {{{
    1 - 2 * qy2 - 2 * qz2,      2 * q.x*q.y + 2 * q.z*q.w,  2 * q.x*q.z - 2 * q.y*q.w,  T(0),
    2 * q.x*q.y - 2 * q.z*q.w,  1 - 2 * qx2 - 2 * qz2    ,  2 * q.y*q.z + 2 * q.x*q.w,  T(0),
    2 * q.x*q.z + 2 * q.y*q.w,  2 * q.y*q.z - 2 * q.x*q.w,  1 - 2 * qx2 - 2 * qy2,      T(0),
    t.x,                        t.y,                        t.z,                        T(1)
  }}};

  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::translation(T x, T y, T z)
{
  udMatrix4x4<T> r = {{{ T(1),T(0),T(0),T(0),
                         T(0),T(1),T(0),T(0),
                         T(0),T(0),T(1),T(0),
                         x,   y,   z, T(1) }}};
  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::scaleNonUniform(T x, T y, T z, const udVector3<T> &t)
{
  udMatrix4x4<T> r = {{{   x,  T(0), T(0), T(0),
                         T(0),   y,  T(0), T(0),
                         T(0), T(0),   z,  T(0),
                         t.x,  t.y,  t.z,  T(1) }}};
  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::perspectiveZO(T fovY, T aspectRatio, T zNear, T zFar)
{
  T fov = udTan(fovY / T(2));
  udMatrix4x4<T> r = {{{ T(1) / (aspectRatio*fov), T(0),         T(0),                                       T(0),
                         T(0),                     T(0),         zFar / (zFar - zNear),                      T(1),
                         T(0),                     T(1)/fov,     T(0),                                       T(0),
                         T(0),                     T(0),         -(zFar * zNear) / (zFar - zNear),           T(0) }}};
  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::perspectiveNO(T fovY, T aspectRatio, T zNear, T zFar)
{
  T fov = udTan(fovY / T(2));
  udMatrix4x4<T> r = {{{ T(1) / (aspectRatio*fov), T(0),         T(0),                                       T(0),
                         T(0),                     T(0),         (zFar + zNear) / (zFar - zNear),            T(1),
                         T(0),                     T(1) / fov,   T(0),                                       T(0),
                         T(0),                     T(0),         -(T(2) * zFar * zNear) / (zFar - zNear),    T(0) } } };
  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::orthoNO(T left, T right, T bottom, T top, T znear, T zfar)
{
  udMatrix4x4<T> r = {{{ T(2) / (right - left),            T(0),                             T(0),                             T(0),
                         T(0),                             T(0),                             T(2) / (zfar - znear),            T(0),
                         T(0),                             T(2) / (top - bottom),            T(0),                             T(0),
                         -(right + left) / (right - left), -(top + bottom) / (top - bottom), -(zfar + znear) / (zfar - znear), T(1) }}};
  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::orthoZO(T left, T right, T bottom, T top, T znear, T zfar)
{
  udMatrix4x4<T> r = {{{ T(2) / (right - left),            T(0),                             T(0),                             T(0),
                         T(0),                             T(0),                             T(1) / (zfar - znear),            T(0),
                         T(0),                             T(2) / (top - bottom),            T(0),                             T(0),
                         -(right + left) / (right - left), -(top + bottom) / (top - bottom), -znear / (zfar - znear),          T(1) }}};
  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::orthoForScreen(T width, T height, T znear, T zfar)
{
  udMatrix4x4<T> r = {{{ T(2) / width, T(0),           T(0),                            T(0),
                         T(0),         -T(2) / height, T(0),                            T(0),
                         T(0),         T(0),           T(2) / (zfar - znear),           T(0),
                         T(-1),        T(1),          -(zfar + znear) / (zfar - znear), T(1) }}};
  return r;
}

template <typename T>
udMatrix4x4<T> udMatrix4x4<T>::lookAt(const udVector3<T> &from, const udVector3<T> &at, const udVector3<T> &up)
{
  udVector3<T> y = udNormalize3(at - from);
  udVector3<T> x = udNormalize3(udCross3(y, up));
  udVector3<T> z = udCross3(x, y);
  udMatrix4x4<T> r = {{{ x.x,    x.y,    x.z,    0,
                         y.x,    y.y,    y.z,    0,
                         z.x,    z.y,    z.z,    0,
                         from.x, from.y, from.z, 1 }}};
  return r;
}

template <typename T>
udQuaternion<T> udQuaternion<T>::create(const udVector3<T> &axis, T rad)
{
  UDASSERT(udIsUnitLength(axis, T(1.0 / 4096)), "axis is not normalized, magnitude %f\n", udMag(axis));
  T a = rad*T(0.5);
  T s = udSin(a);
  udQuaternion<T> r = { axis.x*s,
                        axis.y*s,
                        axis.z*s,
                        udCos(a) };
  return r;
}

template <typename T>
udQuaternion<T> udQuaternion<T>::create(const T _y, const T _p, const T _r)
{
  udQuaternion<T> r;

  // Assuming the angles are in radians.
  T c1 = udCos(_y / 2); //Yaw
  T s1 = udSin(_y / 2); //Yaw
  T c2 = udCos(_p / 2); //Pitch
  T s2 = udSin(_p / 2); //Pitch
  T c3 = udCos(_r / 2); //Roll
  T s3 = udSin(_r / 2); //Roll
  T c1c2 = c1 * c2;
  T s1s2 = s1 * s2;
  T c1s2 = c1 * s2;
  T s1c2 = s1 * c2;

  r.w = c1c2 * c3 - s1s2 * s3;
  r.x = c1s2 * c3 - s1c2 * s3;
  r.y = c1c2 * s3 + s1s2 * c3;
  r.z = c1s2 * s3 + s1c2 * c3;

  return r;
}
