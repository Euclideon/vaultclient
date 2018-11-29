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
  const double gGizmoSizeClipSpace = 0.1;
  const double screenRotateSize = 0.06;

  udDouble4 makeVect(double _x, double _y, double _z = 0.f, double _w = 0.f) { udDouble4 res; res.x = _x; res.y = _y; res.z = _z; res.w = _w; return res; }
  udDouble4 makeVect(ImVec2 v) { udDouble4 res; res.x = v.x; res.y = v.y; res.z = 0.f; res.w = 0.f; return res; }

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
    Context() : mbUsing(false)
    {
    }

    ImDrawList* mDrawList;

    ImGuizmoSpace mMode;

    udDouble4x4 mModel;
    udDouble4x4 mModelInverse;
    udDouble4x4 mModelSource;
    udDouble4x4 mModelSourceInverse;
    udDouble4x4 mMVP;
    udDouble4x4 mViewProjection;

    udDouble3 mModelScaleOrigin;
    udDouble3 mCameraEye;
    udDouble3 mCameraDir;
    udRay<double> mRay;

    double mRadiusSquareCenter;
    ImVec2 mScreenSquareCenter;
    ImVec2 mScreenSquareMin;
    ImVec2 mScreenSquareMax;

    double mScreenFactor;
    udDouble3 mRelativeOrigin;

    bool mbUsing;

    // translation
    udPlane<double> mTranslationPlane;
    udDouble3 mTranslationPlaneOrigin;
    udDouble3 mMatrixOrigin;

    // rotation
    udDouble3 mRotationVectorSource;
    double mRotationAngle;
    double mRotationAngleOrigin;

    // scale
    udDouble3 mScale;
    udDouble3 mScaleValueOrigin;
    float mSaveMousePosx;

    // save axis factor when using gizmo
    bool mBelowAxisLimit[3];
    bool mBelowPlaneLimit[3];
    double mAxisFactor[3];

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

  static const udDouble3 directionUnary[3] = { udDouble3::create(1.f, 0.f, 0.f), udDouble3::create(0.f, 1.f, 0.f), udDouble3::create(0.f, 0.f, 1.f) };
  static const ImU32 directionColor[3] = { 0xFF0000AA, 0xFF00AA00, 0xFFAA0000 };

  // Alpha: 100%: FF, 87%: DE, 70%: B3, 54%: 8A, 50%: 80, 38%: 61, 12%: 1F
  static const ImU32 planeColor[3] = { 0x610000AA, 0x6100AA00, 0x61AA0000 };
  static const ImU32 selectionColor = 0x8A1080FF;

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
  static int GetMoveType(udDouble4 *gizmoHitProportion);
  static int GetRotateType();
  static int GetScaleType();

  static ImVec2 worldToPos(const udDouble3& worldPos, const udDouble4x4& mat)
  {
    udDouble4 trans = mat * udDouble4::create(worldPos, 1);
    trans *= 0.5f / trans.w;
    trans += makeVect(0.5f, 0.5f);
    trans.y = 1.f - trans.y;
    trans.x *= gContext.mWidth;
    trans.y *= gContext.mHeight;
    trans.x += gContext.mX;
    trans.y += gContext.mY;
    return ImVec2((float)trans.x, (float)trans.y);
  }

  static ImVec2 worldToPos(const udDouble4& worldPos, const udDouble4x4& mat)
  {
    return worldToPos(worldPos.toVector3(), mat);
  }

  static double GetSegmentLengthClipSpace(const udDouble3& start, const udDouble3& end)
  {
    udDouble4 startOfSegment = gContext.mMVP * udDouble4::create(start, 1);
    if (startOfSegment.w != 0.0) // check for axis aligned with camera direction
      startOfSegment /= startOfSegment.w;

    udDouble4 endOfSegment = gContext.mMVP * udDouble4::create(end, 1);
    if (endOfSegment.w != 0.0) // check for axis aligned with camera direction
      endOfSegment /= endOfSegment.w;

    udDouble4 clipSpaceAxis = endOfSegment - startOfSegment;
    clipSpaceAxis.y /= gContext.mDisplayRatio;

    return udSqrt(clipSpaceAxis.x*clipSpaceAxis.x + clipSpaceAxis.y*clipSpaceAxis.y);
  }

  static double GetParallelogram(const udDouble3& ptO, const udDouble3& ptA, const udDouble3& ptB)
  {
    udDouble4 pts[] = { udDouble4::create(ptO, 1), udDouble4::create(ptA, 1), udDouble4::create(ptB, 1) };
    for (unsigned int i = 0; i < 3; i++)
    {
      pts[i] = gContext.mMVP * pts[i];
      if (udAbs(pts[i].w) > DBL_EPSILON) // check for axis aligned with camera direction
        pts[i] *= 1.f / pts[i].w;
    }
    udDouble4 segA = pts[1] - pts[0];
    udDouble4 segB = pts[2] - pts[0];
    segA.y /= gContext.mDisplayRatio;
    segB.y /= gContext.mDisplayRatio;
    udDouble4 segAOrtho = udNormalize3(makeVect(-segA.y, segA.x));
    double dt = udDot3(segAOrtho, segB);
    double surface = sqrt(segA.x*segA.x + segA.y*segA.y) * udAbs(dt);
    return surface;
  }

  inline udDouble4 PointOnSegment(const udDouble4 & point, const udDouble4 & vertPos1, const udDouble4 & vertPos2)
  {
    udDouble4 c = point - vertPos1;
    udDouble4 V = udNormalize3(vertPos2 - vertPos1);
    double d = udMag3(vertPos2 - vertPos1);
    double t = udDot3(V, c);

    if (t < 0.f)
      return vertPos1;

    if (t > d)
      return vertPos2;

    return vertPos1 + V * t;
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
    return gContext.mbUsing;
  }

  bool IsOver()
  {
    return (GetMoveType(NULL) != NONE) || GetRotateType() != NONE || GetScaleType() != NONE || IsUsing();
  }

  static void ComputeContext(const udDouble4x4 &view, const udDouble4x4 &projection, const udDouble4x4 &matrix, ImGuizmoSpace mode)
  {
    gContext.mMode = mode;

    if (mode == igsLocal)
    {
      gContext.mModel = matrix;

      gContext.mModel.axis.x = udNormalize3(gContext.mModel.axis.x);
      gContext.mModel.axis.y = udNormalize3(gContext.mModel.axis.y);
      gContext.mModel.axis.z = udNormalize3(gContext.mModel.axis.z);
    }
    else
    {
      gContext.mModel = udDouble4x4::translation(matrix.axis.t.toVector3());
    }

    gContext.mModelSource = matrix;
    gContext.mModelScaleOrigin = udDouble3::create(udMag3(gContext.mModelSource.axis.x), udMag3(gContext.mModelSource.axis.y), udMag3(gContext.mModelSource.axis.z));

    gContext.mModelInverse = udInverse(gContext.mModel);
    gContext.mModelSourceInverse = udInverse(gContext.mModelSource);
    gContext.mViewProjection = projection * view;
    gContext.mMVP = gContext.mViewProjection * gContext.mModel;

    udDouble4x4 viewInverse = udInverse(view);
    gContext.mCameraDir = viewInverse.axis.y.toVector3();
    gContext.mCameraEye = viewInverse.axis.t.toVector3();

    udDouble3 rightViewInverse = (gContext.mModelInverse * udDouble4::create(viewInverse.axis.x.toVector3(), 0)).toVector3();
    double rightLength = GetSegmentLengthClipSpace(udDouble3::zero(), rightViewInverse);
    gContext.mScreenFactor = gGizmoSizeClipSpace / rightLength;

    ImVec2 centerSSpace = worldToPos(makeVect(0.f, 0.f), gContext.mMVP);
    gContext.mScreenSquareCenter = centerSSpace;
    gContext.mScreenSquareMin = ImVec2(centerSSpace.x - 10.f, centerSSpace.y - 10.f);
    gContext.mScreenSquareMax = ImVec2(centerSSpace.x + 10.f, centerSSpace.y + 10.f);
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

  static void ComputeTripodAxisAndVisibility(int axisIndex, udDouble3& dirAxis, udDouble3& dirPlaneX, udDouble3& dirPlaneY, bool& belowAxisLimit, bool& belowPlaneLimit)
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
      double lenDir = GetSegmentLengthClipSpace(udDouble3::zero(), dirAxis);
      double lenDirMinus = GetSegmentLengthClipSpace(udDouble3::zero(), -dirAxis);

      double lenDirPlaneX = GetSegmentLengthClipSpace(udDouble3::zero(), dirPlaneX);
      double lenDirMinusPlaneX = GetSegmentLengthClipSpace(udDouble3::zero(), -dirPlaneX);

      double lenDirPlaneY = GetSegmentLengthClipSpace(udDouble3::zero(), dirPlaneY);
      double lenDirMinusPlaneY = GetSegmentLengthClipSpace(udDouble3::zero(), -dirPlaneY);

      double mulAxis = (lenDir < lenDirMinus && udAbs(lenDir - lenDirMinus) > FLT_EPSILON) ? -1.f : 1.f;
      double mulAxisX = (lenDirPlaneX < lenDirMinusPlaneX && udAbs(lenDirPlaneX - lenDirMinusPlaneX) > FLT_EPSILON) ? -1.f : 1.f;
      double mulAxisY = (lenDirPlaneY < lenDirMinusPlaneY && udAbs(lenDirPlaneY - lenDirMinusPlaneY) > FLT_EPSILON) ? -1.f : 1.f;
      dirAxis *= mulAxis;
      dirPlaneX *= mulAxisX;
      dirPlaneY *= mulAxisY;

      // for axis
      double axisLengthInClipSpace = GetSegmentLengthClipSpace(udDouble3::zero(), dirAxis * gContext.mScreenFactor);

      double paraSurf = GetParallelogram(udDouble3::zero(), dirPlaneX * gContext.mScreenFactor, dirPlaneY * gContext.mScreenFactor);
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

  static void ComputeSnap(udDouble3& value, double snap)
  {
    for (int i = 0; i < 3; i++)
    {
      ComputeSnap(&value[i], snap);
    }
  }

  static double ComputeAngleOnPlane()
  {
    udDouble3 localPos;

    if (udIntersect(gContext.mTranslationPlane, gContext.mRay, &localPos) != udR_Success)
      return 0;

    localPos = udNormalize(localPos - gContext.mTranslationPlane.point);

    udDouble3 perpendicularVector = udNormalize(udCross(gContext.mRotationVectorSource, gContext.mTranslationPlane.normal));
    double acosAngle = udClamp(udDot(localPos, gContext.mRotationVectorSource), -1 + DBL_EPSILON, 1 - DBL_EPSILON);
    return udACos(acosAngle) * ((udDot(localPos, perpendicularVector) < 0.f) ? 1.f : -1.f);
  }

  static void DrawRotationGizmo(int type)
  {
    ImDrawList* drawList = gContext.mDrawList;

    // colors
    ImU32 colors[7];
    ComputeColors(colors, type, igoRotate);

    udDouble4 cameraToModelNormalized = gContext.mModelInverse * udDouble4::create(udNormalize(gContext.mModel.axis.t.toVector3() - gContext.mCameraEye), 0);
    gContext.mRadiusSquareCenter = screenRotateSize * gContext.mHeight;

    for (int axis = 0; axis < 3; axis++)
    {
      ImVec2 circlePos[halfCircleSegmentCount];
      double angleStart = udATan2(cameraToModelNormalized[(4 - axis) % 3], cameraToModelNormalized[(3 - axis) % 3]) + UD_PI * 0.5f;

      for (unsigned int i = 0; i < halfCircleSegmentCount; i++)
      {
        double ng = angleStart + UD_PI * ((float)i / (float)halfCircleSegmentCount);
        udDouble4 axisPos = makeVect(udCos(ng), udSin(ng), 0.f);
        udDouble4 pos = makeVect(axisPos[axis], axisPos[(axis + 1) % 3], axisPos[(axis + 2) % 3]) * gContext.mScreenFactor;
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
        udDouble4x4 rotateVectorMatrix = udDouble4x4::rotationAxis(gContext.mTranslationPlane.normal, ng);
        udDouble4 pos = rotateVectorMatrix * udDouble4::create(gContext.mRotationVectorSource, 1) * gContext.mScreenFactor;
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

  static void DrawHatchedAxis(const udDouble3& axis)
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
    udDouble3 scaleDisplay = { 1.f, 1.f, 1.f };

    if (gContext.mbUsing)
      scaleDisplay = gContext.mScale;

    for (unsigned int i = 0; i < 3; i++)
    {
      udDouble3 dirPlaneX, dirPlaneY, dirAxis;
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
      ImVec2 destinationPosOnScreen = worldToPos(gContext.mModel.axis.t, gContext.mViewProjection);

      char tmps[512];
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
      udDouble3 dirPlaneX, dirPlaneY, dirAxis;
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
          udDouble3 cornerWorldPos = (dirPlaneX * quadUV[j * 2] + dirPlaneY  * quadUV[j * 2 + 1]) * gContext.mScreenFactor;
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
      udDouble4 dif = { destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y, 0.f, 0.f };
      dif = udNormalize3(dif) * 5.f;
      drawList->AddCircle(sourcePosOnScreen, 6.f, translationLineColor);
      drawList->AddCircle(destinationPosOnScreen, 6.f, translationLineColor);
      drawList->AddLine(ImVec2(sourcePosOnScreen.x + (float)dif.x, sourcePosOnScreen.y + (float)dif.y), ImVec2(destinationPosOnScreen.x - (float)dif.x, destinationPosOnScreen.y - (float)dif.y), translationLineColor, 2.f);

      char tmps[512];
      udDouble3 deltaInfo = gContext.mModel.axis.t.toVector3() - gContext.mMatrixOrigin;
      int componentInfoIndex = (type - MOVE_X) * 3;

      if (componentInfoIndex >= 0)
      {
        ImFormatString(tmps, sizeof(tmps), translationInfoMask[type - MOVE_X], deltaInfo[translationInfoIndex[componentInfoIndex]], deltaInfo[translationInfoIndex[componentInfoIndex + 1]], deltaInfo[translationInfoIndex[componentInfoIndex + 2]]);
        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
      }
    }
  }

  static bool CanActivate()
  {
    if (ImGui::IsMouseClicked(0))// && !ImGui::IsAnyItemActive())
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
      udDouble3 dirPlaneX, dirPlaneY, dirAxis;
      bool belowAxisLimit, belowPlaneLimit;
      ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

      udDouble3 posOnPlane;
      if (udIntersect(udPlane<double>::create(gContext.mModel.axis.t.toVector3(), dirAxis), gContext.mRay, &posOnPlane) != udR_Success)
        continue;

      const ImVec2 posOnPlanScreen = worldToPos(posOnPlane, gContext.mViewProjection);
      const ImVec2 axisStartOnScreen = worldToPos(gContext.mModel.axis.t.toVector3() + dirAxis * gContext.mScreenFactor * 0.1f, gContext.mViewProjection);
      const ImVec2 axisEndOnScreen = worldToPos(gContext.mModel.axis.t.toVector3() + dirAxis * gContext.mScreenFactor, gContext.mViewProjection);

      udDouble4 closestPointOnAxis = PointOnSegment(makeVect(posOnPlanScreen), makeVect(axisStartOnScreen), makeVect(axisEndOnScreen));

      if (udMag3(closestPointOnAxis - makeVect(posOnPlanScreen)) < 12.f) // pixel size
        type = SCALE_X + i;
    }
    return type;
  }

  static int GetRotateType()
  {
    ImGuiIO& io = ImGui::GetIO();
    int type = NONE;

    udDouble2 deltaScreen = { io.MousePos.x - gContext.mScreenSquareCenter.x, io.MousePos.y - gContext.mScreenSquareCenter.y };
    double dist = udMag(deltaScreen);
    if (dist >= (gContext.mRadiusSquareCenter - 1.0f) && dist < (gContext.mRadiusSquareCenter + 1.0f))
      type = ROTATE_SCREEN;

    const udDouble3 planeNormals[] = { gContext.mModel.axis.x.toVector3(), gContext.mModel.axis.y.toVector3(), gContext.mModel.axis.z.toVector3() };

    for (unsigned int i = 0; i < 3 && type == NONE; i++)
    {
      // pickup plan
      udPlane<double> pickupPlane = udPlane<double>::create(gContext.mModel.axis.t.toVector3(), planeNormals[i]);

      udDouble3 localPos;
      if (udIntersect(pickupPlane, gContext.mRay, &localPos) != udR_Success)
        continue;
      localPos -= pickupPlane.point;

      if (udDot(gContext.mRay.direction, udNormalize3(localPos)) > FLT_EPSILON)
        continue;

      udDouble4 idealPosOnCircle = gContext.mModelInverse * udDouble4::create(udNormalize3(localPos), 0);
      ImVec2 idealPosOnCircleScreen = worldToPos(idealPosOnCircle * gContext.mScreenFactor, gContext.mMVP);

      ImVec2 distanceOnScreen = idealPosOnCircleScreen - io.MousePos;

      double distance = udMag3(makeVect(distanceOnScreen));

      if (distance < 8.f) // pixel size
        type = ROTATE_X + i;
    }

    return type;
  }

  static int GetMoveType(udDouble4 *gizmoHitProportion)
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
      udDouble3 dirPlaneX, dirPlaneY, dirAxis;
      bool belowAxisLimit, belowPlaneLimit;
      ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
      dirAxis = (gContext.mModel * udDouble4::create(dirAxis, 0)).toVector3();
      dirPlaneX = (gContext.mModel * udDouble4::create(dirPlaneX, 0)).toVector3();
      dirPlaneY = (gContext.mModel * udDouble4::create(dirPlaneY, 0)).toVector3();

      udDouble3 posOnPlane;
      if (udIntersect(udPlane<double>::create(gContext.mModel.axis.t.toVector3(), dirAxis), gContext.mRay, &posOnPlane) != udR_Success)
        continue;

      const ImVec2 posOnPlanScreen = worldToPos(posOnPlane, gContext.mViewProjection);
      const ImVec2 axisStartOnScreen = worldToPos(gContext.mModel.axis.t.toVector3() + dirAxis * gContext.mScreenFactor * 0.1f, gContext.mViewProjection);
      const ImVec2 axisEndOnScreen = worldToPos(gContext.mModel.axis.t.toVector3() + dirAxis * gContext.mScreenFactor, gContext.mViewProjection);

      udDouble4 closestPointOnAxis = PointOnSegment(makeVect(posOnPlanScreen), makeVect(axisStartOnScreen), makeVect(axisEndOnScreen));

      if (udMag3(closestPointOnAxis - makeVect(posOnPlanScreen)) < 12.f) // pixel size
        type = MOVE_X + i;

      const double dx = udDot(dirPlaneX, ((posOnPlane - gContext.mModel.axis.t.toVector3()) * (1.f / gContext.mScreenFactor)));
      const double dy = udDot(dirPlaneY, ((posOnPlane - gContext.mModel.axis.t.toVector3()) * (1.f / gContext.mScreenFactor)));
      if (belowPlaneLimit && dx >= quadUV[0] && dx <= quadUV[4] && dy >= quadUV[1] && dy <= quadUV[3])
        type = MOVE_YZ + i;

      if (gizmoHitProportion)
        *gizmoHitProportion = makeVect(dx, dy, 0.f);
    }
    return type;
  }

  static void HandleTranslation(udDouble4x4 *matrix, udDouble4x4 *deltaMatrix, int& type, double snap)
  {
    ImGuiIO& io = ImGui::GetIO();
    bool applyRotationLocaly = gContext.mMode == igsLocal || type == MOVE_SCREEN;

    // move
    if (gContext.mbUsing)
    {
      ImGui::CaptureMouseFromApp();
      udDouble3 newPos;

      if (udIntersect(gContext.mTranslationPlane, gContext.mRay, &newPos) == udR_Success)
      {
        // compute delta
        udDouble3 newOrigin = newPos - gContext.mRelativeOrigin * gContext.mScreenFactor;
        udDouble3 delta = newOrigin - gContext.mModel.axis.t.toVector3();

        // 1 axis constraint
        if (gContext.mCurrentOperation >= MOVE_X && gContext.mCurrentOperation <= MOVE_Z)
        {
          int axisIndex = gContext.mCurrentOperation - MOVE_X;
          udDouble3 axisValue = gContext.mModel.c[axisIndex].toVector3();
          double lengthOnAxis = udDot3(axisValue, delta);
          delta = axisValue * lengthOnAxis;
        }

        // snap
        if (snap)
        {
          udDouble3 cumulativeDelta = gContext.mModel.axis.t.toVector3() + delta - gContext.mMatrixOrigin;
          if (applyRotationLocaly)
          {
            udDouble4x4 modelSourceNormalized = gContext.mModelSource;

            gContext.mModelSource.axis.x = udNormalize3(gContext.mModelSource.axis.x);
            gContext.mModelSource.axis.y = udNormalize3(gContext.mModelSource.axis.y);
            gContext.mModelSource.axis.z = udNormalize3(gContext.mModelSource.axis.z);

            udDouble4x4 modelSourceNormalizedInverse = udInverse(modelSourceNormalized);
            cumulativeDelta = (modelSourceNormalizedInverse * udDouble4::create(cumulativeDelta, 0)).toVector3();
            ComputeSnap(cumulativeDelta, snap);
            cumulativeDelta = (modelSourceNormalized * udDouble4::create(cumulativeDelta, 0)).toVector3();
          }
          else
          {
            ComputeSnap(cumulativeDelta, snap);
          }
          delta = gContext.mMatrixOrigin + cumulativeDelta - gContext.mModel.axis.t.toVector3();

        }

        udDouble4x4 deltaMatrixTranslation = udDouble4x4::translation(delta);
        if (deltaMatrix)
          *deltaMatrix = deltaMatrixTranslation;

        *matrix = deltaMatrixTranslation * gContext.mModelSource;

        if (!io.MouseDown[0])
          gContext.mbUsing = false;

        type = gContext.mCurrentOperation;
      }
    }
    else
    {
      // find new possible way to move
      udDouble4 gizmoHitProportion;
      type = GetMoveType(&gizmoHitProportion);
      if (type != NONE)
      {
        ImGui::CaptureMouseFromApp();
      }

      if (CanActivate() && type != NONE)
      {
        gContext.mbUsing = true;
        gContext.mCurrentOperation = type;
        udDouble3 movePlanNormal[] = { gContext.mModel.axis.x.toVector3(), gContext.mModel.axis.y.toVector3(), gContext.mModel.axis.z.toVector3(), gContext.mModel.axis.x.toVector3(), gContext.mModel.axis.y.toVector3(), gContext.mModel.axis.z.toVector3(), -gContext.mCameraDir };

        udDouble3 cameraToModelNormalized = udNormalize(gContext.mModel.axis.t.toVector3() - gContext.mCameraEye);
        for (unsigned int i = 0; i < 3; i++)
        {
          movePlanNormal[i] = udNormalize(udCross(udCross(cameraToModelNormalized, movePlanNormal[i]), movePlanNormal[i]));
        }

        // pickup plan
        gContext.mTranslationPlane = udPlane<double>::create(gContext.mModel.axis.t.toVector3(), movePlanNormal[type - MOVE_X]);
        if (udIntersect(gContext.mTranslationPlane, gContext.mRay, &gContext.mTranslationPlaneOrigin) == udR_Success)
        {
          gContext.mMatrixOrigin = gContext.mModel.axis.t.toVector3();
          gContext.mRelativeOrigin = (gContext.mTranslationPlaneOrigin - gContext.mModel.axis.t.toVector3()) * (1.f / gContext.mScreenFactor);
        }
      }
    }
  }

  static void HandleScale(udDouble4x4 *matrix, udDouble4x4 *deltaMatrix, int& type, double snap)
  {
    ImGuiIO& io = ImGui::GetIO();

    if (!gContext.mbUsing)
    {
      // find new possible way to scale
      type = GetScaleType();
      if (type != NONE)
        ImGui::CaptureMouseFromApp();

      if (CanActivate() && type != NONE)
      {
        gContext.mbUsing = true;
        gContext.mCurrentOperation = type;
        const udDouble3 movePlanNormal[] = { gContext.mModel.axis.y.toVector3(), gContext.mModel.axis.z.toVector3(), gContext.mModel.axis.x.toVector3(), gContext.mModel.axis.z.toVector3(), gContext.mModel.axis.y.toVector3(), gContext.mModel.axis.x.toVector3(), -gContext.mCameraDir };

        gContext.mTranslationPlane = udPlane<double>::create(gContext.mModel.axis.t.toVector3(), movePlanNormal[type - SCALE_X]);
        if (udIntersect(gContext.mTranslationPlane, gContext.mRay, &gContext.mTranslationPlaneOrigin) == udR_Success)
        {
          gContext.mMatrixOrigin = gContext.mModel.axis.t.toVector3();
          gContext.mScale = udDouble3::one();
          gContext.mRelativeOrigin = (gContext.mTranslationPlaneOrigin - gContext.mModel.axis.t.toVector3()) * (1.f / gContext.mScreenFactor);
          gContext.mScaleValueOrigin = udDouble3::create(udMag3(gContext.mModelSource.axis.x), udMag3(gContext.mModelSource.axis.y), udMag3(gContext.mModelSource.axis.z));
          gContext.mSaveMousePosx = io.MousePos.x;
        }
      }
    }

    // scale
    if (gContext.mbUsing)
    {
      ImGui::CaptureMouseFromApp();
      udDouble3 newPos;
      if (udIntersect(gContext.mTranslationPlane, gContext.mRay, &newPos) == udR_Success)
      {
        udDouble3 newOrigin = newPos - gContext.mRelativeOrigin * gContext.mScreenFactor;
        udDouble3 delta = newOrigin - gContext.mModel.axis.t.toVector3();

        // 1 axis constraint
        if (gContext.mCurrentOperation >= SCALE_X && gContext.mCurrentOperation <= SCALE_Z)
        {
          int axisIndex = gContext.mCurrentOperation - SCALE_X;
          udDouble3 axisValue = gContext.mModel.c[axisIndex].toVector3();
          double lengthOnAxis = udDot(axisValue, delta);
          delta = axisValue * lengthOnAxis;

          udDouble3 baseVector = gContext.mTranslationPlaneOrigin - gContext.mModel.axis.t.toVector3();
          double ratio = udDot(axisValue, baseVector + delta) / udDot(axisValue, baseVector);

          gContext.mScale[axisIndex] = udMax(ratio, 0.001);
        }
        else
        {
          double scaleDelta = (io.MousePos.x - gContext.mSaveMousePosx)  * 0.01f;
          gContext.mScale = udDouble3::create(udMax(1.0 + scaleDelta, 0.001));
        }

        // snap
        if (snap)
          ComputeSnap(gContext.mScale, snap);

        // no 0 allowed
        for (int i = 0; i < 3;i++)
          gContext.mScale[i] = udMax(gContext.mScale[i], 0.001);

        // compute matrix & delta
        udDouble4x4 deltaMatrixScale = udDouble4x4::scaleNonUniform(gContext.mScale * gContext.mScaleValueOrigin);

        udDouble4x4 res = gContext.mModel * deltaMatrixScale;
        *(udDouble4x4*)matrix = res;

        if (deltaMatrix)
        {
          deltaMatrixScale = udDouble4x4::scaleNonUniform(gContext.mScale);
          memcpy(deltaMatrix, deltaMatrixScale.a, sizeof(double) * 16);
        }

        if (!io.MouseDown[0])
          gContext.mbUsing = false;

        type = gContext.mCurrentOperation;
      }
    }
  }

  static void HandleRotation(udDouble4x4 *matrix, udDouble4x4 *deltaMatrix, int& type, double snap)
  {
    ImGuiIO& io = ImGui::GetIO();
    bool applyRotationLocally = gContext.mMode == igsLocal;

    if (!gContext.mbUsing)
    {
      type = GetRotateType();

      if (type != NONE)
        ImGui::CaptureMouseFromApp();

      if (type == ROTATE_SCREEN)
        applyRotationLocally = true;

      if (CanActivate() && type != NONE)
      {
        gContext.mbUsing = true;
        gContext.mCurrentOperation = type;
        const udDouble3 rotatePlaneNormal[] = { gContext.mModel.axis.x.toVector3(), gContext.mModel.axis.y.toVector3(), gContext.mModel.axis.z.toVector3(), -gContext.mCameraDir };

        if (applyRotationLocally)
          gContext.mTranslationPlane = udPlane<double>::create(gContext.mModel.axis.t.toVector3(), rotatePlaneNormal[type - ROTATE_X]);
        else
          gContext.mTranslationPlane = udPlane<double>::create(gContext.mModelSource.axis.t.toVector3(), directionUnary[type - ROTATE_X]);

        if (udIntersect(gContext.mTranslationPlane, gContext.mRay, &gContext.mRotationVectorSource) == udR_Success)
        {
          gContext.mRotationVectorSource = udNormalize(gContext.mRotationVectorSource - gContext.mTranslationPlane.point);
          gContext.mRotationAngleOrigin = ComputeAngleOnPlane();
        }
      }
    }

    // rotation
    if (gContext.mbUsing)
    {
      ImGui::CaptureMouseFromApp();
      gContext.mRotationAngle = ComputeAngleOnPlane();

      if (snap)
        ComputeSnap(&gContext.mRotationAngle, UD_DEG2RAD(snap));

      udDouble4 rotationAxisLocalSpace = udNormalize3(gContext.mModelInverse * udDouble4::create(gContext.mTranslationPlane.normal, 0.f));

      udDouble4x4 deltaRotation = udDouble4x4::rotationAxis(rotationAxisLocalSpace.toVector3(), gContext.mRotationAngle - gContext.mRotationAngleOrigin);
      gContext.mRotationAngleOrigin = gContext.mRotationAngle;

      if (applyRotationLocally)
      {
        *matrix = gContext.mModel * deltaRotation * udDouble4x4::scaleNonUniform(gContext.mModelScaleOrigin);
      }
      else
      {
        udDouble4x4 res = gContext.mModelSource;
        res.axis.t = udDouble4::zero();

        *matrix = deltaRotation * res;
        matrix->axis.t = gContext.mModelSource.axis.t;
      }

      if (deltaMatrix)
        *deltaMatrix = gContext.mModel * deltaRotation * gContext.mModelInverse;

      if (!io.MouseDown[0])
        gContext.mbUsing = false;

      type = gContext.mCurrentOperation;
    }
  }

  void Manipulate(const udDouble4x4 &view, const udDouble4x4 &projection, ImGuizmoOperation operation, ImGuizmoSpace mode, udDouble4x4 *pMatrix, const udRay<double> &mouseRay, udDouble4x4 *deltaMatrix /*= 0*/, double snap /*= 0*/)
  {
    ComputeContext(view, projection, *pMatrix, mode);
    gContext.mRay = mouseRay;

    if (deltaMatrix)
      deltaMatrix->identity();

    // behind camera
    udDouble4 camSpacePosition = gContext.mMVP * udDouble4::create(0, 0, 0, 1);
    if (camSpacePosition.z < 0.001f)
      return;

    int type = NONE;
    switch (operation)
    {
    case igoRotate:
      HandleRotation(pMatrix, deltaMatrix, type, snap);
      DrawRotationGizmo(type);
      break;
    case igoTranslate:
      HandleTranslation(pMatrix, deltaMatrix, type, snap);
      DrawTranslationGizmo(type);
      break;
    case igoScale:
      HandleScale(pMatrix, deltaMatrix, type, snap);
      DrawScaleGizmo(type);
      break;
    }
  }
};
