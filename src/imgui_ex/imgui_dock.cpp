
#include "imgui_dock_internal.h"

ImGui::DockContext g_dock = {};

namespace ImGui
{
  void ShutdownDock()
  {
    for (int i = 0; i < g_dock.m_docks.size(); ++i)
    {
      g_dock.m_docks[i]->~Dock();
      MemFree(g_dock.m_docks[i]);
    }
    g_dock.m_docks.clear();
  }

  void RootDock(const ImVec2& pos, const ImVec2& size)
  {
    g_dock.rootDock(pos, size);
  }

  void SetDockActive()
  {
    g_dock.setDockActive();
  }

  bool BeginDock(const char* label, bool* opened, ImGuiWindowFlags extra_flags)
  {
    return g_dock.begin(label, opened, extra_flags);
  }

  void EndDock()
  {
    g_dock.end();
  }

  void CaptureDefaults()
  {
    g_dock.captureDefaults();
  }
} // namespace ImGui
