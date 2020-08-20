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

void vcGizmo_Create(vcGizmoContext **ppContext)
{
  *ppContext = udAllocType(vcGizmoContext, 1, udAF_Zero);
}

void vcGizmo_Destroy(vcGizmoContext **ppContext)
{
  udFree(*ppContext);
  *ppContext = nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
int vcGizmo_GetMoveType(vcGizmoContext *pContext, const udDouble3 direction[]);
int vcGizmo_GetRotateType(vcGizmoContext *pContext);
int vcGizmo_GetScaleType(vcGizmoContext *pContext, const udDouble3 direction[]);

ImVec2 vcGizmo_WorldToScreen(vcGizmoContext *pContext, const udDouble3& worldPos, const udDouble4x4& mat)
{
  udDouble4 trans = mat * udDouble4::create(worldPos, 1);
  if (trans.w != 0)
    trans *= 0.5f / trans.w;
  trans += udDouble4::create(0.5, 0.5, 0.0, 0.0);
#if !GRAPHICS_API_OPENGL
  trans.y = 1.0 - trans.y;
#endif
  trans.x *= pContext->mWidth;
  trans.y *= pContext->mHeight;
  trans.x += pContext->mX;
  trans.y += pContext->mY;
  return ImVec2((float)trans.x, (float)trans.y);
}

ImVec2 vcGizmo_WorldToScreen(vcGizmoContext *pContext, const udDouble4& worldPos, const udDouble4x4& mat)
{
  return vcGizmo_WorldToScreen(pContext, worldPos.toVector3(), mat);
}

double vcGizmo_GetSegmentLengthClipSpace(vcGizmoContext *pContext, const udDouble3& start, const udDouble3& end)
{
  udDouble4 startOfSegment = pContext->mMVP * udDouble4::create(start, 1);
  if (startOfSegment.w != 0.0) // check for axis aligned with camera direction
    startOfSegment /= startOfSegment.w;

  udDouble4 endOfSegment = pContext->mMVP * udDouble4::create(end, 1);
  if (endOfSegment.w != 0.0) // check for axis aligned with camera direction
    endOfSegment /= endOfSegment.w;

  udDouble4 clipSpaceAxis = endOfSegment - startOfSegment;
  clipSpaceAxis.y /= pContext->mDisplayRatio;

  return udSqrt(clipSpaceAxis.x*clipSpaceAxis.x + clipSpaceAxis.y*clipSpaceAxis.y);
}

double vcGizmo_GetParallelogram(vcGizmoContext *pContext, const udDouble3& ptO, const udDouble3& ptA, const udDouble3& ptB)
{
  udDouble4 pts[] = { udDouble4::create(ptO, 1), udDouble4::create(ptA, 1), udDouble4::create(ptB, 1) };
  for (unsigned int i = 0; i < 3; i++)
  {
    pts[i] = pContext->mMVP * pts[i];
    if (udAbs(pts[i].w) > DBL_EPSILON) // check for axis aligned with camera direction
      pts[i] *= 1.0 / pts[i].w;
  }
  udDouble3 segA = (pts[1] - pts[0]).toVector3();
  udDouble3 segB = (pts[2] - pts[0]).toVector3();
  segA.y /= pContext->mDisplayRatio;
  segB.y /= pContext->mDisplayRatio;
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

void vcGizmo_SetRect(vcGizmoContext *pContext, float x, float y, float width, float height)
{
  pContext->mX = x;
  pContext->mY = y;
  pContext->mWidth = width;
  pContext->mHeight = height;
  pContext->mXMax = pContext->mX + pContext->mWidth;
  pContext->mYMax = pContext->mY + pContext->mXMax;
  pContext->mDisplayRatio = width / height;
}

void vcGizmo_SetDrawList(vcGizmoContext *pContext)
{
  pContext->ImGuiDrawList = ImGui::GetForegroundDrawList();
}

void vcGizmo_BeginFrame(vcGizmoContext *pContext)
{
  ImGuiIO& io = ImGui::GetIO();

  const ImU32 flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
  ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::SetNextWindowPos(ImVec2(0, 0));

  ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
  ImGui::PushStyleColor(ImGuiCol_Border, 0);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

  ImGui::Begin("gizmo", NULL, flags);
  pContext->ImGuiDrawList = ImGui::GetForegroundDrawList();
  ImGui::End();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor(2);
}

bool vcGizmo_IsActive(vcGizmoContext *pContext)
{
  return pContext->mbUsing;
}

bool vcGizmo_IsHovered(vcGizmoContext *pContext, const udDouble3 direction[])
{
  return (vcGizmo_GetMoveType(pContext, direction) != vcGMT_None || vcGizmo_GetRotateType(pContext) != vcGMT_None || vcGizmo_GetScaleType(pContext, direction) != vcGMT_None || vcGizmo_IsActive(pContext));
}

void vcGizmo_ComputeContext(vcGizmoContext *pContext, const vcCamera *pCamera, const udDouble4x4 &matrix, vcGizmoCoordinateSystem mode, vcGizmoAllowedControls allowedControls)
{
  pContext->mMode = mode;
  pContext->allowedControls = allowedControls;

  pContext->camera = *pCamera;

  if (mode == vcGCS_Local)
  {
    pContext->mModel = matrix;

    pContext->mModel.axis.x = udNormalize3(pContext->mModel.axis.x);
    pContext->mModel.axis.y = udNormalize3(pContext->mModel.axis.y);
    pContext->mModel.axis.z = udNormalize3(pContext->mModel.axis.z);
  }
  else
  {
    pContext->mModel = udDouble4x4::translation(matrix.axis.t.toVector3());
  }

  pContext->mModelScaleOrigin = udDouble3::create(udMag3(matrix.axis.x), udMag3(matrix.axis.y), udMag3(matrix.axis.z));

  pContext->mModelInverse = udInverse(pContext->mModel);
  pContext->mMVP = pContext->camera.matrices.viewProjection * pContext->mModel;

  pContext->mCameraDir = udDirectionFromYPR(udDouble3::create(pContext->camera.headingPitch, 0.0)); //TODO: Fix me in ECEF

  udDouble3 rightViewInverse = (pContext->mModelInverse * udDouble4::create(pContext->camera.matrices.camera.axis.x.toVector3(), 0)).toVector3();
  double rightLength = vcGizmo_GetSegmentLengthClipSpace(pContext, udDouble3::zero(), rightViewInverse);
  pContext->mScreenFactor = gGizmoSizeClipSpace / rightLength;

  ImVec2 centerSSpace = vcGizmo_WorldToScreen(pContext, udDouble3::zero(), pContext->mMVP);
  pContext->mScreenSquareCenter = centerSSpace;
  pContext->mScreenSquareMin = ImVec2(centerSSpace.x - 10.f, centerSSpace.y - 10.f);
  pContext->mScreenSquareMax = ImVec2(centerSSpace.x + 10.f, centerSSpace.y + 10.f);
}

void vcGizmo_ComputeColors(vcGizmoContext *pContext, ImU32 *colors, size_t numColors, int type, vcGizmoOperation operation)
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
    if (pContext->allowedControls & vcGAC_Translation)
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
    if (pContext->allowedControls & vcGAC_RotationScreen)
    {
      colors[0] = (type == vcGMT_RotateScreen) ? SelectionColor : WhiteColor;
    }

    if (pContext->allowedControls & vcGAC_RotationAxis)
    {
      for (int i = 0; i < 3; i++)
        colors[i + 1] = (type == (int)(vcGMT_RotateX + i)) ? SelectionColor : DirectionColor[i];
    }
    break;
  case vcGO_Scale:
    if (pContext->allowedControls & vcGAC_ScaleUniform)
      colors[0] = (type == vcGMT_ScaleXYZ) ? SelectionColor : WhiteColor;

    if (pContext->allowedControls & vcGAC_ScaleNonUniform)
    {
      for (int i = 0; i < 3; i++)
        colors[i + 1] = (type == (int)(vcGMT_ScaleX + i)) ? SelectionColor : DirectionColor[i];
    }
    break;
  case vcGO_NoGizmo:
    break;
  }
}

