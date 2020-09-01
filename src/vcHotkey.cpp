#include "vcHotkey.h"

#include "vcStrings.h"
#include "vcStringFormat.h"
#include "vcSettingsUI.h"

#include "udStringUtil.h"

#include "imgui.h"
#include "imgui_internal.h" // Required for internal Begin/End columns API
#include "imgui_ex/vcImGuiSimpleWidgets.h"

#include <SDL2/SDL.h>

namespace vcHotkey
{
  static int target = -1;
  static int keyBinds[vcB_Count] = {};

  static int alreadyBoundKey = -1;

  const char* bindNames[] =
  {
    "CameraForward",
    "CameraBackward",
    "CameraLeft",
    "CameraRight",
    "CameraUp",
    "CameraDown",
    "DecreaseCameraSpeed",
    "IncreaseCameraSpeed",
    "Remove",
    "Cancel",
    "LockAltitude",
    "GizmoTranslate",
    "GizmoRotate",
    "GizmoScale",
    "GizmoLocalSpace",
    "Fullscreen",
    "RenameSceneItem",
    "AddUDS",
    "BindingsInterface",
    "TakeScreenshot",
    "ToggleSceneExplorer",
    "ToggleSelectTool",
    "ToggleMeasureLineTool",
    "ToggleMeasureAreaTool",
    "ToggleInspectionTool",
    "ToggleAnnotateTool",
    "OpenSettingsMenu",
    "ToggleMeasureHeightTool",
    "AddBoxFilter",
    "AddSphereFilter",
    "AddCylinderFilter",
  };
  UDCOMPILEASSERT(udLengthOf(vcHotkey::bindNames) == vcB_Count, "Hotkey count discrepancy.");

  static const char *modText[] =
  {
    "Shift",
    "Ctrl",
    "Alt",
    "Super"
  };

  static const modifierFlags modList[] =
  {
    vcMOD_Shift,
    vcMOD_Ctrl,
    vcMOD_Alt,
    vcMOD_Super
  };

  void ClearState()
  {
    target = -1;
    alreadyBoundKey = -1;
  }

  bool IsDown(int keyNum)
  {
    ImGuiIO io = ImGui::GetIO();

    if (keyNum > vcMOD_Mask)
    {
      if (((keyNum & vcMOD_Shift) == vcMOD_Shift) != io.KeyShift)
        return false;
      if (((keyNum & vcMOD_Ctrl) == vcMOD_Ctrl) != io.KeyCtrl)
        return false;
      if (((keyNum & vcMOD_Alt) == vcMOD_Alt) != io.KeyAlt)
        return false;
      if (((keyNum & vcMOD_Super) == vcMOD_Super) != io.KeySuper)
        return false;
    }

    return io.KeysDown[keyNum & vcMOD_Mask];
  }

  bool IsDown(vcBind key)
  {
    return IsDown(keyBinds[key]);
  }

  bool IsPressed(int keyNum, bool checkMod)
  {
    if (target != -1)
      return false;

    ImGuiIO io = ImGui::GetIO();

    if (keyNum > vcMOD_Mask || checkMod)
    {
      bool contains = (keyNum & vcMOD_Shift) == vcMOD_Shift;
      if ((contains && !io.KeyShift) || (checkMod && !contains && io.KeyShift))
        return false;

      contains = (keyNum & vcMOD_Ctrl) == vcMOD_Ctrl;
      if ((contains && !io.KeyCtrl) || (checkMod && !contains && io.KeyCtrl))
        return false;

      contains = (keyNum & vcMOD_Alt) == vcMOD_Alt;
      if ((contains && !io.KeyAlt) || (checkMod && !contains && io.KeyAlt))
        return false;

      contains = (keyNum & vcMOD_Super) == vcMOD_Super;
      if ((contains && !io.KeySuper) || (checkMod && !contains && io.KeySuper))
        return false;
    }

    return ImGui::IsKeyPressed((keyNum & vcMOD_Mask), false);
  }

  bool IsPressed(vcBind key, bool checkMod)
  {
    return IsPressed(keyBinds[key], checkMod);
  }

  void NameFromKey(int key, char *pBuffer, uint32_t bufferLen)
  {
    if (key == 0)
    {
      udStrcpy(pBuffer, (size_t)bufferLen, vcString::Get("bindingsClear"));
      return;
    }

    const char *strings[5] = {};

    strings[0] = (key & vcMOD_Shift) == vcMOD_Shift ? "Shift + " : "";
    strings[1] = (key & vcMOD_Ctrl) == vcMOD_Ctrl ? "Ctrl + " : "";
    strings[2] = (key & vcMOD_Alt) == vcMOD_Alt ? "Alt + " : "";

    if ((key & vcMOD_Super) == vcMOD_Super)
    {
#ifdef UDPLATFORM_WINDOWS
      strings[3] = "Win + ";
#elif UDPLATFORM_OSX
      strings[3] = "Cmd + ";
#else
      strings[3] = "Super + ";
#endif
    }
    else
    {
      strings[3] = "";
    }

    strings[4] = SDL_GetScancodeName((SDL_Scancode)(key & vcMOD_Mask));
    vcStringFormat(pBuffer, bufferLen, "{0}{1}{2}{3}{4}", strings, 5);
  }

