#include "vcState.h"

#include "vcRender.h"
#include "vcGIS.h"
#include "gl/vcGLState.h"
#include "gl/vcFramebuffer.h"

#include "vdkContext.h"

#include "imgui.h"
#include "imgui_ex/imgui_impl_sdl.h"
#include "imgui_ex/imgui_impl_gl.h"
#include "imgui_ex/imgui_dock.h"
#include "imgui_ex/imgui_udValue.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
#include "vcConvert.h"

#include "udPlatform/udFile.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#if UDPLATFORM_WINDOWS && !defined(NDEBUG)
#  include <crtdbg.h>
#  include <stdio.h>
#endif

struct vc3rdPartyLicenseText
{
  const char *pName;
  const char *pLicense;
};

vc3rdPartyLicenseText licenses[] = {
  { "Dear ImGui", R"license(The MIT License (MIT)

Copyright (c) 2014-2018 Omar Cornut

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.)license" },
  { "Dear ImGui - Minimal Docking Extension", R"license(This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>)license" },
  { "cURL", R"license(COPYRIGHT AND PERMISSION NOTICE

Copyright (c) 1996 - 2018, Daniel Stenberg, <daniel@haxx.se>, and many
contributors, see the THANKS file.

All rights reserved.

Permission to use, copy, modify, and distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright
notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN
NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of a copyright holder shall not
be used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization of the copyright holder.)license" },
  { "libSDL2", R"license(Simple DirectMedia Layer
Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.)license" },
  { "GLEW", R"license(The OpenGL Extension Wrangler Library
Copyright (C) 2002-2007, Milan Ikits <milan ikits[]ieee org>
Copyright (C) 2002-2007, Marcelo E. Magallon <mmagallo[]debian org>
Copyright (C) 2002, Lev Povalahev
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
* The name of the author may be used to endorse or promote products
  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.


Mesa 3-D graphics library
Version:  7.0

Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


Copyright (c) 2007 The Khronos Group Inc.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and/or associated documentation files (the
"Materials"), to deal in the Materials without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Materials, and to
permit persons to whom the Materials are furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Materials.

THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.)license" },
  { "Nothings/STB", R"license(Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.)license" },
};

struct vcColumnHeader
{
  const char* pLabel;
  float size;
};

void vcRenderWindow(vcState *pProgramState);
int vcMainMenuGui(vcState *pProgramState);

void vcSettings_LoadSettings(vcState *pProgramState, bool forceDefaults);
bool vcLogout(vcState *pProgramState);

int main(int /*argc*/, char ** /*args*/)
{
#if UDPLATFORM_WINDOWS && !defined(NDEBUG)
  _CrtMemState m1, m2, diff;
  _CrtMemCheckpoint(&m1);
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);
#endif //UDPLATFORM_WINDOWS && !defined(NDEBUG)

  SDL_GLContext glcontext = NULL;
  uint32_t windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  windowFlags |= SDL_WINDOW_FULLSCREEN;
#endif

  vcState programState = {};

  udFile_RegisterHTTP();

  // Icon parameters
  SDL_Surface *pIcon = nullptr;
  int iconWidth, iconHeight, iconBytesPerPixel;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  char FontPath[] = ASSETDIR "/NotoSansCJKjp-Regular.otf";
  char IconPath[] = ASSETDIR "EuclideonClientIcon.png";
  char EucWatermarkPath[] = ASSETDIR "EuclideonLogo.png";
#elif UDPLATFORM_OSX
  char FontPath[vcMaxPathLength] = "";
  char IconPath[vcMaxPathLength] = "";
  char EucWatermarkPath[vcMaxPathLength] = "";

  {
    char *pBasePath = SDL_GetBasePath();
    if (pBasePath == nullptr)
      pBasePath = SDL_strdup("./");

    udSprintf(FontPath, vcMaxPathLength, "%s%s", pBasePath, "NotoSansCJKjp-Regular.otf");
    udSprintf(IconPath, vcMaxPathLength, "%s%s", pBasePath, "EuclideonClientIcon.png");
    udSprintf(EucWatermarkPath, vcMaxPathLength, "%s%s", pBasePath, "EuclideonLogo.png");
    SDL_free(pBasePath);
  }
#else
  char FontPath[] = ASSETDIR "fonts/NotoSansCJKjp-Regular.otf";
  char IconPath[] = ASSETDIR "icons/EuclideonClientIcon.png";
  char EucWatermarkPath[] = ASSETDIR "icons/EuclideonLogo.png";
#endif
  unsigned char *pIconData = nullptr;
  unsigned char *pEucWatermarkData = nullptr;
  int pitch;
  long rMask, gMask, bMask, aMask;

  const float FontSize = 16.f;
  ImFontConfig fontCfg = ImFontConfig();

  // default values
  programState.settings.camera.moveMode = vcCMM_Plane;
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  // TODO: Query device and fill screen
  programState.sceneResolution.x = 1920;
  programState.sceneResolution.y = 1080;
  programState.onScreenControls = true;
#else
  programState.sceneResolution.x = 1280;
  programState.sceneResolution.y = 720;
  programState.onScreenControls = false;
#endif
  programState.camMatrix = udDouble4x4::identity();
  vcCamera_Create(&programState.pCamera);

  programState.settings.camera.moveSpeed = 3.f;
  programState.settings.camera.nearPlane = 0.5f;
  programState.settings.camera.farPlane = 10000.f;
  programState.settings.camera.fieldOfView = UD_PIf * 5.f / 18.f; // 50 degrees

  programState.vcModelList.Init(32);
  programState.lastModelLoaded = true;

  // default string values.
  udStrcpy(programState.serverURL, vcMaxPathLength, "http://vau-ubu-pro-001.euclideon.local");
  udStrcpy(programState.modelPath, vcMaxPathLength, "http://vau-win-van-001.euclideon.local/AdelaideCBD_2cm.uds");

  programState.settings.maptiles.mapEnabled = true;
  programState.settings.maptiles.mapHeight = 0.f;
  programState.settings.maptiles.transparency = 1.f;
  udStrcpy(programState.settings.maptiles.tileServerAddress, vcMaxPathLength, "http://10.4.0.151:8123");
  udStrcpy(programState.settings.resourceBase, vcMaxPathLength, "http://vau-win-van-001.euclideon.local");

  vcConvert_Init(&programState);

  Uint64 NOW;
  Uint64 LAST;

  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    goto epilogue;

  // Setup window
#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) != 0)
    goto epilogue;
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0) != 0)
    goto epilogue;
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) != 0)
    goto epilogue;
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) != 0)
    goto epilogue;
