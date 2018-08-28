#include "vcSettings.h"

#include "udPlatform/udValue.h"
#include "udPlatform/udPlatformUtil.h"

#include "imgui.h"
#include "imgui_ex/imgui_dock_internal.h"

extern ImGui::DockContext g_dock;
const char *pDefaultSettings = R"config({"window":{"position":{"x":805240832,"y":805240832},"width":1280,"height":720,"maximized":false,"fullscreen":false},"frames":{"scene":true,"settings":true,"explorer":true},"camera":{"moveSpeed":10.000000,"nearPlane":0.500000,"farPlane":10000.000000,"fieldOfView":0.872665,"lensId":5,"invertX":false,"invertY":false,"moveMode":0},"maptiles":{"enabled":true,"blendMode":0,"transparency":1.000000,"mapHeight":0.000000},"rootDocks":[{"label":"ROOT","status":0,"active":true,"open":false,"position":{"x":0.000000,"y":22.000000},"size":{"x":1280.000000,"y":698.000000},"child":[{"label":"Scene","status":0,"active":true,"open":true,"position":{"x":0.000000,"y":22.000000},"size":{"x":976.000000,"y":698.000000},"location":"1"},{"label":"DOCK","status":0,"active":true,"open":false,"position":{"x":976.000000,"y":22.000000},"size":{"x":304.000000,"y":698.000000},"location":"0","child":[{"label":"Scene Explorer","status":0,"active":true,"open":true,"position":{"x":976.000000,"y":22.000000},"size":{"x":304.000000,"y":288.000000},"location":"20"},{"label":"Settings","status":0,"active":true,"open":true,"position":{"x":976.000000,"y":310.000000},"size":{"x":304.000000,"y":410.000000},"location":"30"}]}]},{"label":"StyleEditor","status":1,"active":true,"open":false,"position":{"x":0.000000,"y":0.000000},"size":{"x":1280.000000,"y":720.000000}}]})config";

void vcSettings_RecursiveLoadDock(const udValue &parentDock, int parentIndex, bool isNextTab = false)
{
  ImGui::DockContext::Dock *new_dock = (ImGui::DockContext::Dock*)ImGui::MemAlloc(sizeof(ImGui::DockContext::Dock));
  IM_PLACEMENT_NEW(new_dock) ImGui::DockContext::Dock();
  g_dock.m_docks.push_back(new_dock);

  int newIndex = g_dock.m_docks.size()-1;

  g_dock.m_docks[newIndex]->label = ImStrdup(parentDock.Get("label").AsString());
  g_dock.m_docks[newIndex]->id = ImHash(g_dock.m_docks[newIndex]->label, 0);

  if (parentDock.Get("child").ArrayLength() > 0) // has children
  {
    vcSettings_RecursiveLoadDock(parentDock.Get("child[0]"), newIndex);
    g_dock.m_docks[newIndex]->children[0] = g_dock.getDockByIndex(newIndex + 1);

    int child1Index = g_dock.m_docks.size();
    vcSettings_RecursiveLoadDock(parentDock.Get("child[1]"), newIndex);
    g_dock.m_docks[newIndex]->children[1] = g_dock.getDockByIndex(child1Index);
  }

  if (isNextTab)
    g_dock.m_docks[newIndex]->prev_tab = g_dock.getDockByIndex(newIndex - 1); // must be previous value

  if (parentDock.Get("next").MemberCount() > 0) // has next_tab
  {
    vcSettings_RecursiveLoadDock(parentDock.Get("next"), parentIndex, true);

    g_dock.m_docks[newIndex]->next_tab = g_dock.getDockByIndex(newIndex + 1); // as tab cannot have children
  }

  g_dock.m_docks[newIndex]->parent = g_dock.getDockByIndex(parentIndex);

  g_dock.m_docks[newIndex]->status = (ImGui::DockContext::Status_)parentDock.Get("status").AsInt();
  g_dock.m_docks[newIndex]->active = parentDock.Get("active").AsBool();
  g_dock.m_docks[newIndex]->opened = parentDock.Get("open").AsBool();

  g_dock.m_docks[newIndex]->pos.x = parentDock.Get("position.x").AsFloat();
  g_dock.m_docks[newIndex]->pos.y = parentDock.Get("position.y").AsFloat();
  g_dock.m_docks[newIndex]->size.x = parentDock.Get("size.x").AsFloat();
  g_dock.m_docks[newIndex]->size.y = parentDock.Get("size.y").AsFloat();

  udStrcpy(g_dock.m_docks[newIndex]->location, sizeof(g_dock.m_docks[newIndex]->location), parentDock.Get("location").AsString(""));

  g_dock.tryDockToStoredLocation(*g_dock.m_docks[newIndex]);
}

