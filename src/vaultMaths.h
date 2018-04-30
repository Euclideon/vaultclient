#ifndef vaultMaths_h__
#define vaultMaths_h__

#include <cmath>
#include <cstring>

struct vaultDouble3;
struct vaultDouble4;
struct vaultMatrix;

vaultMatrix vaultMatrixMul(const vaultMatrix &m, double f);
vaultDouble3 vaultMatrixMul(const vaultMatrix &m, const vaultDouble3 &v);
vaultDouble4 vaultMatrixMul(const vaultMatrix &m, const vaultDouble4 &v);
vaultMatrix vaultMatrixMul(const vaultMatrix &m1, const vaultMatrix &m2);
vaultMatrix vaultMatrixAdd(const vaultMatrix &m1, const vaultMatrix &m2);
vaultMatrix vaultMatrixSub(const vaultMatrix &m1, const vaultMatrix &m2);
vaultDouble3 udNormalize3(const vaultDouble3 &v);
vaultDouble4 udNormalize3(const vaultDouble4 &v);
vaultDouble3 udCross3(const vaultDouble3 &v1, const vaultDouble3 &v2);

template <typename T>
struct vaultVector2
{
  T x, y;
  typedef T ElementType;
  enum { ElementCount = 2 };

  vaultVector2<T> operator -() const { vaultVector2<T> r = { -x, -y }; return r; }
  bool         operator ==(const vaultVector2<T> &v) const { return x == v.x && y == v.y; }
  bool         operator !=(const vaultVector2<T> &v) const { return !(*this == v); }

  vaultVector2<T> operator +(const vaultVector2<T> &v) const { vaultVector2<T> r = { x + v.x, y + v.y }; return r; }
  vaultVector2<T> operator -(const vaultVector2<T> &v) const { vaultVector2<T> r = { x - v.x, y - v.y }; return r; }
  vaultVector2<T> operator *(const vaultVector2<T> &v) const { vaultVector2<T> r = { x*v.x, y*v.y }; return r; }
  template <typename U>
  vaultVector2<T> operator *(U f) const { vaultVector2<T> r = { T(x*f), T(y*f) }; return r; }
  vaultVector2<T> operator /(const vaultVector2<T> &v) const { vaultVector2<T> r = { x / v.x, y / v.y }; return r; }
  template <typename U>
  vaultVector2<T> operator /(U f) const { vaultVector2<T> r = { T(x / f), T(y / f) }; return r; }
  T            operator [](size_t index) const { return ((T*)this)[index]; }

  vaultVector2<T>& operator +=(const vaultVector2<T> &v) { x += v.x; y += v.y; return *this; }
  vaultVector2<T>& operator -=(const vaultVector2<T> &v) { x -= v.x; y -= v.y; return *this; }
  vaultVector2<T>& operator *=(const vaultVector2<T> &v) { x *= v.x; y *= v.y; return *this; }
  template <typename U>
  vaultVector2<T>& operator *=(U f) { x = T(x*f); y = T(y*f); return *this; }
  vaultVector2<T>& operator /=(const vaultVector2<T> &v) { x /= v.x; y /= v.y; return *this; }
  template <typename U>
  vaultVector2<T>& operator /=(U f) { x = T(x / f); y = T(y / f); return *this; }
  T&            operator [](size_t index) { return ((T*)this)[index]; }

  // static members
  static vaultVector2<T> zero() { vaultVector2<T> r = { T(0), T(0) }; return r; }
  static vaultVector2<T> one() { vaultVector2<T> r = { T(1), T(1) }; return r; }

  static vaultVector2<T> xAxis() { vaultVector2<T> r = { T(1), T(0) }; return r; }
  static vaultVector2<T> yAxis() { vaultVector2<T> r = { T(0), T(1) }; return r; }