void vcGizmo_ComputeTripodAxisAndVisibility(vcGizmoContext *pContext, const udDouble3 direction[], int axisIndex, udDouble3& dirAxis, udDouble3& dirPlaneX, udDouble3& dirPlaneY, bool& belowAxisLimit, bool& belowPlaneLimit)
{
  dirAxis = direction[axisIndex];
  dirPlaneX = direction[(axisIndex + 1) % 3];
  dirPlaneY = direction[(axisIndex + 2) % 3];

  if (pContext->mbUsing)
  {
    // when using, use stored factors so the gizmo doesn't flip when we translate
    belowAxisLimit = pContext->mBelowAxisLimit[axisIndex];
    belowPlaneLimit = pContext->mBelowPlaneLimit[axisIndex];

    dirAxis *= pContext->mAxisFactor[axisIndex];
    dirPlaneX *= pContext->mAxisFactor[(axisIndex + 1) % 3];
    dirPlaneY *= pContext->mAxisFactor[(axisIndex + 2) % 3];
  }
  else
  {
    // new method
    double lenDir = vcGizmo_GetSegmentLengthClipSpace(pContext, udDouble3::zero(), dirAxis);
    double lenDirMinus = vcGizmo_GetSegmentLengthClipSpace(pContext, udDouble3::zero(), -dirAxis);

    double lenDirPlaneX = vcGizmo_GetSegmentLengthClipSpace(pContext, udDouble3::zero(), dirPlaneX);
    double lenDirMinusPlaneX = vcGizmo_GetSegmentLengthClipSpace(pContext, udDouble3::zero(), -dirPlaneX);

    double lenDirPlaneY = vcGizmo_GetSegmentLengthClipSpace(pContext, udDouble3::zero(), dirPlaneY);
    double lenDirMinusPlaneY = vcGizmo_GetSegmentLengthClipSpace(pContext, udDouble3::zero(), -dirPlaneY);

    double mulAxis = (lenDir < lenDirMinus && udAbs(lenDir - lenDirMinus) > FLT_EPSILON) ? -1.0 : 1.0;
    double mulAxisX = (lenDirPlaneX < lenDirMinusPlaneX && udAbs(lenDirPlaneX - lenDirMinusPlaneX) > FLT_EPSILON) ? -1.0 : 1.0;
    double mulAxisY = (lenDirPlaneY < lenDirMinusPlaneY && udAbs(lenDirPlaneY - lenDirMinusPlaneY) > FLT_EPSILON) ? -1.0 : 1.0;
    dirAxis *= mulAxis;
    dirPlaneX *= mulAxisX;
    dirPlaneY *= mulAxisY;

    // for axis
    double axisLengthInClipSpace = vcGizmo_GetSegmentLengthClipSpace(pContext, udDouble3::zero(), dirAxis * pContext->mScreenFactor);

    double paraSurf = vcGizmo_GetParallelogram(pContext, udDouble3::zero(), dirPlaneX * pContext->mScreenFactor, dirPlaneY * pContext->mScreenFactor);
    belowPlaneLimit = (paraSurf > 0.0025f);
    belowAxisLimit = (axisLengthInClipSpace > 0.02f);

    // and store values
    pContext->mAxisFactor[axisIndex] = mulAxis;
    pContext->mAxisFactor[(axisIndex + 1) % 3] = mulAxisX;
    pContext->mAxisFactor[(axisIndex + 2) % 3] = mulAxisY;
    pContext->mBelowAxisLimit[axisIndex] = belowAxisLimit;
    pContext->mBelowPlaneLimit[axisIndex] = belowPlaneLimit;
  }
}

