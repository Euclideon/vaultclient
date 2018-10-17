#include "vcState.h"

#include "vcRender.h"
#include "vcGIS.h"
#include "gl/vcGLState.h"
#include "gl/vcFramebuffer.h"

#include "vdkContext.h"
#include "vdkServerAPI.h"

#include <chrono>

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

# undef main
# define main ClientMain
int main(int argc, char **args);

int SDL_main(int argc, char **args)
{
  _CrtMemState m1, m2, diff;
  _CrtMemCheckpoint(&m1);
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);

  int ret = main(argc, args);

  _CrtMemCheckpoint(&m2);
  if (_CrtMemDifference(&diff, &m1, &m2) && diff.lCounts[_NORMAL_BLOCK] > 0)
  {
    _CrtMemDumpAllObjectsSince(&m1);
    printf("%s\n", "Memory leaks found");

    // You've hit this because you've introduced a memory leak!
    // If you need help, define __MEMORY_DEBUG__ in the premake5.lua just before:
    // if _OPTIONS["force-vaultsdk"] then
    // This will emit filenames of what is leaking to assist in tracking down what's leaking.
    // Additionally, you can set _CrtSetBreakAlloc(<allocationNumber>);
    // back up where the initial checkpoint is made.
    __debugbreak();

    ret = 1;
  }

  return ret;
}
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
// Licenses for VDK
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
{ "libdeflate", R"license(Copyright 2016 Eric Biggers

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation files
(the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and / or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so,
subject to the following conditions :

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.)license" },
{ "mbedtls", R"license(Apache License
Version 2.0, January 2004
http://www.apache.org/licenses/

TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION

1. Definitions.

"License" shall mean the terms and conditions for use, reproduction,
and distribution as defined by Sections 1 through 9 of this document.

"Licensor" shall mean the copyright owner or entity authorized by
the copyright owner that is granting the License.

"Legal Entity" shall mean the union of the acting entity and all
other entities that control, are controlled by, or are under common
control with that entity.For the purposes of this definition,
"control" means(i) the power, direct or indirect, to cause the
direction or management of such entity, whether by contract or
otherwise, or (ii)ownership of fifty percent(50 % ) or more of the
outstanding shares, or (iii)beneficial ownership of such entity.

"You" (or "Your") shall mean an individual or Legal Entity
exercising permissions granted by this License.

"Source" form shall mean the preferred form for making modifications,
including but not limited to software source code, documentation
source, and configuration files.

"Object" form shall mean any form resulting from mechanical
transformation or translation of a Source form, including but
not limited to compiled object code, generated documentation,
and conversions to other media types.

"Work" shall mean the work of authorship, whether in Source or
Object form, made available under the License, as indicated by a
copyright notice that is included in or attached to the work
(an example is provided in the Appendix below).

"Derivative Works" shall mean any work, whether in Source or Object
form, that is based on(or derived from) the Work and for which the
editorial revisions, annotations, elaborations, or other modifications
represent, as a whole, an original work of authorship.For the purposes
of this License, Derivative Works shall not include works that remain
separable from, or merely link(or bind by name) to the interfaces of,
the Work and Derivative Works thereof.

"Contribution" shall mean any work of authorship, including
the original version of the Work and any modifications or additions
to that Work or Derivative Works thereof, that is intentionally
submitted to Licensor for inclusion in the Work by the copyright owner
or by an individual or Legal Entity authorized to submit on behalf of
the copyright owner.For the purposes of this definition, "submitted"
means any form of electronic, verbal, or written communication sent
to the Licensor or its representatives, including but not limited to
communication on electronic mailing lists, source code control systems,
and issue tracking systems that are managed by, or on behalf of, the
Licensor for the purpose of discussing and improving the Work, but
excluding communication that is conspicuously marked or otherwise
designated in writing by the copyright owner as "Not a Contribution."

"Contributor" shall mean Licensor and any individual or Legal Entity
on behalf of whom a Contribution has been received by Licensor and
subsequently incorporated within the Work.

2. Grant of Copyright License.Subject to the terms and conditions of
this License, each Contributor hereby grants to You a perpetual,
worldwide, non - exclusive, no - charge, royalty - free, irrevocable
copyright license to reproduce, prepare Derivative Works of,
publicly display, publicly perform, sublicense, and distribute the
Work and such Derivative Works in Source or Object form.

3. Grant of Patent License.Subject to the terms and conditions of
this License, each Contributor hereby grants to You a perpetual,
worldwide, non - exclusive, no - charge, royalty - free, irrevocable
(except as stated in this section) patent license to make, have made,
use, offer to sell, sell, import, and otherwise transfer the Work,
where such license applies only to those patent claims licensable
by such Contributor that are necessarily infringed by their
Contribution(s) alone or by combination of their Contribution(s)
with the Work to which such Contribution(s) was submitted.If You
institute patent litigation against any entity(including a
  cross - claim or counterclaim in a lawsuit) alleging that the Work
  or a Contribution incorporated within the Work constitutes direct
  or contributory patent infringement, then any patent licenses
  granted to You under this License for that Work shall terminate
  as of the date such litigation is filed.

  4. Redistribution.You may reproduce and distribute copies of the
  Work or Derivative Works thereof in any medium, with or without
  modifications, and in Source or Object form, provided that You
  meet the following conditions :

(a)You must give any other recipients of the Work or
Derivative Works a copy of this License; and

(b)You must cause any modified files to carry prominent notices
stating that You changed the files; and

(c)You must retain, in the Source form of any Derivative Works
that You distribute, all copyright, patent, trademark, and
attribution notices from the Source form of the Work,
excluding those notices that do not pertain to any part of
the Derivative Works; and

(d)If the Work includes a "NOTICE" text file as part of its
distribution, then any Derivative Works that You distribute must
include a readable copy of the attribution notices contained
within such NOTICE file, excluding those notices that do not
pertain to any part of the Derivative Works, in at least one
of the following places : within a NOTICE text file distributed
as part of the Derivative Works; within the Source form or
documentation, if provided along with the Derivative Works; or ,
within a display generated by the Derivative Works, if and
wherever such third - party notices normally appear.The contents
of the NOTICE file are for informational purposes only and
do not modify the License.You may add Your own attribution
notices within Derivative Works that You distribute, alongside
or as an addendum to the NOTICE text from the Work, provided
that such additional attribution notices cannot be construed
as modifying the License.

You may add Your own copyright statement to Your modifications and
may provide additional or different license terms and conditions
for use, reproduction, or distribution of Your modifications, or
for any such Derivative Works as a whole, provided Your use,
reproduction, and distribution of the Work otherwise complies with
the conditions stated in this License.

5. Submission of Contributions.Unless You explicitly state otherwise,
any Contribution intentionally submitted for inclusion in the Work
by You to the Licensor shall be under the terms and conditions of
this License, without any additional terms or conditions.
Notwithstanding the above, nothing herein shall supersede or modify
the terms of any separate license agreement you may have executed
with Licensor regarding such Contributions.

6. Trademarks.This License does not grant permission to use the trade
names, trademarks, service marks, or product names of the Licensor,
except as required for reasonable and customary use in describing the
origin of the Work and reproducing the content of the NOTICE file.

7. Disclaimer of Warranty.Unless required by applicable law or
agreed to in writing, Licensor provides the Work(and each
  Contributor provides its Contributions) on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
  implied, including, without limitation, any warranties or conditions
  of TITLE, NON - INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A
  PARTICULAR PURPOSE.You are solely responsible for determining the
  appropriateness of using or redistributing the Work and assume any
  risks associated with Your exercise of permissions under this License.

  8. Limitation of Liability.In no event and under no legal theory,
  whether in tort(including negligence), contract, or otherwise,
  unless required by applicable law(such as deliberate and grossly
    negligent acts) or agreed to in writing, shall any Contributor be
  liable to You for damages, including any direct, indirect, special,
  incidental, or consequential damages of any character arising as a
  result of this License or out of the use or inability to use the
  Work(including but not limited to damages for loss of goodwill,
    work stoppage, computer failure or malfunction, or any and all
    other commercial damages or losses), even if such Contributor
  has been advised of the possibility of such damages.

  9. Accepting Warranty or Additional Liability.While redistributing
  the Work or Derivative Works thereof, You may choose to offer,
  and charge a fee for, acceptance of support, warranty, indemnity,
  or other liability obligations and / or rights consistent with this
  License.However, in accepting such obligations, You may act only
  on Your own behalf and on Your sole responsibility, not on behalf
  of any other Contributor, and only if You agree to indemnify,
  defend, and hold each Contributor harmless for any liability
  incurred by, or claims asserted against, such Contributor by reason
  of your accepting any such warranty or additional liability.

  END OF TERMS AND CONDITIONS

  APPENDIX : How to apply the Apache License to your work.

  To apply the Apache License to your work, attach the following
  boilerplate notice, with the fields enclosed by brackets "[]"
  replaced with your own identifying information. (Don't include
    the brackets!)  The text should be enclosed in the appropriate
  comment syntax for the file format.We also recommend that a
  file or class name and description of purpose be included on the
  same "printed page" as the copyright notice for easier
  identification within third - party archives.

  Copyright[yyyy][name of copyright owner]

  Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.)license" },
{ "miniz", R"license(This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non - commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain.We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors.We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>)license" }
};