#endif

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

#ifdef GIT_BUILD
#define WINDOW_SUFFIX " (" UDSTRINGIFY(GIT_BUILD) " - " __DATE__ ") "
#else
#define WINDOW_SUFFIX " (DEV/DO NOT DISTRIBUTE - " __DATE__ ")"
#endif

  programState.pWindow = ImGui_ImplSDL2_CreateWindow("Euclideon Client" WINDOW_SUFFIX, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, programState.sceneResolution.x, programState.sceneResolution.y, windowFlags);
  if (!programState.pWindow)
    goto epilogue;

  pIconData = stbi_load(IconPath, &iconWidth, &iconHeight, &iconBytesPerPixel, 0);

  pitch = iconWidth * iconBytesPerPixel;
  pitch = (pitch + 3) & ~3;

  rMask = 0xFF << 0;
  gMask = 0xFF << 8;
  bMask = 0xFF << 16;
  aMask = (iconBytesPerPixel == 4) ? (0xFF << 24) : 0;

  if (pIconData != nullptr)
    pIcon = SDL_CreateRGBSurfaceFrom(pIconData, iconWidth, iconHeight, iconBytesPerPixel * 8, pitch, rMask, gMask, bMask, aMask);
  if(pIcon != nullptr)
    SDL_SetWindowIcon(programState.pWindow, pIcon);

  SDL_free(pIcon);

  glcontext = SDL_GL_CreateContext(programState.pWindow);
  if (!glcontext)
    goto epilogue;

  if (!vcGLState_Init(programState.pWindow, &programState.pDefaultFramebuffer))
    goto epilogue;

  ImGui::CreateContext();
  ImGui::GetIO().ConfigResizeWindowsFromEdges = true; // Fix for ImGuiWindowFlags_ResizeFromAnySide being removed
  vcSettings_LoadSettings(&programState, false);

  // setup watermark for background
  pEucWatermarkData = stbi_load(EucWatermarkPath, &iconWidth, &iconHeight, &iconBytesPerPixel, 0); // reusing the variables for width etc
  vcTexture_Create(&programState.pWatermarkTexture, iconWidth, iconHeight, pEucWatermarkData);

  if (!ImGuiGL_Init(programState.pWindow))
    goto epilogue;

  //Get ready...
  NOW = SDL_GetPerformanceCounter();
  LAST = 0;

  if (vcRender_Init(&(programState.pRenderContext), &(programState.settings), programState.pCamera, programState.sceneResolution) != udR_Success)
    goto epilogue;

  // Set back to default buffer, vcRender_Init calls vcRender_ResizeScene which calls vcCreateFramebuffer
  // which binds the 0th framebuffer this isn't valid on iOS when using UIKit.
  vcFramebuffer_Bind(programState.pDefaultFramebuffer);

  ImGui::GetIO().Fonts->AddFontFromFileTTF(FontPath, FontSize);

#if 1 // If load additional fonts
  fontCfg.MergeMode = true;

  ImGui::GetIO().Fonts->AddFontFromFileTTF(FontPath, FontSize, &fontCfg, ImGui::GetIO().Fonts->GetGlyphRangesKorean());
  ImGui::GetIO().Fonts->AddFontFromFileTTF(FontPath, FontSize, &fontCfg, ImGui::GetIO().Fonts->GetGlyphRangesJapanese());
  ImGui::GetIO().Fonts->AddFontFromFileTTF(FontPath, FontSize, &fontCfg, ImGui::GetIO().Fonts->GetGlyphRangesChineseFull());
  ImGui::GetIO().Fonts->AddFontFromFileTTF(FontPath, FontSize, &fontCfg, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
  ImGui::GetIO().Fonts->AddFontFromFileTTF(FontPath, FontSize, &fontCfg, ImGui::GetIO().Fonts->GetGlyphRangesThai());
#endif

  SDL_EnableScreenSaver();

  while (!programState.programComplete)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (!ImGui_ImplSDL2_ProcessEvent(&event))
      {
        if (event.type == SDL_WINDOWEVENT)
        {
          if (event.window.event == SDL_WINDOWEVENT_RESIZED)
          {
            programState.settings.window.width = event.window.data1;
            programState.settings.window.height = event.window.data2;
            vcGLState_ResizeBackBuffer(event.window.data1, event.window.data2);
          }
          else if (event.window.event == SDL_WINDOWEVENT_MOVED)
          {
            programState.settings.window.xpos = event.window.data1;
            programState.settings.window.ypos = event.window.data2;
          }
          else if (event.window.event == SDL_WINDOWEVENT_MAXIMIZED)
          {
            programState.settings.window.maximized = true;
          }
          else if (event.window.event == SDL_WINDOWEVENT_RESTORED)
          {
            programState.settings.window.maximized = false;
          }
        }
        else if (event.type == SDL_MULTIGESTURE)
        {
          // TODO: pinch to zoom
        }
        else if (event.type == SDL_DROPFILE && programState.hasContext)
        {
          if (!vcModel_AddToList(&programState, event.drop.file))
            vcConvert_AddFile(&programState, event.drop.file);
          //TODO: Display a message here that the file couldn't be opened...
        }
        else if (event.type == SDL_QUIT)
        {
          programState.programComplete = true;
        }
      }
    }

    LAST = NOW;
    NOW = SDL_GetPerformanceCounter();
    programState.deltaTime = double(NOW - LAST) / SDL_GetPerformanceFrequency();

    ImGuiGL_NewFrame(programState.pWindow);

    vcGLState_ResetState(true);
    vcRenderWindow(&programState);

    ImGui::Render();
    ImGuiGL_RenderDrawData(ImGui::GetDrawData());

    vcGLState_Present(programState.pWindow);

    if (ImGui::GetIO().WantSaveIniSettings)
      vcSettings_Save(&programState.settings);

    ImGui::GetIO().KeysDown[SDL_SCANCODE_BACKSPACE] = false;
  }

  vcSettings_Save(&programState.settings);
  ImGui::ShutdownDock();
  ImGui::DestroyContext();

epilogue:
  programState.projects.Destroy();
  ImGuiGL_DestroyDeviceObjects();
  vcConvert_Deinit(&programState);
  vcCamera_Destroy(&programState.pCamera);
  vcTexture_Destroy(&programState.pWatermarkTexture);
  free(pIconData);
  free(pEucWatermarkData);
  vcModel_UnloadList(&programState);
  programState.vcModelList.Deinit();
  vcRender_Destroy(&programState.pRenderContext);
  vdkContext_Disconnect(&programState.pVDKContext);

  vcGLState_Deinit();