void vcGizmo_ComputeSnap(double* value, double snap)
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

void vcGizmo_ComputeSnap(udDouble3& value, double snap)
{
  for (int i = 0; i < 3; i++)
  {
    vcGizmo_ComputeSnap(&value[i], snap);
  }
}

double vcGizmo_ComputeAngleOnPlane(vcGizmoContext *pContext)
{
  udDouble3 localPos;

  if (!pContext->mTranslationPlane.intersects(pContext->camera.worldMouseRay, &localPos, nullptr))
    return 0;

  localPos = udNormalize(localPos - pContext->mTranslationPlane.point);

  udDouble3 perpendicularVector = udNormalize(udCross(pContext->mRotationVectorSource, pContext->mTranslationPlane.normal));
  double acosAngle = udClamp(udDot(localPos, pContext->mRotationVectorSource), -1 + DBL_EPSILON, 1 - DBL_EPSILON);
  return udACos(acosAngle) * ((udDot(localPos, perpendicularVector) < 0.0) ? 1.0 : -1.0);
}

void vcGizmo_DrawRotationGizmo(vcGizmoContext *pContext, int type)
{
  ImDrawList* drawList = pContext->ImGuiDrawList;

  // colors
  ImU32 colors[7];
  vcGizmo_ComputeColors(pContext, colors, udLengthOf(colors), type, vcGO_Rotate);

  udDouble4 cameraToModelNormalized = pContext->mModelInverse * udDouble4::create(udNormalize(pContext->mModel.axis.t.toVector3() - pContext->camera.position), 0);
  pContext->mRadiusSquareCenter = gScreenRotateSize * pContext->mHeight;

  for (int axis = 0; axis < 3; axis++)
  {
    ImVec2 circlePos[sHalfCircleSegmentCount];
    double angleStart = udATan2(cameraToModelNormalized[(4 - axis) % 3], cameraToModelNormalized[(3 - axis) % 3]) + UD_PI * 0.5f;

    for (unsigned int i = 0; i < sHalfCircleSegmentCount; i++)
    {
      double ng = angleStart + UD_PI * ((float)i / (float)sHalfCircleSegmentCount);
      udDouble3 axisPos = udDouble3::create(udCos(ng), udSin(ng), 0.0);
      udDouble3 pos = udDouble3::create(axisPos[axis], axisPos[(axis + 1) % 3], axisPos[(axis + 2) % 3]) * pContext->mScreenFactor;
      circlePos[i] = vcGizmo_WorldToScreen(pContext, pos, pContext->mMVP);
    }

    float radiusAxis = udSqrt((ImLengthSqr(vcGizmo_WorldToScreen(pContext, pContext->mModel.axis.t, pContext->camera.matrices.viewProjection) - circlePos[0])));
    if (radiusAxis > pContext->mRadiusSquareCenter)
      pContext->mRadiusSquareCenter = radiusAxis;

    drawList->AddPolyline(circlePos, sHalfCircleSegmentCount, colors[3 - axis], false, 2);
  }

  drawList->AddCircle(vcGizmo_WorldToScreen(pContext, pContext->mModel.axis.t, pContext->camera.matrices.viewProjection), (float)pContext->mRadiusSquareCenter, colors[0], 64, 3.0);

  if (pContext->mbUsing)
  {
    ImVec2 circlePos[sHalfCircleSegmentCount + 1];

    circlePos[0] = vcGizmo_WorldToScreen(pContext, pContext->mModel.axis.t, pContext->camera.matrices.viewProjection);

    for (unsigned int i = 1; i < sHalfCircleSegmentCount; i++)
    {
      double ng = pContext->mRotationAngle * ((float)(i - 1) / (float)(sHalfCircleSegmentCount - 1));
      udDouble4x4 rotateVectorMatrix = udDouble4x4::rotationAxis(pContext->mTranslationPlane.normal, ng);
      udDouble4 pos = rotateVectorMatrix * udDouble4::create(pContext->mRotationVectorSource, 1) * pContext->mScreenFactor;
      circlePos[i] = vcGizmo_WorldToScreen(pContext, pos + pContext->mModel.axis.t, pContext->camera.matrices.viewProjection);
    }
    drawList->AddConvexPolyFilled(circlePos, sHalfCircleSegmentCount, 0x801080FF);
    drawList->AddPolyline(circlePos, sHalfCircleSegmentCount, 0xFF1080FF, true, 2);

    ImVec2 destinationPosOnScreen = circlePos[1];
    char tmps[512];
    ImFormatString(tmps, sizeof(tmps), sTextInfoMask[type], UD_RAD2DEG(pContext->mRotationAngle), pContext->mRotationAngle);
    drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
    drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
  }
}

void vcGizmo_DrawHatchedAxis(vcGizmoContext *pContext, const udDouble3& axis)
{
  for (int j = 1; j < 10; j++)
  {
    ImVec2 baseSSpace2 = vcGizmo_WorldToScreen(pContext, axis * 0.05f * (float)(j * 2) * pContext->mScreenFactor, pContext->mMVP);
    ImVec2 worldDirSSpace2 = vcGizmo_WorldToScreen(pContext, axis * 0.05f * (float)(j * 2 + 1) * pContext->mScreenFactor, pContext->mMVP);
    pContext->ImGuiDrawList->AddLine(baseSSpace2, worldDirSSpace2, 0x80000000, 6.f);
  }
}

