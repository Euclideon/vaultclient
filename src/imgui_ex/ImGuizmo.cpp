// The MIT License(MIT)
//
// Copyright(c) 2016 Cedric Guillemet
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"
#include "ImGuizmo.h"

namespace ImGuizmo
{
  static const float gGizmoSizeClipSpace = 0.1f;
  const float screenRotateSize = 0.06f;

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // utility and math

  void FPU_MatrixF_x_MatrixF(const double *a, const double *b, double *r)
  {
    r[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
    r[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
    r[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
    r[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

    r[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
    r[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
    r[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
    r[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

    r[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
    r[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
    r[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
    r[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];

    r[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
    r[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
    r[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
    r[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
  }

  struct matrix_t;
  struct vec_t
  {
  public:
    double x, y, z, w;

    void Lerp(const vec_t& v, double t)
    {
      x += (v.x - x) * t;
      y += (v.y - y) * t;
      z += (v.z - z) * t;
      w += (v.w - w) * t;
    }

    void Set(double v) { x = y = z = w = v; }
    void Set(double _x, double _y, double _z = 0.f, double _w = 0.f) { x = _x; y = _y; z = _z; w = _w; }

    vec_t& operator -= (const vec_t& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    vec_t& operator += (const vec_t& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    vec_t& operator *= (const vec_t& v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
    vec_t& operator *= (double v) { x *= v;    y *= v;    z *= v;    w *= v;    return *this; }

    vec_t operator * (double f) const;
    vec_t operator - () const;
    vec_t operator - (const vec_t& v) const;
    vec_t operator + (const vec_t& v) const;
    vec_t operator * (const vec_t& v) const;

    const vec_t& operator + () const { return (*this); }
    double Length() const { return sqrt(x*x + y*y + z*z); };
    double LengthSq() const { return (x*x + y*y + z*z); };
    vec_t Normalize() { (*this) *= (1.0 / Length()); return (*this); }
    vec_t Normalize(const vec_t& v) { this->Set(v.x, v.y, v.z, v.w); this->Normalize(); return (*this); }
    vec_t Abs() const;
    void Cross(const vec_t& v)
    {
      vec_t res;
      res.x = y * v.z - z * v.y;
      res.y = z * v.x - x * v.z;
      res.z = x * v.y - y * v.x;

      x = res.x;
      y = res.y;
      z = res.z;
      w = 0.f;
    }
    void Cross(const vec_t& v1, const vec_t& v2)
    {
      x = v1.y * v2.z - v1.z * v2.y;
      y = v1.z * v2.x - v1.x * v2.z;
      z = v1.x * v2.y - v1.y * v2.x;
      w = 0.f;
    }
    double Dot(const vec_t &v) const
    {
      return (x * v.x) + (y * v.y) + (z * v.z) + (w * v.w);
    }
    double Dot3(const vec_t &v) const
    {
      return (x * v.x) + (y * v.y) + (z * v.z);
    }

    void Transform(const matrix_t& matrix);
    void Transform(const vec_t & s, const matrix_t& matrix);

    void TransformVector(const matrix_t& matrix);
    void TransformPoint(const matrix_t& matrix);
    void TransformVector(const vec_t& v, const matrix_t& matrix) { (*this) = v; this->TransformVector(matrix); }
    void TransformPoint(const vec_t& v, const matrix_t& matrix) { (*this) = v; this->TransformPoint(matrix); }

    double& operator [] (size_t index) { return ((double*)&x)[index]; }
    const double& operator [] (size_t index) const { return ((double*)&x)[index]; }
  };

  vec_t makeVect(double _x, double _y, double _z = 0.f, double _w = 0.f) { vec_t res; res.x = _x; res.y = _y; res.z = _z; res.w = _w; return res; }
  vec_t makeVect(ImVec2 v) { vec_t res; res.x = v.x; res.y = v.y; res.z = 0.f; res.w = 0.f; return res; }
  vec_t vec_t::operator * (double f) const { return makeVect(x * f, y * f, z * f, w *f); }
  vec_t vec_t::operator - () const { return makeVect(-x, -y, -z, -w); }
  vec_t vec_t::operator - (const vec_t& v) const { return makeVect(x - v.x, y - v.y, z - v.z, w - v.w); }
  vec_t vec_t::operator + (const vec_t& v) const { return makeVect(x + v.x, y + v.y, z + v.z, w + v.w); }
  vec_t vec_t::operator * (const vec_t& v) const { return makeVect(x * v.x, y * v.y, z * v.z, w * v.w); }
  vec_t vec_t::Abs() const { return makeVect(udAbs(x), udAbs(y), udAbs(z)); }

  vec_t Normalized(const vec_t& v) { vec_t res; res = v; res.Normalize(); return res; }
  vec_t Cross(const vec_t& v1, const vec_t& v2)
  {
    vec_t res;
    res.x = v1.y * v2.z - v1.z * v2.y;
    res.y = v1.z * v2.x - v1.x * v2.z;
    res.z = v1.x * v2.y - v1.y * v2.x;
    res.w = 0.f;
    return res;
  }

  double Dot(const vec_t &v1, const vec_t &v2)
  {
    return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
  }

  vec_t BuildPlan(const vec_t & p_point1, const vec_t & p_normal)
  {
    vec_t normal, res;
    normal.Normalize(p_normal);
    res.w = normal.Dot(p_point1);
    res.x = normal.x;
    res.y = normal.y;
    res.z = normal.z;
    return res;
  }

  struct matrix_t
  {
  public:

    union
    {
      double m[4][4];
      double a[16];
      struct
      {
        vec_t x, y, z, t;
      } axis;
      vec_t c[4];
    };

    matrix_t(const matrix_t& other) { memcpy(&a[0], &other.a[0], sizeof(double) * 16); }
    matrix_t() {}

    operator double * () { return a; }
    operator const double* () const { return a; }
    void Translation(double _x, double _y, double _z) { this->Translation(makeVect(_x, _y, _z)); }

    void Translation(const vec_t& vt)
    {
      axis.x.Set(1.f, 0.f, 0.f, 0.f);
      axis.y.Set(0.f, 1.f, 0.f, 0.f);
      axis.z.Set(0.f, 0.f, 1.f, 0.f);
      axis.t.Set(vt.x, vt.y, vt.z, 1.f);
    }

    void Scale(double _x, double _y, double _z)
    {
      axis.x.Set(_x, 0.f, 0.f, 0.f);
      axis.y.Set(0.f, _y, 0.f, 0.f);
      axis.z.Set(0.f, 0.f, _z, 0.f);
      axis.t.Set(0.f, 0.f, 0.f, 1.f);
    }
    void Scale(const vec_t& s) { Scale(s.x, s.y, s.z); }

    matrix_t& operator *= (const matrix_t& mat)
    {
      matrix_t tmpMat;
      tmpMat = *this;
      tmpMat.Multiply(mat);
      *this = tmpMat;
      return *this;
    }
    matrix_t operator * (const matrix_t& mat) const
    {
      matrix_t matT;
      matT.Multiply(*this, mat);
      return matT;
    }

    void Multiply(const matrix_t &matrix)
    {
      matrix_t tmp;
      tmp = *this;

      FPU_MatrixF_x_MatrixF((double*)&tmp, (double*)&matrix, (double*)this);
    }

    void Multiply(const matrix_t &m1, const matrix_t &m2)
    {
      FPU_MatrixF_x_MatrixF((double*)&m1, (double*)&m2, (double*)this);
    }

    double GetDeterminant() const
    {
      return m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[0][2] * m[1][0] * m[2][1] -
        m[0][2] * m[1][1] * m[2][0] - m[0][1] * m[1][0] * m[2][2] - m[0][0] * m[1][2] * m[2][1];
    }

    double Inverse(const matrix_t &srcMatrix, bool affine = false);
    void SetToIdentity()
    {
      axis.x.Set(1.f, 0.f, 0.f, 0.f);
      axis.y.Set(0.f, 1.f, 0.f, 0.f);
      axis.z.Set(0.f, 0.f, 1.f, 0.f);
      axis.t.Set(0.f, 0.f, 0.f, 1.f);
    }
    void Transpose()
    {
      matrix_t tmpm;
      for (int l = 0; l < 4; l++)
      {
        for (int c = 0; c < 4; c++)
        {
          tmpm.m[l][c] = m[c][l];
        }
      }
      (*this) = tmpm;
    }

    void RotationAxis(const vec_t & axis, double angle);

    void OrthoNormalize()
    {
      axis.x.Normalize();
      axis.y.Normalize();
      axis.z.Normalize();
    }
  };

  void vec_t::Transform(const matrix_t& matrix)
  {
    vec_t out;

    out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0] + w * matrix.m[3][0];
    out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1] + w * matrix.m[3][1];
    out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2] + w * matrix.m[3][2];
    out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3] + w * matrix.m[3][3];

    x = out.x;
    y = out.y;
    z = out.z;
    w = out.w;
  }

  void vec_t::Transform(const vec_t & s, const matrix_t& matrix)
  {
    *this = s;
    Transform(matrix);
  }

  void vec_t::TransformPoint(const matrix_t& matrix)
  {
    vec_t out;

    out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0] + matrix.m[3][0];
    out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1] + matrix.m[3][1];
    out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2] + matrix.m[3][2];
    out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3] + matrix.m[3][3];

    x = out.x;
    y = out.y;
    z = out.z;
    w = out.w;
  }


  void vec_t::TransformVector(const matrix_t& matrix)
  {
    vec_t out;

    out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0];
    out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1];
    out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2];
    out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3];

    x = out.x;
    y = out.y;
    z = out.z;
    w = out.w;
  }

  double matrix_t::Inverse(const matrix_t &srcMatrix, bool affine)
  {
    double det = 0;

    if (affine)
    {
      det = GetDeterminant();
      double s = 1 / det;
      m[0][0] = (srcMatrix.m[1][1] * srcMatrix.m[2][2] - srcMatrix.m[1][2] * srcMatrix.m[2][1]) * s;
      m[0][1] = (srcMatrix.m[2][1] * srcMatrix.m[0][2] - srcMatrix.m[2][2] * srcMatrix.m[0][1]) * s;
      m[0][2] = (srcMatrix.m[0][1] * srcMatrix.m[1][2] - srcMatrix.m[0][2] * srcMatrix.m[1][1]) * s;
      m[1][0] = (srcMatrix.m[1][2] * srcMatrix.m[2][0] - srcMatrix.m[1][0] * srcMatrix.m[2][2]) * s;
      m[1][1] = (srcMatrix.m[2][2] * srcMatrix.m[0][0] - srcMatrix.m[2][0] * srcMatrix.m[0][2]) * s;
      m[1][2] = (srcMatrix.m[0][2] * srcMatrix.m[1][0] - srcMatrix.m[0][0] * srcMatrix.m[1][2]) * s;
      m[2][0] = (srcMatrix.m[1][0] * srcMatrix.m[2][1] - srcMatrix.m[1][1] * srcMatrix.m[2][0]) * s;
      m[2][1] = (srcMatrix.m[2][0] * srcMatrix.m[0][1] - srcMatrix.m[2][1] * srcMatrix.m[0][0]) * s;
      m[2][2] = (srcMatrix.m[0][0] * srcMatrix.m[1][1] - srcMatrix.m[0][1] * srcMatrix.m[1][0]) * s;
      m[3][0] = -(m[0][0] * srcMatrix.m[3][0] + m[1][0] * srcMatrix.m[3][1] + m[2][0] * srcMatrix.m[3][2]);
      m[3][1] = -(m[0][1] * srcMatrix.m[3][0] + m[1][1] * srcMatrix.m[3][1] + m[2][1] * srcMatrix.m[3][2]);
      m[3][2] = -(m[0][2] * srcMatrix.m[3][0] + m[1][2] * srcMatrix.m[3][1] + m[2][2] * srcMatrix.m[3][2]);
    }
    else
    {
      // transpose matrix
      double src[16];
      for (int i = 0; i < 4; ++i)
      {
        src[i] = srcMatrix.a[i * 4];
        src[i + 4] = srcMatrix.a[i * 4 + 1];
        src[i + 8] = srcMatrix.a[i * 4 + 2];
        src[i + 12] = srcMatrix.a[i * 4 + 3];
      }

      // calculate pairs for first 8 elements (cofactors)
      double tmp[12]; // temp array for pairs
      tmp[0] = src[10] * src[15];
      tmp[1] = src[11] * src[14];
      tmp[2] = src[9] * src[15];
      tmp[3] = src[11] * src[13];
      tmp[4] = src[9] * src[14];
      tmp[5] = src[10] * src[13];
      tmp[6] = src[8] * src[15];
      tmp[7] = src[11] * src[12];
      tmp[8] = src[8] * src[14];
      tmp[9] = src[10] * src[12];
      tmp[10] = src[8] * src[13];
      tmp[11] = src[9] * src[12];

      // calculate first 8 elements (cofactors)
      a[0] = (tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7]) - (tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7]);
      a[1] = (tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7]) - (tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7]);
      a[2] = (tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7]) - (tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7]);
      a[3] = (tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6]) - (tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6]);
      a[4] = (tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3]) - (tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3]);
      a[5] = (tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3]) - (tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3]);
      a[6] = (tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3]) - (tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3]);
      a[7] = (tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2]) - (tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2]);

      // calculate pairs for second 8 elements (cofactors)
      tmp[0] = src[2] * src[7];
      tmp[1] = src[3] * src[6];
      tmp[2] = src[1] * src[7];
      tmp[3] = src[3] * src[5];
      tmp[4] = src[1] * src[6];
      tmp[5] = src[2] * src[5];
      tmp[6] = src[0] * src[7];
      tmp[7] = src[3] * src[4];
      tmp[8] = src[0] * src[6];
      tmp[9] = src[2] * src[4];
      tmp[10] = src[0] * src[5];
      tmp[11] = src[1] * src[4];

      // calculate second 8 elements (cofactors)
      a[8] = (tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15]) - (tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15]);
      a[9] = (tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15]) - (tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15]);
      a[10] = (tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15]) - (tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15]);
      a[11] = (tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14]) - (tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14]);
      a[12] = (tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9]) - (tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10]);
      a[13] = (tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10]) - (tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8]);
      a[14] = (tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8]) - (tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9]);
      a[15] = (tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9]) - (tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8]);

      // calculate determinant
      det = src[0] * a[0] + src[1] * a[1] + src[2] * a[2] + src[3] * a[3];

      // calculate matrix inverse
      double invdet = 1 / det;
      for (int j = 0; j < 16; ++j)
      {
        a[j] *= invdet;
      }
    }

    return det;
  }

  void matrix_t::RotationAxis(const vec_t & axis, double angle)
  {
    double length2 = axis.LengthSq();
    if (length2 < FLT_EPSILON)
    {
      SetToIdentity();
      return;
    }

    vec_t n = axis * (1.f / sqrt(length2));
    double s = sin(angle);
    double c = cos(angle);
    double k = 1.f - c;

    double xx = n.x * n.x * k + c;
    double yy = n.y * n.y * k + c;
    double zz = n.z * n.z * k + c;
    double xy = n.x * n.y * k;
    double yz = n.y * n.z * k;
    double zx = n.z * n.x * k;
    double xs = n.x * s;
    double ys = n.y * s;
    double zs = n.z * s;

    m[0][0] = xx;
    m[0][1] = xy + zs;
    m[0][2] = zx - ys;
    m[0][3] = 0.f;
    m[1][0] = xy - zs;
    m[1][1] = yy;
    m[1][2] = yz + xs;
    m[1][3] = 0.f;
    m[2][0] = zx + ys;
    m[2][1] = yz - xs;
    m[2][2] = zz;
    m[2][3] = 0.f;
    m[3][0] = 0.f;
    m[3][1] = 0.f;
    m[3][2] = 0.f;
    m[3][3] = 1.f;
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //

  enum MOVETYPE
  {
    NONE,
    MOVE_X,
    MOVE_Y,
    MOVE_Z,
    MOVE_YZ,
    MOVE_ZX,
    MOVE_XY,
    MOVE_SCREEN,
    ROTATE_X,
    ROTATE_Y,
    ROTATE_Z,
    ROTATE_SCREEN,
    SCALE_X,
    SCALE_Y,
    SCALE_Z,
    SCALE_XYZ
  };

  struct Context
  {
    Context() : mbUsing(false), mbUsingBounds(false)
    {
    }

    ImDrawList* mDrawList;

    ImGuizmoSpace mMode;
    matrix_t mViewMat;
    matrix_t mProjectionMat;
    matrix_t mModel;
    matrix_t mModelInverse;
    matrix_t mModelSource;
    matrix_t mModelSourceInverse;
    matrix_t mMVP;
    matrix_t mViewProjection;

    vec_t mModelScaleOrigin;
    vec_t mCameraEye;
    vec_t mCameraRight;
    vec_t mCameraDir;
    vec_t mCameraUp;
    vec_t mRayOrigin;
    vec_t mRayVector;

    double mRadiusSquareCenter;
    ImVec2 mScreenSquareCenter;
    ImVec2 mScreenSquareMin;
    ImVec2 mScreenSquareMax;

    double mScreenFactor;
    vec_t mRelativeOrigin;

    bool mbUsing;

    // translation
    vec_t mTranslationPlan;
    vec_t mTranslationPlanOrigin;
    vec_t mMatrixOrigin;

    // rotation
    vec_t mRotationVectorSource;
    double mRotationAngle;
    double mRotationAngleOrigin;
    //vec_t mWorldToLocalAxis;

    // scale
    vec_t mScale;
    vec_t mScaleValueOrigin;
    float mSaveMousePosx;

    // save axis factor when using gizmo
    bool mBelowAxisLimit[3];
    bool mBelowPlaneLimit[3];
    double mAxisFactor[3];

    // bounds stretching
    vec_t mBoundsPivot;
    vec_t mBoundsAnchor;
    vec_t mBoundsPlan;
    vec_t mBoundsLocalPivot;
    int mBoundsBestAxis;
    int mBoundsAxis[2];
    bool mbUsingBounds;
    matrix_t mBoundsMatrix;

    //
    int mCurrentOperation;

    float mX = 0.f;
    float mY = 0.f;
    float mWidth = 0.f;
    float mHeight = 0.f;
    float mXMax = 0.f;
    float mYMax = 0.f;
    float mDisplayRatio = 1.f;
  };

  static Context gContext;

  static const float angleLimit = 0.96f;
  static const float planeLimit = 0.2f;

  static const vec_t directionUnary[3] = { makeVect(1.f, 0.f, 0.f), makeVect(0.f, 1.f, 0.f), makeVect(0.f, 0.f, 1.f) };
  static const ImU32 directionColor[3] = { 0xFF0000AA, 0xFF00AA00, 0xFFAA0000 };

  // Alpha: 100%: FF, 87%: DE, 70%: B3, 54%: 8A, 50%: 80, 38%: 61, 12%: 1F
  static const ImU32 planeColor[3] = { 0x610000AA, 0x6100AA00, 0x61AA0000 };
  static const ImU32 selectionColor = 0x8A1080FF;
  static const ImU32 inactiveColor = 0x99999999;
  static const ImU32 translationLineColor = 0xAAAAAAAA;
  static const char *translationInfoMask[] = { "X : %5.3f", "Y : %5.3f", "Z : %5.3f",
     "Y : %5.3f Z : %5.3f", "X : %5.3f Z : %5.3f", "X : %5.3f Y : %5.3f",
     "X : %5.3f Y : %5.3f Z : %5.3f" };
  static const char *scaleInfoMask[] = { "X : %5.2f", "Y : %5.2f", "Z : %5.2f", "XYZ : %5.2f" };
  static const char *rotationInfoMask[] = { "X : %5.2f deg %5.2f rad", "Y : %5.2f deg %5.2f rad", "Z : %5.2f deg %5.2f rad", "Screen : %5.2f deg %5.2f rad" };
  static const int translationInfoIndex[] = { 0,0,0, 1,0,0, 2,0,0, 1,2,0, 0,2,0, 0,1,0, 0,1,2 };
  static const float quadMin = 0.5f;
  static const float quadMax = 0.8f;
  static const float quadUV[8] = { quadMin, quadMin, quadMin, quadMax, quadMax, quadMax, quadMax, quadMin };
  static const int halfCircleSegmentCount = 64;
  static const float snapTension = 0.5f;

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //
  static int GetMoveType(vec_t *gizmoHitProportion);
  static int GetRotateType();
  static int GetScaleType();

  static ImVec2 worldToPos(const vec_t& worldPos, const matrix_t& mat)
  {
    vec_t trans;
    trans.TransformPoint(worldPos, mat);
    trans *= 0.5f / trans.w;
    trans += makeVect(0.5f, 0.5f);
    trans.y = 1.f - trans.y;
    trans.x *= gContext.mWidth;
    trans.y *= gContext.mHeight;
    trans.x += gContext.mX;
    trans.y += gContext.mY;
    return ImVec2((float)trans.x, (float)trans.y);
  }

  static void ComputeCameraRay(vec_t &rayOrigin, vec_t &rayDir)
  {
    ImGuiIO& io = ImGui::GetIO();

    matrix_t mViewProjInverse;
    mViewProjInverse.Inverse(gContext.mViewMat * gContext.mProjectionMat);

    double mox = ((io.MousePos.x - gContext.mX) / gContext.mWidth) * 2.f - 1.f;
    double moy = (1.f - ((io.MousePos.y - gContext.mY) / gContext.mHeight)) * 2.f - 1.f;

    rayOrigin.Transform(makeVect(mox, moy, 0.f, 1.f), mViewProjInverse);
    rayOrigin *= 1.f / rayOrigin.w;
    vec_t rayEnd;
    rayEnd.Transform(makeVect(mox, moy, 1.f, 1.f), mViewProjInverse);
    rayEnd *= 1.f / rayEnd.w;
    rayDir = Normalized(rayEnd - rayOrigin);
  }

  static double GetSegmentLengthClipSpace(const vec_t& start, const vec_t& end)
  {
    vec_t startOfSegment = start;
    startOfSegment.TransformPoint(gContext.mMVP);
    if (udAbs(startOfSegment.w) > FLT_EPSILON) // check for axis aligned with camera direction
      startOfSegment *= 1.f / startOfSegment.w;

    vec_t endOfSegment = end;
    endOfSegment.TransformPoint(gContext.mMVP);
    if (udAbs(endOfSegment.w) > FLT_EPSILON) // check for axis aligned with camera direction
      endOfSegment *= 1.f / endOfSegment.w;

    vec_t clipSpaceAxis = endOfSegment - startOfSegment;
    clipSpaceAxis.y /= gContext.mDisplayRatio;
    double segmentLengthInClipSpace = sqrt(clipSpaceAxis.x*clipSpaceAxis.x + clipSpaceAxis.y*clipSpaceAxis.y);
    return segmentLengthInClipSpace;
  }

  static double GetParallelogram(const vec_t& ptO, const vec_t& ptA, const vec_t& ptB)
  {
    vec_t pts[] = { ptO, ptA, ptB };
    for (unsigned int i = 0; i < 3; i++)
    {
      pts[i].TransformPoint(gContext.mMVP);
      if (udAbs(pts[i].w) > DBL_EPSILON) // check for axis aligned with camera direction
        pts[i] *= 1.f / pts[i].w;
    }
    vec_t segA = pts[1] - pts[0];
    vec_t segB = pts[2] - pts[0];
    segA.y /= gContext.mDisplayRatio;
    segB.y /= gContext.mDisplayRatio;
    vec_t segAOrtho = makeVect(-segA.y, segA.x);
    segAOrtho.Normalize();
    double dt = segAOrtho.Dot3(segB);
    double surface = sqrt(segA.x*segA.x + segA.y*segA.y) * udAbs(dt);
    return surface;
  }

  inline vec_t PointOnSegment(const vec_t & point, const vec_t & vertPos1, const vec_t & vertPos2)
  {
    vec_t c = point - vertPos1;
    vec_t V;

    V.Normalize(vertPos2 - vertPos1);
    double d = (vertPos2 - vertPos1).Length();
    double t = V.Dot3(c);

    if (t < 0.f)
      return vertPos1;

    if (t > d)
      return vertPos2;

    return vertPos1 + V * t;
  }

  static double IntersectRayPlane(const vec_t & rOrigin, const vec_t& rVector, const vec_t& plan)
  {
    double numer = plan.Dot3(rOrigin) - plan.w;
    double denom = plan.Dot3(rVector);

    if (udAbs(denom) < DBL_EPSILON)  // normal is orthogonal to vector, cant intersect
      return -1.0f;

    return -(numer / denom);
  }

  void SetRect(float x, float y, float width, float height)
  {
    gContext.mX = x;
    gContext.mY = y;
    gContext.mWidth = width;
    gContext.mHeight = height;
    gContext.mXMax = gContext.mX + gContext.mWidth;
    gContext.mYMax = gContext.mY + gContext.mXMax;
    gContext.mDisplayRatio = width / height;
  }

  void SetDrawlist()
  {
    gContext.mDrawList = ImGui::GetWindowDrawList();
  }

  void BeginFrame()
  {
    ImGuiIO& io = ImGui::GetIO();

    const ImU32 flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    ImGui::Begin("gizmo", NULL, flags);
    gContext.mDrawList = ImGui::GetWindowDrawList();
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
  }

  bool IsUsing()
  {
    return gContext.mbUsing || gContext.mbUsingBounds;
  }

  bool IsOver()
  {
    return (GetMoveType(NULL) != NONE) || GetRotateType() != NONE || GetScaleType() != NONE || IsUsing();
  }

  static void ComputeContext(const double *view, const double *projection, double *matrix, ImGuizmoSpace mode)
  {
    gContext.mMode = mode;
    gContext.mViewMat = *(matrix_t*)view;
    gContext.mProjectionMat = *(matrix_t*)projection;

    if (mode == igsLocal)
    {
      gContext.mModel = *(matrix_t*)matrix;
      gContext.mModel.OrthoNormalize();
    }
    else
    {
      gContext.mModel.Translation(((matrix_t*)matrix)->axis.t);
    }
    gContext.mModelSource = *(matrix_t*)matrix;
    gContext.mModelScaleOrigin.Set(gContext.mModelSource.axis.x.Length(), gContext.mModelSource.axis.y.Length(), gContext.mModelSource.axis.z.Length());

    gContext.mModelInverse.Inverse(gContext.mModel);
    gContext.mModelSourceInverse.Inverse(gContext.mModelSource);
    gContext.mViewProjection = gContext.mViewMat * gContext.mProjectionMat;
    gContext.mMVP = gContext.mModel * gContext.mViewProjection;

    matrix_t viewInverse;
    viewInverse.Inverse(gContext.mViewMat);
    gContext.mCameraDir = viewInverse.axis.z;
    gContext.mCameraEye = viewInverse.axis.t;
    gContext.mCameraRight = viewInverse.axis.x;
    gContext.mCameraUp = viewInverse.axis.y;

    // compute scale from the size of camera right vector projected on screen at the matrix position
    vec_t pointRight = viewInverse.axis.x;
    pointRight.TransformPoint(gContext.mViewProjection);
    gContext.mScreenFactor = gGizmoSizeClipSpace / (pointRight.x / pointRight.w - gContext.mMVP.axis.t.x / gContext.mMVP.axis.t.w);

    vec_t rightViewInverse = viewInverse.axis.x;
    rightViewInverse.TransformVector(gContext.mModelInverse);
    double rightLength = GetSegmentLengthClipSpace(makeVect(0.f, 0.f), rightViewInverse);
    gContext.mScreenFactor = gGizmoSizeClipSpace / rightLength;

    ImVec2 centerSSpace = worldToPos(makeVect(0.f, 0.f), gContext.mMVP);
    gContext.mScreenSquareCenter = centerSSpace;
    gContext.mScreenSquareMin = ImVec2(centerSSpace.x - 10.f, centerSSpace.y - 10.f);
    gContext.mScreenSquareMax = ImVec2(centerSSpace.x + 10.f, centerSSpace.y + 10.f);

    ComputeCameraRay(gContext.mRayOrigin, gContext.mRayVector);
  }

  static void ComputeColors(ImU32 *colors, int type, ImGuizmoOperation operation)
  {
    switch (operation)
    {
    case igoTranslate:
      colors[0] = (type == MOVE_SCREEN) ? selectionColor : 0xFFFFFFFF;
      for (int i = 0; i < 3; i++)
      {
        colors[i + 1] = (type == (int)(MOVE_X + i)) ? selectionColor : directionColor[i];
        colors[i + 4] = (type == (int)(MOVE_YZ + i)) ? selectionColor : planeColor[i];
        colors[i + 4] = (type == MOVE_SCREEN) ? selectionColor : colors[i + 4];
      }
      break;
    case igoRotate:
      colors[0] = (type == ROTATE_SCREEN) ? selectionColor : 0xFFFFFFFF;
      for (int i = 0; i < 3; i++)
        colors[i + 1] = (type == (int)(ROTATE_X + i)) ? selectionColor : directionColor[i];
      break;
    case igoScale:
      colors[0] = (type == SCALE_XYZ) ? selectionColor : 0xFFFFFFFF;
      for (int i = 0; i < 3; i++)
        colors[i + 1] = (type == (int)(SCALE_X + i)) ? selectionColor : directionColor[i];
      break;
    }
  }

  static void ComputeTripodAxisAndVisibility(int axisIndex, vec_t& dirAxis, vec_t& dirPlaneX, vec_t& dirPlaneY, bool& belowAxisLimit, bool& belowPlaneLimit)
  {
    dirAxis = directionUnary[axisIndex];
    dirPlaneX = directionUnary[(axisIndex + 1) % 3];
    dirPlaneY = directionUnary[(axisIndex + 2) % 3];

    if (gContext.mbUsing)
    {
      // when using, use stored factors so the gizmo doesn't flip when we translate
      belowAxisLimit = gContext.mBelowAxisLimit[axisIndex];
      belowPlaneLimit = gContext.mBelowPlaneLimit[axisIndex];

      dirAxis *= gContext.mAxisFactor[axisIndex];
      dirPlaneX *= gContext.mAxisFactor[(axisIndex + 1) % 3];
      dirPlaneY *= gContext.mAxisFactor[(axisIndex + 2) % 3];
    }
    else
    {
      // new method
      double lenDir = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirAxis);
      double lenDirMinus = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), -dirAxis);

      double lenDirPlaneX = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirPlaneX);
      double lenDirMinusPlaneX = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), -dirPlaneX);

      double lenDirPlaneY = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirPlaneY);
      double lenDirMinusPlaneY = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), -dirPlaneY);

      double mulAxis = (lenDir < lenDirMinus && udAbs(lenDir - lenDirMinus) > FLT_EPSILON) ? -1.f : 1.f;
      double mulAxisX = (lenDirPlaneX < lenDirMinusPlaneX && udAbs(lenDirPlaneX - lenDirMinusPlaneX) > FLT_EPSILON) ? -1.f : 1.f;
      double mulAxisY = (lenDirPlaneY < lenDirMinusPlaneY && udAbs(lenDirPlaneY - lenDirMinusPlaneY) > FLT_EPSILON) ? -1.f : 1.f;
      dirAxis *= mulAxis;
      dirPlaneX *= mulAxisX;
      dirPlaneY *= mulAxisY;

      // for axis
      double axisLengthInClipSpace = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirAxis * gContext.mScreenFactor);

      double paraSurf = GetParallelogram(makeVect(0.f, 0.f, 0.f), dirPlaneX * gContext.mScreenFactor, dirPlaneY * gContext.mScreenFactor);
      belowPlaneLimit = (paraSurf > 0.0025f);
      belowAxisLimit = (axisLengthInClipSpace > 0.02f);

      // and store values
      gContext.mAxisFactor[axisIndex] = mulAxis;
      gContext.mAxisFactor[(axisIndex + 1) % 3] = mulAxisX;
      gContext.mAxisFactor[(axisIndex + 2) % 3] = mulAxisY;
      gContext.mBelowAxisLimit[axisIndex] = belowAxisLimit;
      gContext.mBelowPlaneLimit[axisIndex] = belowPlaneLimit;
    }
  }

  static void ComputeSnap(double* value, double snap)
  {
    if (snap <= DBL_EPSILON)
      return;
    double modulo = fmod(*value, snap);
    double moduloRatio = udAbs(modulo) / snap;
    if (moduloRatio < snapTension)
      *value -= modulo;
    else if (moduloRatio > (1.f - snapTension))
      *value = *value - modulo + snap * ((*value < 0.f) ? -1.f : 1.f);
  }

  static void ComputeSnap(vec_t& value, double snap)
  {
    for (int i = 0; i < 3; i++)
    {
      ComputeSnap(&value[i], snap);
    }
  }

  static double ComputeAngleOnPlan()
  {
    const double len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
    vec_t localPos = Normalized(gContext.mRayOrigin + gContext.mRayVector * len - gContext.mModel.axis.t);

    vec_t perpendicularVector;
    perpendicularVector.Cross(gContext.mRotationVectorSource, gContext.mTranslationPlan);
    perpendicularVector.Normalize();
    double acosAngle = udClamp(Dot(localPos, gContext.mRotationVectorSource), -0.9999, 0.9999);
    double angle = acos(acosAngle);
    angle *= (Dot(localPos, perpendicularVector) < 0.f) ? 1.f : -1.f;
    return angle;
  }

  static void DrawRotationGizmo(int type)
  {
    ImDrawList* drawList = gContext.mDrawList;

    // colors
    ImU32 colors[7];
    ComputeColors(colors, type, igoRotate);

    vec_t cameraToModelNormalized;
    cameraToModelNormalized = Normalized(gContext.mModel.axis.t - gContext.mCameraEye);

    cameraToModelNormalized.TransformVector(gContext.mModelInverse);

    gContext.mRadiusSquareCenter = screenRotateSize * gContext.mHeight;

    for (int axis = 0; axis < 3; axis++)
    {
      ImVec2 circlePos[halfCircleSegmentCount];

      double angleStart = atan2(cameraToModelNormalized[(4 - axis) % 3], cameraToModelNormalized[(3 - axis) % 3]) + UD_PI * 0.5f;

      for (unsigned int i = 0; i < halfCircleSegmentCount; i++)
      {
        double ng = angleStart + UD_PI * ((float)i / (float)halfCircleSegmentCount);
        vec_t axisPos = makeVect(cos(ng), sin(ng), 0.f);
        vec_t pos = makeVect(axisPos[axis], axisPos[(axis + 1) % 3], axisPos[(axis + 2) % 3]) * gContext.mScreenFactor;
        circlePos[i] = worldToPos(pos, gContext.mMVP);
      }

      float radiusAxis = sqrtf((ImLengthSqr(worldToPos(gContext.mModel.axis.t, gContext.mViewProjection) - circlePos[0])));
      if (radiusAxis > gContext.mRadiusSquareCenter)
        gContext.mRadiusSquareCenter = radiusAxis;

      drawList->AddPolyline(circlePos, halfCircleSegmentCount, colors[3 - axis], false, 2);
    }
    drawList->AddCircle(worldToPos(gContext.mModel.axis.t, gContext.mViewProjection), (float)gContext.mRadiusSquareCenter, colors[0], 64, 3.f);

    if (gContext.mbUsing)
    {
      ImVec2 circlePos[halfCircleSegmentCount + 1];

      circlePos[0] = worldToPos(gContext.mModel.axis.t, gContext.mViewProjection);
      for (unsigned int i = 1; i < halfCircleSegmentCount; i++)
      {
        double ng = gContext.mRotationAngle * ((float)(i - 1) / (float)(halfCircleSegmentCount - 1));
        matrix_t rotateVectorMatrix;
        rotateVectorMatrix.RotationAxis(gContext.mTranslationPlan, ng);
        vec_t pos;
        pos.TransformPoint(gContext.mRotationVectorSource, rotateVectorMatrix);
        pos *= gContext.mScreenFactor;
        circlePos[i] = worldToPos(pos + gContext.mModel.axis.t, gContext.mViewProjection);
      }
      drawList->AddConvexPolyFilled(circlePos, halfCircleSegmentCount, 0x801080FF);
      drawList->AddPolyline(circlePos, halfCircleSegmentCount, 0xFF1080FF, true, 2);

      ImVec2 destinationPosOnScreen = circlePos[1];
      char tmps[512];
      ImFormatString(tmps, sizeof(tmps), rotationInfoMask[type - ROTATE_X], UD_RAD2DEG(gContext.mRotationAngle), gContext.mRotationAngle);
      drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
      drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
    }
  }

  static void DrawHatchedAxis(const vec_t& axis)
  {
    for (int j = 1; j < 10; j++)
    {
      ImVec2 baseSSpace2 = worldToPos(axis * 0.05f * (float)(j * 2) * gContext.mScreenFactor, gContext.mMVP);
      ImVec2 worldDirSSpace2 = worldToPos(axis * 0.05f * (float)(j * 2 + 1) * gContext.mScreenFactor, gContext.mMVP);
      gContext.mDrawList->AddLine(baseSSpace2, worldDirSSpace2, 0x80000000, 6.f);
    }
  }

  static void DrawScaleGizmo(int type)
  {
    ImDrawList* drawList = gContext.mDrawList;

    // colors
    ImU32 colors[7];
    ComputeColors(colors, type, igoScale);

    // draw
    vec_t scaleDisplay = { 1.f, 1.f, 1.f, 1.f };

    if (gContext.mbUsing)
      scaleDisplay = gContext.mScale;

    for (unsigned int i = 0; i < 3; i++)
    {
      vec_t dirPlaneX, dirPlaneY, dirAxis;
      bool belowAxisLimit, belowPlaneLimit;
      ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

      // draw axis
      if (belowAxisLimit)
      {
        ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * gContext.mScreenFactor, gContext.mMVP);
        ImVec2 worldDirSSpaceNoScale = worldToPos(dirAxis * gContext.mScreenFactor, gContext.mMVP);
        ImVec2 worldDirSSpace = worldToPos((dirAxis * scaleDisplay[i]) * gContext.mScreenFactor, gContext.mMVP);

        if (gContext.mbUsing)
        {
          drawList->AddLine(baseSSpace, worldDirSSpaceNoScale, 0xFF404040, 3.f);
          drawList->AddCircleFilled(worldDirSSpaceNoScale, 6.f, 0xFF404040);
        }

        drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], 3.f);
        drawList->AddCircleFilled(worldDirSSpace, 6.f, colors[i + 1]);

        if (gContext.mAxisFactor[i] < 0.f)
          DrawHatchedAxis(dirAxis * scaleDisplay[i]);
      }
    }

    // draw screen cirle
    drawList->AddCircleFilled(gContext.mScreenSquareCenter, 6.f, colors[0], 32);

    if (gContext.mbUsing)
    {
      //ImVec2 sourcePosOnScreen = worldToPos(gContext.mMatrixOrigin, gContext.mViewProjection);
      ImVec2 destinationPosOnScreen = worldToPos(gContext.mModel.axis.t, gContext.mViewProjection);
      /*vec_t dif(destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y);
      dif.Normalize();
      dif *= 5.f;
      drawList->AddCircle(sourcePosOnScreen, 6.f, translationLineColor);
      drawList->AddCircle(destinationPosOnScreen, 6.f, translationLineColor);
      drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.f);
      */
      char tmps[512];
      //vec_t deltaInfo = gContext.mModel.axis.position - gContext.mMatrixOrigin;
      int componentInfoIndex = (type - SCALE_X) * 3;
      ImFormatString(tmps, sizeof(tmps), scaleInfoMask[type - SCALE_X], scaleDisplay[translationInfoIndex[componentInfoIndex]]);
      drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
      drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
    }
  }


  static void DrawTranslationGizmo(int type)
  {
    ImDrawList* drawList = gContext.mDrawList;
    if (!drawList)
      return;

    // colors
    ImU32 colors[7];
    ComputeColors(colors, type, igoTranslate);

    const ImVec2 origin = worldToPos(gContext.mModel.axis.t, gContext.mViewProjection);

    // draw
    bool belowAxisLimit = false;
    bool belowPlaneLimit = false;
    for (unsigned int i = 0; i < 3; ++i)
    {
      vec_t dirPlaneX, dirPlaneY, dirAxis;
      ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

      // draw axis
      if (belowAxisLimit)
      {
        ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * gContext.mScreenFactor, gContext.mMVP);
        ImVec2 worldDirSSpace = worldToPos(dirAxis * gContext.mScreenFactor, gContext.mMVP);

        drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], 3.f);

        // Arrow head begin
        ImVec2 dir(origin - worldDirSSpace);

        float d = sqrtf(ImLengthSqr(dir));
        dir /= d; // Normalize
        dir *= 6.0f;

        ImVec2 ortogonalDir(dir.y, -dir.x); // Perpendicular vector
        ImVec2 a(worldDirSSpace + dir);
        drawList->AddTriangleFilled(worldDirSSpace - dir, a + ortogonalDir, a - ortogonalDir, colors[i + 1]);
        // Arrow head end

        if (gContext.mAxisFactor[i] < 0.f)
          DrawHatchedAxis(dirAxis);
      }

      // draw plane
      if (belowPlaneLimit)
      {
        ImVec2 screenQuadPts[4];
        for (int j = 0; j < 4; ++j)
        {
          vec_t cornerWorldPos = (dirPlaneX * quadUV[j * 2] + dirPlaneY  * quadUV[j * 2 + 1]) * gContext.mScreenFactor;
          screenQuadPts[j] = worldToPos(cornerWorldPos, gContext.mMVP);
        }
        drawList->AddPolyline(screenQuadPts, 4, directionColor[i], true, 1.0f);
        drawList->AddConvexPolyFilled(screenQuadPts, 4, colors[i + 4]);
      }
    }

    drawList->AddCircleFilled(gContext.mScreenSquareCenter, 6.f, colors[0], 32);

    if (gContext.mbUsing)
    {
      ImVec2 sourcePosOnScreen = worldToPos(gContext.mMatrixOrigin, gContext.mViewProjection);
      ImVec2 destinationPosOnScreen = worldToPos(gContext.mModel.axis.t, gContext.mViewProjection);
      vec_t dif = { destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y, 0.f, 0.f };
      dif.Normalize();
      dif *= 5.f;
      drawList->AddCircle(sourcePosOnScreen, 6.f, translationLineColor);
      drawList->AddCircle(destinationPosOnScreen, 6.f, translationLineColor);
      drawList->AddLine(ImVec2(sourcePosOnScreen.x + (float)dif.x, sourcePosOnScreen.y + (float)dif.y), ImVec2(destinationPosOnScreen.x - (float)dif.x, destinationPosOnScreen.y - (float)dif.y), translationLineColor, 2.f);

      char tmps[512];
      vec_t deltaInfo = gContext.mModel.axis.t - gContext.mMatrixOrigin;
      int componentInfoIndex = (type - MOVE_X) * 3;
      ImFormatString(tmps, sizeof(tmps), translationInfoMask[type - MOVE_X], deltaInfo[translationInfoIndex[componentInfoIndex]], deltaInfo[translationInfoIndex[componentInfoIndex + 1]], deltaInfo[translationInfoIndex[componentInfoIndex + 2]]);
      drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
      drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
    }
  }

  static bool CanActivate()
  {
    if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemActive())
      return true;
    return false;
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //

  static int GetScaleType()
  {
    ImGuiIO& io = ImGui::GetIO();
    int type = NONE;

    // screen
    if (io.MousePos.x >= gContext.mScreenSquareMin.x && io.MousePos.x <= gContext.mScreenSquareMax.x &&
      io.MousePos.y >= gContext.mScreenSquareMin.y && io.MousePos.y <= gContext.mScreenSquareMax.y)
      type = SCALE_XYZ;

    // compute
    for (unsigned int i = 0; i < 3 && type == NONE; i++)
    {
      vec_t dirPlaneX, dirPlaneY, dirAxis;
      bool belowAxisLimit, belowPlaneLimit;
      ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

      const double len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, BuildPlan(gContext.mModel.axis.t, dirAxis));
      vec_t posOnPlan = gContext.mRayOrigin + gContext.mRayVector * len;

      const ImVec2 posOnPlanScreen = worldToPos(posOnPlan, gContext.mViewProjection);
      const ImVec2 axisStartOnScreen = worldToPos(gContext.mModel.axis.t + dirAxis * gContext.mScreenFactor * 0.1f, gContext.mViewProjection);
      const ImVec2 axisEndOnScreen = worldToPos(gContext.mModel.axis.t + dirAxis * gContext.mScreenFactor, gContext.mViewProjection);

      vec_t closestPointOnAxis = PointOnSegment(makeVect(posOnPlanScreen), makeVect(axisStartOnScreen), makeVect(axisEndOnScreen));

      if ((closestPointOnAxis - makeVect(posOnPlanScreen)).Length() < 12.f) // pixel size
        type = SCALE_X + i;
    }
    return type;
  }

  static int GetRotateType()
  {
    ImGuiIO& io = ImGui::GetIO();
    int type = NONE;

    vec_t deltaScreen = { io.MousePos.x - gContext.mScreenSquareCenter.x, io.MousePos.y - gContext.mScreenSquareCenter.y, 0.f, 0.f };
    double dist = deltaScreen.Length();
    if (dist >= (gContext.mRadiusSquareCenter - 1.0f) && dist < (gContext.mRadiusSquareCenter + 1.0f))
      type = ROTATE_SCREEN;

    const vec_t planNormals[] = { gContext.mModel.axis.x, gContext.mModel.axis.y, gContext.mModel.axis.z };

    for (unsigned int i = 0; i < 3 && type == NONE; i++)
    {
      // pickup plan
      vec_t pickupPlan = BuildPlan(gContext.mModel.axis.t, planNormals[i]);

      const double len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, pickupPlan);
      vec_t localPos = gContext.mRayOrigin + gContext.mRayVector * len - gContext.mModel.axis.t;

      if (Dot(Normalized(localPos), gContext.mRayVector) > FLT_EPSILON)
        continue;
      vec_t idealPosOnCircle = Normalized(localPos);
      idealPosOnCircle.TransformVector(gContext.mModelInverse);
      ImVec2 idealPosOnCircleScreen = worldToPos(idealPosOnCircle * gContext.mScreenFactor, gContext.mMVP);

      //gContext.mDrawList->AddCircle(idealPosOnCircleScreen, 5.f, 0xFFFFFFFF);
      ImVec2 distanceOnScreen = idealPosOnCircleScreen - io.MousePos;

      double distance = makeVect(distanceOnScreen).Length();
      if (distance < 8.f) // pixel size
        type = ROTATE_X + i;
    }

    return type;
  }

  static int GetMoveType(vec_t *gizmoHitProportion)
  {
    ImGuiIO& io = ImGui::GetIO();
    int type = NONE;

    // screen
    if (io.MousePos.x >= gContext.mScreenSquareMin.x && io.MousePos.x <= gContext.mScreenSquareMax.x &&
      io.MousePos.y >= gContext.mScreenSquareMin.y && io.MousePos.y <= gContext.mScreenSquareMax.y)
      type = MOVE_SCREEN;

    // compute
    for (unsigned int i = 0; i < 3 && type == NONE; i++)
    {
      vec_t dirPlaneX, dirPlaneY, dirAxis;
      bool belowAxisLimit, belowPlaneLimit;
      ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
      dirAxis.TransformVector(gContext.mModel);
      dirPlaneX.TransformVector(gContext.mModel);
      dirPlaneY.TransformVector(gContext.mModel);

      const double len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, BuildPlan(gContext.mModel.axis.t, dirAxis));
      vec_t posOnPlan = gContext.mRayOrigin + gContext.mRayVector * len;

      const ImVec2 posOnPlanScreen = worldToPos(posOnPlan, gContext.mViewProjection);
      const ImVec2 axisStartOnScreen = worldToPos(gContext.mModel.axis.t + dirAxis * gContext.mScreenFactor * 0.1f, gContext.mViewProjection);
      const ImVec2 axisEndOnScreen = worldToPos(gContext.mModel.axis.t + dirAxis * gContext.mScreenFactor, gContext.mViewProjection);

      vec_t closestPointOnAxis = PointOnSegment(makeVect(posOnPlanScreen), makeVect(axisStartOnScreen), makeVect(axisEndOnScreen));

      if ((closestPointOnAxis - makeVect(posOnPlanScreen)).Length() < 12.f) // pixel size
        type = MOVE_X + i;

      const double dx = dirPlaneX.Dot3((posOnPlan - gContext.mModel.axis.t) * (1.f / gContext.mScreenFactor));
      const double dy = dirPlaneY.Dot3((posOnPlan - gContext.mModel.axis.t) * (1.f / gContext.mScreenFactor));
      if (belowPlaneLimit && dx >= quadUV[0] && dx <= quadUV[4] && dy >= quadUV[1] && dy <= quadUV[3])
        type = MOVE_YZ + i;

      if (gizmoHitProportion)
        *gizmoHitProportion = makeVect(dx, dy, 0.f);
    }
    return type;
  }

  static void HandleTranslation(double *matrix, double *deltaMatrix, int& type, double snap)
  {
    ImGuiIO& io = ImGui::GetIO();
    bool applyRotationLocaly = gContext.mMode == igsLocal || type == MOVE_SCREEN;

    // move
    if (gContext.mbUsing)
    {
      ImGui::CaptureMouseFromApp();
      const double len = udAbs(IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan)); // near plan
      vec_t newPos = gContext.mRayOrigin + gContext.mRayVector * len;



      // compute delta
      vec_t newOrigin = newPos - gContext.mRelativeOrigin * gContext.mScreenFactor;
      vec_t delta = newOrigin - gContext.mModel.axis.t;

      // 1 axis constraint
      if (gContext.mCurrentOperation >= MOVE_X && gContext.mCurrentOperation <= MOVE_Z)
      {
        int axisIndex = gContext.mCurrentOperation - MOVE_X;
        const vec_t& axisValue = *(vec_t*)&gContext.mModel.m[axisIndex];
        double lengthOnAxis = Dot(axisValue, delta);
        delta = axisValue * lengthOnAxis;
      }

      // snap
      if (snap)
      {
        vec_t cumulativeDelta = gContext.mModel.axis.t + delta - gContext.mMatrixOrigin;
        if (applyRotationLocaly)
        {
          matrix_t modelSourceNormalized = gContext.mModelSource;
          modelSourceNormalized.OrthoNormalize();
          matrix_t modelSourceNormalizedInverse;
          modelSourceNormalizedInverse.Inverse(modelSourceNormalized);
          cumulativeDelta.TransformVector(modelSourceNormalizedInverse);
          ComputeSnap(cumulativeDelta, snap);
          cumulativeDelta.TransformVector(modelSourceNormalized);
        }
        else
        {
          ComputeSnap(cumulativeDelta, snap);
        }
        delta = gContext.mMatrixOrigin + cumulativeDelta - gContext.mModel.axis.t;

      }

      // compute matrix & delta
      matrix_t deltaMatrixTranslation;
      deltaMatrixTranslation.Translation(delta);
      if (deltaMatrix)
        memcpy(deltaMatrix, deltaMatrixTranslation.a, sizeof(double) * 16);


      matrix_t res = gContext.mModelSource * deltaMatrixTranslation;
      *(matrix_t*)matrix = res;

      if (!io.MouseDown[0])
        gContext.mbUsing = false;

      type = gContext.mCurrentOperation;
    }
    else
    {
      // find new possible way to move
      vec_t gizmoHitProportion;
      type = GetMoveType(&gizmoHitProportion);
      if (type != NONE)
      {
        ImGui::CaptureMouseFromApp();
      }
      if (CanActivate() && type != NONE)
      {
        gContext.mbUsing = true;
        gContext.mCurrentOperation = type;
        vec_t movePlanNormal[] = { gContext.mModel.axis.x, gContext.mModel.axis.y, gContext.mModel.axis.z,
           gContext.mModel.axis.x, gContext.mModel.axis.y, gContext.mModel.axis.z,
           -gContext.mCameraDir };

        vec_t cameraToModelNormalized = Normalized(gContext.mModel.axis.t - gContext.mCameraEye);
        for (unsigned int i = 0; i < 3; i++)
        {
          vec_t orthoVector = Cross(movePlanNormal[i], cameraToModelNormalized);
          movePlanNormal[i].Cross(orthoVector);
          movePlanNormal[i].Normalize();
        }
        // pickup plan
        gContext.mTranslationPlan = BuildPlan(gContext.mModel.axis.t, movePlanNormal[type - MOVE_X]);
        const double len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
        gContext.mTranslationPlanOrigin = gContext.mRayOrigin + gContext.mRayVector * len;
        gContext.mMatrixOrigin = gContext.mModel.axis.t;

        gContext.mRelativeOrigin = (gContext.mTranslationPlanOrigin - gContext.mModel.axis.t) * (1.f / gContext.mScreenFactor);
      }
    }
  }

  static void HandleScale(double *matrix, double *deltaMatrix, int& type, double snap)
  {
    ImGuiIO& io = ImGui::GetIO();

    if (!gContext.mbUsing)
    {
      // find new possible way to scale
      type = GetScaleType();
      if (type != NONE)
      {
        ImGui::CaptureMouseFromApp();
      }
      if (CanActivate() && type != NONE)
      {
        gContext.mbUsing = true;
        gContext.mCurrentOperation = type;
        const vec_t movePlanNormal[] = { gContext.mModel.axis.y, gContext.mModel.axis.z, gContext.mModel.axis.x, gContext.mModel.axis.z, gContext.mModel.axis.y, gContext.mModel.axis.x, -gContext.mCameraDir };
        // pickup plan

        gContext.mTranslationPlan = BuildPlan(gContext.mModel.axis.t, movePlanNormal[type - SCALE_X]);
        const double len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
        gContext.mTranslationPlanOrigin = gContext.mRayOrigin + gContext.mRayVector * len;
        gContext.mMatrixOrigin = gContext.mModel.axis.t;
        gContext.mScale.Set(1.f, 1.f, 1.f);
        gContext.mRelativeOrigin = (gContext.mTranslationPlanOrigin - gContext.mModel.axis.t) * (1.f / gContext.mScreenFactor);
        gContext.mScaleValueOrigin = makeVect(gContext.mModelSource.axis.x.Length(), gContext.mModelSource.axis.y.Length(), gContext.mModelSource.axis.z.Length());
        gContext.mSaveMousePosx = io.MousePos.x;
      }
    }
    // scale
    if (gContext.mbUsing)
    {
      ImGui::CaptureMouseFromApp();
      const double len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
      vec_t newPos = gContext.mRayOrigin + gContext.mRayVector * len;
      vec_t newOrigin = newPos - gContext.mRelativeOrigin * gContext.mScreenFactor;
      vec_t delta = newOrigin - gContext.mModel.axis.t;

      // 1 axis constraint
      if (gContext.mCurrentOperation >= SCALE_X && gContext.mCurrentOperation <= SCALE_Z)
      {
        int axisIndex = gContext.mCurrentOperation - SCALE_X;
        const vec_t& axisValue = *(vec_t*)&gContext.mModel.m[axisIndex];
        double lengthOnAxis = Dot(axisValue, delta);
        delta = axisValue * lengthOnAxis;

        vec_t baseVector = gContext.mTranslationPlanOrigin - gContext.mModel.axis.t;
        double ratio = Dot(axisValue, baseVector + delta) / Dot(axisValue, baseVector);

        gContext.mScale[axisIndex] = max(ratio, 0.001);
      }
      else
      {
        double scaleDelta = (io.MousePos.x - gContext.mSaveMousePosx)  * 0.01f;
        gContext.mScale.Set(max(1.0 + scaleDelta, 0.001));
      }

      // snap
      if (snap)
        ComputeSnap(gContext.mScale, snap);

      // no 0 allowed
      for (int i = 0; i < 3;i++)
        gContext.mScale[i] = max(gContext.mScale[i], 0.001);

      // compute matrix & delta
      matrix_t deltaMatrixScale;
      deltaMatrixScale.Scale(gContext.mScale * gContext.mScaleValueOrigin);

      matrix_t res = deltaMatrixScale * gContext.mModel;
      *(matrix_t*)matrix = res;

      if (deltaMatrix)
      {
        deltaMatrixScale.Scale(gContext.mScale);
        memcpy(deltaMatrix, deltaMatrixScale.a, sizeof(double) * 16);
      }

      if (!io.MouseDown[0])
        gContext.mbUsing = false;

      type = gContext.mCurrentOperation;
    }
  }

  static void HandleRotation(double *matrix, double *deltaMatrix, int& type, double snap)
  {
    ImGuiIO& io = ImGui::GetIO();
    bool applyRotationLocaly = gContext.mMode == igsLocal;

    if (!gContext.mbUsing)
    {
      type = GetRotateType();

      if (type != NONE)
      {
        ImGui::CaptureMouseFromApp();
      }

      if (type == ROTATE_SCREEN)
      {
        applyRotationLocaly = true;
      }

      if (CanActivate() && type != NONE)
      {
        gContext.mbUsing = true;
        gContext.mCurrentOperation = type;
        const vec_t rotatePlanNormal[] = { gContext.mModel.axis.x, gContext.mModel.axis.y, gContext.mModel.axis.z, -gContext.mCameraDir };
        // pickup plan
        if (applyRotationLocaly)
        {
          gContext.mTranslationPlan = BuildPlan(gContext.mModel.axis.t, rotatePlanNormal[type - ROTATE_X]);
        }
        else
        {
          gContext.mTranslationPlan = BuildPlan(gContext.mModelSource.axis.t, directionUnary[type - ROTATE_X]);
        }

        const double len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
        vec_t localPos = gContext.mRayOrigin + gContext.mRayVector * len - gContext.mModel.axis.t;
        gContext.mRotationVectorSource = Normalized(localPos);
        gContext.mRotationAngleOrigin = ComputeAngleOnPlan();
      }
    }

    // rotation
    if (gContext.mbUsing)
    {
      ImGui::CaptureMouseFromApp();
      gContext.mRotationAngle = ComputeAngleOnPlan();
      if (snap)
        ComputeSnap(&gContext.mRotationAngle, UD_DEG2RAD(snap));
      vec_t rotationAxisLocalSpace;

      rotationAxisLocalSpace.TransformVector(makeVect(gContext.mTranslationPlan.x, gContext.mTranslationPlan.y, gContext.mTranslationPlan.z, 0.f), gContext.mModelInverse);
      rotationAxisLocalSpace.Normalize();

      matrix_t deltaRotation;
      deltaRotation.RotationAxis(rotationAxisLocalSpace, gContext.mRotationAngle - gContext.mRotationAngleOrigin);
      gContext.mRotationAngleOrigin = gContext.mRotationAngle;

      matrix_t scaleOrigin;
      scaleOrigin.Scale(gContext.mModelScaleOrigin);

      if (applyRotationLocaly)
      {
        *(matrix_t*)matrix = scaleOrigin * deltaRotation * gContext.mModel;
      }
      else
      {
        matrix_t res = gContext.mModelSource;
        res.axis.t.Set(0.f);

        *(matrix_t*)matrix = res * deltaRotation;
        ((matrix_t*)matrix)->axis.t = gContext.mModelSource.axis.t;
      }

      if (deltaMatrix)
      {
        *(matrix_t*)deltaMatrix = gContext.mModelInverse * deltaRotation * gContext.mModel;
      }

      if (!io.MouseDown[0])
        gContext.mbUsing = false;

      type = gContext.mCurrentOperation;
    }
  }

  void Manipulate(const udDouble4x4 &view, const udDouble4x4 &projection, ImGuizmoOperation operation, ImGuizmoSpace mode, udDouble4x4 *pMatrix, udDouble4x4 *deltaMatrix /*= 0*/, double snap /*= 0*/)
  {
    ComputeContext(view.a, projection.a, pMatrix->a, mode);

    if (deltaMatrix)
      deltaMatrix->identity();

    // behind camera
    vec_t camSpacePosition;
    camSpacePosition.TransformPoint(makeVect(0.f, 0.f, 0.f), gContext.mMVP);
    if (camSpacePosition.z < 0.001f)
      return;

    // --
    int type = NONE;
    if (!gContext.mbUsingBounds)
    {
      switch (operation)
      {
      case igoRotate:
        HandleRotation(pMatrix->a, deltaMatrix->a, type, snap);
        break;
      case igoTranslate:
        HandleTranslation(pMatrix->a, deltaMatrix->a, type, snap);
        break;
      case igoScale:
        HandleScale(pMatrix->a, deltaMatrix->a, type, snap);
        break;
      }
    }

    if (!gContext.mbUsingBounds)
    {
      switch (operation)
      {
      case igoRotate:
        DrawRotationGizmo(type);
        break;
      case igoTranslate:
        DrawTranslationGizmo(type);
        break;
      case igoScale:
        DrawScaleGizmo(type);
        break;
      }
    }
  }
};

