#include "vaultContext.h"
#include "vaultUDRenderer.h"
#include "vaultUDRenderView.h"
#include "vaultUDModel.h"
#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include "imgui_dock.h"
#include "SDL2/SDL.h"
#include "GL/glew.h"
#include "udPlatform/udMath.h"
#include "udPlatform/udChunkedArray.h"
#include "udPlatform/udPlatformUtil.h"

#include <stdlib.h>
#include <stdio.h>

const GLchar* const g_udFragmentShader = R"shader(#version 330 core

uniform sampler2D u_texture;
uniform sampler2D u_depth;

//Input Format
in vec2 v_texCoord;

//Output Format
out vec4 out_Colour;

void main()
{
  out_Colour = texture(u_texture, v_texCoord).bgra;
  gl_FragDepth = texture(u_depth, v_texCoord).x;
}
)shader";

const GLchar* const g_udVertexShader = R"shader(#version 330 core

//Input format
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texCoord;

//Output Format
out vec2 v_texCoord;

void main()
{
  gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);
  v_texCoord = a_texCoord;
}
)shader";

GLuint GenerateFbVBO(const udFloat2 *pFloats, size_t len)
{
  GLuint vboID = 0;
  // Create a new VBO and use the variable id to store the VBO id
  glGenBuffers(1, &vboID);

  // Make the new VBO active
  glBindBuffer(GL_ARRAY_BUFFER, vboID);

  // Upload vertex data to the video device
  glBufferData(GL_ARRAY_BUFFER, sizeof(udFloat2) * len, pFloats, GL_STATIC_DRAW); // GL_DYNAMIC_DRAW

  // Draw Triangle from VBO - do each time window, view point or data changes
  // Establish its 3 coordinates per vertex with zero stride in this array; necessary here
  //glVertexPointer(2, GL_FLOAT, 0, NULL);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  return vboID;
}

GLint glBuildShader(GLenum type, const GLchar *shaderCode)
{
  GLint compiled;
  GLint shaderCodeLen = (GLint)strlen(shaderCode);
  GLint shaderObject = glCreateShader(type);
  glShaderSource(shaderObject, 1, &shaderCode, &shaderCodeLen);
  glCompileShader(shaderObject);
  glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &compiled);
  if (!compiled)
  {
    GLint blen = 1024;
    GLsizei slen = 0;
    glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &blen);
    if (blen > 1)
    {
      GLchar* compiler_log = (GLchar*)malloc(blen);
      glGetShaderInfoLog(shaderObject, blen, &slen, compiler_log);
      free(compiler_log);
    }
    return -1;
  }
  return shaderObject;
}

GLint glBuildProgram(GLint vertexShader, GLint fragmentShader)
{
  GLint programObject = glCreateProgram();
  if (vertexShader != -1)
    glAttachShader(programObject, vertexShader);
  if (fragmentShader != -1)
    glAttachShader(programObject, fragmentShader);

  glLinkProgram(programObject);
  GLint linked;
  glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
  if (!linked)
  {
    GLint blen = 1024;
    GLsizei slen = 0;
    glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &blen);
    if (blen > 1)
    {
      GLchar* linker_log = (GLchar*)malloc(blen);
      glGetProgramInfoLog(programObject, blen, &slen, linker_log);
      free(linker_log);
    }
    return -1;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return programObject;
}

struct vaultContainer
{
  vaultContext *pContext;
  vaultUDRenderer *pRenderer;
  vaultUDRenderView *pRenderView;
 // vaultUDModel *pModel;
};

enum vcDocks
{
  vcdScene,
  vcdSettings,
  vcdSceneExplorer,

  vcdStyling,
  vcdUIDemo,

  vcdTotalDocks
};

struct vcSimpleVertex
{
  udFloat3 Position;
  udFloat2 UVs;
};