void vcGizmo_DrawScaleGizmo(vcGizmoContext *pContext, const udDouble3 direction[], int type)
{
  ImDrawList* drawList = pContext->ImGuiDrawList;

  // colors
  ImU32 colors[7];
  vcGizmo_ComputeColors(pContext, colors, udLengthOf(colors), type, vcGO_Scale);

  // draw
  udDouble3 scaleDisplay = { 1.0, 1.0, 1.0 };

  if (pContext->mbUsing)
    scaleDisplay = pContext->mScaleValueOrigin;

  for (unsigned int i = 0; i < 3; i++)
  {
    udDouble3 dirPlaneX, dirPlaneY, dirAxis;
    bool belowAxisLimit, belowPlaneLimit;
    vcGizmo_ComputeTripodAxisAndVisibility(pContext, direction, i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

    // draw axis
    if (belowAxisLimit)
    {
      ImVec2 baseSSpace = vcGizmo_WorldToScreen(pContext, dirAxis * 0.1f * pContext->mScreenFactor, pContext->mMVP);
      ImVec2 worldDirSSpaceNoScale = vcGizmo_WorldToScreen(pContext, dirAxis * pContext->mScreenFactor, pContext->mMVP);
      ImVec2 worldDirSSpace = vcGizmo_WorldToScreen(pContext, (dirAxis * scaleDisplay[i]) * pContext->mScreenFactor, pContext->mMVP);

      if (pContext->mbUsing)
      {
        drawList->AddLine(baseSSpace, worldDirSSpaceNoScale, 0xFF404040, 3.f);
        drawList->AddCircleFilled(worldDirSSpaceNoScale, 6.f, 0xFF404040);
      }

      drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], 3.f);
      drawList->AddCircleFilled(worldDirSSpace, 6.f, colors[i + 1]);

      if (pContext->mAxisFactor[i] < 0.0)
        vcGizmo_DrawHatchedAxis(pContext, dirAxis * scaleDisplay[i]);
    }
  }

  // draw screen cirle
  drawList->AddCircleFilled(pContext->mScreenSquareCenter, 6.f, colors[0], 32);

  if (pContext->mbUsing && type != vcGMT_None)
  {
    ImVec2 destinationPosOnScreen = vcGizmo_WorldToScreen(pContext, pContext->mModel.axis.t, pContext->camera.matrices.viewProjection);

    char tmps[512];
    int componentInfoIndex = (type - vcGMT_ScaleX) * 3;
    ImFormatString(tmps, sizeof(tmps), sTextInfoMask[type], scaleDisplay[sTranslationInfoIndex[componentInfoIndex]]);
    drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
    drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
  }
}