#if UDPLATFORM_WINDOWS && !defined(NDEBUG)
  _CrtMemCheckpoint(&m2);
  if (_CrtMemDifference(&diff, &m1, &m2) && diff.lCounts[_NORMAL_BLOCK] > 0)
  {
    _CrtMemDumpAllObjectsSince(&m1);
    printf("%s\n", "Memory leaks found");
    return 1;
  }
#endif //UDPLATFORM_WINDOWS && !defined(NDEBUG)

  return 0;
}

void vcRenderSceneWindow(vcState *pProgramState)
{
  //Rendering
  ImVec2 size = ImGui::GetContentRegionAvail();
  ImVec2 windowPos = ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x, ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y);

  udDouble3 moveOffset = udDouble3::zero();

  if (size.x < 1 || size.y < 1)
    return;

  if (pProgramState->sceneResolution.x != size.x || pProgramState->sceneResolution.y != size.y) //Resize buffers
  {
    pProgramState->sceneResolution = udUInt2::create((uint32_t)size.x, (uint32_t)size.y);
    vcRender_ResizeScene(pProgramState->pRenderContext, pProgramState->sceneResolution.x, pProgramState->sceneResolution.y);

    // Set back to default buffer, vcRender_ResizeScene calls vcCreateFramebuffer which binds the 0th framebuffer
    // this isn't valid on iOS when using UIKit.
    vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer);
  }

  vcRenderData renderData = {};
  renderData.cameraMatrix = pProgramState->camMatrix;
  renderData.pCameraSettings = &pProgramState->settings.camera;
  renderData.pGISSpace = &pProgramState->gis;
  renderData.models.Init(32);

  ImGuiIO &io = ImGui::GetIO();
  renderData.mouse.x = (uint32_t)(io.MousePos.x - windowPos.x);
  renderData.mouse.y = (uint32_t)(io.MousePos.y - windowPos.y);

  for (size_t i = 0; i < pProgramState->vcModelList.length; ++i)
  {
    renderData.models.PushBack(&pProgramState->vcModelList[i]);
  }

  vcTexture *pTexture = vcRender_RenderScene(pProgramState->pRenderContext, renderData, pProgramState->pDefaultFramebuffer);

  renderData.models.Deinit();

  {
    pProgramState->worldMousePos = renderData.worldMousePos;
    pProgramState->pickingSuccess = renderData.pickingSuccess;

    ImGui::SetNextWindowPos(ImVec2(windowPos.x + size.x - 5.f, windowPos.y + 5.f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(200, 0), ImVec2(FLT_MAX, FLT_MAX)); // Set minimum width to include the header
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

    if (ImGui::Begin("Geographic Information", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
    {
      if (pProgramState->gis.SRID != 0 && pProgramState->gis.isProjected)
        ImGui::Text("SRID: %d", pProgramState->gis.SRID);
      else if (pProgramState->gis.SRID == 0)
        ImGui::Text("Not Geolocated");
      else
        ImGui::Text("Unsupported SRID: %d", pProgramState->gis.SRID);

      if (pProgramState->settings.showAdvancedGIS)
      {
        int newSRID = pProgramState->gis.SRID;
        if (ImGui::InputInt("Override SRID", &newSRID) && vcGIS_AcceptableSRID((vcSRID)newSRID))
        {
          if (vcGIS_ChangeSpace(&pProgramState->gis, (vcSRID)newSRID, &pProgramState->pCamera->position))
            vcModel_UpdateMatrix(pProgramState, nullptr); // Update all models to new zone
        }
      }

      if (pProgramState->settings.showDiagnosticInfo)
      {
        ImGui::Separator();
        ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
      }

      ImGui::Separator();
      if (ImGui::IsMousePosValid())
      {
        ImGui::Text("Mouse Position: (%.1f,%.1f)", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
        if (pProgramState->pickingSuccess)
        {
          ImGui::Text("Mouse World Pos (x/y/z): (%f,%f,%f)", renderData.worldMousePos.x, renderData.worldMousePos.y, renderData.worldMousePos.z);

          if (pProgramState->gis.isProjected)
          {
            udDouble3 mousePointInLatLong = udGeoZone_ToLatLong(pProgramState->gis.zone, renderData.worldMousePos);
            ImGui::Text("Mouse World Pos (L/L): (%f,%f)", mousePointInLatLong.x, mousePointInLatLong.y);
          }
        }

        ImGui::Text("Selected Pos (x/y/z): (%f,%f,%f)", pProgramState->currentMeasurePoint.x, pProgramState->currentMeasurePoint.y, pProgramState->currentMeasurePoint.z);
      }
      else
      {
        ImGui::Text("Mouse Position: <invalid>");
      }
    }

    ImGui::End();
  }

  // On Screen Camera Settings
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + 5.f, windowPos.y + 5.f), ImGuiCond_Always, ImVec2(0.f, 0.f));
    ImGui::SetNextWindowBgAlpha(0.5f);
    if (ImGui::Begin("Camera Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::InputScalarN("Camera Position", ImGuiDataType_Double, &pProgramState->pCamera->position.x, 3);
      ImGui::InputScalarN("Camera Rotation", ImGuiDataType_Double, &pProgramState->pCamera->yprRotation.x, 3);

      if (pProgramState->gis.isProjected)
      {
        udDouble3 cameraLatLong = udGeoZone_ToCartesian(pProgramState->gis.zone, pProgramState->camMatrix.axis.t.toVector3());
        ImGui::Text("Lat: %.7f, Long: %.7f, Alt: %.2fm", cameraLatLong.x, cameraLatLong.y, cameraLatLong.z);
      }
      ImGui::RadioButton("Plane", (int*)&pProgramState->settings.camera.moveMode, vcCMM_Plane);
      ImGui::SameLine();
      ImGui::RadioButton("Heli", (int*)&pProgramState->settings.camera.moveMode, vcCMM_Helicopter);

      if (ImGui::SliderFloat("Move Speed", &(pProgramState->settings.camera.moveSpeed), vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed, "%.3f m/s", 4.f))
        pProgramState->settings.camera.moveSpeed = udMax(pProgramState->settings.camera.moveSpeed, 0.f);

    }

    ImGui::End();
  }

  // On Screen Controls Overlay
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + 5.f, windowPos.y + size.y - 5.f), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

    if (pProgramState->onScreenControls)
    {
      if (ImGui::Begin("OnScreenControls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
      {
        ImGui::SetWindowSize(ImVec2(175, 150));
        ImGui::Text("Controls");

        ImGui::Separator();


        ImGui::Columns(2, NULL, false);

        ImGui::SetColumnWidth(0, 50);

        double forward = 0;
        double right = 0;
        float vertical = 0;

        ImGui::PushID("oscUDSlider");

        if(ImGui::VSliderFloat("",ImVec2(40,100), &vertical, -1, 1, "U/D"))
          vertical = udClamp(vertical, -1.f, 1.f);

        ImGui::PopID();

        ImGui::NextColumn();

        ImGui::Button("Move Camera", ImVec2(100,100));
        if (ImGui::IsItemActive())
        {
          // Draw a line between the button and the mouse cursor
          ImDrawList* draw_list = ImGui::GetWindowDrawList();
          draw_list->PushClipRectFullScreen();
          draw_list->AddLine(io.MouseClickedPos[0], io.MousePos, ImGui::GetColorU32(ImGuiCol_Button), 4.0f);
          draw_list->PopClipRect();

          ImVec2 value_raw = ImGui::GetMouseDragDelta(0, 0.0f);

          forward = -1.f * value_raw.y / vcSL_OSCPixelRatio;
          right = value_raw.x / vcSL_OSCPixelRatio;
        }

        moveOffset += udDouble3::create(right, forward, (double)vertical);

        ImGui::Columns(1);
      }

      ImGui::End();
    }
    ImVec2 uv0 = ImVec2(0, 0);
    ImVec2 uv1 = ImVec2(1, 1);
#if GRAPHICS_API_OPENGL
    uv1.y = -1;
#endif
    ImGui::ImageButton(pTexture, size, uv0, uv1, 0);

    vcCamera_HandleSceneInput(pProgramState, moveOffset);
  }
}

int vcMainMenuGui(vcState *pProgramState)
{
  int menuHeight = 0;

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("System"))
    {
      if (ImGui::MenuItem("Logout"))
      {
        if (!vcLogout(pProgramState))
          ImGui::OpenPopup("Logout Error");
      }

      if (ImGui::MenuItem("Restore Defaults", nullptr))
        vcSettings_LoadSettings(pProgramState, true);

      ImGui::MenuItem("About", nullptr, &pProgramState->popupTrigger[vcPopup_About]);

#if UDPLATFORM_WINDOWS || UDPLATFORM_LINUX || UDPLATFORM_OSX
      if (ImGui::MenuItem("Quit", "Alt+F4"))
        pProgramState->programComplete = true;
#endif

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Windows"))
    {
      ImGui::MenuItem("Scene", nullptr, &pProgramState->settings.window.windowsOpen[vcdScene]);
      ImGui::MenuItem("Scene Explorer", nullptr, &pProgramState->settings.window.windowsOpen[vcdSceneExplorer]);
      ImGui::MenuItem("Settings", nullptr, &pProgramState->settings.window.windowsOpen[vcdSettings]);
      ImGui::MenuItem("Convert", nullptr, &pProgramState->settings.window.windowsOpen[vcdConvert]);
      ImGui::Separator();
      ImGui::EndMenu();
    }

    udValueArray *pProjectList = pProgramState->projects.Get("projects").AsArray();
    if (ImGui::BeginMenu("Projects", pProjectList != nullptr && pProjectList->length > 0 && !udStrEqual(pProgramState->settings.resourceBase, "")))
    {
      for (size_t i = 0; i < pProjectList->length; ++i)
      {
        if (ImGui::MenuItem(pProjectList->GetElement(i)->Get("name").AsString("<Unnamed>"), nullptr, nullptr))
        {
          vcModel_UnloadList(pProgramState);

          for (size_t j = 0; j < pProjectList->GetElement(i)->Get("models").ArrayLength(); ++j)
          {
            char buffer[vcMaxPathLength];
            udSprintf(buffer, vcMaxPathLength, "%s/%s", pProgramState->settings.resourceBase, pProjectList->GetElement(i)->Get("models[%d]", j).AsString());
            vcModel_AddToList(pProgramState, buffer);
          }
        }
      }

      ImGui::EndMenu();
    }

    menuHeight = (int)ImGui::GetWindowSize().y;

    ImGui::EndMainMenuBar();
  }

  return menuHeight;
}