struct vcColumnHeader
{
  const char* pLabel;
  float size;
};

void vcRenderWindow(vcState *pProgramState);
int vcMainMenuGui(vcState *pProgramState);

void vcSettings_LoadSettings(vcState *pProgramState, bool forceDefaults);
void vcLogin(vcState *pProgramState);
bool vcLogout(vcState *pProgramState);

int64_t vcMain_GetCurrentTime()
{
  return std::chrono::system_clock::now().time_since_epoch().count() / std::chrono::system_clock::period::den;
}

vdkError vcMain_UpdateSessionInfo(vcState *pProgramState)
{
  const char *pSessionRawData = nullptr;
  udJSON info;

  pProgramState->lastServerAttempt = vcMain_GetCurrentTime();
  vdkError response = vdkServerAPI_Query(pProgramState->pVDKContext, "v1/session/info", nullptr, &pSessionRawData);

  if (response == vE_Success)
  {
    if (info.Parse(pSessionRawData) == udR_Success)
    {
      if (info.Get("success").AsBool() == true)
      {
        udStrcpy(pProgramState->username, udLengthOf(pProgramState->username), info.Get("user.realname").AsString("Guest"));
        pProgramState->lastServerResponse = vcMain_GetCurrentTime();
      }
      else
      {
        response = vE_NotAllowed;
      }
    }
    else
    {
      response = vE_Failure;
    }
  }
  vdkServerAPI_ReleaseResult(pProgramState->pVDKContext, &pSessionRawData);

  return response;
}