void vcGizmo_DrawTranslationGizmo(vcGizmoContext *pContext, const udDouble3 direction[], int type)
{
  ImDrawList* drawList = pContext->ImGuiDrawList;
  if (!drawList)
    return;

  drawList->PushClipRect(ImVec2(pContext->mX, pContext->mY), ImVec2(pContext->mX + pContext->mWidth, pContext->mY + pContext->mHeight), false);

  // colors
  ImU32 colors[7];
  vcGizmo_ComputeColors(pContext, colors, udLengthOf(colors), type, vcGO_Translate);

  const ImVec2 origin = vcGizmo_WorldToScreen(pContext, pContext->mModel.axis.t, pContext->camera.matrices.viewProjection);

  // draw
  bool belowAxisLimit = false;
  bool belowPlaneLimit = false;
  for (unsigned int i = 0; i < 3; ++i)
  {
    udDouble3 dirPlaneX, dirPlaneY, dirAxis;
    vcGizmo_ComputeTripodAxisAndVisibility(pContext, direction, i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

    // draw axis
    if (belowAxisLimit)
    {
      ImVec2 baseSSpace = vcGizmo_WorldToScreen(pContext, dirAxis * 0.1f * pContext->mScreenFactor, pContext->mMVP);
      ImVec2 worldDirSSpace = vcGizmo_WorldToScreen(pContext, dirAxis * pContext->mScreenFactor, pContext->mMVP);

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

      if (pContext->mAxisFactor[i] < 0.0)
        vcGizmo_DrawHatchedAxis(pContext, dirAxis);
    }

    // draw plane
    if (belowPlaneLimit)
    {
      ImVec2 screenQuadPts[4];
      for (int j = 0; j < 4; ++j)
      {
        udDouble3 cornerWorldPos = (dirPlaneX * sQuadUV[j * 2] + dirPlaneY  * sQuadUV[j * 2 + 1]) * pContext->mScreenFactor;
        screenQuadPts[j] = vcGizmo_WorldToScreen(pContext, cornerWorldPos, pContext->mMVP);
      }
      drawList->AddPolyline(screenQuadPts, 4, DirectionColor[i], true, 1.0f);
      drawList->AddConvexPolyFilled(screenQuadPts, 4, colors[i + 4]);
    }
  }

  drawList->AddCircleFilled(pContext->mScreenSquareCenter, 6.f, colors[0], 32);

  if (pContext->mbUsing)
  {
    ImVec2 sourcePosOnScreen = vcGizmo_WorldToScreen(pContext, pContext->mMatrixOrigin, pContext->camera.matrices.viewProjection);
    ImVec2 destinationPosOnScreen = vcGizmo_WorldToScreen(pContext, pContext->mModel.axis.t, pContext->camera.matrices.viewProjection);
    udDouble4 dif = { destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y, 0.0, 0.0 };
    dif = udNormalize3(dif) * 5.0;
    drawList->AddCircle(sourcePosOnScreen, 6.f, TranslationLineColor);
    drawList->AddCircle(destinationPosOnScreen, 6.f, TranslationLineColor);
    drawList->AddLine(ImVec2(sourcePosOnScreen.x + (float)dif.x, sourcePosOnScreen.y + (float)dif.y), ImVec2(destinationPosOnScreen.x - (float)dif.x, destinationPosOnScreen.y - (float)dif.y), TranslationLineColor, 2.f);

    char tmps[512];
    udDouble3 deltaInfo = pContext->mModel.axis.t.toVector3() - pContext->mMatrixOrigin;
    int componentInfoIndex = (type - vcGMT_MoveX) * 3;

    if (componentInfoIndex >= 0)
    {
      ImFormatString(tmps, sizeof(tmps), sTextInfoMask[type], deltaInfo[sTranslationInfoIndex[componentInfoIndex]], deltaInfo[sTranslationInfoIndex[componentInfoIndex + 1]], deltaInfo[sTranslationInfoIndex[componentInfoIndex + 2]]);
      drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
      drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
    }
  }

  drawList->PopClipRect();
}

bool vcGizmo_CanActivate()
{
  if (ImGui::IsMouseClicked(0))// && !ImGui::IsAnyItemActive())
    return true;
  return false;
}

int vcGizmo_GetScaleType(vcGizmoContext *pContext, const udDouble3 direction[])
{
  int type = vcGMT_None;

  // Check Uniform Scale


  if ((pContext->allowedControls & vcGAC_ScaleUniform) && ImGui::IsMouseHoveringRect(pContext->mScreenSquareMin, pContext->mScreenSquareMax))
    type = vcGMT_ScaleXYZ;

  // Check Non-Uniform scale
  if (pContext->allowedControls & vcGAC_ScaleNonUniform)
  {
    for (unsigned int i = 0; i < 3 && type == vcGMT_None; i++)
    {
      udDouble3 dirPlaneX, dirPlaneY, dirAxis;
      bool belowAxisLimit, belowPlaneLimit;
      vcGizmo_ComputeTripodAxisAndVisibility(pContext, direction, i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
      dirAxis = (pContext->mModel * udDouble4::create(dirAxis, 0)).toVector3();

      udDouble3 posOnPlane;
      if (!udPlane<double>::create(pContext->mModel.axis.t.toVector3(), -pContext->mCameraDir).intersects(pContext->camera.worldMouseRay, &posOnPlane, nullptr))
        continue;

      const ImVec2 posOnPlanScreen = vcGizmo_WorldToScreen(pContext, posOnPlane, pContext->camera.matrices.viewProjection);
      const ImVec2 axisStartOnScreen = vcGizmo_WorldToScreen(pContext, pContext->mModel.axis.t.toVector3() + dirAxis * pContext->mScreenFactor * 0.1f, pContext->camera.matrices.viewProjection);
      const ImVec2 axisEndOnScreen = vcGizmo_WorldToScreen(pContext, pContext->mModel.axis.t.toVector3() + dirAxis * pContext->mScreenFactor, pContext->camera.matrices.viewProjection);

      udDouble4 closestPointOnAxis = vcGizmo_PointOnSegment(vcGizmo_MakeVect(posOnPlanScreen), vcGizmo_MakeVect(axisStartOnScreen), vcGizmo_MakeVect(axisEndOnScreen));

      double distSqr = udMagSq3(closestPointOnAxis - vcGizmo_MakeVect(posOnPlanScreen));
      if (distSqr < 12.0 * 12.0) // pixel size
        type = vcGMT_ScaleX + i;
    }
  }
  return type;
}

int vcGizmo_GetRotateType(vcGizmoContext *pContext)
{
  ImGuiIO& io = ImGui::GetIO();
  int type = vcGMT_None;

  if (pContext->allowedControls & vcGAC_RotationScreen)
  {
    udDouble2 deltaScreen = { io.MousePos.x - pContext->mScreenSquareCenter.x, io.MousePos.y - pContext->mScreenSquareCenter.y };
    double dist = udMag(deltaScreen);
    if (dist >= (pContext->mRadiusSquareCenter - 1.0) && dist < (pContext->mRadiusSquareCenter + 1.0))
      type = vcGMT_RotateScreen;
  }

  if ((pContext->allowedControls & vcGAC_RotationAxis) && (type == vcGMT_None))
  {
    const udDouble3 planeNormals[] = { pContext->mModel.axis.x.toVector3(), pContext->mModel.axis.y.toVector3(), pContext->mModel.axis.z.toVector3() };

    for (unsigned int i = 0; i < 3 && type == vcGMT_None; i++)
    {
      // pickup plane
      udPlane<double> pickupPlane = udPlane<double>::create(pContext->mModel.axis.t.toVector3(), planeNormals[i]);

      udDouble3 localPos;
      if (!pickupPlane.intersects(pContext->camera.worldMouseRay, &localPos, nullptr))
        continue;
      localPos -= pickupPlane.point;

      if (udDot(pContext->camera.worldMouseRay.direction, udNormalize3(localPos)) > DBL_EPSILON)
        continue;

      udDouble4 idealPosOnCircle = pContext->mModelInverse * udDouble4::create(udNormalize3(localPos), 0);
      ImVec2 idealPosOnCircleScreen = vcGizmo_WorldToScreen(pContext, idealPosOnCircle * pContext->mScreenFactor, pContext->mMVP);

      ImVec2 distanceOnScreen = idealPosOnCircleScreen - io.MousePos;

      if (udMagSq3(vcGizmo_MakeVect(distanceOnScreen)) < 8.0 * 8.0) // pixel size
        type = vcGMT_RotateX + i;
    }
  }

  return type;
}

int vcGizmo_GetMoveType(vcGizmoContext *pContext, const udDouble3 direction[])
{
  int type = vcGMT_None;

  if (pContext->allowedControls & vcGAC_Translation)
  {
    // screen
    if (ImGui::IsMouseHoveringRect(pContext->mScreenSquareMin, pContext->mScreenSquareMax))
      type = vcGMT_MoveScreen;

    // compute
    for (unsigned int i = 0; i < 3 && type == vcGMT_None; i++)
    {
      udDouble3 dirPlaneX, dirPlaneY, dirAxis;
      bool belowAxisLimit, belowPlaneLimit;
      vcGizmo_ComputeTripodAxisAndVisibility(pContext, direction, i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
      dirAxis = (pContext->mModel * udDouble4::create(dirAxis, 0)).toVector3();
      dirPlaneX = (pContext->mModel * udDouble4::create(dirPlaneX, 0)).toVector3();
      dirPlaneY = (pContext->mModel * udDouble4::create(dirPlaneY, 0)).toVector3();

      udDouble3 posOnPlane;
      if (udPlane<double>::create(pContext->mModel.axis.t.toVector3(), -pContext->mCameraDir).intersects(pContext->camera.worldMouseRay, &posOnPlane, nullptr))
      {
        const ImVec2 posOnPlaneScreen = vcGizmo_WorldToScreen(pContext, posOnPlane, pContext->camera.matrices.viewProjection);
        const ImVec2 axisStartOnScreen = vcGizmo_WorldToScreen(pContext, pContext->mModel.axis.t.toVector3() + dirAxis * pContext->mScreenFactor * 0.1f, pContext->camera.matrices.viewProjection);
        const ImVec2 axisEndOnScreen = vcGizmo_WorldToScreen(pContext, pContext->mModel.axis.t.toVector3() + dirAxis * pContext->mScreenFactor, pContext->camera.matrices.viewProjection);

        udDouble4 closestPointOnAxis = vcGizmo_PointOnSegment(vcGizmo_MakeVect(posOnPlaneScreen), vcGizmo_MakeVect(axisStartOnScreen), vcGizmo_MakeVect(axisEndOnScreen));

        if (udMagSq3(closestPointOnAxis - vcGizmo_MakeVect(posOnPlaneScreen)) < 12.0 * 12.0) // pixel size
          type = vcGMT_MoveX + i;
      }

      if (udPlane<double>::create(pContext->mModel.axis.t.toVector3(), dirAxis).intersects(pContext->camera.worldMouseRay, &posOnPlane, nullptr))
      {
        const double dx = udDot(dirPlaneX, ((posOnPlane - pContext->mModel.axis.t.toVector3()) * (1.0 / pContext->mScreenFactor)));
        const double dy = udDot(dirPlaneY, ((posOnPlane - pContext->mModel.axis.t.toVector3()) * (1.0 / pContext->mScreenFactor)));
        if (belowPlaneLimit && dx >= sQuadUV[0] && dx <= sQuadUV[4] && dy >= sQuadUV[1] && dy <= sQuadUV[3])
          type = vcGMT_MoveYZ + i;
      }
    }
  }

  return type;
}

void vcGizmo_HandleTranslation(vcGizmoContext *pContext, const udDouble3 direction[], udDouble4x4 *deltaMatrix, int& type, double snap)
{
  ImGuiIO& io = ImGui::GetIO();
  bool applyRotationLocaly = pContext->mMode == vcGCS_Local || type == vcGMT_MoveScreen;

  // move
  if (pContext->mbUsing)
  {
    ImGui::CaptureMouseFromApp();
    udDouble3 newPos;

    if (pContext->mTranslationPlane.intersects(pContext->camera.worldMouseRay, &newPos, nullptr))
    {
      // compute delta
      udDouble3 newOrigin = newPos - pContext->mRelativeOrigin * pContext->mScreenFactor;
      udDouble3 delta = newOrigin - pContext->mModel.axis.t.toVector3();

      // 1 axis constraint
      if (pContext->mCurrentOperation >= vcGMT_MoveX && pContext->mCurrentOperation <= vcGMT_MoveZ)
      {
        int axisIndex = pContext->mCurrentOperation - vcGMT_MoveX;
        udDouble3 axisValue = pContext->mModel.c[axisIndex].toVector3();

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
        udDouble3 cumulativeDelta = pContext->mModel.axis.t.toVector3() + delta - pContext->mMatrixOrigin;

        if (applyRotationLocaly)
        {
          udDouble4x4 modelSourceNormalized = pContext->mModel;

          pContext->mModel.axis.x = udNormalize3(pContext->mModel.axis.x);
          pContext->mModel.axis.y = udNormalize3(pContext->mModel.axis.y);
          pContext->mModel.axis.z = udNormalize3(pContext->mModel.axis.z);

          udDouble4x4 modelSourceNormalizedInverse = udInverse(modelSourceNormalized);
          cumulativeDelta = (modelSourceNormalizedInverse * udDouble4::create(cumulativeDelta, 0)).toVector3();
          vcGizmo_ComputeSnap(cumulativeDelta, snap);
          cumulativeDelta = (modelSourceNormalized * udDouble4::create(cumulativeDelta, 0)).toVector3();
        }
        else
        {
          vcGizmo_ComputeSnap(cumulativeDelta, snap);
        }

        delta = pContext->mMatrixOrigin + cumulativeDelta - pContext->mModel.axis.t.toVector3();
      }

      udDouble4x4 deltaMatrixTranslation = udDouble4x4::translation(delta);
      if (deltaMatrix)
        *deltaMatrix = deltaMatrixTranslation;

      if (!io.MouseDown[0])
        pContext->mbUsing = false;

      type = pContext->mCurrentOperation;
    }
  }
  else
  {
    // find new possible way to move
    type = vcGizmo_GetMoveType(pContext, direction);
    if (type != vcGMT_None)
    {
      ImGui::CaptureMouseFromApp();
    }

    if (vcGizmo_CanActivate() && type != vcGMT_None)
    {
      pContext->mbUsing = true;
      pContext->mCurrentOperation = type;
      udDouble3 movePlanNormal[] = { pContext->mModel.axis.x.toVector3(), pContext->mModel.axis.y.toVector3(), pContext->mModel.axis.z.toVector3(), pContext->mModel.axis.x.toVector3(), pContext->mModel.axis.y.toVector3(), pContext->mModel.axis.z.toVector3(), -pContext->mCameraDir };

      udDouble3 cameraToModelNormalized = udNormalize(pContext->mModel.axis.t.toVector3() - pContext->camera.position);
      for (unsigned int i = 0; i < 3; i++)
      {
        movePlanNormal[i] = udNormalize(udCross(udCross(cameraToModelNormalized, movePlanNormal[i]), movePlanNormal[i]));
      }

      // pickup plan
      pContext->mTranslationPlane = udPlane<double>::create(pContext->mModel.axis.t.toVector3(), movePlanNormal[type - vcGMT_MoveX]);
      if (pContext->mTranslationPlane.intersects(pContext->camera.worldMouseRay, &pContext->mTranslationPlaneOrigin, nullptr))
      {
        pContext->mMatrixOrigin = pContext->mModel.axis.t.toVector3();
        pContext->mRelativeOrigin = (pContext->mTranslationPlaneOrigin - pContext->mModel.axis.t.toVector3()) * (1.0 / pContext->mScreenFactor);
      }
    }
  }
}

void vcGizmo_HandleScale(vcGizmoContext *pContext, const udDouble3 direction[], udDouble4x4 *pDeltaMatrix, int& type, double snap)
{
  ImGuiIO& io = ImGui::GetIO();

  if (!pContext->mbUsing)
  {
    // find new possible way to scale
    type = vcGizmo_GetScaleType(pContext, direction);
    if (type != vcGMT_None)
      ImGui::CaptureMouseFromApp();

    if (vcGizmo_CanActivate() && type != vcGMT_None)
    {
      pContext->mbUsing = true;
      pContext->mCurrentOperation = type;
      pContext->mLastScaleDelta = udDouble3::zero();
      const udDouble3 movePlanNormal[] = { pContext->mModel.axis.y.toVector3(), pContext->mModel.axis.z.toVector3(), pContext->mModel.axis.x.toVector3(), pContext->mModel.axis.z.toVector3(), pContext->mModel.axis.y.toVector3(), pContext->mModel.axis.x.toVector3(), -pContext->mCameraDir };

      pContext->mTranslationPlane = udPlane<double>::create(pContext->mModel.axis.t.toVector3(), movePlanNormal[type - vcGMT_ScaleX]);
      if (pContext->mTranslationPlane.intersects(pContext->camera.worldMouseRay, &pContext->mTranslationPlaneOrigin, nullptr))
      {
        pContext->mMatrixOrigin = pContext->mModel.axis.t.toVector3();
        pContext->mScale = udDouble3::one();
        pContext->mRelativeOrigin = (pContext->mTranslationPlaneOrigin - pContext->mModel.axis.t.toVector3()) * (1.0 / pContext->mScreenFactor);
        pContext->mSaveMousePosx = io.MousePos.x;
        pContext->mLastMousePosx = io.MousePos.x;
      }
    }
  }

  // scale
  if (pContext->mbUsing)
  {
    ImGui::CaptureMouseFromApp();
    udDouble3 newPos;
    if (pContext->mTranslationPlane.intersects(pContext->camera.worldMouseRay, &newPos, nullptr))
    {
      udDouble3 newOrigin = newPos - pContext->mRelativeOrigin * pContext->mScreenFactor;
      udDouble3 delta = newOrigin - pContext->mModel.axis.t.toVector3();

      double scaleDelta = 0.0;
      int axisIndex = 0;
      bool uniformScaling = false;

      // 1 axis constraint
      if (pContext->mCurrentOperation >= vcGMT_ScaleX && pContext->mCurrentOperation <= vcGMT_ScaleZ)
      {
        axisIndex = pContext->mCurrentOperation - vcGMT_ScaleX;
        udDouble3 axisValue = pContext->mModel.c[axisIndex].toVector3();
        double lengthOnAxis = udDot(axisValue, delta);
        delta = axisValue * lengthOnAxis;

        udDouble3 baseVector = pContext->mTranslationPlaneOrigin - pContext->mModel.axis.t.toVector3();
        scaleDelta = udDot(axisValue, baseVector + delta) / udDot(axisValue, baseVector) - 1.0;
      }
      else
      {
        uniformScaling = true;
        scaleDelta = (io.MousePos.x - pContext->mSaveMousePosx)  * 0.01;
      }

      scaleDelta = udMax(-0.99, scaleDelta);
      vcGizmo_ComputeSnap(&scaleDelta, snap);
      scaleDelta = udMax((snap - 1.0), scaleDelta);

      pContext->mScaleValueOrigin[axisIndex] = 1.0 + scaleDelta;

      double inverseLastScaleDelta = 100.0; // 0.01 is our minimum scale, 100 == (1.0 / 0.01)
      if (pContext->mLastScaleDelta[axisIndex] != -1.0)
        inverseLastScaleDelta = (1.0 / (1.0 + pContext->mLastScaleDelta[axisIndex]));

      pContext->mLastScaleDelta[axisIndex] = scaleDelta;
      pContext->mScale[axisIndex] = (1.0 + scaleDelta) * inverseLastScaleDelta;

      if (uniformScaling)
      {
        // spread to all 3 axis
        pContext->mScaleValueOrigin = udDouble3::create(1.0 + scaleDelta);
        pContext->mLastScaleDelta = udDouble3::create(scaleDelta);
        pContext->mScale = udDouble3::create(pContext->mScale[axisIndex]);
      }

      // compute matrix & delta
      udDouble4x4 deltaMatrixScale = pContext->mModel * udDouble4x4::scaleNonUniform(pContext->mScale) * udInverse(pContext->mModel);

      if (pDeltaMatrix)
        *pDeltaMatrix = deltaMatrixScale;

      if (!io.MouseDown[0])
        pContext->mbUsing = false;

      type = pContext->mCurrentOperation;
      pContext->mLastMousePosx = io.MousePos.x;
    }
  }
}

void vcGizmo_HandleRotation(vcGizmoContext *pContext, const udDouble3 direction[], udDouble4x4 *deltaMatrix, int& type, double snap)
{
  ImGuiIO& io = ImGui::GetIO();
  bool applyRotationLocally = pContext->mMode == vcGCS_Local;

  if (!pContext->mbUsing)
  {
    type = vcGizmo_GetRotateType(pContext);

    if (type != vcGMT_None)
      ImGui::CaptureMouseFromApp();

    if (type == vcGMT_RotateScreen)
      applyRotationLocally = true;

    if (vcGizmo_CanActivate() && type != vcGMT_None)
    {
      pContext->mbUsing = true;
      pContext->mCurrentOperation = type;
      const udDouble3 rotatePlaneNormal[] = { pContext->mModel.axis.x.toVector3(), pContext->mModel.axis.y.toVector3(), pContext->mModel.axis.z.toVector3(), -pContext->mCameraDir };

      if (applyRotationLocally)
        pContext->mTranslationPlane = udPlane<double>::create(pContext->mModel.axis.t.toVector3(), rotatePlaneNormal[type - vcGMT_RotateX]);
      else
        pContext->mTranslationPlane = udPlane<double>::create(pContext->mModel.axis.t.toVector3(), direction[type - vcGMT_RotateX]);

      if (pContext->mTranslationPlane.intersects(pContext->camera.worldMouseRay, &pContext->mRotationVectorSource, nullptr))
      {
        pContext->mRotationVectorSource = udNormalize(pContext->mRotationVectorSource - pContext->mTranslationPlane.point);
        pContext->mRotationAngleOrigin = vcGizmo_ComputeAngleOnPlane(pContext);
      }
    }
  }

  // rotation
  if (pContext->mbUsing)
  {
    ImGui::CaptureMouseFromApp();
    pContext->mRotationAngle = vcGizmo_ComputeAngleOnPlane(pContext);

    if (snap)
      vcGizmo_ComputeSnap(&pContext->mRotationAngle, UD_DEG2RAD(snap));

    udDouble4x4 deltaRotation = udDouble4x4::translation(pContext->mModel.axis.t.toVector3()) * udDouble4x4::rotationAxis(pContext->mTranslationPlane.normal, pContext->mRotationAngle - pContext->mRotationAngleOrigin) * udDouble4x4::translation(-pContext->mModel.axis.t.toVector3());
    pContext->mRotationAngleOrigin = pContext->mRotationAngle;

    if (deltaMatrix)
      *deltaMatrix = deltaRotation;

    if (!io.MouseDown[0])
      pContext->mbUsing = false;

    type = pContext->mCurrentOperation;
  }
}

void vcGizmo_Manipulate(vcGizmoContext *pContext, const vcCamera *pCamera, const udDouble3 direction[], vcGizmoOperation operation, vcGizmoCoordinateSystem mode, const udDouble4x4 &matrix, udDouble4x4 *pDeltaMatrix, vcGizmoAllowedControls allowedControls, double snap /*= 0.0*/)
{
  if (operation == vcGO_Scale)
    mode = vcGCS_Local;

  vcGizmo_ComputeContext(pContext, pCamera, matrix, mode, allowedControls);

  if (pDeltaMatrix)
    *pDeltaMatrix = udDouble4x4::identity();

  // behind camera
  udDouble4 camSpacePosition = pContext->mMVP.axis.t;
  if (camSpacePosition.z < -0.001f)
  {
    pContext->mbUsing = false;
    return;
  }

  int type = vcGMT_None;
  switch (operation)
  {
  case vcGO_Rotate:
    vcGizmo_HandleRotation(pContext, direction, pDeltaMatrix, type, snap);
    vcGizmo_DrawRotationGizmo(pContext, type);
    break;
  case vcGO_Translate:
    vcGizmo_HandleTranslation(pContext, direction, pDeltaMatrix, type, snap);
    vcGizmo_DrawTranslationGizmo(pContext, direction, type);
    break;
  case vcGO_Scale:
    vcGizmo_HandleScale(pContext, direction, pDeltaMatrix, type, snap);
    vcGizmo_DrawScaleGizmo(pContext, direction, type);
    break;
  case vcGO_NoGizmo:
    break;
  }
}

void vcGizmo_ResetState(vcGizmoContext *pContext)
{
  pContext->mbUsing = false;
}