int qrIndices[6] = { 0, 1, 2, 0, 2, 3 };
vcSimpleVertex qrSqVertices[4]{ {{-1.f, 1.f, 0.f}, { 0, 0}}, {{ -1.f, -1.f, 0.f }, { 0, 1 }}, { { 1.f, -1.f, 0.f }, { 1, 1 }}, {{ 1.f, 1.f, 0.f }, { 1, 0 }} };
GLuint qrSqVboID = GL_INVALID_INDEX;
GLuint qrSqVaoID = GL_INVALID_INDEX;
GLuint qrSqIboID = GL_INVALID_INDEX;

struct RenderingState
{
  bool programComplete;
  SDL_Window *pWindow;

  uint32_t *pColorBuffer;
  float *pDepthBuffer;
  GLuint texId;
  GLint udProgramObject;

  GLuint framebuffer;
  GLuint texture;
  GLuint depthbuffer;

  bool planeMode;
  double deltaTime;
  udDouble4x4 camMatrix;
  udUInt2 sceneResolution;

  udDouble4x4 modelMatrix;

  bool hasContext;
  bool windowsOpen[vcdTotalDocks];

  char *pServerURL;
  char *pUsername;
  char *pPassword;

  char *pModelPath;
};

static bool isFullscreen;
static bool F11_Pressed;

struct vcModel{
  char modelPath[1024];
  bool modelLoaded;
  vaultUDModel *pVaultModel;
};
udChunkedArray<vcModel> modelList;

static bool lastModelLoaded;

bool vcUnloadModelList(vaultContainer *pVaultContainer);

void vcRender(RenderingState *pRenderingState, vaultContainer *pVaultContainer);

void cuglQR_CreateQuads(vcSimpleVertex *pVerts, int totalVerts, int *pIndices, int totalIndices, GLuint &vboID, GLuint &iboID, GLuint &vaoID)
{
  const size_t BufferSize = sizeof(vcSimpleVertex) * totalVerts;
  const size_t VertexSize = sizeof(vcSimpleVertex);

  glGenVertexArrays(1, &vaoID);
  glBindVertexArray(vaoID);

  glGenBuffers(1, &vboID);
  glBindBuffer(GL_ARRAY_BUFFER, vboID);
  glBufferData(GL_ARRAY_BUFFER, BufferSize, pVerts, GL_DYNAMIC_DRAW);

  glGenBuffers(1, &iboID);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * totalIndices, pIndices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)(0 * sizeof(float)));
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)(3 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  // Unbind
  glBindVertexArray(0);
}

#undef main
#ifdef SDL_MAIN_NEEDED
int SDL_main(int /*argc*/, char ** /*args*/)
#else
int main(int /*argc*/, char ** /*args*/)
#endif
{
  SDL_GLContext glcontext = NULL;
  GLint udTextureLocation = -1;
  isFullscreen = false; // defaults to not fullscreen
  F11_Pressed = false;

  const udFloat2 fboDataArr[] = { { -1.f,-1.f },{ -1.f,1.f },{ 1,1 },{ -1.f,-1.f },{ 1.f,-1.f },{ 1,1 } };
  GLuint fbVboId = (GLuint)-1;

  RenderingState renderingState = {};
  vaultContainer vContainer = {};

  // default values
  renderingState.planeMode = true;
  renderingState.sceneResolution.x = 1280;
  renderingState.sceneResolution.y = 720;
  renderingState.camMatrix = udDouble4x4::identity();
  renderingState.texId = GL_INVALID_INDEX;
  renderingState.udProgramObject = GL_INVALID_INDEX;
  modelList.Init(32);
  lastModelLoaded = true;

  // default string values.
  const char *plocalHost = "http://vau-ubu-pro-001.euclideon.local";
  const char *pUsername = "";
  const char *pPassword = "";
  const char *pModelPath = "V:/QA/For Tests/Geoverse MDM/AdelaideCBD_2cm.uds";

  renderingState.pServerURL = new char[1024];
  renderingState.pUsername = new char[1024];
  renderingState.pPassword = new char[1024];
  renderingState.pModelPath = new char[1024];

  memset(renderingState.pServerURL, 0, 1024);
  memset(renderingState.pUsername, 0, 1024);
  memset(renderingState.pPassword, 0, 1024);
  memset(renderingState.pModelPath, 0, 1024);

  memcpy(renderingState.pServerURL, plocalHost, strlen(plocalHost));
  memcpy(renderingState.pUsername, pUsername, strlen(pUsername));
  memcpy(renderingState.pPassword, pPassword, strlen(pPassword));
  memcpy(renderingState.pModelPath, pModelPath, strlen(pModelPath));

  Uint64 NOW;
  Uint64 LAST;

  GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };

  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    goto epilogue;

  // Setup window
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) != 0)
    goto epilogue;
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) != 0)
    goto epilogue;

  //TODO: Get a STRINGIFY macro in udPlatform somewhere