  static vaultVector2<T> create(T _f) { vaultVector2<T> r = { _f, _f }; return r; }
  static vaultVector2<T> create(T _x, T _y) { vaultVector2<T> r = { _x, _y }; return r; }
  template <typename U>
  static vaultVector2<T> create(const vaultVector2<U> &_v) { vaultVector2<T> r = { T(_v.x), T(_v.y) }; return r; }
};
template <typename T>
vaultVector2<T> operator *(T f, const vaultVector2<T> &v) { return v * f; }

typedef vaultVector2<int> vaultInt2;
typedef vaultVector2<uint32_t> vaultUint322;
typedef vaultVector2<float> vaultFloat2;
typedef vaultVector2<double> vaultDouble2;

struct vaultDouble3
{
  double x, y, z;

  vaultDouble3 operator -() const { vaultDouble3 r = { -x, -y, -z }; return r; }
  bool         operator ==(const vaultDouble3 &v) const { return x == v.x && y == v.y && z == v.z; }
  bool         operator !=(const vaultDouble3 &v) const { return !(*this == v); }

  vaultDouble3 operator +(const vaultDouble3 &v) const { vaultDouble3 r = { x + v.x, y + v.y, z + v.z }; return r; }
  vaultDouble3 operator -(const vaultDouble3 &v) const { vaultDouble3 r = { x - v.x, y - v.y, z - v.z }; return r; }
  vaultDouble3 operator *(const vaultDouble3 &v) const { vaultDouble3 r = { x*v.x, y*v.y, z*v.z  }; return r; }
  vaultDouble3 operator *(double f) const { vaultDouble3 r = { x*f, y*f, z*f }; return r; }
  vaultDouble3 operator /(const vaultDouble3 &v) const { vaultDouble3 r = { x / v.x, y / v.y, z / v.z }; return r; }
  vaultDouble3 operator /(double f) const { vaultDouble3 r = { x / f, y / f, z / f }; return r; }
  double       operator [](size_t index) const { return ((double*)this)[index]; }