  void GetKeyName(vcBind key, char *pBuffer, uint32_t bufferLen)
  {
    int mappedKey = (key == vcB_Count ? 0 : keyBinds[key]);
    NameFromKey(mappedKey, pBuffer, bufferLen);
  }

  vcBind BindFromName(const char* pName)
  {
    int i = 0;
    for (; i < (int)udLengthOf(bindNames); ++i)
    {
      if (udStrEquali(bindNames[i], pName))
        break;
    }

    return (vcBind)i;
  }

  const char* GetBindName(vcBind key)
  {
    return bindNames[key];
  }

  void Set(vcBind key, int value)
  {
    keyBinds[(int)key] = value;
  }

  int Get(vcBind key)
  {
    return keyBinds[key];
  }

  void DisplayBindings(vcState *pProgramState)
  {
    if (target != -1)
    {
      if (pProgramState->currentKey)
      {
        ImGuiIO io = ImGui::GetIO();
        if (io.KeyShift)
          pProgramState->currentKey |= vcMOD_Shift;
        if (io.KeyCtrl)
          pProgramState->currentKey |= vcMOD_Ctrl;
        if (io.KeyAlt)
          pProgramState->currentKey |= vcMOD_Alt;
        if (io.KeySuper)
          pProgramState->currentKey |= vcMOD_Super;

        int i = 0;
        for (; i < vcB_Count; ++i)
        {
          if (keyBinds[i] == pProgramState->currentKey && i != target)
            break;
        }

        if (i == vcB_Count)
        {
          Set((vcBind)target, pProgramState->currentKey);
          alreadyBoundKey = -1;
          target = -1;
        }
        else
        {
          alreadyBoundKey = i;
        }
      }
    }

    ImGui::BeginColumns(nullptr, 3, ImGuiColumnsFlags_NoResize);

    ImGui::SetColumnWidth(0, 175);
    ImGui::SetColumnWidth(1, 125);
    ImGui::SetColumnWidth(2, 600);

    // Header Row
    ImGui::SetWindowFontScale(1.05f);
    ImGui::TextUnformatted(vcString::Get("bindingsColumnName"));
    ImGui::NextColumn();
    ImGui::TextUnformatted(vcString::Get("bindingsColumnKeyCombination"));
    ImGui::NextColumn();
    ImGui::TextUnformatted(vcString::Get("bindingsColumnDescription"));
    ImGui::NextColumn();
    ImGui::SetWindowFontScale(1.f);

    for (int i = 0; i < vcB_Count; ++i)
    {
#if UDPLATFORM_EMSCRIPTEN
      if (i == vcB_Fullscreen)
      {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      }
#endif
      if (ImGui::Button(bindNames[i], ImVec2(-1, 0)))
      {
        if (target == i)
        {
          ClearState();
        }
        else
        {
          pProgramState->currentKey = 0;
          target = i;
          alreadyBoundKey = -1;
        }
      }
#if UDPLATFORM_EMSCRIPTEN
      if (i == vcB_Fullscreen)
      {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
      }
#endif

      if (target == i)
      {
        ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::GetColorU32(ImGuiCol_ButtonActive), 0.0f, ImDrawCornerFlags_All, 2.0f);

        vcIGSW_ShowLoadStatusIndicator(vcSLS_Loading);
        ImGui::TextWrapped("%s", vcString::Get("bindingsSelectKey"));
      }

      ImGui::NextColumn();

      char key[100];
      GetKeyName((vcBind)i, key, (uint32_t)udLengthOf(key));
      ImGui::TextUnformatted(key);

      if (target == i)
      {
        if (alreadyBoundKey != -1)
        {
          vcIGSW_ShowLoadStatusIndicator(vcSLS_Failed);
          ImGui::TextWrapped("%s", vcStringFormat(key, udLengthOf(key), vcString::Get("bindingsErrorBound"), bindNames[alreadyBoundKey]));
        }
      }

      ImGui::NextColumn();

      ImGui::TextWrapped("%s", vcString::Get(udTempStr("bindings%s", bindNames[i])));

      ImGui::NextColumn();
    }

    ImGui::Columns(1);
  }

  int DecodeKeyString(const char *pBind)
  {
    if (pBind == nullptr || *pBind == '\0')
      return 0;

    int len = (int)udStrlen(pBind);
    size_t index = 0;
    int value = 0;

    if (udStrrchr(pBind, "+", (size_t *)&index) != nullptr)
    {
      size_t modIndex = 0;

      for (int i = 0; i < (int)udLengthOf(modList); ++i)
      {
        udStrstr(pBind, len, modText[i], &modIndex);
        if ((int)modIndex != len)
          value |= modList[i];
      }

      pBind += index + 2;
    }

    value += SDL_GetScancodeFromName(pBind);

    return value;
  }
}