void vcSettings_LoadDocks(udValue &settings)
{
  for (int i = 0; i < g_dock.m_docks.size(); ++i)
  {
    g_dock.m_docks[i]->~Dock();
    MemFree(g_dock.m_docks[i]);
  }
  g_dock.m_docks.clear();

  int numRootDocks = (int)settings.Get("rootDocks").ArrayLength();

  for (int i = 0; i < numRootDocks; ++i)
  {
    vcSettings_RecursiveLoadDock(settings.Get("rootDocks[%d]", i), -1);
  }
}

void vcSettings_RecursiveSaveDock(udValue &parentJSON, ImGui::DockContext::Dock *pParentDock, const char *pParentString)
{
  udValue dockJSON;
  dockJSON.SetObject();

  ImGui::DockContext::Dock &dock = *pParentDock;

  dockJSON.Set("label = '%s'", dock.parent ? (dock.label[0] == '\0' ? "DOCK" : dock.label) : (dock.label[0] == '\0' ? "ROOT" : dock.label));

  dockJSON.Set("status = %d", dock.status);
  dockJSON.Set("active = %s", dock.active ? "true" : "false");
  dockJSON.Set("open = %s", dock.opened ? "true" : "false");

  dockJSON.Set("position.x = %f", dock.pos.x);
  dockJSON.Set("position.y = %f", dock.pos.y);
  dockJSON.Set("size.x = %f", dock.size.x);
  dockJSON.Set("size.y = %f", dock.size.y);

  g_dock.fillLocation(dock);
  if(udStrlen(dock.location))
    dockJSON.Set("location = '%s'", dock.location);

  if (dock.children[0])
    vcSettings_RecursiveSaveDock(dockJSON, dock.children[0], "child[0]");

  if (dock.children[1])
    vcSettings_RecursiveSaveDock(dockJSON, dock.children[1], "child[1]");

  if (dock.next_tab)
    vcSettings_RecursiveSaveDock(dockJSON, dock.next_tab, "next");

  parentJSON.Set(&dockJSON, pParentString);
}

void vcSettings_SaveDocks(udValue &settings)
{
  for (int i = 0; i < g_dock.m_docks.size(); ++i)
  {
    ImGui::DockContext::Dock& dock = *g_dock.m_docks[i];
    bool dockIsValidRoot = (g_dock.getDockIndex(dock.parent) == -1);

    if ((!dock.children[0] || !dock.children[1]) && udStrlen(dock.label) == 0) // if either of the children are nullptr and the label has a length of 0
      dockIsValidRoot = false; // is a root dock with no children, do not save

    if (dockIsValidRoot)
      vcSettings_RecursiveSaveDock(settings, &dock, "rootDocks[]");
  }
}

void vcSettings_InitializePrefPath(vcSettings *pSettings)
{
  if (pSettings->noLocalStorage)
    return;

  char *pPath = SDL_GetPrefPath("euclideon", "client");

  if (pPath != nullptr)
    pSettings->pSaveFilePath = pPath;
  else
    pSettings->noLocalStorage = true;
}