  vaultDouble3& operator +=(const vaultDouble3 &v) { x += v.x; y += v.y; z += v.z; return *this; }
  vaultDouble3& operator -=(const vaultDouble3 &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
  vaultDouble3& operator *=(const vaultDouble3 &v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
  vaultDouble3& operator *=(double f) { x = x*f; y = y*f; z = z*f; return *this; }
  vaultDouble3& operator /=(const vaultDouble3 &v) { x /= v.x; y /= v.y; z /= v.z; return *this; }
  template <typename U>
  vaultDouble3& operator /=(U f) { x = T(x / f); y = T(y / f); z = T(z / f); return *this; }
  double&       operator [](size_t index) { return ((double*)this)[index]; }
};
struct vaultDouble4
{
  double x, y, z, w;

  vaultDouble4 operator -() const { vaultDouble4 r = { -x, -y, -z, -w }; return r; }
  bool         operator ==(const vaultDouble4 &v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
  bool         operator !=(const vaultDouble4 &v) const { return !(*this == v); }

  vaultDouble4 operator +(const vaultDouble4 &v) const { vaultDouble4 r = { x + v.x, y + v.y, z + v.z, w + v.w }; return r; }
  vaultDouble4 operator -(const vaultDouble4 &v) const { vaultDouble4 r = { x - v.x, y - v.y, z - v.z, w - v.w }; return r; }
  vaultDouble4 operator *(const vaultDouble4 &v) const { vaultDouble4 r = { x*v.x, y*v.y, z*v.z, w*v.w }; return r; }
  vaultDouble4 operator *(double f) const { vaultDouble4 r = { double(x*f), double(y*f), double(z*f), double(w*f) }; return r; }
  vaultDouble4 operator /(const vaultDouble4 &v) const { vaultDouble4 r = { x / v.x, y / v.y, z / v.z, w / v.w }; return r; }
  vaultDouble4 operator /(double f) const { vaultDouble4 r = { (x / f), (y / f), (z / f), (w / f) }; return r; }
  double       operator [](size_t index) const { return ((double*)this)[index]; }

  vaultDouble4& operator +=(const vaultDouble4 &v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
  vaultDouble4& operator -=(const vaultDouble4 &v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
  vaultDouble4& operator *=(const vaultDouble4 &v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
  template <typename U>
  vaultDouble4& operator *=(U f) { x = T(x*f); y = T(y*f); z = T(z*f); w = T(w*f); return *this; }
  vaultDouble4& operator /=(const vaultDouble4 &v) { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }
  template <typename U>
  vaultDouble4& operator /=(U f) { x = T(x / f); y = T(y / f); z = T(z / f); w = T(w / f); return *this; }
  double&       operator [](size_t index) { return ((double*)this)[index]; }
};

struct vaultMatrix
{
  union
  {
    double a[16];
    vaultDouble4 c[4];
    struct
    {
      vaultDouble4 x, y, z, t;
    } axis;
    struct
    {
      double // remember, we store columns (axis) sequentially! (so appears transposed here)
        _00, _10, _20, _30,
        _01, _11, _21, _31,
        _02, _12, _22, _32,
        _03, _13, _23, _33;
    } m;
  };

  vaultMatrix operator *(const vaultMatrix &m2) const { return vaultMatrixMul(*this, m2); }
  vaultMatrix operator *(double f)              const { return vaultMatrixMul(*this, f); }
  vaultMatrix operator +(const vaultMatrix &m2) const { return vaultMatrixAdd(*this, m2); }
  vaultMatrix operator -(const vaultMatrix &m2) const { return vaultMatrixSub(*this, m2); }
  vaultDouble4 operator *(const vaultDouble4 &v) const { return vaultMatrixMul(*this, v); }

  vaultMatrix& operator +=(const vaultMatrix &m2) { *this = vaultMatrixAdd(*this, m2); return *this; }
  vaultMatrix& operator -=(const vaultMatrix &m2) { *this = vaultMatrixSub(*this, m2); return *this; }
  vaultMatrix& operator *=(const vaultMatrix &m2) { *this = vaultMatrixMul(*this, m2); return *this; }
  vaultMatrix& operator *=(double f) { *this = vaultMatrixMul(*this, f); return *this; }

  bool operator ==(const vaultMatrix& rh) const { return memcmp(this, &rh, sizeof(*this)) == 0; }
};

vaultMatrix  vaultMatrix_LookAt(const vaultDouble3 &from, const vaultDouble3 &at, const vaultDouble3 &up)
{
  vaultDouble3 y = udNormalize3(at - from);
  vaultDouble3 x = udNormalize3(udCross3(y, up));
  vaultDouble3 z = udCross3(x, y);
  vaultMatrix r = { { { x.x,    x.y,    x.z,    0,
    y.x,    y.y,    y.z,    0,
    z.x,    z.y,    z.z,    0,
    from.x, from.y, from.z, 1 } } };
  return r;
}

vaultMatrix vaultMatrix_Identity()
{
  vaultMatrix r = { { { double(1),double(0),double(0),double(0),
    double(0),double(1),double(0),double(0),
    double(0),double(0),double(1),double(0),
    double(0),double(0),double(0),double(1) } } };
  return r;
}

vaultMatrix vaultMatrix_RotationAxis(const vaultDouble3 &axis, double rad, const vaultDouble3 &t = { 0,0,0 })
{
  double c = cos(rad);
  double s = sin(rad);
  vaultDouble3 n = axis;
  vaultDouble3 a = axis * (1.0 - c);
  vaultMatrix r = { { { a.x*n.x + c,     a.x*n.y + s*n.z, a.x*n.z - s*n.y, 0.0,
    a.y*n.x - s*n.z, a.y*n.y + c,     a.y*n.z + s*n.x, 0.0,
    a.z*n.x + s*n.y, a.z*n.y - s*n.x, a.z*n.z + c,     0.0,
    t.x,             t.y,             t.z,             1.0 } } };
  return r;
}


vaultMatrix vaultMatrixMul(const vaultMatrix &m, double f)
{
  vaultMatrix r = { { { double(m.a[0] * f), double(m.a[1] * f), double(m.a[2] * f), double(m.a[3] * f),
    double(m.a[4] * f), double(m.a[5] * f), double(m.a[6] * f), double(m.a[7] * f),
    double(m.a[8] * f), double(m.a[9] * f), double(m.a[10] * f),double(m.a[11] * f),
    double(m.a[12] * f),double(m.a[13] * f),double(m.a[14] * f),double(m.a[15] * f) } } };
  return r;
}
vaultDouble3 vaultMatrixMul(const vaultMatrix &m, const vaultDouble3 &v)
{
  vaultDouble3 r;
  r.x = m.m._00*v.x + m.m._01*v.y + m.m._02*v.z + m.m._03;
  r.y = m.m._10*v.x + m.m._11*v.y + m.m._12*v.z + m.m._13;
  r.z = m.m._20*v.x + m.m._21*v.y + m.m._22*v.z + m.m._23;
  return r;
}

vaultDouble4 vaultMatrixMul(const vaultMatrix &m, const vaultDouble4 &v)
{
  vaultDouble4 r;
  r.x = m.m._00*v.x + m.m._01*v.y + m.m._02*v.z + m.m._03*v.w;
  r.y = m.m._10*v.x + m.m._11*v.y + m.m._12*v.z + m.m._13*v.w;
  r.z = m.m._20*v.x + m.m._21*v.y + m.m._22*v.z + m.m._23*v.w;
  r.w = m.m._30*v.x + m.m._31*v.y + m.m._32*v.z + m.m._33*v.w;
  return r;
}

vaultMatrix vaultMatrixMul(const vaultMatrix &m1, const vaultMatrix &m2)
{
  vaultMatrix r;
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


vaultMatrix vaultMatrixAdd(const vaultMatrix &m1, const vaultMatrix &m2)
{
  vaultMatrix r;
  r.m._00 = m1.m._00 + m2.m._00;
  r.m._01 = m1.m._01 + m2.m._01;
  r.m._02 = m1.m._02 + m2.m._02;
  r.m._03 = m1.m._03 + m2.m._03;
  r.m._10 = m1.m._10 + m2.m._10;
  r.m._11 = m1.m._11 + m2.m._11;
  r.m._12 = m1.m._12 + m2.m._12;
  r.m._13 = m1.m._13 + m2.m._13;
  r.m._20 = m1.m._20 + m2.m._20;
  r.m._21 = m1.m._21 + m2.m._21;
  r.m._22 = m1.m._22 + m2.m._22;
  r.m._23 = m1.m._23 + m2.m._23;
  r.m._30 = m1.m._30 + m2.m._30;
  r.m._31 = m1.m._31 + m2.m._31;
  r.m._32 = m1.m._32 + m2.m._32;
  r.m._33 = m1.m._33 + m2.m._33;
  return r;
}


vaultMatrix vaultMatrixSub(const vaultMatrix &m1, const vaultMatrix &m2)
{
  vaultMatrix r;
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

vaultDouble3 udNormalize3(const vaultDouble3 &v) { double s = 1.0 / sqrt(v.x*v.x + v.y*v.y + v.z*v.z); vaultDouble3 r = { v.x*s, v.y*s, v.z*s }; return r; }
vaultDouble4 udNormalize3(const vaultDouble4 &v) { double s = 1.0 / sqrt(v.x*v.x + v.y*v.y + v.z*v.z); vaultDouble4 r = { v.x*s, v.y*s, v.z*s, v.w }; return r; }

vaultDouble3 udCross3(const vaultDouble3 &v1, const vaultDouble3 &v2)
{
  vaultDouble3 r = { v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x };
  return r;
}

#endif // vaultMaths_h__