int main(int argc, char **args)
{
#if UDPLATFORM_WINDOWS
  if (argc > 0)
  {
    udFilename currentPath(args[0]);
    char cPathBuffer[256];
    currentPath.ExtractFolder(cPathBuffer, (int)udLengthOf(cPathBuffer));
    SetCurrentDirectoryW(udOSString(cPathBuffer));
  }
#endif //UDPLATFORM_WINDOWS

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
  double frametimeMS = 0.0;
  uint32_t sleepMS = 0;

  const float FontSize = 16.f;
  ImFontConfig fontCfg = ImFontConfig();

  const char *pNextLoad = nullptr;

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
  programState.loadList.Init(32);

  for (int i = 1; i < argc; ++i)
    programState.loadList.PushBack(udStrdup(args[i]));

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

  // Stop window from being minimized while fullscreened and focus is lost
  SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

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

  SDL_GL_SetSwapInterval(0); // disable v-sync

  ImGui::CreateContext();
  ImGui::GetIO().ConfigResizeWindowsFromEdges = true; // Fix for ImGuiWindowFlags_ResizeFromAnySide being removed
  vcSettings_LoadSettings(&programState, false);

  // setup watermark for background
  pEucWatermarkData = stbi_load(EucWatermarkPath, &iconWidth, &iconHeight, &iconBytesPerPixel, 0); // reusing the variables for width etc
  vcTexture_Create(&programState.pCompanyLogo, iconWidth, iconHeight, pEucWatermarkData);

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

  static ImWchar characterRanges[] =
  {
    0x0020, 0x00FF, // Basic Latin + Latin Supplement
    0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
    0x0E00, 0x0E7F, // Thai
    0x2010, 0x205E, // Punctuations
    0x25A0, 0x25FF, // Geometric Shapes
    0x2DE0, 0x2DFF, // Cyrillic Extended-A
    0x3000, 0x30FF, // Punctuations, Hiragana, Katakana
    0x3131, 0x3163, // Korean alphabets
    0x31F0, 0x31FF, // Katakana Phonetic Extensions
    0x4e00, 0x9FAF, // CJK Ideograms
    0xA640, 0xA69F, // Cyrillic Extended-B
    0xAC00, 0xD79D, // Korean characters
    0xFF00, 0xFFEF, // Half-width characters
    0
  };

  ImGui::GetIO().Fonts->AddFontFromFileTTF(FontPath, FontSize, &fontCfg, characterRanges);
  ImGui::GetIO().Fonts->AddFontFromFileTTF(FontPath, FontSize, &fontCfg, ImGui::GetIO().Fonts->GetGlyphRangesJapanese()); // Still need to load Japanese seperately

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
          programState.loadList.PushBack(udStrdup(event.drop.file));
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

    frametimeMS = 0.03333333; // 30 FPS cap
    if ((SDL_GetWindowFlags(programState.pWindow) & SDL_WINDOW_INPUT_FOCUS) == 0 && programState.settings.presentation.limitFPSInBackground)
      frametimeMS = 0.250; // 4 FPS cap when not focused

    sleepMS = (uint32_t)udMax((frametimeMS - programState.deltaTime) * 1000.0, 0.0);
    udSleep(sleepMS);
    programState.deltaTime += sleepMS * 0.001; // adjust delta

    ImGuiGL_NewFrame(programState.pWindow);

    vcGLState_ResetState(true);
    vcRenderWindow(&programState);

    ImGui::Render();
    ImGuiGL_RenderDrawData(ImGui::GetDrawData());

    vcGLState_Present(programState.pWindow);

    if (ImGui::GetIO().WantSaveIniSettings)
      vcSettings_Save(&programState.settings);

    ImGui::GetIO().KeysDown[SDL_SCANCODE_BACKSPACE] = false;

    if (programState.hasContext)
    {
      // Load next file in the load list (if there is one and the user has a context)
      if (programState.loadList.length > 0)
      {
        pNextLoad = nullptr;
        programState.loadList.PopFront(&pNextLoad);

        //TODO: Display a message here that the file couldn't be opened...
        if (!vcModel_AddToList(&programState, pNextLoad))
          vcConvert_AddFile(&programState, pNextLoad);

        udFree(pNextLoad);
      }

      // Ping the server every 30 seconds
      if (vcMain_GetCurrentTime() > programState.lastServerAttempt + 30)
      {
        if (vcMain_UpdateSessionInfo(&programState) == vE_NotAllowed)
          vcLogout(&programState);
      }
    }
  }

  vcSettings_Save(&programState.settings);
  ImGui::ShutdownDock();
  ImGui::DestroyContext();