#define _STRINGIFY(a) #a
#define STRINGIFY(a) _STRINGIFY(a)
#ifdef GIT_BUILD
#define WINDOW_SUFFIX " (" STRINGIFY(GIT_BUILD) " - " __DATE__ ") "
#else
#define WINDOW_SUFFIX " (DEV/DO NOT DISTRIBUTE - " __DATE__ ")"
#endif

  renderingState.pWindow = SDL_CreateWindow("Euclideon Client" WINDOW_SUFFIX, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!renderingState.pWindow)
    goto epilogue;

  glcontext = SDL_GL_CreateContext(renderingState.pWindow);
  if (!glcontext)
    goto epilogue;

  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK)
    goto epilogue;

  ImGui::CreateContext();
  if (!ImGui_ImplSdlGL3_Init(renderingState.pWindow, "#version 150 core"))
    goto epilogue;

  renderingState.udProgramObject = glBuildProgram(glBuildShader(GL_VERTEX_SHADER, g_udVertexShader), glBuildShader(GL_FRAGMENT_SHADER, g_udFragmentShader));
  if (renderingState.udProgramObject == -1)
    goto epilogue;

  glUseProgram(renderingState.udProgramObject);

  udTextureLocation = glGetUniformLocation(renderingState.udProgramObject, "udTexture");
  glUniform1i(udTextureLocation, 0);


  cuglQR_CreateQuads(qrSqVertices, 4, qrIndices, 6, qrSqVboID, qrSqIboID, qrSqVaoID);

  fbVboId = GenerateFbVBO(fboDataArr, 6);

  glBindBuffer(GL_ARRAY_BUFFER, fbVboId);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(float) * 2, 0);

  glBindVertexArray(0);
  glUseProgram(0);

  glGenTextures(1, &renderingState.texId);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, renderingState.texId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  renderingState.pColorBuffer = new uint32_t[renderingState.sceneResolution.x*renderingState.sceneResolution.y];
  renderingState.pDepthBuffer = new float[renderingState.sceneResolution.x*renderingState.sceneResolution.y];

  glGenFramebuffers(1, &renderingState.framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, renderingState.framebuffer);

  glGenRenderbuffers(1, &renderingState.depthbuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, renderingState.depthbuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, renderingState.sceneResolution.x, renderingState.sceneResolution.y);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderingState.depthbuffer);

  glGenTextures(1, &renderingState.texture);
  glBindTexture(GL_TEXTURE_2D, renderingState.texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, renderingState.sceneResolution.x, renderingState.sceneResolution.y, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderingState.texture, 0);
  glDrawBuffers(1, DrawBuffers);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    //TODO: Handle this properly
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  //Get ready...
  NOW = SDL_GetPerformanceCounter();
  LAST = 0;

  ImGui::LoadDock();
  ImGui::GetIO().Fonts->AddFontFromFileTTF("NotoSansCJKjp-Regular.otf", 16.0f, NULL, ImGui::GetIO().Fonts->GetGlyphRangesChinese());

  while (!renderingState.programComplete)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSdlGL3_ProcessEvent(&event);
      renderingState.programComplete = (event.type == SDL_QUIT);
    }

    LAST = NOW;
    NOW = SDL_GetPerformanceCounter();
    renderingState.deltaTime = double(NOW - LAST) / SDL_GetPerformanceFrequency();

    ImGui_ImplSdlGL3_NewFrame(renderingState.pWindow);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, renderingState.sceneResolution.x, renderingState.sceneResolution.y);

    vcRender(&renderingState, &vContainer);

    ImGui::Render();
    SDL_GL_SwapWindow(renderingState.pWindow);
  }

  ImGui::SaveDock();
  ImGui::DestroyContext();

