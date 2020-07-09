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

const double gGizmoSizeClipSpace = 0.1;
const double gScreenRotateSize = 0.06;

udDouble4 vcGizmo_MakeVect(ImVec2 v) { udDouble4 res; res.x = v.x; res.y = v.y; res.z = 0.0; res.w = 0.0; return res; }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum vcGizmoMoveType
{
  vcGMT_None,
  vcGMT_MoveX,
  vcGMT_MoveY,
  vcGMT_MoveZ,
  vcGMT_MoveYZ,
  vcGMT_MoveZX,
  vcGMT_MoveXY,
  vcGMT_MoveScreen,
  vcGMT_RotateX,
  vcGMT_RotateY,
  vcGMT_RotateZ,
  vcGMT_RotateScreen,
  vcGMT_ScaleX,
  vcGMT_ScaleY,
  vcGMT_ScaleZ,
  vcGMT_ScaleXYZ,

  vcGMT_Count
};

struct vcGizmoContext
{
  ImDrawList* ImGuiDrawList;

  vcGizmoCoordinateSystem mMode;
  vcGizmoAllowedControls allowedControls;

  vcCamera camera;

  udDouble4x4 mModel;
  udDouble4x4 mModelInverse;
  udDouble4x4 mMVP;

  udDouble3 mModelScaleOrigin;
  udDouble3 mCameraDir;

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
  udDouble3 mLastScaleDelta;
  float mSaveMousePosx;
  float mLastMousePosx;

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

static vcGizmoContext sGizmoContext = {};

static const ImU32 DirectionColor[3] = { 0xFF0000AA, 0xFF00AA00, 0xFFAA0000 };
static const ImU32 PlaneColors[3] = { 0x610000AA, 0x6100AA00, 0x61AA0000 };
static const ImU32 SelectionColor = 0x8A1080FF;
static const ImU32 TranslationLineColor = 0xAAAAAAAA;
static const ImU32 DisabledColor = 0x99999999;
static const ImU32 WhiteColor = 0xFFFFFFFF;

static const char *sTextInfoMask[] = { "?", "X : %5.3f", "Y : %5.3f", "Z : %5.3f", "Y : %5.3f Z : %5.3f", "X : %5.3f Z : %5.3f", "X : %5.3f Y : %5.3f", "X : %5.3f Y : %5.3f Z : %5.3f", "X : %5.2f deg %5.2f rad", "Y : %5.2f deg %5.2f rad", "Z : %5.2f deg %5.2f rad", "Screen : %5.2f deg %5.2f rad", "X : %5.2f", "Y : %5.2f", "Z : %5.2f", "XYZ : %5.2f" };
UDCOMPILEASSERT(udLengthOf(sTextInfoMask) == vcGMT_Count, "Not Enough Text Info");

static const int sTranslationInfoIndex[] = { 0,0,0, 1,0,0, 2,0,0, 1,2,0, 0,2,0, 0,1,0, 0,1,2 };
static const float sQuadMin = 0.5f;
static const float sQuadMax = 0.8f;
static const float sQuadUV[8] = { sQuadMin, sQuadMin, sQuadMin, sQuadMax, sQuadMax, sQuadMax, sQuadMax, sQuadMin };
static const int sHalfCircleSegmentCount = 64;
static const float sSnapTension = 0.5f;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
static int vcGizmo_GetMoveType(const udDouble3 direction[]);
static int vcGizmo_GetRotateType();
static int vcGizmo_GetScaleType(const udDouble3 direction[]);

static ImVec2 vcGizmo_WorldToScreen(const udDouble3& worldPos, const udDouble4x4& mat)
{
  udDouble4 trans = mat * udDouble4::create(worldPos, 1);
  if (trans.w != 0)
    trans *= 0.5f / trans.w;
  trans += udDouble4::create(0.5, 0.5, 0.0, 0.0);
#if !GRAPHICS_API_OPENGL
  trans.y = 1.0 - trans.y;
#endif
  trans.x *= sGizmoContext.mWidth;
  trans.y *= sGizmoContext.mHeight;
  trans.x += sGizmoContext.mX;
  trans.y += sGizmoContext.mY;
  return ImVec2((float)trans.x, (float)trans.y);
}

static ImVec2 vcGizmo_WorldToScreen(const udDouble4& worldPos, const udDouble4x4& mat)
{
  return vcGizmo_WorldToScreen(worldPos.toVector3(), mat);
}

static double vcGizmo_GetSegmentLengthClipSpace(const udDouble3& start, const udDouble3& end)
{
  udDouble4 startOfSegment = sGizmoContext.mMVP * udDouble4::create(start, 1);
  if (startOfSegment.w != 0.0) // check for axis aligned with camera direction
    startOfSegment /= startOfSegment.w;

  udDouble4 endOfSegment = sGizmoContext.mMVP * udDouble4::create(end, 1);
  if (endOfSegment.w != 0.0) // check for axis aligned with camera direction
    endOfSegment /= endOfSegment.w;

  udDouble4 clipSpaceAxis = endOfSegment - startOfSegment;
  clipSpaceAxis.y /= sGizmoContext.mDisplayRatio;

  return udSqrt(clipSpaceAxis.x*clipSpaceAxis.x + clipSpaceAxis.y*clipSpaceAxis.y);
}

static double vcGizmo_GetParallelogram(const udDouble3& ptO, const udDouble3& ptA, const udDouble3& ptB)
{
  udDouble4 pts[] = { udDouble4::create(ptO, 1), udDouble4::create(ptA, 1), udDouble4::create(ptB, 1) };
  for (unsigned int i = 0; i < 3; i++)
  {
    pts[i] = sGizmoContext.mMVP * pts[i];
    if (udAbs(pts[i].w) > DBL_EPSILON) // check for axis aligned with camera direction
      pts[i] *= 1.0 / pts[i].w;
  }
  udDouble3 segA = (pts[1] - pts[0]).toVector3();
  udDouble3 segB = (pts[2] - pts[0]).toVector3();
  segA.y /= sGizmoContext.mDisplayRatio;
  segB.y /= sGizmoContext.mDisplayRatio;
  udDouble3 segAOrtho = udNormalize(udDouble3::create(-segA.y, segA.x, 0.0));
  double dt = udDot(segAOrtho, segB);
  double surface = sqrt(segA.x*segA.x + segA.y*segA.y) * udAbs(dt);
  return surface;
}

inline udDouble4 vcGizmo_PointOnSegment(const udDouble4 &point, const udDouble4 &vertPos1, const udDouble4 &vertPos2)
{
  udDouble4 c = point - vertPos1;
  udDouble4 V = udNormalize3(vertPos2 - vertPos1);
  double d = udMag3(vertPos2 - vertPos1);
  double t = udDot3(V, c);

  if (t < 0.0)
    return vertPos1;

  if (t > d)
    return vertPos2;

  return vertPos1 + V * t;
}

void vcGizmo_SetRect(float x, float y, float width, float height)
{
  sGizmoContext.mX = x;
  sGizmoContext.mY = y;
  sGizmoContext.mWidth = width;
  sGizmoContext.mHeight = height;
  sGizmoContext.mXMax = sGizmoContext.mX + sGizmoContext.mWidth;
  sGizmoContext.mYMax = sGizmoContext.mY + sGizmoContext.mXMax;
  sGizmoContext.mDisplayRatio = width / height;
}

void vcGizmo_SetDrawList()
{
  sGizmoContext.ImGuiDrawList = ImGui::GetForegroundDrawList();
}

void vcGizmo_BeginFrame()
{
  ImGuiIO& io = ImGui::GetIO();

  const ImU32 flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
  ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::SetNextWindowPos(ImVec2(0, 0));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
  ImGui::PushStyleColor(ImGuiCol_Border, 0);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

  ImGui::Begin("gizmo", NULL, flags);
  sGizmoContext.ImGuiDrawList = ImGui::GetForegroundDrawList();
  ImGui::End();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor(2);
}

bool vcGizmo_IsActive()
{
  return sGizmoContext.mbUsing;
}

bool vcGizmo_IsHovered(const udDouble3 direction[])
{
  return (vcGizmo_GetMoveType(direction) != vcGMT_None || vcGizmo_GetRotateType() != vcGMT_None || vcGizmo_GetScaleType(direction) != vcGMT_None || vcGizmo_IsActive());
}

static void vcGizmo_ComputeContext(const vcCamera *pCamera, const udDouble4x4 &matrix, vcGizmoCoordinateSystem mode, vcGizmoAllowedControls allowedControls)
{
  sGizmoContext.mMode = mode;
  sGizmoContext.allowedControls = allowedControls;

  sGizmoContext.camera = *pCamera;

  if (mode == vcGCS_Local)
  {
    sGizmoContext.mModel = matrix;

    sGizmoContext.mModel.axis.x = udNormalize3(sGizmoContext.mModel.axis.x);
    sGizmoContext.mModel.axis.y = udNormalize3(sGizmoContext.mModel.axis.y);
    sGizmoContext.mModel.axis.z = udNormalize3(sGizmoContext.mModel.axis.z);
  }
  else
  {
    sGizmoContext.mModel = udDouble4x4::translation(matrix.axis.t.toVector3());
  }

  sGizmoContext.mModelScaleOrigin = udDouble3::create(udMag3(matrix.axis.x), udMag3(matrix.axis.y), udMag3(matrix.axis.z));

  sGizmoContext.mModelInverse = udInverse(sGizmoContext.mModel);
  sGizmoContext.mMVP = sGizmoContext.camera.matrices.viewProjection * sGizmoContext.mModel;

  sGizmoContext.mCameraDir = udDirectionFromYPR(udDouble3::create(sGizmoContext.camera.headingPitch, 0.0)); //TODO: Fix me in ECEF

  udDouble3 rightViewInverse = (sGizmoContext.mModelInverse * udDouble4::create(sGizmoContext.camera.matrices.camera.axis.x.toVector3(), 0)).toVector3();
  double rightLength = vcGizmo_GetSegmentLengthClipSpace(udDouble3::zero(), rightViewInverse);
  sGizmoContext.mScreenFactor = gGizmoSizeClipSpace / rightLength;

  ImVec2 centerSSpace = vcGizmo_WorldToScreen(udDouble3::zero(), sGizmoContext.mMVP);
  sGizmoContext.mScreenSquareCenter = centerSSpace;
  sGizmoContext.mScreenSquareMin = ImVec2(centerSSpace.x - 10.f, centerSSpace.y - 10.f);
  sGizmoContext.mScreenSquareMax = ImVec2(centerSSpace.x + 10.f, centerSSpace.y + 10.f);
}

static void vcGizmo_ComputeColors(ImU32 *colors, size_t numColors, int type, vcGizmoOperation operation)
{
  // Presets all the colours to a disabled colour; that way if they aren't otherwise set below they are disabled
  for (size_t i = 0; i < numColors; i++)
    colors[i] = DisabledColor;

  // The loops below require at least this many colors
  if (numColors < 5 || (numColors < 7 && operation != vcGO_Translate))
    return;

  switch (operation)
  {
  case vcGO_Translate:
    if (sGizmoContext.allowedControls & vcGAC_Translation)
    {
      colors[0] = (type == vcGMT_MoveScreen) ? SelectionColor : WhiteColor;
      for (int i = 0; i < 3; i++)
      {
        colors[i + 1] = (type == (int)(vcGMT_MoveX + i)) ? SelectionColor : DirectionColor[i];
        colors[i + 4] = (type == (int)(vcGMT_MoveYZ + i)) ? SelectionColor : PlaneColors[i];
        colors[i + 4] = (type == vcGMT_MoveScreen) ? SelectionColor : colors[i + 4];
      }
    }
    break;
  case vcGO_Rotate:
    if (sGizmoContext.allowedControls & vcGAC_RotationScreen)
    {
      colors[0] = (type == vcGMT_RotateScreen) ? SelectionColor : WhiteColor;
    }

    if (sGizmoContext.allowedControls & vcGAC_RotationAxis)
    {
      for (int i = 0; i < 3; i++)
        colors[i + 1] = (type == (int)(vcGMT_RotateX + i)) ? SelectionColor : DirectionColor[i];
    }
    break;
  case vcGO_Scale:
    if (sGizmoContext.allowedControls & vcGAC_ScaleUniform)
      colors[0] = (type == vcGMT_ScaleXYZ) ? SelectionColor : WhiteColor;

    if (sGizmoContext.allowedControls & vcGAC_ScaleNonUniform)
    {
      for (int i = 0; i < 3; i++)
        colors[i + 1] = (type == (int)(vcGMT_ScaleX + i)) ? SelectionColor : DirectionColor[i];
    }
    break;
  case vcGO_NoGizmo:
    break;
  }
}

static void vcGizmo_ComputeTripodAxisAndVisibility(const udDouble3 direction[], int axisIndex, udDouble3& dirAxis, udDouble3& dirPlaneX, udDouble3& dirPlaneY, bool& belowAxisLimit, bool& belowPlaneLimit)
{
  dirAxis = direction[axisIndex];
  dirPlaneX = direction[(axisIndex + 1) % 3];
  dirPlaneY = direction[(axisIndex + 2) % 3];

  if (sGizmoContext.mbUsing)
  {
    // when using, use stored factors so the gizmo doesn't flip when we translate
    belowAxisLimit = sGizmoContext.mBelowAxisLimit[axisIndex];
    belowPlaneLimit = sGizmoContext.mBelowPlaneLimit[axisIndex];

    dirAxis *= sGizmoContext.mAxisFactor[axisIndex];
    dirPlaneX *= sGizmoContext.mAxisFactor[(axisIndex + 1) % 3];
    dirPlaneY *= sGizmoContext.mAxisFactor[(axisIndex + 2) % 3];
  }
  else
  {
    // new method
    double lenDir = vcGizmo_GetSegmentLengthClipSpace(udDouble3::zero(), dirAxis);
    double lenDirMinus = vcGizmo_GetSegmentLengthClipSpace(udDouble3::zero(), -dirAxis);

    double lenDirPlaneX = vcGizmo_GetSegmentLengthClipSpace(udDouble3::zero(), dirPlaneX);
    double lenDirMinusPlaneX = vcGizmo_GetSegmentLengthClipSpace(udDouble3::zero(), -dirPlaneX);

    double lenDirPlaneY = vcGizmo_GetSegmentLengthClipSpace(udDouble3::zero(), dirPlaneY);
    double lenDirMinusPlaneY = vcGizmo_GetSegmentLengthClipSpace(udDouble3::zero(), -dirPlaneY);

    double mulAxis = (lenDir < lenDirMinus && udAbs(lenDir - lenDirMinus) > FLT_EPSILON) ? -1.0 : 1.0;
    double mulAxisX = (lenDirPlaneX < lenDirMinusPlaneX && udAbs(lenDirPlaneX - lenDirMinusPlaneX) > FLT_EPSILON) ? -1.0 : 1.0;
    double mulAxisY = (lenDirPlaneY < lenDirMinusPlaneY && udAbs(lenDirPlaneY - lenDirMinusPlaneY) > FLT_EPSILON) ? -1.0 : 1.0;
    dirAxis *= mulAxis;
    dirPlaneX *= mulAxisX;
    dirPlaneY *= mulAxisY;

    // for axis
    double axisLengthInClipSpace = vcGizmo_GetSegmentLengthClipSpace(udDouble3::zero(), dirAxis * sGizmoContext.mScreenFactor);

    double paraSurf = vcGizmo_GetParallelogram(udDouble3::zero(), dirPlaneX * sGizmoContext.mScreenFactor, dirPlaneY * sGizmoContext.mScreenFactor);
    belowPlaneLimit = (paraSurf > 0.0025f);
    belowAxisLimit = (axisLengthInClipSpace > 0.02f);

    // and store values
    sGizmoContext.mAxisFactor[axisIndex] = mulAxis;
    sGizmoContext.mAxisFactor[(axisIndex + 1) % 3] = mulAxisX;
    sGizmoContext.mAxisFactor[(axisIndex + 2) % 3] = mulAxisY;
    sGizmoContext.mBelowAxisLimit[axisIndex] = belowAxisLimit;
    sGizmoContext.mBelowPlaneLimit[axisIndex] = belowPlaneLimit;
  }
}

static void vcGizmo_ComputeSnap(double* value, double snap)
{
  if (snap <= DBL_EPSILON)
    return;

  double modulo = udMod(*value, snap);
  double moduloRatio = udAbs(modulo) / snap;

  if (moduloRatio < sSnapTension)
    *value -= modulo;
  else if (moduloRatio > (1.0 - sSnapTension))
    *value = *value - modulo + snap * ((*value < 0.0) ? -1.0 : 1.0);
}

static void vcGizmo_ComputeSnap(udDouble3& value, double snap)
{
  for (int i = 0; i < 3; i++)
  {
    vcGizmo_ComputeSnap(&value[i], snap);
  }
}

static double vcGizmo_ComputeAngleOnPlane()
{
  udDouble3 localPos;

  if (!sGizmoContext.mTranslationPlane.intersects(sGizmoContext.camera.worldMouseRay, &localPos, nullptr))
    return 0;

  localPos = udNormalize(localPos - sGizmoContext.mTranslationPlane.point);

  udDouble3 perpendicularVector = udNormalize(udCross(sGizmoContext.mRotationVectorSource, sGizmoContext.mTranslationPlane.normal));
  double acosAngle = udClamp(udDot(localPos, sGizmoContext.mRotationVectorSource), -1 + DBL_EPSILON, 1 - DBL_EPSILON);
  return udACos(acosAngle) * ((udDot(localPos, perpendicularVector) < 0.0) ? 1.0 : -1.0);
}

static void vcGizmo_DrawRotationGizmo(int type)
{
  ImDrawList* drawList = sGizmoContext.ImGuiDrawList;

  // colors
  ImU32 colors[7];
  vcGizmo_ComputeColors(colors, udLengthOf(colors), type, vcGO_Rotate);

  udDouble4 cameraToModelNormalized = sGizmoContext.mModelInverse * udDouble4::create(udNormalize(sGizmoContext.mModel.axis.t.toVector3() - sGizmoContext.camera.position), 0);
  sGizmoContext.mRadiusSquareCenter = gScreenRotateSize * sGizmoContext.mHeight;

  for (int axis = 0; axis < 3; axis++)
  {
    ImVec2 circlePos[sHalfCircleSegmentCount];
    double angleStart = udATan2(cameraToModelNormalized[(4 - axis) % 3], cameraToModelNormalized[(3 - axis) % 3]) + UD_PI * 0.5f;

    for (unsigned int i = 0; i < sHalfCircleSegmentCount; i++)
    {
      double ng = angleStart + UD_PI * ((float)i / (float)sHalfCircleSegmentCount);
      udDouble3 axisPos = udDouble3::create(udCos(ng), udSin(ng), 0.0);
      udDouble3 pos = udDouble3::create(axisPos[axis], axisPos[(axis + 1) % 3], axisPos[(axis + 2) % 3]) * sGizmoContext.mScreenFactor;
      circlePos[i] = vcGizmo_WorldToScreen(pos, sGizmoContext.mMVP);
    }

    float radiusAxis = udSqrt((ImLengthSqr(vcGizmo_WorldToScreen(sGizmoContext.mModel.axis.t, sGizmoContext.camera.matrices.viewProjection) - circlePos[0])));
    if (radiusAxis > sGizmoContext.mRadiusSquareCenter)
      sGizmoContext.mRadiusSquareCenter = radiusAxis;

    drawList->AddPolyline(circlePos, sHalfCircleSegmentCount, colors[3 - axis], false, 2);
  }

  drawList->AddCircle(vcGizmo_WorldToScreen(sGizmoContext.mModel.axis.t, sGizmoContext.camera.matrices.viewProjection), (float)sGizmoContext.mRadiusSquareCenter, colors[0], 64, 3.0);

  if (sGizmoContext.mbUsing)
  {
    ImVec2 circlePos[sHalfCircleSegmentCount + 1];

    circlePos[0] = vcGizmo_WorldToScreen(sGizmoContext.mModel.axis.t, sGizmoContext.camera.matrices.viewProjection);

    for (unsigned int i = 1; i < sHalfCircleSegmentCount; i++)
    {
      double ng = sGizmoContext.mRotationAngle * ((float)(i - 1) / (float)(sHalfCircleSegmentCount - 1));
      udDouble4x4 rotateVectorMatrix = udDouble4x4::rotationAxis(sGizmoContext.mTranslationPlane.normal, ng);
      udDouble4 pos = rotateVectorMatrix * udDouble4::create(sGizmoContext.mRotationVectorSource, 1) * sGizmoContext.mScreenFactor;
      circlePos[i] = vcGizmo_WorldToScreen(pos + sGizmoContext.mModel.axis.t, sGizmoContext.camera.matrices.viewProjection);
    }
    drawList->AddConvexPolyFilled(circlePos, sHalfCircleSegmentCount, 0x801080FF);
    drawList->AddPolyline(circlePos, sHalfCircleSegmentCount, 0xFF1080FF, true, 2);

    ImVec2 destinationPosOnScreen = circlePos[1];
    char tmps[512];
    ImFormatString(tmps, sizeof(tmps), sTextInfoMask[type], UD_RAD2DEG(sGizmoContext.mRotationAngle), sGizmoContext.mRotationAngle);
    drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
    drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
  }
}

static void vcGizmo_DrawHatchedAxis(const udDouble3& axis)
{
  for (int j = 1; j < 10; j++)
  {
    ImVec2 baseSSpace2 = vcGizmo_WorldToScreen(axis * 0.05f * (float)(j * 2) * sGizmoContext.mScreenFactor, sGizmoContext.mMVP);
    ImVec2 worldDirSSpace2 = vcGizmo_WorldToScreen(axis * 0.05f * (float)(j * 2 + 1) * sGizmoContext.mScreenFactor, sGizmoContext.mMVP);
    sGizmoContext.ImGuiDrawList->AddLine(baseSSpace2, worldDirSSpace2, 0x80000000, 6.f);
  }
}

static void vcGizmo_DrawScaleGizmo(const udDouble3 direction[], int type)
{
  ImDrawList* drawList = sGizmoContext.ImGuiDrawList;

  // colors
  ImU32 colors[7];
  vcGizmo_ComputeColors(colors, udLengthOf(colors), type, vcGO_Scale);

  // draw
  udDouble3 scaleDisplay = { 1.0, 1.0, 1.0 };

  if (sGizmoContext.mbUsing)
    scaleDisplay = sGizmoContext.mScaleValueOrigin;

  for (unsigned int i = 0; i < 3; i++)
  {
    udDouble3 dirPlaneX, dirPlaneY, dirAxis;
    bool belowAxisLimit, belowPlaneLimit;
    vcGizmo_ComputeTripodAxisAndVisibility(direction, i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

    // draw axis
    if (belowAxisLimit)
    {
      ImVec2 baseSSpace = vcGizmo_WorldToScreen(dirAxis * 0.1f * sGizmoContext.mScreenFactor, sGizmoContext.mMVP);
      ImVec2 worldDirSSpaceNoScale = vcGizmo_WorldToScreen(dirAxis * sGizmoContext.mScreenFactor, sGizmoContext.mMVP);
      ImVec2 worldDirSSpace = vcGizmo_WorldToScreen((dirAxis * scaleDisplay[i]) * sGizmoContext.mScreenFactor, sGizmoContext.mMVP);

      if (sGizmoContext.mbUsing)
      {
        drawList->AddLine(baseSSpace, worldDirSSpaceNoScale, 0xFF404040, 3.f);
        drawList->AddCircleFilled(worldDirSSpaceNoScale, 6.f, 0xFF404040);
      }

      drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], 3.f);
      drawList->AddCircleFilled(worldDirSSpace, 6.f, colors[i + 1]);

      if (sGizmoContext.mAxisFactor[i] < 0.0)
        vcGizmo_DrawHatchedAxis(dirAxis * scaleDisplay[i]);
    }
  }