const char* SDL_fileread(const char* filename)
{
  SDL_RWops *rw = SDL_RWFromFile(filename, "rb");
  if (rw == NULL)
    return NULL;

  Sint64 res_size = SDL_RWsize(rw);
  char *res = udAllocType(char, res_size + 1, udAF_None);

  Sint64 nb_read_total = 0, nb_read = 1;
  char* buf = res;

  while (nb_read_total < res_size && nb_read != 0)
  {
    nb_read = SDL_RWread(rw, buf, 1, (res_size - nb_read_total));
    nb_read_total += nb_read;
    buf += nb_read;
  }

  SDL_RWclose(rw);

  if (nb_read_total != res_size)
  {
    udFree(res);
    return nullptr;
  }

  res[nb_read_total] = '\0';
  return res;
}

bool SDL_filewrite(const char* filename, const char *pStr, size_t bytes)
{
  bool success = false;

  SDL_RWops *rw = SDL_RWFromFile(filename, "wb");
  if (rw != NULL)
  {
    if (SDL_RWwrite(rw, pStr, 1, bytes) != bytes)
      success = false;
    else
      success = true;

    SDL_RWclose(rw);
  }

  return success;
}

bool vcSettings_Load(vcSettings *pSettings, bool forceReset /*= false*/)
{
  ImGui::GetIO().IniFilename = NULL; // Disables auto save and load

  const char *pSavedData = nullptr;

  if (!pSettings->noLocalStorage && pSettings->pSaveFilePath == nullptr)
    vcSettings_InitializePrefPath(pSettings);

  if (pSettings->noLocalStorage || pSettings->pSaveFilePath == nullptr || forceReset)
  {
    pSavedData = pDefaultSettings;
  }
  else
  {
    char buffer[vcMaxPathLength];
    udSprintf(buffer, vcMaxPathLength, "%ssettings.json", pSettings->pSaveFilePath);

    const char *pSettingFileData = SDL_fileread(buffer);

    if (pSettingFileData != nullptr)
      pSavedData = pSettingFileData;
    else
      pSavedData = pDefaultSettings;
  }

  if (pSavedData != nullptr)
  {
    udValue data;
    data.Parse(pSavedData);

    if (data.Get("docks").IsArray())
    {
      vcSettings_Load(pSettings, true);
      goto epilogue;
    }

    // Misc Settings
    pSettings->showDiagnosticInfo = data.Get("showDiagnosticInfo").AsBool(false);
    pSettings->showAdvancedGIS = data.Get("showAdvGISOptions").AsBool(false);
    pSettings->styleIndex = data.Get("style").AsInt(1); // dark style by default

    switch (pSettings->styleIndex)
    {
    case 0: ImGui::StyleColorsClassic(); break;
    case 1: ImGui::StyleColorsDark(); break;
    case 2: ImGui::StyleColorsLight(); break;
    }

    // Windows
    pSettings->window.xpos = data.Get("window.position.x").AsInt(SDL_WINDOWPOS_CENTERED);
    pSettings->window.ypos = data.Get("window.position.y").AsInt(SDL_WINDOWPOS_CENTERED);
    pSettings->window.width = data.Get("window.width").AsInt(1280);
    pSettings->window.height = data.Get("window.height").AsInt(720);
    pSettings->window.maximized = data.Get("window.maximized").AsBool(false);
    pSettings->window.fullscreen = data.Get("window.fullscreen").AsBool(false);
    pSettings->window.touchscreenFriendly = data.Get("window.touchscreenFriendly").AsBool(false);

    pSettings->window.windowsOpen[vcdScene] = data.Get("frames.scene").AsBool(true);
    pSettings->window.windowsOpen[vcdSettings] = data.Get("frames.settings").AsBool(true);
    pSettings->window.windowsOpen[vcdSceneExplorer] = data.Get("frames.explorer").AsBool(true);

    // Camera
    pSettings->camera.moveSpeed = data.Get("camera.moveSpeed").AsFloat(10.f);
    pSettings->camera.nearPlane = data.Get("camera.nearPlane").AsFloat(0.5f);
    pSettings->camera.farPlane = data.Get("camera.farPlane").AsFloat(10000.f);
    pSettings->camera.fieldOfView = data.Get("camera.fieldOfView").AsFloat(vcLens30mm);
    pSettings->camera.lensIndex = data.Get("camera.lensId").AsInt(vcLS_30mm);
    pSettings->camera.invertX = data.Get("camera.invertX").AsBool(false);
    pSettings->camera.invertY = data.Get("camera.invertY").AsBool(false);
    pSettings->camera.moveMode = (vcCameraMoveMode)data.Get("camera.moveMode").AsInt(0);
    pSettings->camera.cameraMouseBindings[0] = (vcCameraPivotMode)data.Get("camera.cameraMouseBindings[0]").AsInt(vcCPM_Tumble);
    pSettings->camera.cameraMouseBindings[1] = (vcCameraPivotMode)data.Get("camera.cameraMouseBindings[1]").AsInt(vcCPM_Pan);
    pSettings->camera.cameraMouseBindings[2] = (vcCameraPivotMode)data.Get("camera.cameraMouseBindings[2]").AsInt(vcCPM_Orbit);

    // Visualization
    pSettings->visualization.mode = (vcVisualizatationMode)data.Get("visualization.mode").AsInt(0);
    pSettings->visualization.minIntensity = data.Get("visualization.minIntensity").AsInt(0);
    pSettings->visualization.maxIntensity = data.Get("visualization.maxIntensity").AsInt(65535);

    // Post visualization - Edge Highlighting
    pSettings->postVisualization.edgeOutlines.enable = data.Get("postVisualization.edgeOutlines.enabled").AsBool(false);
    pSettings->postVisualization.edgeOutlines.width = data.Get("postVisualization.edgeOutlines.width").AsInt(1);
    pSettings->postVisualization.edgeOutlines.threshold = data.Get("postVisualization.edgeOutlines.threshold").AsFloat(0.001f);
    for (int i = 0; i < 4; i++)
      pSettings->postVisualization.edgeOutlines.colour[i] = data.Get("postVisualization.edgeOutlines.colour[%d]", i).AsFloat(1.f);

    // Post visualization - Colour by height
    pSettings->postVisualization.colourByHeight.enable = data.Get("postVisualization.colourByHeight.enabled").AsBool(false);
    for (int i = 0; i < 4; i++)
      pSettings->postVisualization.colourByHeight.minColour[i] = data.Get("postVisualization.colourByHeight.minColour[%d]", i).AsFloat((i < 3) ? 0.f : 1.f); // 0.f, 0.f, 1.f, 1.f
    for (int i = 0; i < 4; i++)
      pSettings->postVisualization.colourByHeight.maxColour[i] = data.Get("postVisualization.colourByHeight.maxColour[%d]", i).AsFloat((i % 2) ? 1.f : 0.f); // 0.f, 1.f, 0.f, 1.f
    pSettings->postVisualization.colourByHeight.startHeight = data.Get("postVisualization.colourByHeight.startHeight").AsFloat(30.f);
    pSettings->postVisualization.colourByHeight.endHeight = data.Get("postVisualization.colourByHeight.endHeight").AsFloat(50.f);

    // Post visualization - Colour by depth
    pSettings->postVisualization.colourByDepth.enable = data.Get("postVisualization.colourByDepth.enabled").AsBool(false);
    for (int i = 0; i < 4; i++)
      pSettings->postVisualization.colourByDepth.colour[i] = data.Get("postVisualization.colourByDepth.colour[%d]", i).AsFloat((i == 0 || i == 3) ? 1.f : 0.f); // 1.f, 0.f, 0.f, 1.f
    pSettings->postVisualization.colourByDepth.startDepth = data.Get("postVisualization.colourByDepth.startDepth").AsFloat(100.f);
    pSettings->postVisualization.colourByDepth.endDepth = data.Get("postVisualization.colourByDepth.endDepth").AsFloat(1000.f);

    // Post visualization - Contours
    pSettings->postVisualization.contours.enable = data.Get("postVisualization.contours.enabled").AsBool(false);
    for (int i = 0; i < 4; i++)
      pSettings->postVisualization.contours.colour[i] = data.Get("postVisualization.contours.colour[%d]", i).AsFloat((i == 3) ? 1.f : 0.f); // 0.f, 0.f, 0.f, 1.f
    pSettings->postVisualization.contours.distances = data.Get("postVisualization.contours.distances").AsFloat(50.f);
    pSettings->postVisualization.contours.bandHeight = data.Get("postVisualization.contours.bandHeight").AsFloat(1.f);

    // Map Tiles
    pSettings->maptiles.mapEnabled = data.Get("maptiles.enabled").AsBool(true);
    pSettings->maptiles.blendMode = (vcMapTileBlendMode)data.Get("maptiles.blendMode").AsInt(1);
    pSettings->maptiles.transparency = data.Get("maptiles.transparency").AsFloat(1.f);
    pSettings->maptiles.mapHeight = data.Get("maptiles.mapHeight").AsFloat(0.f);

    // Docks
    vcSettings_LoadDocks(data);
  }

epilogue:
  if (pSavedData != pDefaultSettings)
    udFree(pSavedData);

  return true;
}