epilogue:
  programState.projects.Destroy();
  ImGuiGL_DestroyDeviceObjects();
  vcConvert_Deinit(&programState);
  vcCamera_Destroy(&programState.pCamera);
  vcTexture_Destroy(&programState.pCompanyLogo);
  free(pIconData);
  free(pEucWatermarkData);
  for (size_t i = 0; i < programState.loadList.length; i++)
    udFree(programState.loadList[i]);
  programState.loadList.Deinit();
  vcModel_UnloadList(&programState);
  programState.vcModelList.Deinit();
  vcLogout(&programState);
  vcRender_Destroy(&programState.pRenderContext);

  vcGLState_Deinit();

  return 0;
}

void vcRenderSceneWindow(vcState *pProgramState)
{
  //Rendering
  udInt2 sizei;
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

  if (pProgramState->cameraInput.isUsingAnchorPoint)
    renderData.pWorldAnchorPos = &pProgramState->cameraInput.worldAnchorPoint;

  ImGuiIO &io = ImGui::GetIO();
  renderData.mouse.x = (uint32_t)(io.MousePos.x - windowPos.x);
  renderData.mouse.y = (uint32_t)(io.MousePos.y - windowPos.y);

  for (size_t i = 0; i < pProgramState->vcModelList.length; ++i)
    renderData.models.PushBack(&pProgramState->vcModelList[i]);

  vcTexture *pTexture = vcRender_RenderScene(pProgramState->pRenderContext, renderData, pProgramState->pDefaultFramebuffer);

  renderData.models.Deinit();
  pProgramState->pSceneWatermark = renderData.pWatermarkTexture;

  {
    pProgramState->worldMousePos = renderData.worldMousePos;
    pProgramState->pickingSuccess = renderData.pickingSuccess;

    ImGui::SetNextWindowPos(ImVec2(windowPos.x + size.x, windowPos.y), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(200, 0), ImVec2(FLT_MAX, FLT_MAX)); // Set minimum width to include the header
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

    if (ImGui::Begin("Geographic Information", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar))
    {
      if (pProgramState->gis.SRID != 0 && pProgramState->gis.isProjected)
        ImGui::Text("%s (SRID: %d)", pProgramState->gis.zone.zoneName, pProgramState->gis.SRID);
      else if (pProgramState->gis.SRID == 0)
        ImGui::Text("Not Geolocated");
      else
        ImGui::Text("Unsupported SRID: %d", pProgramState->gis.SRID);

      if (pProgramState->settings.presentation.showAdvancedGIS)
      {
        int newSRID = pProgramState->gis.SRID;
        if (ImGui::InputInt("Override SRID", &newSRID) && vcGIS_AcceptableSRID((vcSRID)newSRID))
        {
          if (vcGIS_ChangeSpace(&pProgramState->gis, (vcSRID)newSRID, &pProgramState->pCamera->position))
            vcModel_UpdateMatrix(pProgramState, nullptr); // Update all models to new zone
        }
      }

      if (pProgramState->settings.presentation.showDiagnosticInfo)
      {
        ImGui::Separator();
        if (ImGui::IsMousePosValid())
        {
          if (pProgramState->pickingSuccess)
          {
            ImGui::Text("Mouse World Pos (x/y/z): (%f,%f,%f)", renderData.worldMousePos.x, renderData.worldMousePos.y, renderData.worldMousePos.z);

            if (pProgramState->gis.isProjected)
            {
              udDouble3 mousePointInLatLong = udGeoZone_ToLatLong(pProgramState->gis.zone, renderData.worldMousePos);
              ImGui::Text("Mouse World Pos (L/L): (%f,%f)", mousePointInLatLong.x, mousePointInLatLong.y);
            }
          }
        }
      }
    }

    ImGui::End();
  }

  // On Screen Camera Settings
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x, windowPos.y), ImGuiCond_Always, ImVec2(0.f, 0.f));
    ImGui::SetNextWindowBgAlpha(0.5f);
    if (ImGui::Begin("Camera Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
    {
      ImGui::InputScalarN("Camera Position", ImGuiDataType_Double, &pProgramState->pCamera->position.x, 3);
      ImGui::InputScalarN("Camera Rotation", ImGuiDataType_Double, &pProgramState->pCamera->eulerRotation.x, 3);

      if (ImGui::SliderFloat("Move Speed", &(pProgramState->settings.camera.moveSpeed), vcSL_CameraMinMoveSpeed, vcSL_CameraMaxMoveSpeed, "%.3f m/s", 4.f))
        pProgramState->settings.camera.moveSpeed = udMax(pProgramState->settings.camera.moveSpeed, 0.f);

      ImGui::RadioButton("Plane", (int*)&pProgramState->settings.camera.moveMode, vcCMM_Plane);
      ImGui::SameLine();
      ImGui::RadioButton("Heli", (int*)&pProgramState->settings.camera.moveMode, vcCMM_Helicopter);

      if (pProgramState->gis.isProjected)
      {
        ImGui::Separator();

        udDouble3 cameraLatLong = udGeoZone_ToLatLong(pProgramState->gis.zone, pProgramState->camMatrix.axis.t.toVector3());
        ImGui::Text("Lat: %.7f, Long: %.7f, Alt: %.2fm", cameraLatLong.x, cameraLatLong.y, cameraLatLong.z);

        if (pProgramState->gis.zone.latLongBoundMin != pProgramState->gis.zone.latLongBoundMax)
        {
          udDouble2 &minBound = pProgramState->gis.zone.latLongBoundMin;
          udDouble2 &maxBound = pProgramState->gis.zone.latLongBoundMax;

          if (cameraLatLong.x < minBound.x || cameraLatLong.y < minBound.y || cameraLatLong.x > maxBound.x || cameraLatLong.y > maxBound.y)
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Camera is outside recommended limits of this GeoZone");
        }
      }
    }

    ImGui::End();
  }

  // On Screen Controls Overlay
  float bottomLeftOffset = 0.f;
  if (pProgramState->onScreenControls)
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + bottomLeftOffset, windowPos.y + size.y), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

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

      if (ImGui::VSliderFloat("##oscUDSlider", ImVec2(40, 100), &vertical, -1, 1, "U/D"))
        vertical = udClamp(vertical, -1.f, 1.f);

      ImGui::NextColumn();

      ImGui::Button("Move Camera", ImVec2(100, 100));
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

      bottomLeftOffset += ImGui::GetWindowWidth();
    }

    ImGui::End();
  }

  if (pProgramState->pSceneWatermark != nullptr) // Watermark
  {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    vcTexture_GetSize(pProgramState->pSceneWatermark, &sizei.x, &sizei.y);
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + bottomLeftOffset, windowPos.y + size.y), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2((float)sizei.x, (float)sizei.y));
    ImGui::SetNextWindowBgAlpha(0.5f);

    if (ImGui::Begin("ModelWatermark", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
      ImGui::Image(pProgramState->pSceneWatermark, ImVec2((float)sizei.x, (float)sizei.y));
    ImGui::End();
    ImGui::PopStyleVar();
  }


  if (pProgramState->settings.maptiles.mapEnabled && pProgramState->gis.isProjected)
  {
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + size.x, windowPos.y + size.y), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.5f);

    if (ImGui::Begin("MapCopyright", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
      ImGui::Text("Map Data \xC2\xA9 OpenStreetMap contributors");
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

    udJSONArray *pProjectList = pProgramState->projects.Get("projects").AsArray();
    if (ImGui::BeginMenu("Projects", pProjectList != nullptr && pProjectList->length > 0))
    {
      for (size_t i = 0; i < pProjectList->length; ++i)
      {
        if (ImGui::MenuItem(pProjectList->GetElement(i)->Get("name").AsString("<Unnamed>"), nullptr, nullptr))
        {
          vcModel_UnloadList(pProgramState);

          for (size_t j = 0; j < pProjectList->GetElement(i)->Get("models").ArrayLength(); ++j)
            pProgramState->loadList.PushBack(udStrdup(pProjectList->GetElement(i)->Get("models[%d]", j).AsString()));
        }
      }

      ImGui::EndMenu();
    }

    char endBarInfo[512] = {};

    if (pProgramState->loadList.length > 0)
      udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("(%llu Files Queued) / ", pProgramState->loadList.length));

    if ((SDL_GetWindowFlags(pProgramState->pWindow) & SDL_WINDOW_INPUT_FOCUS) == 0)
      udStrcat(endBarInfo, udLengthOf(endBarInfo), "Inactive / ");

    if (pProgramState->settings.presentation.showDiagnosticInfo)
      udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("FPS: %.3f (%.2fms) / ", 1.f / pProgramState->deltaTime, pProgramState->deltaTime * 1000.f));

    int64_t currentTime = vcMain_GetCurrentTime();

    for (int i = 0; i < vdkLT_Total; ++i)
    {
      vdkLicenseInfo info = {};
      if (vdkContext_GetLicenseInfo(pProgramState->pVDKContext, (vdkLicenseType)i, &info) == vE_Success)
      {
        if (info.queuePosition < 0 && (uint64_t)currentTime < info.expiresTimestamp)
          udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("%s License (%llusecs) / ", i == vdkLT_Render ? "Render" : "Convert", (info.expiresTimestamp - currentTime)));
        else if (info.queuePosition < 0)
          udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("%s License (expired) / ", i == vdkLT_Render ? "Render" : "Convert"));
        else
          udStrcat(endBarInfo, udLengthOf(endBarInfo), udTempStr("%s License (%d in Queue) / ", i == vdkLT_Render ? "Render" : "Convert", info.queuePosition));
      }
    }

    udStrcat(endBarInfo, udLengthOf(endBarInfo), pProgramState->username);

    ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(endBarInfo).x - 25);
    ImGui::Text("%s", endBarInfo);

    // Connection status indicator
    {
      ImGui::SameLine(ImGui::GetContentRegionMax().x - 20);
      if (pProgramState->lastServerResponse + 30 > currentTime)
        ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), u8"\x25CF");
      else if (pProgramState->lastServerResponse + 60 > currentTime)
        ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), u8"\x25CF");
      else
        ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), u8"\x25CF");
      if (ImGui::IsItemHovered())
      {
        ImGui::BeginTooltip();
        ImGui::Text("Connection Status");
        ImGui::EndTooltip();
      }
    }

    menuHeight = (int)ImGui::GetWindowSize().y;

    ImGui::EndMainMenuBar();
  }

  return menuHeight;
}