  // draw screen cirle
  drawList->AddCircleFilled(sGizmoContext.mScreenSquareCenter, 6.f, colors[0], 32);

  if (sGizmoContext.mbUsing && type != vcGMT_None)
  {
    ImVec2 destinationPosOnScreen = vcGizmo_WorldToScreen(sGizmoContext.mModel.axis.t, sGizmoContext.camera.matrices.viewProjection);

    char tmps[512];
    int componentInfoIndex = (type - vcGMT_ScaleX) * 3;
    ImFormatString(tmps, sizeof(tmps), sTextInfoMask[type], scaleDisplay[sTranslationInfoIndex[componentInfoIndex]]);
    drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
    drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
  }
}


static void vcGizmo_DrawTranslationGizmo(const udDouble3 direction[], int type)
{
  ImDrawList* drawList = sGizmoContext.ImGuiDrawList;
  if (!drawList)
    return;

  // colors
  ImU32 colors[7];
  vcGizmo_ComputeColors(colors, udLengthOf(colors), type, vcGO_Translate);

  const ImVec2 origin = vcGizmo_WorldToScreen(sGizmoContext.mModel.axis.t, sGizmoContext.camera.matrices.viewProjection);

  // draw
  bool belowAxisLimit = false;
  bool belowPlaneLimit = false;
  for (unsigned int i = 0; i < 3; ++i)
  {
    udDouble3 dirPlaneX, dirPlaneY, dirAxis;
    vcGizmo_ComputeTripodAxisAndVisibility(direction, i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

    // draw axis
    if (belowAxisLimit)
    {
      ImVec2 baseSSpace = vcGizmo_WorldToScreen(dirAxis * 0.1f * sGizmoContext.mScreenFactor, sGizmoContext.mMVP);
      ImVec2 worldDirSSpace = vcGizmo_WorldToScreen(dirAxis * sGizmoContext.mScreenFactor, sGizmoContext.mMVP);

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

      if (sGizmoContext.mAxisFactor[i] < 0.0)
        vcGizmo_DrawHatchedAxis(dirAxis);
    }

    // draw plane
    if (belowPlaneLimit)
    {
      ImVec2 screenQuadPts[4];
      for (int j = 0; j < 4; ++j)
      {
        udDouble3 cornerWorldPos = (dirPlaneX * sQuadUV[j * 2] + dirPlaneY  * sQuadUV[j * 2 + 1]) * sGizmoContext.mScreenFactor;
        screenQuadPts[j] = vcGizmo_WorldToScreen(cornerWorldPos, sGizmoContext.mMVP);
      }
      drawList->AddPolyline(screenQuadPts, 4, DirectionColor[i], true, 1.0f);
      drawList->AddConvexPolyFilled(screenQuadPts, 4, colors[i + 4]);
    }
  }

  drawList->AddCircleFilled(sGizmoContext.mScreenSquareCenter, 6.f, colors[0], 32);

  if (sGizmoContext.mbUsing)
  {
    ImVec2 sourcePosOnScreen = vcGizmo_WorldToScreen(sGizmoContext.mMatrixOrigin, sGizmoContext.camera.matrices.viewProjection);
    ImVec2 destinationPosOnScreen = vcGizmo_WorldToScreen(sGizmoContext.mModel.axis.t, sGizmoContext.camera.matrices.viewProjection);
    udDouble4 dif = { destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y, 0.0, 0.0 };
    dif = udNormalize3(dif) * 5.0;
    drawList->AddCircle(sourcePosOnScreen, 6.f, TranslationLineColor);
    drawList->AddCircle(destinationPosOnScreen, 6.f, TranslationLineColor);
    drawList->AddLine(ImVec2(sourcePosOnScreen.x + (float)dif.x, sourcePosOnScreen.y + (float)dif.y), ImVec2(destinationPosOnScreen.x - (float)dif.x, destinationPosOnScreen.y - (float)dif.y), TranslationLineColor, 2.f);

    char tmps[512];
    udDouble3 deltaInfo = sGizmoContext.mModel.axis.t.toVector3() - sGizmoContext.mMatrixOrigin;
    int componentInfoIndex = (type - vcGMT_MoveX) * 3;

    if (componentInfoIndex >= 0)
    {
      ImFormatString(tmps, sizeof(tmps), sTextInfoMask[type], deltaInfo[sTranslationInfoIndex[componentInfoIndex]], deltaInfo[sTranslationInfoIndex[componentInfoIndex + 1]], deltaInfo[sTranslationInfoIndex[componentInfoIndex + 2]]);
      drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
      drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
    }
  }
}

static bool vcGizmo_CanActivate()
{
  if (ImGui::IsMouseClicked(0))// && !ImGui::IsAnyItemActive())
    return true;
  return false;
}

static int vcGizmo_GetScaleType(const udDouble3 direction[])
{
  int type = vcGMT_None;

  // Check Uniform Scale


  if ((sGizmoContext.allowedControls & vcGAC_ScaleUniform) && ImGui::IsMouseHoveringRect(sGizmoContext.mScreenSquareMin, sGizmoContext.mScreenSquareMax))
    type = vcGMT_ScaleXYZ;

  // Check Non-Uniform scale
  if (sGizmoContext.allowedControls & vcGAC_ScaleNonUniform)
  {
    for (unsigned int i = 0; i < 3 && type == vcGMT_None; i++)
    {
      udDouble3 dirPlaneX, dirPlaneY, dirAxis;
      bool belowAxisLimit, belowPlaneLimit;
      vcGizmo_ComputeTripodAxisAndVisibility(direction, i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
      dirAxis = (sGizmoContext.mModel * udDouble4::create(dirAxis, 0)).toVector3();

      udDouble3 posOnPlane;
      if (!udPlane<double>::create(sGizmoContext.mModel.axis.t.toVector3(), -sGizmoContext.mCameraDir).intersects(sGizmoContext.camera.worldMouseRay, &posOnPlane, nullptr))
        continue;

      const ImVec2 posOnPlanScreen = vcGizmo_WorldToScreen(posOnPlane, sGizmoContext.camera.matrices.viewProjection);
      const ImVec2 axisStartOnScreen = vcGizmo_WorldToScreen(sGizmoContext.mModel.axis.t.toVector3() + dirAxis * sGizmoContext.mScreenFactor * 0.1f, sGizmoContext.camera.matrices.viewProjection);
      const ImVec2 axisEndOnScreen = vcGizmo_WorldToScreen(sGizmoContext.mModel.axis.t.toVector3() + dirAxis * sGizmoContext.mScreenFactor, sGizmoContext.camera.matrices.viewProjection);

      udDouble4 closestPointOnAxis = vcGizmo_PointOnSegment(vcGizmo_MakeVect(posOnPlanScreen), vcGizmo_MakeVect(axisStartOnScreen), vcGizmo_MakeVect(axisEndOnScreen));

      double distSqr = udMagSq3(closestPointOnAxis - vcGizmo_MakeVect(posOnPlanScreen));
      if (distSqr < 12.0 * 12.0) // pixel size
        type = vcGMT_ScaleX + i;
    }
  }
  return type;
}

static int vcGizmo_GetRotateType()
{
  ImGuiIO& io = ImGui::GetIO();
  int type = vcGMT_None;

  if (sGizmoContext.allowedControls & vcGAC_RotationScreen)
  {
    udDouble2 deltaScreen = { io.MousePos.x - sGizmoContext.mScreenSquareCenter.x, io.MousePos.y - sGizmoContext.mScreenSquareCenter.y };
    double dist = udMag(deltaScreen);
    if (dist >= (sGizmoContext.mRadiusSquareCenter - 1.0) && dist < (sGizmoContext.mRadiusSquareCenter + 1.0))
      type = vcGMT_RotateScreen;
  }

  if ((sGizmoContext.allowedControls & vcGAC_RotationAxis) && (type == vcGMT_None))
  {
    const udDouble3 planeNormals[] = { sGizmoContext.mModel.axis.x.toVector3(), sGizmoContext.mModel.axis.y.toVector3(), sGizmoContext.mModel.axis.z.toVector3() };

    for (unsigned int i = 0; i < 3 && type == vcGMT_None; i++)
    {
      // pickup plane
      udPlane<double> pickupPlane = udPlane<double>::create(sGizmoContext.mModel.axis.t.toVector3(), planeNormals[i]);

      udDouble3 localPos;
      if (!pickupPlane.intersects(sGizmoContext.camera.worldMouseRay, &localPos, nullptr))
        continue;
      localPos -= pickupPlane.point;

      if (udDot(sGizmoContext.camera.worldMouseRay.direction, udNormalize3(localPos)) > DBL_EPSILON)
        continue;

      udDouble4 idealPosOnCircle = sGizmoContext.mModelInverse * udDouble4::create(udNormalize3(localPos), 0);
      ImVec2 idealPosOnCircleScreen = vcGizmo_WorldToScreen(idealPosOnCircle * sGizmoContext.mScreenFactor, sGizmoContext.mMVP);

      ImVec2 distanceOnScreen = idealPosOnCircleScreen - io.MousePos;

      if (udMagSq3(vcGizmo_MakeVect(distanceOnScreen)) < 8.0 * 8.0) // pixel size
        type = vcGMT_RotateX + i;
    }
  }

  return type;
}

static int vcGizmo_GetMoveType(const udDouble3 direction[])
{
  int type = vcGMT_None;

  if (sGizmoContext.allowedControls & vcGAC_Translation)
  {
    // screen
    if (ImGui::IsMouseHoveringRect(sGizmoContext.mScreenSquareMin, sGizmoContext.mScreenSquareMax))
      type = vcGMT_MoveScreen;

    // compute
    for (unsigned int i = 0; i < 3 && type == vcGMT_None; i++)
    {
      udDouble3 dirPlaneX, dirPlaneY, dirAxis;
      bool belowAxisLimit, belowPlaneLimit;
      vcGizmo_ComputeTripodAxisAndVisibility(direction, i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
      dirAxis = (sGizmoContext.mModel * udDouble4::create(dirAxis, 0)).toVector3();
      dirPlaneX = (sGizmoContext.mModel * udDouble4::create(dirPlaneX, 0)).toVector3();
      dirPlaneY = (sGizmoContext.mModel * udDouble4::create(dirPlaneY, 0)).toVector3();

      udDouble3 posOnPlane;
      if (udPlane<double>::create(sGizmoContext.mModel.axis.t.toVector3(), -sGizmoContext.mCameraDir).intersects(sGizmoContext.camera.worldMouseRay, &posOnPlane, nullptr))
      {
        const ImVec2 posOnPlaneScreen = vcGizmo_WorldToScreen(posOnPlane, sGizmoContext.camera.matrices.viewProjection);
        const ImVec2 axisStartOnScreen = vcGizmo_WorldToScreen(sGizmoContext.mModel.axis.t.toVector3() + dirAxis * sGizmoContext.mScreenFactor * 0.1f, sGizmoContext.camera.matrices.viewProjection);
        const ImVec2 axisEndOnScreen = vcGizmo_WorldToScreen(sGizmoContext.mModel.axis.t.toVector3() + dirAxis * sGizmoContext.mScreenFactor, sGizmoContext.camera.matrices.viewProjection);

        udDouble4 closestPointOnAxis = vcGizmo_PointOnSegment(vcGizmo_MakeVect(posOnPlaneScreen), vcGizmo_MakeVect(axisStartOnScreen), vcGizmo_MakeVect(axisEndOnScreen));

        if (udMagSq3(closestPointOnAxis - vcGizmo_MakeVect(posOnPlaneScreen)) < 12.0 * 12.0) // pixel size
          type = vcGMT_MoveX + i;
      }

      if (udPlane<double>::create(sGizmoContext.mModel.axis.t.toVector3(), dirAxis).intersects(sGizmoContext.camera.worldMouseRay, &posOnPlane, nullptr))
      {
        const double dx = udDot(dirPlaneX, ((posOnPlane - sGizmoContext.mModel.axis.t.toVector3()) * (1.0 / sGizmoContext.mScreenFactor)));
        const double dy = udDot(dirPlaneY, ((posOnPlane - sGizmoContext.mModel.axis.t.toVector3()) * (1.0 / sGizmoContext.mScreenFactor)));
        if (belowPlaneLimit && dx >= sQuadUV[0] && dx <= sQuadUV[4] && dy >= sQuadUV[1] && dy <= sQuadUV[3])
          type = vcGMT_MoveYZ + i;
      }
    }
  }

  return type;
}

static void vcGizmo_HandleTranslation(const udDouble3 direction[], udDouble4x4 *deltaMatrix, int& type, double snap)
{
  ImGuiIO& io = ImGui::GetIO();
  bool applyRotationLocaly = sGizmoContext.mMode == vcGCS_Local || type == vcGMT_MoveScreen;

  // move
  if (sGizmoContext.mbUsing)
  {
    ImGui::CaptureMouseFromApp();
    udDouble3 newPos;

    if (sGizmoContext.mTranslationPlane.intersects(sGizmoContext.camera.worldMouseRay, &newPos, nullptr))
    {
      // compute delta
      udDouble3 newOrigin = newPos - sGizmoContext.mRelativeOrigin * sGizmoContext.mScreenFactor;
      udDouble3 delta = newOrigin - sGizmoContext.mModel.axis.t.toVector3();

      // 1 axis constraint
      if (sGizmoContext.mCurrentOperation >= vcGMT_MoveX && sGizmoContext.mCurrentOperation <= vcGMT_MoveZ)
      {
        int axisIndex = sGizmoContext.mCurrentOperation - vcGMT_MoveX;
        udDouble3 axisValue = sGizmoContext.mModel.c[axisIndex].toVector3();

        // Remove small deltas
        static const double MoveThreshold = 0.0000001;
        if (udAbs(delta.x) <= MoveThreshold)
          delta.x = 0;       
        if (udAbs(delta.y) <= MoveThreshold)
          delta.y = 0;        
        if (udAbs(delta.z) <= MoveThreshold)
          delta.z = 0;

        double lengthOnAxis = udDot3(axisValue, delta);
        delta = axisValue * lengthOnAxis;
      }

      // snap
      if (snap)
      {
        udDouble3 cumulativeDelta = sGizmoContext.mModel.axis.t.toVector3() + delta - sGizmoContext.mMatrixOrigin;

        if (applyRotationLocaly)
        {
          udDouble4x4 modelSourceNormalized = sGizmoContext.mModel;

          sGizmoContext.mModel.axis.x = udNormalize3(sGizmoContext.mModel.axis.x);
          sGizmoContext.mModel.axis.y = udNormalize3(sGizmoContext.mModel.axis.y);
          sGizmoContext.mModel.axis.z = udNormalize3(sGizmoContext.mModel.axis.z);

          udDouble4x4 modelSourceNormalizedInverse = udInverse(modelSourceNormalized);
          cumulativeDelta = (modelSourceNormalizedInverse * udDouble4::create(cumulativeDelta, 0)).toVector3();
          vcGizmo_ComputeSnap(cumulativeDelta, snap);
          cumulativeDelta = (modelSourceNormalized * udDouble4::create(cumulativeDelta, 0)).toVector3();
        }
        else
        {
          vcGizmo_ComputeSnap(cumulativeDelta, snap);
        }

        delta = sGizmoContext.mMatrixOrigin + cumulativeDelta - sGizmoContext.mModel.axis.t.toVector3();
      }

      udDouble4x4 deltaMatrixTranslation = udDouble4x4::translation(delta);
      if (deltaMatrix)
        *deltaMatrix = deltaMatrixTranslation;

      if (!io.MouseDown[0])
        sGizmoContext.mbUsing = false;

      type = sGizmoContext.mCurrentOperation;
    }
  }
  else
  {
    // find new possible way to move
    type = vcGizmo_GetMoveType(direction);
    if (type != vcGMT_None)
    {
      ImGui::CaptureMouseFromApp();
    }

    if (vcGizmo_CanActivate() && type != vcGMT_None)
    {
      sGizmoContext.mbUsing = true;
      sGizmoContext.mCurrentOperation = type;
      udDouble3 movePlanNormal[] = { sGizmoContext.mModel.axis.x.toVector3(), sGizmoContext.mModel.axis.y.toVector3(), sGizmoContext.mModel.axis.z.toVector3(), sGizmoContext.mModel.axis.x.toVector3(), sGizmoContext.mModel.axis.y.toVector3(), sGizmoContext.mModel.axis.z.toVector3(), -sGizmoContext.mCameraDir };

      udDouble3 cameraToModelNormalized = udNormalize(sGizmoContext.mModel.axis.t.toVector3() - sGizmoContext.camera.position);
      for (unsigned int i = 0; i < 3; i++)
      {
        movePlanNormal[i] = udNormalize(udCross(udCross(cameraToModelNormalized, movePlanNormal[i]), movePlanNormal[i]));
      }

      // pickup plan
      sGizmoContext.mTranslationPlane = udPlane<double>::create(sGizmoContext.mModel.axis.t.toVector3(), movePlanNormal[type - vcGMT_MoveX]);
      if (sGizmoContext.mTranslationPlane.intersects(sGizmoContext.camera.worldMouseRay, &sGizmoContext.mTranslationPlaneOrigin, nullptr))
      {
        sGizmoContext.mMatrixOrigin = sGizmoContext.mModel.axis.t.toVector3();
        sGizmoContext.mRelativeOrigin = (sGizmoContext.mTranslationPlaneOrigin - sGizmoContext.mModel.axis.t.toVector3()) * (1.0 / sGizmoContext.mScreenFactor);
      }
    }
  }
}

static void vcGizmo_HandleScale(const udDouble3 direction[], udDouble4x4 *pDeltaMatrix, int& type, double snap)
{
  ImGuiIO& io = ImGui::GetIO();

  if (!sGizmoContext.mbUsing)
  {
    // find new possible way to scale
    type = vcGizmo_GetScaleType(direction);
    if (type != vcGMT_None)
      ImGui::CaptureMouseFromApp();

    if (vcGizmo_CanActivate() && type != vcGMT_None)
    {
      sGizmoContext.mbUsing = true;
      sGizmoContext.mCurrentOperation = type;
      sGizmoContext.mLastScaleDelta = udDouble3::zero();
      const udDouble3 movePlanNormal[] = { sGizmoContext.mModel.axis.y.toVector3(), sGizmoContext.mModel.axis.z.toVector3(), sGizmoContext.mModel.axis.x.toVector3(), sGizmoContext.mModel.axis.z.toVector3(), sGizmoContext.mModel.axis.y.toVector3(), sGizmoContext.mModel.axis.x.toVector3(), -sGizmoContext.mCameraDir };

      sGizmoContext.mTranslationPlane = udPlane<double>::create(sGizmoContext.mModel.axis.t.toVector3(), movePlanNormal[type - vcGMT_ScaleX]);
      if (sGizmoContext.mTranslationPlane.intersects(sGizmoContext.camera.worldMouseRay, &sGizmoContext.mTranslationPlaneOrigin, nullptr))
      {
        sGizmoContext.mMatrixOrigin = sGizmoContext.mModel.axis.t.toVector3();
        sGizmoContext.mScale = udDouble3::one();
        sGizmoContext.mRelativeOrigin = (sGizmoContext.mTranslationPlaneOrigin - sGizmoContext.mModel.axis.t.toVector3()) * (1.0 / sGizmoContext.mScreenFactor);
        sGizmoContext.mSaveMousePosx = io.MousePos.x;
        sGizmoContext.mLastMousePosx = io.MousePos.x;
      }
    }
  }

  // scale
  if (sGizmoContext.mbUsing)
  {
    ImGui::CaptureMouseFromApp();
    udDouble3 newPos;
    if (sGizmoContext.mTranslationPlane.intersects(sGizmoContext.camera.worldMouseRay, &newPos, nullptr))
    {
      udDouble3 newOrigin = newPos - sGizmoContext.mRelativeOrigin * sGizmoContext.mScreenFactor;
      udDouble3 delta = newOrigin - sGizmoContext.mModel.axis.t.toVector3();

      double scaleDelta = 0.0;
      int axisIndex = 0;
      bool uniformScaling = false;

      // 1 axis constraint
      if (sGizmoContext.mCurrentOperation >= vcGMT_ScaleX && sGizmoContext.mCurrentOperation <= vcGMT_ScaleZ)
      {
        axisIndex = sGizmoContext.mCurrentOperation - vcGMT_ScaleX;
        udDouble3 axisValue = sGizmoContext.mModel.c[axisIndex].toVector3();
        double lengthOnAxis = udDot(axisValue, delta);
        delta = axisValue * lengthOnAxis;

        udDouble3 baseVector = sGizmoContext.mTranslationPlaneOrigin - sGizmoContext.mModel.axis.t.toVector3();
        scaleDelta = udDot(axisValue, baseVector + delta) / udDot(axisValue, baseVector) - 1.0;
      }
      else
      {
        uniformScaling = true;
        scaleDelta = (io.MousePos.x - sGizmoContext.mSaveMousePosx)  * 0.01;
      }

      scaleDelta = udMax(-0.99, scaleDelta);
      vcGizmo_ComputeSnap(&scaleDelta, snap);
      scaleDelta = udMax((snap - 1.0), scaleDelta);

      sGizmoContext.mScaleValueOrigin[axisIndex] = 1.0 + scaleDelta;

      double inverseLastScaleDelta = 100.0; // 0.01 is our minimum scale, 100 == (1.0 / 0.01)
      if (sGizmoContext.mLastScaleDelta[axisIndex] != -1.0)
        inverseLastScaleDelta = (1.0 / (1.0 + sGizmoContext.mLastScaleDelta[axisIndex]));

      sGizmoContext.mLastScaleDelta[axisIndex] = scaleDelta;
      sGizmoContext.mScale[axisIndex] = (1.0 + scaleDelta) * inverseLastScaleDelta;

      if (uniformScaling)
      {
        // spread to all 3 axis
        sGizmoContext.mScaleValueOrigin = udDouble3::create(1.0 + scaleDelta);
        sGizmoContext.mLastScaleDelta = udDouble3::create(scaleDelta);
        sGizmoContext.mScale = udDouble3::create(sGizmoContext.mScale[axisIndex]);
      }

      // compute matrix & delta
      udDouble4x4 deltaMatrixScale = sGizmoContext.mModel * udDouble4x4::scaleNonUniform(sGizmoContext.mScale) * udInverse(sGizmoContext.mModel);

      if (pDeltaMatrix)
        *pDeltaMatrix = deltaMatrixScale;

      if (!io.MouseDown[0])
        sGizmoContext.mbUsing = false;

      type = sGizmoContext.mCurrentOperation;
      sGizmoContext.mLastMousePosx = io.MousePos.x;
    }
  }
}

static void vcGizmo_HandleRotation(const udDouble3 direction[], udDouble4x4 *deltaMatrix, int& type, double snap)
{
  ImGuiIO& io = ImGui::GetIO();
  bool applyRotationLocally = sGizmoContext.mMode == vcGCS_Local;

  if (!sGizmoContext.mbUsing)
  {
    type = vcGizmo_GetRotateType();

    if (type != vcGMT_None)
      ImGui::CaptureMouseFromApp();

    if (type == vcGMT_RotateScreen)
      applyRotationLocally = true;

    if (vcGizmo_CanActivate() && type != vcGMT_None)
    {
      sGizmoContext.mbUsing = true;
      sGizmoContext.mCurrentOperation = type;
      const udDouble3 rotatePlaneNormal[] = { sGizmoContext.mModel.axis.x.toVector3(), sGizmoContext.mModel.axis.y.toVector3(), sGizmoContext.mModel.axis.z.toVector3(), -sGizmoContext.mCameraDir };

      if (applyRotationLocally)
        sGizmoContext.mTranslationPlane = udPlane<double>::create(sGizmoContext.mModel.axis.t.toVector3(), rotatePlaneNormal[type - vcGMT_RotateX]);
      else
        sGizmoContext.mTranslationPlane = udPlane<double>::create(sGizmoContext.mModel.axis.t.toVector3(), direction[type - vcGMT_RotateX]);

      if (sGizmoContext.mTranslationPlane.intersects(sGizmoContext.camera.worldMouseRay, &sGizmoContext.mRotationVectorSource, nullptr))
      {
        sGizmoContext.mRotationVectorSource = udNormalize(sGizmoContext.mRotationVectorSource - sGizmoContext.mTranslationPlane.point);
        sGizmoContext.mRotationAngleOrigin = vcGizmo_ComputeAngleOnPlane();
      }
    }
  }

  // rotation
  if (sGizmoContext.mbUsing)
  {
    ImGui::CaptureMouseFromApp();
    sGizmoContext.mRotationAngle = vcGizmo_ComputeAngleOnPlane();

    if (snap)
      vcGizmo_ComputeSnap(&sGizmoContext.mRotationAngle, UD_DEG2RAD(snap));

    udDouble4x4 deltaRotation = udDouble4x4::translation(sGizmoContext.mModel.axis.t.toVector3()) * udDouble4x4::rotationAxis(sGizmoContext.mTranslationPlane.normal, sGizmoContext.mRotationAngle - sGizmoContext.mRotationAngleOrigin) * udDouble4x4::translation(-sGizmoContext.mModel.axis.t.toVector3());
    sGizmoContext.mRotationAngleOrigin = sGizmoContext.mRotationAngle;

    if (deltaMatrix)
      *deltaMatrix = deltaRotation;

    if (!io.MouseDown[0])
      sGizmoContext.mbUsing = false;

    type = sGizmoContext.mCurrentOperation;
  }
}

void vcGizmo_Manipulate(const vcCamera *pCamera, const udDouble3 direction[], vcGizmoOperation operation, vcGizmoCoordinateSystem mode, const udDouble4x4 &matrix, udDouble4x4 *pDeltaMatrix, vcGizmoAllowedControls allowedControls, double snap /*= 0.0*/)
{
  if (operation == vcGO_Scale)
    mode = vcGCS_Local;

  vcGizmo_ComputeContext(pCamera, matrix, mode, allowedControls);

  if (pDeltaMatrix)
    *pDeltaMatrix = udDouble4x4::identity();

  // behind camera
  udDouble4 camSpacePosition = sGizmoContext.mMVP.axis.t;
  if (camSpacePosition.z < -0.001f)
  {
    sGizmoContext.mbUsing = false;
    return;
  }

  int type = vcGMT_None;
  switch (operation)
  {
  case vcGO_Rotate:
    vcGizmo_HandleRotation(direction, pDeltaMatrix, type, snap);
    vcGizmo_DrawRotationGizmo(type);
    break;
  case vcGO_Translate:
    vcGizmo_HandleTranslation(direction, pDeltaMatrix, type, snap);
    vcGizmo_DrawTranslationGizmo(direction, type);
    break;
  case vcGO_Scale:
    vcGizmo_HandleScale(direction, pDeltaMatrix, type, snap);
    vcGizmo_DrawScaleGizmo(direction, type);
    break;
  case vcGO_NoGizmo:
    break;
  }
}

void vcGizmo_ResetState()
{
  sGizmoContext.mbUsing = false;
}