bool vcSettings_Save(vcSettings *pSettings)
{
  if (pSettings->noLocalStorage)
    return false;

  bool success = false;

  if (pSettings->pSaveFilePath == nullptr)
    vcSettings_InitializePrefPath(pSettings);

  ImGui::GetIO().WantSaveIniSettings = false;

  udValue data;

  // Misc Settings
  if (pSettings->showDiagnosticInfo) // This hides the option if its false
    data.Set("showDiagnosticInfo = true");

  if (pSettings->showAdvancedGIS)
    data.Set("showAdvancedGISOptions = true");

  data.Set("style = %i", pSettings->styleIndex);

  // Windows
  data.Set("window.position.x = %d", pSettings->window.xpos);
  data.Set("window.position.y = %d", pSettings->window.ypos);
  data.Set("window.width = %d", pSettings->window.width);
  data.Set("window.height = %d", pSettings->window.height);
  data.Set("window.maximized = %s", pSettings->window.maximized ? "true" : "false");
  data.Set("window.fullscreen = %s", pSettings->window.fullscreen ? "true" : "false");
  data.Set("window.touchscreenFriendly = %s", pSettings->window.touchscreenFriendly ? "true" : "false");

  data.Set("frames.scene = %s", pSettings->window.windowsOpen[vcdScene] ? "true" : "false");
  data.Set("frames.settings = %s", pSettings->window.windowsOpen[vcdSettings] ? "true" : "false");
  data.Set("frames.explorer = %s", pSettings->window.windowsOpen[vcdSceneExplorer] ? "true" : "false");

  // Camera
  data.Set("camera.moveSpeed = %f", pSettings->camera.moveSpeed);
  data.Set("camera.nearPlane = %f", pSettings->camera.nearPlane);
  data.Set("camera.farPlane = %f", pSettings->camera.farPlane);
  data.Set("camera.fieldOfView = %f", pSettings->camera.fieldOfView);
  data.Set("camera.lensId = %i", pSettings->camera.lensIndex);
  data.Set("camera.invertX = %s", pSettings->camera.invertX ? "true" : "false");
  data.Set("camera.invertY = %s", pSettings->camera.invertY ? "true" : "false");
  data.Set("camera.moveMode = %d", pSettings->camera.moveMode);
  data.Set("camera.cameraMouseBindings = [%d, %d, %d]", pSettings->camera.cameraMouseBindings[0], pSettings->camera.cameraMouseBindings[1], pSettings->camera.cameraMouseBindings[2]);

  // Visualization
  data.Set("visualization.mode = %d", pSettings->visualization.mode);
  data.Set("visualization.minIntensity = %d", pSettings->visualization.minIntensity);
  data.Set("visualization.maxIntensity = %d", pSettings->visualization.maxIntensity);

  // Post visualization - Edge Highlighting
  data.Set("postVisualization.edgeOutlines.enabled = %s", pSettings->postVisualization.edgeOutlines.enable ? "true" : "false");
  data.Set("postVisualization.edgeOutlines.width = %d", pSettings->postVisualization.edgeOutlines.width);
  data.Set("postVisualization.edgeOutlines.threshold = %f", pSettings->postVisualization.edgeOutlines.threshold);
  for (int i = 0; i < 4; i++)
    data.Set("postVisualization.edgeOutlines.colour[] = %f", pSettings->postVisualization.edgeOutlines.colour[i]);

  // Post visualization - Colour by height
  data.Set("postVisualization.colourByHeight.enabled = %s", pSettings->postVisualization.colourByHeight.enable ? "true" : "false");
  for (int i = 0; i < 4; i++)
    data.Set("postVisualization.colourByHeight.minColour[] = %f", pSettings->postVisualization.colourByHeight.minColour[i]);
  for (int i = 0; i < 4; i++)
    data.Set("postVisualization.colourByHeight.maxColour[] = %f", pSettings->postVisualization.colourByHeight.maxColour[i]);
  data.Set("postVisualization.colourByHeight.startHeight = %f", pSettings->postVisualization.colourByHeight.startHeight);
  data.Set("postVisualization.colourByHeight.endHeight = %f", pSettings->postVisualization.colourByHeight.endHeight);

  // Post visualization - Colour by depth
  data.Set("postVisualization.colourByDepth.enabled = %s", pSettings->postVisualization.colourByDepth.enable ? "true" : "false");
  for (int i = 0; i < 4; i++)
    data.Set("postVisualization.colourByDepth.colour[] = %f", pSettings->postVisualization.colourByDepth.colour[i]);
  data.Set("postVisualization.colourByDepth.startDepth = %f", pSettings->postVisualization.colourByDepth.startDepth);
  data.Set("postVisualization.colourByDepth.endDepth = %f", pSettings->postVisualization.colourByDepth.endDepth);

  // Post visualization - Contours
  data.Set("postVisualization.contours.enabled = %s", pSettings->postVisualization.contours.enable ? "true" : "false");
  for (int i = 0; i < 4; i++)
    data.Set("postVisualization.contours.colour[] = %f", pSettings->postVisualization.contours.colour[i]);
  data.Set("postVisualization.contours.distances = %f", pSettings->postVisualization.contours.distances);
  data.Set("postVisualization.contours.bandHeight = %f", pSettings->postVisualization.contours.bandHeight);

  // Map Tiles
  data.Set("maptiles.enabled = %s", pSettings->maptiles.mapEnabled ? "true" : "false");
  data.Set("maptiles.blendMode = %d", pSettings->maptiles.blendMode);
  data.Set("maptiles.transparency = %f", pSettings->maptiles.transparency);
  data.Set("maptiles.mapHeight = %f", pSettings->maptiles.mapHeight);

  // Docks
  vcSettings_SaveDocks(data);

  const char *pSettingsStr;

  if (data.Export(&pSettingsStr) == udR_Success)
  {
    char buffer[vcMaxPathLength];
    udSprintf(buffer, vcMaxPathLength, "%ssettings.json", pSettings->pSaveFilePath);

    success = SDL_filewrite(buffer, pSettingsStr, udStrlen(pSettingsStr));

    udFree(pSettingsStr);
  }

  return success;
}
