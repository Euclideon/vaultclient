#ifndef vcGizmo_h__
#define vcGizmo_h__

#include "vcMath.h"
#include "vcCamera.h"

enum vcGizmoOperation
{
  vcGO_NoGizmo,
  vcGO_Translate,
  vcGO_Rotate,
  vcGO_Scale,
};

enum vcGizmoCoordinateSystem
{
  vcGCS_Local,
  vcGCS_Scene
};

enum vcGizmoAllowedControls
{
  vcGAC_Translation = (1 << 0),
  vcGAC_ScaleUniform = (1 << 1),
  vcGAC_ScaleNonUniform = (1 << 2),
  vcGAC_RotationAxis = (1 << 3),
  vcGAC_RotationScreen = (1 << 4),

  vcGAC_Scale = vcGAC_ScaleNonUniform | vcGAC_ScaleUniform,
  vcGAC_Rotation = vcGAC_RotationAxis | vcGAC_RotationScreen,
  vcGAC_AllUniform = vcGAC_Translation | vcGAC_Rotation | vcGAC_ScaleUniform,
  vcGAC_All = vcGAC_Translation | vcGAC_Rotation | vcGAC_Scale
};

void vcGizmo_SetDrawList(); // call inside your own window and before vcGizmo_Manipulate() in order to draw gizmo to that window.

void vcGizmo_BeginFrame(); // call vcGizmo_BeginFrame right after ImGui_XXXX_NewFrame();
bool vcGizmo_IsHovered(const udDouble3 direction[]); // return true if mouse cursor is over any gizmo control (axis, plan or screen component)
bool vcGizmo_IsActive(); // return true if mouse vcGizmo_IsHovered or if the gizmo is in moving state

void vcGizmo_SetRect(float x, float y, float width, float height);

void vcGizmo_Manipulate(const vcCamera *pCamera, const udDouble3 direction[], vcGizmoOperation operation, vcGizmoCoordinateSystem mode, const udDouble4x4 &matrix, udDouble4x4 *pDeltaMatrix, vcGizmoAllowedControls allowedControls, double snap = 0.0);
void vcGizmo_ResetState();

#endif

