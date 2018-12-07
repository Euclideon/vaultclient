#ifndef vcGizmo_h__
#define vcGizmo_h__

#include "vcMath.h"
#include "vcCamera.h"

enum vcGizmoOperation
{
  vcGO_Translate,
  vcGO_Rotate,
  vcGO_Scale,
};

enum vcGizmoCoordinateSystem
{
  vcGCS_Local,
  vcGCS_Scene
};

void vcGizmo_SetDrawList(); // call inside your own window and before vcGizmo_Manipulate() in order to draw gizmo to that window.

void vcGizmo_BeginFrame(); // call vcGizmo_BeginFrame right after ImGui_XXXX_NewFrame();
bool vcGizmo_IsHovered(); // return true if mouse cursor is over any gizmo control (axis, plan or screen component)
bool vcGizmo_IsActive(); // return true if mouse vcGizmo_IsHovered or if the gizmo is in moving state

void vcGizmo_SetRect(float x, float y, float width, float height);

void vcGizmo_Manipulate(const vcCamera *pCamera, vcGizmoOperation operation, vcGizmoCoordinateSystem mode, udDouble4x4 *pMatrix, udDouble4x4 *deltaMatrix = 0, double snap = 0.0);

#endif