void vcRenderWindow(vcState *pProgramState)
{
  vcFramebuffer_Bind(pProgramState->pDefaultFramebuffer);
  vcGLState_SetViewport(0, 0, pProgramState->settings.window.width, pProgramState->settings.window.height);
  vcFramebuffer_Clear(pProgramState->pDefaultFramebuffer, 0xFF000000);

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
    ImGui::Image(pProgramState->pCompanyLogo, ImVec2(301, 161), ImVec2(0, 0), ImVec2(1, 1));
    ImGui::End();
    ImGui::PopStyleColor();

    ImGui::SetNextWindowSize(ImVec2(500, 160));
    if (ImGui::Begin("Login", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
    {
      if (pProgramState->pLoginErrorMessage != nullptr)
        ImGui::Text("%s", pProgramState->pLoginErrorMessage);

      bool tryLogin = false;

      // Server URL
      tryLogin |= ImGui::InputText("ServerURL", pProgramState->settings.loginInfo.serverURL, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);
      if (pProgramState->pLoginErrorMessage == nullptr && !pProgramState->settings.loginInfo.rememberServer)
        ImGui::SetKeyboardFocusHere(ImGuiCond_Appearing);
      ImGui::SameLine();
      ImGui::Checkbox("Remember##rememberServerURL", &pProgramState->settings.loginInfo.rememberServer);

      // Username
      tryLogin |= ImGui::InputText("Username", pProgramState->settings.loginInfo.username, vcMaxPathLength, ImGuiInputTextFlags_EnterReturnsTrue);
      if (pProgramState->pLoginErrorMessage == nullptr && pProgramState->settings.loginInfo.rememberServer && !pProgramState->settings.loginInfo.rememberUsername)
        ImGui::SetKeyboardFocusHere(ImGuiCond_Appearing);
      ImGui::SameLine();
      ImGui::Checkbox("Remember##rememberUsername", &pProgramState->settings.loginInfo.rememberUsername);

      // Password
      tryLogin |= ImGui::InputText("Password", pProgramState->password, vcMaxPathLength, ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
      if (pProgramState->pLoginErrorMessage == nullptr && pProgramState->settings.loginInfo.rememberServer && pProgramState->settings.loginInfo.rememberUsername)
        ImGui::SetKeyboardFocusHere(ImGuiCond_Appearing);

      if (pProgramState->pLoginErrorMessage == nullptr)
        pProgramState->pLoginErrorMessage = "Please enter your credentials...";

      if (ImGui::Button("Login!") || tryLogin)
        vcLogin(pProgramState);
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

    if (ImGui::BeginDock("Scene Explorer", &pProgramState->settings.window.windowsOpen[vcdSceneExplorer]))
    {
      ImGui::InputText("", pProgramState->modelPath, vcMaxPathLength);
      ImGui::SameLine();
      if (ImGui::Button("Load Model!"))
        pProgramState->loadList.PushBack(udStrdup(pProgramState->modelPath));

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

          ImGui::Separator();

          if (pProgramState->vcModelList[i].pZone != nullptr && ImGui::Selectable("Use Projection"))
          {
            if (vcGIS_ChangeSpace(&pProgramState->gis, pProgramState->vcModelList[i].pZone->srid, &pProgramState->pCamera->position))
              vcModel_UpdateMatrix(pProgramState, nullptr); // Update all models to new zone
          }

          if (ImGui::Selectable("Move To"))
          {
            udDouble3 localSpaceCenter = vcModel_GetMidPointLocalSpace(pProgramState, &pProgramState->vcModelList[i]);

            // Transform the camera position. Don't do the entire matrix as it may lead to inaccuracy/de-normalised camera
            if (pProgramState->gis.isProjected && pProgramState->vcModelList[i].pZone != nullptr && pProgramState->vcModelList[i].pZone->srid != pProgramState->gis.SRID)
              localSpaceCenter = udGeoZone_TransformPoint(localSpaceCenter, *pProgramState->vcModelList[i].pZone, pProgramState->gis.zone);

            pProgramState->cameraInput.inputState = vcCIS_MovingToPoint;
            pProgramState->cameraInput.startPosition = vcCamera_GetMatrix(pProgramState->pCamera).axis.t.toVector3();
            pProgramState->cameraInput.worldAnchorPoint = localSpaceCenter;
            pProgramState->cameraInput.progress = 0.0;
          }

          ImGui::Separator();

          if (ImGui::Selectable("Properties"))
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
                vcModel_RemoveFromList(pProgramState, j);
                j--;
              }
            }

            i = (pProgramState->numSelectedModels > i) ? 0 : (i - pProgramState->numSelectedModels);
          }
          else
          {
            vcModel_RemoveFromList(pProgramState, i);
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

    if (ImGui::BeginDock("Scene", &pProgramState->settings.window.windowsOpen[vcdScene], ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus))
      vcRenderSceneWindow(pProgramState);
    ImGui::EndDock();

    if (ImGui::BeginDock("Convert", &pProgramState->settings.window.windowsOpen[vcdConvert]))
      vcConvert_ShowUI(pProgramState);
    ImGui::EndDock();

    if (ImGui::BeginDock("Settings", &pProgramState->settings.window.windowsOpen[vcdSettings]))
    {
      if (ImGui::CollapsingHeader("Appearance##Settings"))
      {
        if (ImGui::Combo("Theme", &pProgramState->settings.presentation.styleIndex, "Classic\0Dark\0Light\0"))
        {
          switch (pProgramState->settings.presentation.styleIndex)
          {
          case 0: ImGui::StyleColorsClassic(); break;
          case 1: ImGui::StyleColorsDark(); break;
          case 2: ImGui::StyleColorsLight(); break;
          }
        }

        ImGui::Checkbox("Show Diagnostic Information", &pProgramState->settings.presentation.showDiagnosticInfo);
        ImGui::Checkbox("Show On Screen Compass", &pProgramState->settings.presentation.showCompass);
        ImGui::Checkbox("Show Advanced GIS Settings", &pProgramState->settings.presentation.showAdvancedGIS);
        ImGui::Checkbox("Limit FPS In Background", &pProgramState->settings.presentation.limitFPSInBackground);
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

          const char* blendModes[] = { "Hybrid", "Overlay", "Underlay" };
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
          ImGui::SliderFloat("Min Intensity", &temp[0], 0.f, temp[1], "%.0f", 4.f);
          ImGui::SliderFloat("Max Intensity", &temp[1], temp[0], 65535.f, "%.0f", 4.f);
          pProgramState->settings.visualization.minIntensity = (int)temp[0];
          pProgramState->settings.visualization.maxIntensity = (int)temp[1];
        }

        // Post visualization - Edge Highlighting
        ImGui::Checkbox("Enable Edge Highlighting", &pProgramState->settings.postVisualization.edgeOutlines.enable);
        if (pProgramState->settings.postVisualization.edgeOutlines.enable)
        {
          ImGui::SliderInt("Edge Highlighting Width", &pProgramState->settings.postVisualization.edgeOutlines.width, 1, 10);

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
      pProgramState->selectedModelProperties.pWatermarkTexture = pProgramState->vcModelList[pProgramState->selectedModelProperties.index].pWatermark;
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

          udInt2 imageSizei;
          vcTexture_GetSize(pProgramState->selectedModelProperties.pWatermarkTexture, &imageSizei.x, &imageSizei.y);

          ImVec2 imageSize = ImVec2((float)imageSizei.x, (float)imageSizei.y);
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
        ImGui::CloseCurrentPopup();

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
        // ImGui::Text has a limitation of 3072 bytes.
        ImGui::TextUnformatted(licenses[i].pName);
        ImGui::TextUnformatted(licenses[i].pLicense);
        ImGui::Separator();
      }
      ImGui::EndChild();

      ImGui::EndPopup();
    }
  }
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

void vcLogin(vcState *pProgramState)
{
  vdkError result;

  result = vdkContext_Connect(&pProgramState->pVDKContext, pProgramState->settings.loginInfo.serverURL, "ClientSample", pProgramState->settings.loginInfo.username, pProgramState->password);
  if (result == vE_ConnectionFailure)
    pProgramState->pLoginErrorMessage = "Could not connect to server.";
  else if (result == vE_NotAllowed)
    pProgramState->pLoginErrorMessage = "Username or Password incorrect.";
  else if (result == vE_OutOfSync)
    pProgramState->pLoginErrorMessage = "Your clock doesn't match the remote server clock.";
  else if (result != vE_Success)
    pProgramState->pLoginErrorMessage = "Unknown error occurred, please try again later.";

  if (result != vE_Success)
    return;

  vcRender_CreateTerrain(pProgramState->pRenderContext, &pProgramState->settings);
  vcRender_SetVaultContext(pProgramState->pRenderContext, pProgramState->pVDKContext);

  const char *pProjData = nullptr;
  if (vdkServerAPI_Query(pProgramState->pVDKContext, "dev/projects", nullptr, &pProjData) == vE_Success)
    pProgramState->projects.Parse(pProjData);
  vdkServerAPI_ReleaseResult(pProgramState->pVDKContext, &pProjData);

  vcMain_UpdateSessionInfo(pProgramState);

  //Context Login successful
  memset(pProgramState->password, 0, sizeof(pProgramState->password));
  if (!pProgramState->settings.loginInfo.rememberServer)
    memset(pProgramState->settings.loginInfo.serverURL, 0, sizeof(pProgramState->settings.loginInfo.serverURL));

  if (!pProgramState->settings.loginInfo.rememberUsername)
    memset(pProgramState->settings.loginInfo.username, 0, sizeof(pProgramState->settings.loginInfo.username));

  pProgramState->pLoginErrorMessage = nullptr;
  pProgramState->hasContext = true;
}

bool vcLogout(vcState *pProgramState)
{
  bool success = true;

  success &= vcModel_UnloadList(pProgramState);
  success &= (vcRender_DestroyTerrain(pProgramState->pRenderContext) == udR_Success);

  pProgramState->projects.Destroy();
  memset(&pProgramState->gis, 0, sizeof(pProgramState->gis));

  success = success && vdkContext_Disconnect(&pProgramState->pVDKContext) == vE_Success;
  pProgramState->hasContext = !success;

  return success;
}