epilogue:
  vcUnloadModelList(&vContainer);
  modelList.Deinit();

  vaultUDRenderView_Destroy(vContainer.pContext, &vContainer.pRenderView);
  vaultUDRenderer_Destroy(vContainer.pContext, &vContainer.pRenderer);

  vaultContext_Disconnect(&vContainer.pContext);
  delete[] renderingState.pColorBuffer;
  delete[] renderingState.pDepthBuffer;

  delete[] renderingState.pServerURL;
  delete[] renderingState.pUsername;
  delete[] renderingState.pPassword;
  delete[] renderingState.pModelPath;

  return 0;
}

void vcRenderScene(RenderingState *pRenderingState, vaultContainer *pVaultContainer)
{
  //Rendering
  ImVec2 size = ImGui::GetContentRegionAvail();
  ImGuiIO& io = ImGui::GetIO();

  if (size.x < 1 || size.y < 1)
    return;

  if (pRenderingState->sceneResolution.x != size.x || pRenderingState->sceneResolution.y != size.y) //Resize buffers
  {
    pRenderingState->sceneResolution.x = (uint32_t)size.x;
    pRenderingState->sceneResolution.y = (uint32_t)size.y;

    //Resize GPU Targets
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pRenderingState->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pRenderingState->sceneResolution.x, pRenderingState->sceneResolution.y, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, pRenderingState->framebuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, pRenderingState->sceneResolution.x, pRenderingState->sceneResolution.y);

    //Resize CPU Targets
    vaultUDRenderView_Destroy(pVaultContainer->pContext, &pVaultContainer->pRenderView);

    delete pRenderingState->pColorBuffer;
    delete pRenderingState->pDepthBuffer;

    pRenderingState->pColorBuffer = new uint32_t[pRenderingState->sceneResolution.x*pRenderingState->sceneResolution.y];
    pRenderingState->pDepthBuffer = new float[pRenderingState->sceneResolution.x*pRenderingState->sceneResolution.y];



    //TODO: Error Detection
    vaultUDRenderView_Create(pVaultContainer->pContext, &pVaultContainer->pRenderView, pVaultContainer->pRenderer, pRenderingState->sceneResolution.x, pRenderingState->sceneResolution.y);
    vaultUDRenderView_SetTargets(pVaultContainer->pContext, pVaultContainer->pRenderView, pRenderingState->pColorBuffer, 0, pRenderingState->pDepthBuffer);

    float rad = UD_PIf / 3.f;
    float aspect = size.x / (float)size.y;
    float zNear = 0.5f;
    float zFar = 10000.f;
    udDouble4x4 projMat = udDouble4x4::perspective(rad, aspect, zNear, zFar);
    vaultUDRenderView_SetMatrix(pVaultContainer->pContext, pVaultContainer->pRenderView, vUDRVM_Projection, projMat.a);
  }

  const Uint8 *pKeysArray = SDL_GetKeyboardState(NULL);
  SDL_Keymod modState = SDL_GetModState();

  bool isHovered = true;// ImGui::IsItemHovered();
  bool isLeftClicked = ImGui::IsMouseClicked(0, false);
  bool isRightClicked = ImGui::IsMouseClicked(1, false);
  bool isFocused = ImGui::IsWindowFocused();

  static bool clickedLeftWhileHovered = false;
  static bool clickedRightWhileHovered = false;
  if (isHovered && isLeftClicked)
    clickedLeftWhileHovered = true;

  if (isHovered && isRightClicked)
    clickedRightWhileHovered = true;

  if (isFocused)
  {
    ImVec2 mouseDelta = io.MouseDelta;

    if (clickedLeftWhileHovered)
    {
      clickedLeftWhileHovered = io.MouseDown[0];
      if (io.MouseDown[0])
      {
        //cam_x -= mouseDelta.x / size.x*cam_width;
        //cam_y += mouseDelta.y / size.y*cam_height;
        udDouble4 translation = pRenderingState->camMatrix.axis.t;
        pRenderingState->camMatrix.axis.t = { 0,0,0,1 };
        pRenderingState->camMatrix = udDouble4x4::rotationAxis({ 0,0,-1 }, mouseDelta.x / 100.0) * pRenderingState->camMatrix; // rotate on global axis and add back in the position
        pRenderingState->camMatrix.axis.t = translation;
        pRenderingState->camMatrix *= udDouble4x4::rotationAxis({ -1,0,0 }, mouseDelta.y / 100.0); // rotate on local axis, since this is the only one there will be no problem
      }
    }

    if (clickedRightWhileHovered)
    {
      clickedRightWhileHovered = io.MouseDown[1];
      if (io.MouseDown[1])
      {
        // do something here on right click
      }
    }

    float speed = 3; // 3 units per second
    if ((modState & KMOD_CTRL) > 0)
      speed *= 0.1; // slow

    if ((modState & KMOD_SHIFT) > 0)
      speed *= 10.0;  // fast

    float deltaMoveForward = speed * ((int)pKeysArray[SDL_SCANCODE_W] - (int)pKeysArray[SDL_SCANCODE_S]);
    float deltaMoveRight = speed * ((int)pKeysArray[SDL_SCANCODE_D] - (int)pKeysArray[SDL_SCANCODE_A]);
    float deltaMoveUp = speed * ((int)pKeysArray[SDL_SCANCODE_R] - (int)pKeysArray[SDL_SCANCODE_F]);

    // Move the camera
    udDouble4 direction;
    if (pRenderingState->planeMode)
    {
      direction = pRenderingState->camMatrix.axis.y * deltaMoveForward + pRenderingState->camMatrix.axis.x * deltaMoveRight + udDouble4{ 0, 0, (double)deltaMoveUp, 0 }; // don't use the camera orientation
    }
    else
    {
      direction = pRenderingState->camMatrix.axis.y * deltaMoveForward + pRenderingState->camMatrix.axis.x * deltaMoveRight;
      direction.z = 0;
      direction += udDouble4{ 0, 0, (double)deltaMoveUp, 0 }; // don't use the camera orientation
    }

    pRenderingState->camMatrix.axis.t += direction * pRenderingState->deltaTime;
  }

  bool wipeUDBuffers = true;
  vaultError err = vaultUDRenderView_SetMatrix(pVaultContainer->pContext, pVaultContainer->pRenderView, vUDRVM_Camera, pRenderingState->camMatrix.a);

  if (err != vE_Success)
    goto epilogue;

  if (modelList.length > 0)
  {
    vaultUDModel *pModelArray[32];
    int len = (int) modelList.length;
    int numValidModels = 0;
    for (int i = 0; i < len; i++)
    {
      pModelArray[numValidModels] = modelList[i].pVaultModel;
      if (pModelArray[numValidModels] != nullptr)
        numValidModels++;
    }
    if (numValidModels > 0)
      wipeUDBuffers = (vaultUDRenderer_Render(pVaultContainer->pContext, pVaultContainer->pRenderer, pVaultContainer->pRenderView, pModelArray, numValidModels) != vE_Success);
  }
  if (wipeUDBuffers)
    memset(pRenderingState->pColorBuffer, 0, sizeof(uint32_t) * pRenderingState->sceneResolution.x * pRenderingState->sceneResolution.y);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, pRenderingState->texId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pRenderingState->sceneResolution.x, pRenderingState->sceneResolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pRenderingState->pColorBuffer);

  glBindFramebuffer(GL_FRAMEBUFFER, pRenderingState->framebuffer);
  glViewport(0, 0, pRenderingState->sceneResolution.x, pRenderingState->sceneResolution.y);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(pRenderingState->udProgramObject);
  glBindTexture(GL_TEXTURE_2D, pRenderingState->texId);

  glBindVertexArray(qrSqVaoID);
  glBindBuffer(GL_ARRAY_BUFFER, qrSqVboID);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, qrSqIboID);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
  glUseProgram(0);
  glBindTexture(GL_TEXTURE_2D, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  ImGui::Image((ImTextureID)((size_t)pRenderingState->texture), size, ImVec2(0, 0), ImVec2(1, -1));

epilogue:
  return;
}

