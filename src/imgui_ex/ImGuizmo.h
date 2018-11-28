// https://github.com/CedricGuillemet/ImGuizmo
// v 1.61 WIP
//
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
//
// -------------------------------------------------------------------------------------------
// History :
// 2016/09/11 Behind camera culling. Scaling Delta matrix not multiplied by source matrix scales. local/world rotation and translation fixed. Display message is incorrect (X: ... Y:...) in local mode.
// 2016/09/09 Hatched negative axis. Snapping. Documentation update.
// 2016/09/04 Axis switch and translation plan autohiding. Scale transform stability improved
// 2016/09/01 Mogwai changed to Manipulate. Draw debug cube. Fixed inverted scale. Mixing scale and translation/rotation gives bad results.
// 2016/08/31 First version
//
// -------------------------------------------------------------------------------------------
// Future (no order):
//
// - Multi view
// - display rotation/translation/scale infos in local/world space and not only local
// - finish local/world matrix application
// - ImGuizmoOperation as bitmask
//
// -------------------------------------------------------------------------------------------
// Example
#pragma once

#ifdef USE_IMGUI_API
#include "imconfig.h"
#endif
#ifndef IMGUI_API
#define IMGUI_API
#endif

#include "udPlatform/udMath.h"

namespace ImGuizmo
{
	IMGUI_API void SetDrawlist(); // call inside your own window and before Manipulate() in order to draw gizmo to that window.

	IMGUI_API void BeginFrame(); // call BeginFrame right after ImGui_XXXX_NewFrame();
	IMGUI_API bool IsOver(); // return true if mouse cursor is over any gizmo control (axis, plan or screen component)
	IMGUI_API bool IsUsing(); // return true if mouse IsOver or if the gizmo is in moving state

	IMGUI_API void SetRect(float x, float y, float width, float height);

	// call it when you want a gizmo
	// Needs view and projection matrices.
	// matrix parameter is the source matrix (where will be gizmo be drawn) and might be transformed by the function. Return deltaMatrix is optional
	// translation is applied in world space
	enum ImGuizmoOperation
	{
		igoTranslate,
		igoRotate,
		igoScale,
	};

	enum ImGuizmoSpace
	{
		igsLocal,
		igsWorld
	};

	IMGUI_API void Manipulate(const udDouble4x4 &view, const udDouble4x4 &projection, ImGuizmoOperation operation, ImGuizmoSpace mode, udDouble4x4 *pMatrix, udDouble4x4 *deltaMatrix = 0, double snap = 0.0);
};