void vcRenderWindow(vcState *pProgramState)
{
  vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer);
  vcFramebuffer_Clear(pProgramState->pDefaultFramebuffer, 0xFF000000);

  vdkError err;
  SDL_Keymod modState = SDL_GetModState();

  //keyboard/mouse handling
  if (ImGui::IsKeyReleased(SDL_SCANCODE_F11))
  {
    pProgramState->settings.window.fullscreen = !pProgramState->settings.window.fullscreen;
    if (pProgramState->settings.window.fullscreen)
      SDL_SetWindowFullscreen(pProgramState->pWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
    else
      SDL_SetWindowFullscreen(pProgramState->pWindow, 0);
  }

  ImGuiIO& io = ImGui::GetIO(); // for future key commands as well
  ImVec2 size = io.DisplaySize;

#if UDPLATFORM_WINDOWS
  if (io.KeyAlt && ImGui::IsKeyPressed(SDL_SCANCODE_F4))
    pProgramState->programComplete = true;
#endif

  //end keyboard/mouse handling

  if (pProgramState->hasContext)
  {
    float menuHeight = (float)vcMainMenuGui(pProgramState);
    ImGui::RootDock(ImVec2(0, menuHeight), ImVec2(size.x, size.y - menuHeight));
  }
  else
  {
    ImGui::RootDock(ImVec2(0, 0), ImVec2(size.x, size.y));
  }

  if (!pProgramState->hasContext)
  {
    ImGui::SetNextWindowBgAlpha(0.f);
    ImGui::SetNextWindowPos(ImVec2(size.x - 5, size.y - 5), ImGuiCond_Always, ImVec2(1.0f, 1.0f));

    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::Begin("Watermark", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
    ImGui::Image(pProgramState->pWatermarkTexture, ImVec2(301, 161), ImVec2(0, 0), ImVec2(1, 1));
    ImGui::End();
    ImGui::PopStyleColor();

    ImGui::SetNextWindowSize(ImVec2(500, 150));
    if (ImGui::Begin("Login", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
    {
      static const char *pErrorMessage = nullptr;
      if (pErrorMessage != nullptr)
        ImGui::Text("%s", pErrorMessage);

      ImGui::InputText("ServerURL", pProgramState->serverURL, vcMaxPathLength);
      ImGui::InputText("Username", pProgramState->username, vcMaxPathLength);
      ImGui::InputText("Password", pProgramState->password, vcMaxPathLength, ImGuiInputTextFlags_Password | ImGuiInputTextFlags_CharsNoBlank);

      if (ImGui::Button("Login!"))
      {
        err = vdkContext_Connect(&pProgramState->pVDKContext, pProgramState->serverURL, "ClientSample");
        if (err != vE_Success)
        {
          pErrorMessage = "Could not connect to server...";
        }
        else
        {
          err = vdkContext_Login(pProgramState->pVDKContext, pProgramState->username, pProgramState->password);
          if (err != vE_Success)
          {
            pErrorMessage = "Could not log in...";
          }
          else
          {
            err = vdkContext_GetLicense(pProgramState->pVDKContext, vdkLT_Render);
            if (err != vE_Success)
            {
              pErrorMessage = "Could not get license...";
            }
            else
            {
              //Context Login successful
              vcRender_CreateTerrain(pProgramState->pRenderContext, &pProgramState->settings);
              vcRender_SetVaultContext(pProgramState->pRenderContext, pProgramState->pVDKContext);

              void *pProjData = nullptr;
              if (udFile_Load(udTempStr("%s/api/dev/projects", pProgramState->serverURL), &pProjData) == udR_Success)
              {
                pProgramState->projects.Parse((char*)pProjData);
                udFree(pProjData);
              }

              pProgramState->hasContext = true;
            }
          }
        }
      }
    }

    ImGui::End();
  }
  else
  {
    if (ImGui::BeginPopupModal("Logout Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::Text("Error logging out! (0x00)");
      if (ImGui::Button("OK", ImVec2(120, 0)))
        ImGui::CloseCurrentPopup();

      ImGui::SetItemDefaultFocus();
      ImGui::EndPopup();
    }

    if (ImGui::BeginDock("Scene", &pProgramState->settings.window.windowsOpen[vcdScene], ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus))
      vcRenderSceneWindow(pProgramState);

    ImGui::EndDock();

    if (ImGui::BeginDock("Scene Explorer", &pProgramState->settings.window.windowsOpen[vcdSceneExplorer]))
    {
      ImGui::InputText("Model Path", pProgramState->modelPath, vcMaxPathLength);
      if (ImGui::Button("Load Model!"))
        vcModel_AddToList(pProgramState, pProgramState->modelPath);

      if (!pProgramState->lastModelLoaded)
        ImGui::Text("Invalid File/Not Found...");

      // Models

      int minMaxColumnSize[][2] =
      {
        {50,500},
        {40,40},
        {35,35},
        {1,1}
      };

      vcColumnHeader headers[] =
      {
        { "Model List", 400 },
        { "Show", 40 },
        { "Del", 35 }, // unload column
        { "", 1 } // Null Column at end
      };

      int col1Size = (int)ImGui::GetContentRegionAvailWidth();
      col1Size -= 40 + 35; // subtract size of two buttons

      if (col1Size > minMaxColumnSize[0][1])
        col1Size = minMaxColumnSize[0][1];

      if (col1Size < minMaxColumnSize[0][0])
        col1Size = minMaxColumnSize[0][0];

      headers[0].size = (float)col1Size;


      ImGui::Columns((int)udLengthOf(headers), "ModelTableColumns", true);
      ImGui::Separator();

      float offset = 0.f;
      for (size_t i = 0; i < UDARRAYSIZE(headers); ++i)
      {

        ImGui::Text("%s", headers[i].pLabel);
        ImGui::SetColumnOffset(-1, offset);
        offset += headers[i].size;
        ImGui::NextColumn();
      }

      ImGui::Separator();
      // Table Contents

      for (size_t i = 0; i < pProgramState->vcModelList.length; ++i)
      {
        // Column 1 - Model
        char modelLabelID[32] = "";
        udSprintf(modelLabelID, UDARRAYSIZE(modelLabelID), "ModelLabel%i", i);
        ImGui::PushID(modelLabelID);
        if (ImGui::Selectable(pProgramState->vcModelList[i].modelPath, pProgramState->vcModelList[i].modelSelected))
        {
          if ((modState & KMOD_CTRL) == 0)
          {
            for (size_t j = 0; j < pProgramState->vcModelList.length; ++j)
            {
              pProgramState->vcModelList[j].modelSelected = false;
            }

            pProgramState->numSelectedModels = 0;
          }

          if (modState & KMOD_SHIFT)
          {
            size_t startInd = udMin(i, pProgramState->prevSelectedModel);
            size_t endInd = udMax(i, pProgramState->prevSelectedModel);
            for (size_t j = startInd; j <= endInd; ++j)
            {
              pProgramState->vcModelList[j].modelSelected = true;
              pProgramState->numSelectedModels++;
            }
          }
          else
          {
            pProgramState->vcModelList[i].modelSelected = !pProgramState->vcModelList[i].modelSelected;
            pProgramState->numSelectedModels += pProgramState->vcModelList[i].modelSelected ? 1 : 0;
          }

          pProgramState->prevSelectedModel = i;
        }

        if (ImGui::BeginPopupContextItem(modelLabelID))
        {
          if (ImGui::Checkbox("Flip Y/Z Up", &pProgramState->vcModelList[i].flipYZ)) //Technically this is a rotation around X actually...
            vcModel_UpdateMatrix(pProgramState, &pProgramState->vcModelList[i]);

          if (ImGui::Selectable("Properties", false))
          {
            pProgramState->popupTrigger[vcPopup_ModelProperties] = true;
            pProgramState->selectedModelProperties.index = i;
            ImGui::CloseCurrentPopup();
          }
          ImGui::EndPopup();
        }

        if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered())
          vcModel_MoveToModelProjection(pProgramState, &pProgramState->vcModelList[i]);

        ImVec2 textSize = ImGui::CalcTextSize(pProgramState->vcModelList[i].modelPath);
        if (ImGui::IsItemHovered() && (textSize.x >= headers[0].size))
          ImGui::SetTooltip("%s", pProgramState->vcModelList[i].modelPath);

        ImGui::PopID();
        ImGui::NextColumn();
        // Column 2 - Visible
        char checkboxID[32] = "";
        udSprintf(checkboxID, UDARRAYSIZE(checkboxID), "ModelVisibleCheckbox%i", i);
        ImGui::PushID(checkboxID);
        if (ImGui::Checkbox("", &(pProgramState->vcModelList[i].modelVisible)) && pProgramState->vcModelList[i].modelSelected && pProgramState->numSelectedModels > 1)
        {
          for (size_t j = 0; j < pProgramState->vcModelList.length; ++j)
          {
            if (pProgramState->vcModelList[j].modelSelected)
              pProgramState->vcModelList[j].modelVisible = pProgramState->vcModelList[i].modelVisible;
          }
        }

        ImGui::PopID();
        ImGui::NextColumn();
        // Column 3 - Unload Model
        char unloadModelID[32] = "";
        udSprintf(unloadModelID, UDARRAYSIZE(unloadModelID), "UnloadModelButton%i", i);
        ImGui::PushID(unloadModelID);
        if (ImGui::Button("X", ImVec2(20, 20)))
        {
          if (pProgramState->numSelectedModels > 1 && pProgramState->vcModelList[i].modelSelected) // if multiple selected and removed
          {
            //unload selected models
            for (size_t j = 0; j < pProgramState->vcModelList.length; ++j)
            {
              if (pProgramState->vcModelList[j].modelSelected)
              {
                // unload model
                err = vdkModel_Unload(pProgramState->pVDKContext, &(pProgramState->vcModelList[j].pVaultModel));
                if (err != vE_Success)
                  goto epilogue;

                pProgramState->vcModelList.RemoveAt(j);

                pProgramState->lastModelLoaded = true;
                j--;
              }
            }

            i = (pProgramState->numSelectedModels > i) ? 0 : (i - pProgramState->numSelectedModels);
          }
          else
          {
            // unload model
            err = vdkModel_Unload(pProgramState->pVDKContext, &(pProgramState->vcModelList[i].pVaultModel));
            if (err != vE_Success)
              goto epilogue;

            pProgramState->vcModelList.RemoveAt(i);

            pProgramState->lastModelLoaded = true;
            i--;
          }
        }

        ImGui::PopID();
        ImGui::NextColumn();
        // Null Column
        ImGui::NextColumn();
      }

      ImGui::Columns(1);
      // End Models
    }

    ImGui::EndDock();


    if (ImGui::BeginDock("Convert", &pProgramState->settings.window.windowsOpen[vcdConvert]))
      vcConvert_ShowUI(pProgramState);

    ImGui::EndDock();

    if (ImGui::BeginDock("Settings", &pProgramState->settings.window.windowsOpen[vcdSettings]))
    {
      if (ImGui::CollapsingHeader("Appearance##Settings"))
      {
        if (ImGui::Combo("Theme", &pProgramState->settings.styleIndex, "Classic\0Dark\0Light\0"))
        {
          switch (pProgramState->settings.styleIndex)
          {
          case 0: ImGui::StyleColorsClassic(); break;
          case 1: ImGui::StyleColorsDark(); break;
          case 2: ImGui::StyleColorsLight(); break;
          }
        }

        ImGui::Checkbox("Show Diagnostic Information", &pProgramState->settings.showDiagnosticInfo);
        ImGui::Checkbox("Show Advanced GIS Settings", &pProgramState->settings.showAdvancedGIS);
      }

      if (ImGui::CollapsingHeader("Input & Controls##Settings"))
      {
        ImGui::Checkbox("On Screen Controls", &pProgramState->onScreenControls);

        if (ImGui::Checkbox("Touch Friendly UI", &pProgramState->settings.window.touchscreenFriendly))
        {
          ImGuiStyle& style = ImGui::GetStyle();
          style.TouchExtraPadding = pProgramState->settings.window.touchscreenFriendly ? ImVec2(4, 4) : ImVec2();
        }

        ImGui::Checkbox("Invert X-axis", &pProgramState->settings.camera.invertX);
        ImGui::Checkbox("Invert Y-axis", &pProgramState->settings.camera.invertY);

        ImGui::Text("Mouse Pivot Bindings");
        const char *mouseModes[] = { "Tumble", "Orbit", "Pan" };

        int mouseBindingIndex = pProgramState->settings.camera.cameraMouseBindings[0];
        ImGui::Combo("Left", &mouseBindingIndex, mouseModes, (int)udLengthOf(mouseModes));
        pProgramState->settings.camera.cameraMouseBindings[0] = (vcCameraPivotMode)mouseBindingIndex;

        mouseBindingIndex = pProgramState->settings.camera.cameraMouseBindings[2];
        ImGui::Combo("Middle", &mouseBindingIndex, mouseModes, (int)udLengthOf(mouseModes));
        pProgramState->settings.camera.cameraMouseBindings[2] = (vcCameraPivotMode)mouseBindingIndex;

        mouseBindingIndex = pProgramState->settings.camera.cameraMouseBindings[1];
        ImGui::Combo("Right", &mouseBindingIndex, mouseModes, (int)udLengthOf(mouseModes));
        pProgramState->settings.camera.cameraMouseBindings[1] = (vcCameraPivotMode)mouseBindingIndex;
      }

      if (ImGui::CollapsingHeader("Viewport##Settings"))
      {
        if (ImGui::SliderFloat("Near Plane", &pProgramState->settings.camera.nearPlane, vcSL_CameraNearPlaneMin, vcSL_CameraNearPlaneMax, "%.3fm", 2.f))
        {
          pProgramState->settings.camera.nearPlane = udClamp(pProgramState->settings.camera.nearPlane, vcSL_CameraNearPlaneMin, vcSL_CameraNearPlaneMax);
          pProgramState->settings.camera.farPlane = udMin(pProgramState->settings.camera.farPlane, pProgramState->settings.camera.nearPlane * vcSL_CameraNearFarPlaneRatioMax);
        }

        if (ImGui::SliderFloat("Far Plane", &pProgramState->settings.camera.farPlane, vcSL_CameraFarPlaneMin, vcSL_CameraFarPlaneMax, "%.3fm", 2.f))
        {
          pProgramState->settings.camera.farPlane = udClamp(pProgramState->settings.camera.farPlane, vcSL_CameraFarPlaneMin, vcSL_CameraFarPlaneMax);
          pProgramState->settings.camera.nearPlane = udMax(pProgramState->settings.camera.nearPlane, pProgramState->settings.camera.farPlane / vcSL_CameraNearFarPlaneRatioMax);
        }

        //const char *pLensOptions = " Custom FoV\0 7mm\0 11mm\0 15mm\0 24mm\0 30mm\0 50mm\0 70mm\0 100mm\0";
        if (ImGui::Combo("Camera Lens (fov)", &pProgramState->settings.camera.lensIndex, vcCamera_GetLensNames(), vcLS_TotalLenses))
        {
          switch (pProgramState->settings.camera.lensIndex)
          {
          case vcLS_Custom:
            /*Custom FoV*/
            break;
          case vcLS_15mm:
            pProgramState->settings.camera.fieldOfView = vcLens15mm;
            break;
          case vcLS_24mm:
            pProgramState->settings.camera.fieldOfView = vcLens24mm;
            break;
          case vcLS_30mm:
            pProgramState->settings.camera.fieldOfView = vcLens30mm;
            break;
          case vcLS_50mm:
            pProgramState->settings.camera.fieldOfView = vcLens50mm;
            break;
          case vcLS_70mm:
            pProgramState->settings.camera.fieldOfView = vcLens70mm;
            break;
          case vcLS_100mm:
            pProgramState->settings.camera.fieldOfView = vcLens100mm;
            break;
          }
        }

        if (pProgramState->settings.camera.lensIndex == vcLS_Custom)
        {
          float fovDeg = UD_RAD2DEGf(pProgramState->settings.camera.fieldOfView);
          if (ImGui::SliderFloat("Field Of View", &fovDeg, vcSL_CameraFieldOfViewMin, vcSL_CameraFieldOfViewMax, "%.0f Degrees"))
            pProgramState->settings.camera.fieldOfView = UD_DEG2RADf(udClamp(fovDeg, vcSL_CameraFieldOfViewMin, vcSL_CameraFieldOfViewMax));
        }
      }

      if (ImGui::CollapsingHeader("Maps & Elevation##Settings"))
      {
        ImGui::Checkbox("Map Tiles", &pProgramState->settings.maptiles.mapEnabled);

        if (pProgramState->settings.maptiles.mapEnabled)
        {
          ImGui::InputText("Tile Server", pProgramState->settings.maptiles.tileServerAddress, vcMaxPathLength);

          ImGui::SliderFloat("Map Height", &pProgramState->settings.maptiles.mapHeight, -1000.f, 1000.f, "%.3fm", 2.f);

          const char* blendModes[] = { "Hybrid", "Overlay" };
          if (ImGui::BeginCombo("Blending", blendModes[pProgramState->settings.maptiles.blendMode]))
          {
            for (size_t n = 0; n < UDARRAYSIZE(blendModes); ++n)
            {
              bool isSelected = (pProgramState->settings.maptiles.blendMode == n);

              if (ImGui::Selectable(blendModes[n], isSelected))
                pProgramState->settings.maptiles.blendMode = (vcMapTileBlendMode)n;

              if (isSelected)
                ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
          }

          if (ImGui::SliderFloat("Transparency", &pProgramState->settings.maptiles.transparency, 0.f, 1.f, "%.3f"))
            pProgramState->settings.maptiles.transparency = udClamp(pProgramState->settings.maptiles.transparency, 0.f, 1.f);

          if (ImGui::Button("Set to Camera Height"))
            pProgramState->settings.maptiles.mapHeight = (float)pProgramState->camMatrix.axis.t.z;
        }
      }

      if (ImGui::CollapsingHeader("Visualization##Settings"))
      {
        const char *visualizationModes[] = { "Colour", "Intensity", "Classification" };
        ImGui::Combo("Display Mode", (int*)&pProgramState->settings.visualization.mode, visualizationModes, (int)udLengthOf(visualizationModes));

        if (pProgramState->settings.visualization.mode == vcVM_Intensity)
        {
          // Temporary until https://github.com/ocornut/imgui/issues/467 is resolved, then use commented out code below
          float temp[] = { (float)pProgramState->settings.visualization.minIntensity, (float)pProgramState->settings.visualization.maxIntensity };
          ImGui::SliderFloat("Min Intensity", &temp[0], 0.f, temp[1], "%.0f");
          ImGui::SliderFloat("Max Intensity", &temp[1], temp[0], 65535.f, "%.0f");
          pProgramState->settings.visualization.minIntensity = (int)temp[0];
          pProgramState->settings.visualization.maxIntensity = (int)temp[1];

          //ImGui::SliderInt("Min Intensity", &pProgramState->settings.visualization.minIntensity, 0, pProgramState->settings.visualization.maxIntensity);
          //ImGui::SliderInt("Max Intensity", &pProgramState->settings.visualization.maxIntensity, pProgramState->settings.visualization.minIntensity, 65535);
        }

        // Post visualization - Edge Highlighting
        ImGui::Checkbox("Enable Edge Highlighting", &pProgramState->settings.postVisualization.edgeOutlines.enable);
        if (pProgramState->settings.postVisualization.edgeOutlines.enable)
        {
          ImGui::SliderInt("Edge Highlighting Width", &pProgramState->settings.postVisualization.edgeOutlines.width, 1, 100);

          // TODO: Make this less awful. 0-100 would make more sense than 0.0001 to 0.001.
          ImGui::SliderFloat("Edge Highlighting Threshold", &pProgramState->settings.postVisualization.edgeOutlines.threshold, 0.001f, 10.0f, "%.3f");
          ImGui::ColorEdit4("Edge Highlighting Colour", &pProgramState->settings.postVisualization.edgeOutlines.colour.x);
        }

        // Post visualization - Colour by Height
        ImGui::Checkbox("Enable Colour by Height", &pProgramState->settings.postVisualization.colourByHeight.enable);
        if (pProgramState->settings.postVisualization.colourByHeight.enable)
        {
          ImGui::ColorEdit4("Colour by Height Start Colour", &pProgramState->settings.postVisualization.colourByHeight.minColour.x);
          ImGui::ColorEdit4("Colour by Height End Colour", &pProgramState->settings.postVisualization.colourByHeight.maxColour.x);

          // TODO: Set min/max to the bounds of the model? Currently set to 0m -> 1km with accuracy of 1mm
          ImGui::SliderFloat("Colour by Height Start Height", &pProgramState->settings.postVisualization.colourByHeight.startHeight, 0.f, 1000.f, "%.3f");
          ImGui::SliderFloat("Colour by Height End Height", &pProgramState->settings.postVisualization.colourByHeight.endHeight, 0.f, 1000.f, "%.3f");
        }

        // Post visualization - Colour by Depth
        ImGui::Checkbox("Enable Colour by Depth", &pProgramState->settings.postVisualization.colourByDepth.enable);
        if (pProgramState->settings.postVisualization.colourByDepth.enable)
        {
          ImGui::ColorEdit4("Colour by Depth Colour", &pProgramState->settings.postVisualization.colourByDepth.colour.x);

          // TODO: Find better min and max values? Currently set to 0m -> 1km with accuracy of 1mm
          ImGui::SliderFloat("Colour by Depth Start Depth", &pProgramState->settings.postVisualization.colourByDepth.startDepth, 0.f, 1000.f, "%.3f");
          ImGui::SliderFloat("Colour by Depth End Depth", &pProgramState->settings.postVisualization.colourByDepth.endDepth, 0.f, 1000.f, "%.3f");
        }

        // Post visualization - Contours
        ImGui::Checkbox("Enable Contours", &pProgramState->settings.postVisualization.contours.enable);
        if (pProgramState->settings.postVisualization.contours.enable)
        {
          ImGui::ColorEdit4("Contours Colour", &pProgramState->settings.postVisualization.contours.colour.x);

          // TODO: Find better min and max values? Currently set to 0m -> 1km with accuracy of 1mm
          ImGui::SliderFloat("Contours Distances", &pProgramState->settings.postVisualization.contours.distances, 0.f, 1000.f, "%.3f");
          ImGui::SliderFloat("Contours Band Height", &pProgramState->settings.postVisualization.contours.bandHeight, 0.f, 1000.f, "%.3f");
        }
      }
    }

    ImGui::EndDock();

    //Handle popups produced when vdkContext exists

    if (pProgramState->popupTrigger[vcPopup_ModelProperties])
    {
      ImGui::OpenPopup("Model Properties");

      pProgramState->selectedModelProperties.pMetadata = pProgramState->vcModelList[pProgramState->selectedModelProperties.index].pMetadata;

      const char *pWatermark = pProgramState->selectedModelProperties.pMetadata->Get("Watermark").AsString();
      if (pWatermark)
      {
        uint8_t *pImage = nullptr;
        size_t imageLen = 0;
        if (udBase64Decode(&pImage, &imageLen, pWatermark) == udR_Success)
        {
          int imageWidth, imageHeight, imageChannels;
          unsigned char *pImageData = stbi_load_from_memory(pImage, (int)imageLen, &imageWidth, &imageHeight, &imageChannels, 4);
          pProgramState->selectedModelProperties.watermarkWidth = imageWidth;
          pProgramState->selectedModelProperties.watermarkHeight = imageHeight;
          vcTexture_Create(&pProgramState->selectedModelProperties.pWatermarkTexture, imageWidth, imageHeight, pImageData, vcTextureFormat_RGBA8, vcTFM_Nearest, false);
          STBI_FREE(pImageData);
        }

        udFree(pImage);
      }

      ImGui::SetNextWindowSize(ImVec2(400, 600));

      pProgramState->popupTrigger[vcPopup_ModelProperties] = false;
    }

    if (ImGui::BeginPopupModal("Model Properties", NULL))
    {
      ImGui::Text("File:");

      ImGui::TextWrapped("  %s", pProgramState->vcModelList[pProgramState->selectedModelProperties.index].modelPath);

      ImGui::Separator();

      if (pProgramState->selectedModelProperties.pMetadata == nullptr)
      {
        ImGui::Text("No model information found.");
      }
      else
      {
        vcImGuiValueTreeObject(pProgramState->selectedModelProperties.pMetadata);
        ImGui::Separator();

        if (pProgramState->selectedModelProperties.pWatermarkTexture != nullptr)
        {
          ImGui::Text("Watermark");

          ImVec2 imageSize = ImVec2((float)pProgramState->selectedModelProperties.watermarkWidth, (float)pProgramState->selectedModelProperties.watermarkHeight);
          ImVec2 imageLimits = ImVec2(ImGui::GetContentRegionAvailWidth(), 100.f);

          if (imageSize.y > imageLimits.y)
          {
            imageSize.x *= imageLimits.y / imageSize.y;
            imageSize.y = imageLimits.y;
          }

          if (imageSize.x > imageLimits.x)
          {
            imageSize.y *= imageLimits.x / imageSize.x;
            imageSize.x = imageLimits.x;
          }

          ImGui::Image((ImTextureID)(size_t)pProgramState->selectedModelProperties.pWatermarkTexture, imageSize);
          ImGui::Separator();
        }
      }

      if (ImGui::Button("Close"))
      {
        if (pProgramState->selectedModelProperties.pWatermarkTexture != nullptr)
          vcTexture_Destroy(&pProgramState->selectedModelProperties.pWatermarkTexture);

        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    if (pProgramState->popupTrigger[vcPopup_About])
    {
      ImGui::OpenPopup("About");
      pProgramState->popupTrigger[vcPopup_About] = false;
      ImGui::SetNextWindowSize(ImVec2(500, 600));
    }

    if (ImGui::BeginPopupModal("About"))
    {
      ImGui::Columns(2, NULL, false);
      ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 100.f);
      ImGui::Text("Euclideon Client");

      ImGui::NextColumn();
      if (ImGui::Button("Close", ImVec2(-1, 0)))
        ImGui::CloseCurrentPopup();

      ImGui::Columns(1);

      ImGui::Separator();

      ImGui::Text("3rd Party License Information");
      ImGui::Spacing();
      ImGui::Spacing();

      ImGui::BeginChild("Licenses");
      for (int i = 0; i < (int)UDARRAYSIZE(licenses); i++)
      {
        ImGui::Text("%s\n%s", licenses[i].pName, licenses[i].pLicense);
        ImGui::Separator();
      }
      ImGui::EndChild();

      ImGui::EndPopup();
    }
  }

epilogue:
  //TODO: Cleanup
  return;
}

void vcSettings_LoadSettings(vcState *pProgramState, bool forceDefaults)
{
  if (vcSettings_Load(&pProgramState->settings, forceDefaults))
  {
#if UDPLATFORM_WINDOWS || UDPLATFORM_LINUX || UDPLATFORM_OSX
    if (pProgramState->settings.window.fullscreen)
      SDL_SetWindowFullscreen(pProgramState->pWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
    else
      SDL_SetWindowFullscreen(pProgramState->pWindow, 0);

    if (pProgramState->settings.window.maximized)
      SDL_MaximizeWindow(pProgramState->pWindow);
    else
      SDL_RestoreWindow(pProgramState->pWindow);

    SDL_SetWindowPosition(pProgramState->pWindow, pProgramState->settings.window.xpos, pProgramState->settings.window.ypos);
    //SDL_SetWindowSize(pProgramState->pWindow, pProgramState->settings.window.width, pProgramState->settings.window.height);
#endif
  }
}


bool vcLogout(vcState *pProgramState)
{
  bool success = true;

  success &= vcModel_UnloadList(pProgramState);
  success &= (vcRender_DestroyTerrain(pProgramState->pRenderContext) == udR_Success);

  memset(&pProgramState->gis, 0, sizeof(vcGISSpace));

  success = success && vdkContext_Logout(pProgramState->pVDKContext) == vE_Success;
  pProgramState->hasContext = !success;

  return success;
}