int vcMainMenuGui(RenderingState *pRenderingState)
{
  int menuHeight = 0;

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      if (ImGui::MenuItem("Quit", "Alt+F4"))
        pRenderingState->programComplete = true;
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
      ImGui::MenuItem("Scene", nullptr, &pRenderingState->windowsOpen[vcdScene]);
      ImGui::MenuItem("Scene Explorer", nullptr, &pRenderingState->windowsOpen[vcdSceneExplorer]);
      ImGui::MenuItem("Settings", nullptr, &pRenderingState->windowsOpen[vcdSettings]);
      ImGui::Separator();
      if (ImGui::BeginMenu("Debug Windows"))
      {
        ImGui::MenuItem("Styling", nullptr, &pRenderingState->windowsOpen[vcdStyling]);
        ImGui::MenuItem("UI Debug Menu", nullptr, &pRenderingState->windowsOpen[vcdUIDemo]);
        ImGui::EndMenu();
      }

      ImGui::EndMenu();
    }

    menuHeight = (int)ImGui::GetWindowSize().y;

    ImGui::EndMainMenuBar();
  }

  return menuHeight;
}

void vcRender(RenderingState *pRenderingState, vaultContainer *pVaultContainer)
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  vaultError err;

  int menuHeight = (!pRenderingState->hasContext) ? 0 : vcMainMenuGui(pRenderingState);

  const Uint8 *pKeysArray = SDL_GetKeyboardState(NULL); // potential duplicate with one in vcRenderScene
  if (pKeysArray[SDL_SCANCODE_F11] != 0)
  {
    if (!F11_Pressed)
    {
      isFullscreen ^= 1;
      F11_Pressed = true;
      if (isFullscreen)
      {
        printf("Fullscreen\n");
        SDL_MaximizeWindow(pRenderingState->pWindow);
      }
      else
      {
        printf("Windowed\n");
        SDL_RestoreWindow(pRenderingState->pWindow);
      }
    }
  }
  else
  {
    F11_Pressed = false;
  }

  if (ImGui::GetIO().DisplaySize.y > 0)
  {
    ImVec2 pos = ImVec2(0.f, (float)menuHeight);
    ImVec2 size = ImGui::GetIO().DisplaySize;
    size.y -= pos.y;

    ImGui::RootDock(pos, ImVec2(size.x, size.y - 25.0f));

    // Draw status bar (no docking)
    ImGui::SetNextWindowSize(ImVec2(size.x, 25.0f), ImGuiSetCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, size.y - 6.0f), ImGuiSetCond_Always);
    ImGui::Begin("statusbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize);
    ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
    ImGui::End();
  }

  if (!pRenderingState->hasContext)
  {
    if (ImGui::Begin("Login"))
    {
      static const char *pErrorMessage = nullptr;
      if (pErrorMessage != nullptr)
        ImGui::Text("%s", pErrorMessage);

      ImGui::InputText("ServerURL", pRenderingState->pServerURL, 1024);
      ImGui::InputText("Username", pRenderingState->pUsername, 1024);
      ImGui::InputText("Password", pRenderingState->pPassword, 1024, ImGuiInputTextFlags_Password | ImGuiInputTextFlags_CharsNoBlank);

      if (ImGui::Button("Login!"))
      {
        err = vaultContext_Connect(&pVaultContainer->pContext, pRenderingState->pServerURL, "ClientSample");
        if (err != vE_Success)
        {
          pErrorMessage = "Could not connect to server...";
        }
        else
        {
          err = vaultContext_Login(pVaultContainer->pContext, pRenderingState->pUsername, pRenderingState->pPassword);
          if (err != vE_Success)
          {
            pErrorMessage = "Could not log in...";
          }
          else
          {
            err = vaultContext_GetLicense(pVaultContainer->pContext, vaultLT_Basic);
            if (err != vE_Success)
            {
              pErrorMessage = "Could not get license...";
            }
            else
            {
              err = vaultUDRenderer_Create(pVaultContainer->pContext, &pVaultContainer->pRenderer);
              if (err == vE_Success)
              {
                err = vaultUDRenderView_Create(pVaultContainer->pContext, &pVaultContainer->pRenderView, pVaultContainer->pRenderer, pRenderingState->sceneResolution.x, pRenderingState->sceneResolution.y);
                if (err == vE_Success)
                  pRenderingState->hasContext = true;
              }
            }
          }
        }
      }

      if (ImGui::Button("Guest Login!"))
      {
        err = vaultContext_Connect(&pVaultContainer->pContext, pRenderingState->pServerURL, "ClientSample");
        if (err != vE_Success)
        {
          pErrorMessage = "Could not connect to server...";
        }
        else
        {
          err = vaultContext_Login(pVaultContainer->pContext, pRenderingState->pUsername, pRenderingState->pPassword);
          if (err != vE_Success)
          {
            pErrorMessage = "Could not log in...";
          }
          else
          {
            err = vaultContext_GetLicense(pVaultContainer->pContext, vaultLT_Basic);
            if (err != vE_Success)
            {
              pErrorMessage = "Could not get license...";
            }
            else
            {
              err = vaultUDRenderer_Create(pVaultContainer->pContext, &pVaultContainer->pRenderer);
              if (err == vE_Success)
              {
                err = vaultUDRenderView_Create(pVaultContainer->pContext, &pVaultContainer->pRenderView, pVaultContainer->pRenderer, pRenderingState->sceneResolution.x, pRenderingState->sceneResolution.y);
                if (err == vE_Success)
                  pRenderingState->hasContext = true;
              }
            }
          }
        }

        if (pErrorMessage != nullptr)
        {
          ImGui::Text("%s", pErrorMessage);
        }
      }
    }

    ImGui::End();
  }
  else
  {
    if (ImGui::BeginDock("Scene", &pRenderingState->windowsOpen[vcdScene], ImGuiWindowFlags_NoScrollbar))
      vcRenderScene(pRenderingState, pVaultContainer);
    ImGui::EndDock();

    if (ImGui::BeginDock("Scene Explorer", &pRenderingState->windowsOpen[vcdSceneExplorer]))
    {
      ImGui::InputText("Model Path", pRenderingState->pModelPath, 1024);

      if (ImGui::Button("Load Model!"))
      {
        // add models to list
        if (modelList.length < 32)
        {
          vcModel model;
          model.modelLoaded = true;
          udStrcpy(model.modelPath, UDARRAYSIZE(model.modelPath), pRenderingState->pModelPath);
          //if (model.pVaultModel != nullptr)
          //  vaultUDModel_Unload(pVaultContainer->pContext, &pVaultContainer->pModel);

          double midPoint[3];
          err = vaultUDModel_Load(pVaultContainer->pContext, &model.pVaultModel, pRenderingState->pModelPath);
          if (err == vE_Success)
          {
            lastModelLoaded = true;
            vaultUDModel_GetLocalMatrix(pVaultContainer->pContext, model.pVaultModel, pRenderingState->modelMatrix.a);
            vaultUDModel_GetModelCenter(pVaultContainer->pContext, model.pVaultModel, midPoint);
            pRenderingState->camMatrix.axis.t = udDouble4::create(midPoint[0], midPoint[1], midPoint[2], 1.0);
            modelList.PushBack(model);
          }
          else
          {
            lastModelLoaded = false;
          }

        }
        else
        {
          //dont load model
        }

      }
      if (!lastModelLoaded)
      {
        ImGui::SameLine();
        ImGui::Text("File not found...");
      }

      udFloat3 modelT = udFloat3::create(pRenderingState->camMatrix.axis.t.toVector3());
      if (ImGui::InputFloat3("Camera Position", &modelT.x))
        pRenderingState->camMatrix.axis.t = udDouble4::create(udDouble3::create(modelT), 1.f);

      if (ImGui::Button("Logout"))
      {
        static const char *pErrorMessage = nullptr;

        if (!vcUnloadModelList(pVaultContainer))
        {
          pErrorMessage = "Error unloading models from list";
        }

        if (pErrorMessage == nullptr)
        {
          err = vaultContext_Logout(pVaultContainer->pContext);
          if (err == vE_Success)
            pRenderingState->hasContext = false;
        }
        else
        {
          ImGui::Text("%s", pErrorMessage);
        }
      }

      if (ImGui::RadioButton("PlaneMode", pRenderingState->planeMode))
        pRenderingState->planeMode = true;
      if (ImGui::RadioButton("HeliMode", !pRenderingState->planeMode))
        pRenderingState->planeMode = false;

      if (ImGui::TreeNode("Model List"))
      {
        int len = (int) modelList.length;

        static int selection = len;
        bool selected = false;
        for (int i = 0; i < len; i++)
        {

          selected = (i == selection);
          if (ImGui::Selectable(modelList[i].modelPath, selected, ImGuiSelectableFlags_AllowDoubleClick))
          {
            selection = i;
            if (ImGui::IsMouseDoubleClicked(0))
            {
              // move camera to midpoint of selected object
              double midPoint[3];
              vaultUDModel_GetModelCenter(pVaultContainer->pContext, modelList[i].pVaultModel, midPoint);
              pRenderingState->camMatrix.axis.t = udDouble4::create(midPoint[0], midPoint[1], midPoint[2], 1.0);
            }
          }
          ImGui::SameLine();

          char buttonID[32] = "";
          udSprintf(buttonID, UDARRAYSIZE(buttonID), "UnloadModel%i", i);
          ImGui::PushID(buttonID);
          if (ImGui::Button("Unload Model"))
          {
            // unload model
            err = vaultUDModel_Unload(pVaultContainer->pContext, &(modelList[i].pVaultModel));
            if (err != vE_Success)
              goto epilogue;
            modelList.RemoveAt(i);
            i--;
            len--;
          }
          ImGui::PopID();
        }
        ImGui::TreePop();
      }

      ImGui::EndDock();
    }

    if (ImGui::BeginDock("StyleEditor", &pRenderingState->windowsOpen[vcdStyling]))
      ImGui::ShowStyleEditor();
    ImGui::EndDock();

    if (ImGui::BeginDock("UIDebugMenu", &pRenderingState->windowsOpen[vcdUIDemo]))
      ImGui::ShowDemoWindow();
    ImGui::EndDock();

    if (ImGui::BeginDock("Settings", &pRenderingState->windowsOpen[vcdSettings]))
    {
      // settings dock
      ImGui::Text("Put settings here");
    }
    ImGui::EndDock();
  }

epilogue:
  //TODO: Cleanup
  return;
}

bool vcUnloadModelList(vaultContainer *pVaultContainer)
{
  vaultError err;
  for (int i = 0; i < (int) modelList.length; i++)
  {
    vaultUDModel *pVaultModel;
    pVaultModel = modelList[i].pVaultModel;
    err = vaultUDModel_Unload(pVaultContainer->pContext, &pVaultModel);
    if (err != vE_Success)
      return false;
    modelList[i].modelLoaded = false;
  }
  while (modelList.length > 0)
    modelList.PopFront();
  //modelList.Deinit();

  return true;
}
