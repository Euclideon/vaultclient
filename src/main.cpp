#include "vaultContext.h"
#include "vaultUDRenderer.h"
#include "vaultUDRenderView.h"
#include "vaultUDModel.h"
#include "vaultMaths.h"
#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include "imgui_dock.h"
#include "SDL2/SDL.h"
#include "GL/glew.h"

#include <stdlib.h>

struct Float2
{
  float x, y;
};

bool fullscreen = false;
int resx = 1280;
int resy = 720;
GLuint framebuffer;
GLuint texture;
GLuint depthbuffer;

#define GL_VERSION_HEADER "#version 330 core\n"
#define GL_VERTEX_IN "in"
#define GL_VERTEX_OUT "out"
#define GL_FRAGMENT_IN "in"

const GLchar* const g_udFragmentShader =
GL_VERSION_HEADER
"precision highp float;                        \n"
"uniform sampler2D udTexture;                  \n"
GL_FRAGMENT_IN " vec2 texCoord;                \n"
"out vec4 fColor;                              \n"
"void main ()                                  \n"
"{                                             \n"
"  vec2 uv = texCoord.xy;                      \n"
"  vec4 colour = texture2D(udTexture, uv);     \n"
//"  if (colour.a == 0.0) discard;               \n"
"  fColor = vec4(colour.bgr, 1.0);             \n"
"}                                             \n"
"";

const GLchar* const g_udVertexShader =
GL_VERSION_HEADER
GL_VERTEX_IN " vec2 vertex;                    \n"
GL_VERTEX_OUT " vec2 texCoord;                 \n"
"void main(void)                               \n"
"{                                             \n"
"  gl_Position = vec4(vertex, 0.0, 1.0);       \n"
"  texCoord = vertex*vec2(0.5)+vec2(0.5);      \n"
"  texCoord.y = 1-texCoord.y;                  \n"
"}                                             \n"
"";

GLuint GenerateFbVBO(const Float2 *pFloats, size_t len)
{
  GLuint vboID = 0;
  // Create a new VBO and use the variable id to store the VBO id
  glGenBuffers(1, &vboID);

  // Make the new VBO active
  glBindBuffer(GL_ARRAY_BUFFER, vboID);

  // Upload vertex data to the video device
  glBufferData(GL_ARRAY_BUFFER, sizeof(Float2) * len, pFloats, GL_STATIC_DRAW); // GL_DYNAMIC_DRAW

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
  vaultUDModel *pModel;
};

struct RenderingData
{
  SDL_Window *pWindow;
  uint32_t *pColorBuffer;
  float *pDepthBuffer;
  GLuint texId;
  GLuint udVAO;
  GLint udProgramObject;
};

enum ProgramState
{
  PS_Login,
  PS_LoadModel,
  PS_Render
};

struct RenderingState
{
  bool planeMode;
  double deltaTime;
  vaultInt2 deltaMousePos;
  vaultMatrix camMatrix;
  vaultUint322 resolution;
  ProgramState currentState;

  char *pServerURL;
  char *pUsername;
  char *pPassword;
        
  char *pModelPath;
};

bool Render(RenderingState &renderingState, vaultContainer &vContainer, RenderingData &renderingData);

#undef main
#ifdef SDL_MAIN_NEEDED
int SDL_main(int /*argc*/, char ** /*args*/)
#else
int main(int /*argc*/, char ** /*args*/)
#endif
{
  SDL_GLContext glcontext = NULL;
  GLint udTextureLocation = -1;
  
  const Float2 fboDataArr[] = { { -1.f,-1.f },{ -1.f,1.f },{ 1,1 },{ -1.f,-1.f },{ 1.f,-1.f },{ 1,1 } };
  GLuint fbVboId = (GLuint)-1;

  RenderingData renderingData = { NULL, NULL, NULL, (GLuint)-1, (GLuint)-1, (GLint)-1 };
  RenderingState renderingState = { true, 0, { }, vaultMatrix_Identity(), { 1280, 800 }, PS_Login, NULL, NULL, NULL, NULL };
  vaultContainer vContainer = { };

  // default string values.
  const char *plocalHost = "http://vau-ubu-pro-001.euclideon.local";
  const char *pUsername = "Name";
  const char *pPassword = "Pass";
  const char *pModelPath = "R:\\ConvertedModels\\Aerometrex\\Aerometrix_GoldCoast_Model_1CM.uds";

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

  bool done = false;
  Uint64 NOW;
  Uint64 LAST;

  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    goto epilogue;

  // Setup window
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) != 0)
    goto epilogue;
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) != 0)
    goto epilogue;

  renderingData.pWindow = SDL_CreateWindow("Euclideon Client", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, renderingState.resolution.x, renderingState.resolution.y, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!renderingData.pWindow)
    goto epilogue;

  glcontext = SDL_GL_CreateContext(renderingData.pWindow);
  if (!glcontext)
    goto epilogue;

  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK)
    goto epilogue;

  if (!ImGui_ImplSdlGL3_Init(renderingData.pWindow))
    goto epilogue;

  renderingData.udProgramObject = glBuildProgram(glBuildShader(GL_VERTEX_SHADER, g_udVertexShader), glBuildShader(GL_FRAGMENT_SHADER, g_udFragmentShader));
  if (renderingData.udProgramObject == -1)
    goto epilogue;

  glUseProgram(renderingData.udProgramObject);

  udTextureLocation = glGetUniformLocation(renderingData.udProgramObject, "udTexture");
  glUniform1i(udTextureLocation, 0);

  glGenVertexArrays(1, &renderingData.udVAO);
  glBindVertexArray(renderingData.udVAO);

  fbVboId = GenerateFbVBO(fboDataArr, 6);

  glBindBuffer(GL_ARRAY_BUFFER, fbVboId);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(float) * 2, 0);

  glBindVertexArray(0);
  glUseProgram(0);

  glGenTextures(1, &renderingData.texId);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, renderingData.texId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  renderingData.pColorBuffer = new uint32_t[renderingState.resolution.x*renderingState.resolution.y];
  renderingData.pDepthBuffer = new float[renderingState.resolution.x*renderingState.resolution.y];

  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  glGenRenderbuffers(1, &depthbuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, resx, resy);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer);

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, resx, resy, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
  GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
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

  while (!done)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSdlGL3_ProcessEvent(&event);
      done = event.type == SDL_QUIT;
    }

    LAST = NOW;
    NOW = SDL_GetPerformanceCounter();
    renderingState.deltaTime = double(NOW - LAST) / SDL_GetPerformanceFrequency();

    SDL_GetRelativeMouseState(&renderingState.deltaMousePos.x, &renderingState.deltaMousePos.y);

    ImGui_ImplSdlGL3_NewFrame(renderingData.pWindow);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, renderingState.resolution.x, renderingState.resolution.y);

    if (!Render(renderingState, vContainer, renderingData))
      goto epilogue;

    ImGui::Render();
    SDL_GL_SwapWindow(renderingData.pWindow);
  }

  ImGui::SaveDock();

epilogue:
  vaultUDModel_Unload(vContainer.pContext, &vContainer.pModel);

  vaultUDRenderView_Destroy(vContainer.pContext, &vContainer.pRenderView);
  vaultUDRenderer_Destroy(vContainer.pContext, &vContainer.pRenderer);

  vaultContext_Disconnect(&vContainer.pContext);
  delete[] renderingData.pColorBuffer;
  delete[] renderingData.pDepthBuffer;

  delete[] renderingState.pServerURL;
  delete[] renderingState.pUsername;
  delete[] renderingState.pPassword;
  delete[] renderingState.pModelPath;

  return 0;
}

void vcRenderScene(RenderingState &renderingState, vaultContainer &vContainer, RenderingData &renderingData)
{
  //Rendering
  ImVec2 size = ImGui::GetContentRegionAvail();
  ImGuiIO& io = ImGui::GetIO();
  ImVec2 cursorPos = ImGui::GetCursorScreenPos();          // "cursor" is where imgui will draw the image
  ImVec2 mousePos = io.MousePos;

  if (renderingState.resolution.x != size.x || renderingState.resolution.y != size.y) //Resize buffers
  {
    renderingState.resolution.x = (uint32_t)size.x;
    renderingState.resolution.y = (uint32_t)size.y;

    //Resize GPU Targets
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, renderingState.resolution.x, renderingState.resolution.y, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, renderingState.resolution.x, renderingState.resolution.y);

    //Resize CPU Targets
    delete renderingData.pColorBuffer;
    delete renderingData.pDepthBuffer;

    renderingData.pColorBuffer = new uint32_t[renderingState.resolution.x*renderingState.resolution.y];
    renderingData.pDepthBuffer = new float[renderingState.resolution.x*renderingState.resolution.y];

    vaultUDRenderView_Destroy(vContainer.pContext, &vContainer.pRenderView);
    vaultUDRenderView_Create(vContainer.pContext, &vContainer.pRenderView, vContainer.pRenderer, renderingState.resolution.x, renderingState.resolution.y);
  }

  bool isHovered = ImGui::IsItemHovered();
  bool isLeftClicked = ImGui::IsMouseClicked(0, false);
  bool isRightClicked = ImGui::IsMouseClicked(1, false);
  bool isFocused = ImGui::IsWindowFocused();

  const Uint8 *pKeysArray = SDL_GetKeyboardState(NULL);

  int forwardMovement = (int)pKeysArray[SDL_SCANCODE_W] - (int)pKeysArray[SDL_SCANCODE_S];
  int rightMovement = (int)pKeysArray[SDL_SCANCODE_D] - (int)pKeysArray[SDL_SCANCODE_A];
  int upMovement = (int)pKeysArray[SDL_SCANCODE_R] - (int)pKeysArray[SDL_SCANCODE_F];

  vaultDouble4 direction;
  if (renderingState.planeMode)
  {
    direction = renderingState.camMatrix.axis.y * forwardMovement + renderingState.camMatrix.axis.x * rightMovement + vaultDouble4{ 0, 0, (double)upMovement, 0 }; // don't use the camera orientation
  }
  else
  {
    direction = renderingState.camMatrix.axis.y * forwardMovement + renderingState.camMatrix.axis.x * rightMovement;
    direction.z = 0;
    direction += vaultDouble4{ 0, 0, (double)upMovement, 0 }; // don't use the camera orientation
  }

  renderingState.camMatrix.axis.t += direction * renderingState.deltaTime;

  if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT) && !ImGui::IsMouseHoveringAnyWindow())
  {
    vaultDouble4 translation = renderingState.camMatrix.axis.t;
    renderingState.camMatrix.axis.t = { 0,0,0,1 };
    renderingState.camMatrix = vaultMatrix_RotationAxis({ 0,0,-1 }, renderingState.deltaMousePos.x / 100.0) * renderingState.camMatrix; // rotate on global axis and add back in the position
    renderingState.camMatrix.axis.t = translation;
    renderingState.camMatrix *= vaultMatrix_RotationAxis({ -1,0,0 }, renderingState.deltaMousePos.y / 100.0); // rotate on local axis, since this is the only one there will be no problem
  }

  vaultError err = vaultUDRenderView_SetMatrix(vContainer.pContext, vContainer.pRenderView, vUDRVM_Camera, renderingState.camMatrix.a);
  if (err != vE_Success)
    goto epilogue;

  if (vContainer.pModel != nullptr)
  {
    err = vaultUDRenderer_Render(vContainer.pContext, vContainer.pRenderer, vContainer.pRenderView, &vContainer.pModel, 1);
    if (err != vE_Success)
      goto epilogue;
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, renderingData.texId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderingState.resolution.x, renderingState.resolution.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, renderingData.pColorBuffer);
  
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glViewport(0, 0, renderingState.resolution.x, renderingState.resolution.y);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(renderingData.udProgramObject);
  glBindTexture(GL_TEXTURE_2D, renderingData.texId);
  glBindVertexArray(renderingData.udVAO);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  glBindVertexArray(0);
  glUseProgram(0);
  glBindTexture(GL_TEXTURE_2D, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  ImGui::Image((ImTextureID)((size_t)texture), size, ImVec2(0, 0), ImVec2(1, -1));

epilogue:
  return;
}

int vcMainMenuGui()
{
  int menuHeight = 0;

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      if (ImGui::MenuItem("New"))
      {

      }

      if (ImGui::MenuItem("Open", "Ctrl+O"))
      {

      }

      if (ImGui::BeginMenu("Open Recent"))
      {
        //ImGui::MenuItem("fish_hat.h");
        ImGui::EndMenu();
      }

      if (ImGui::MenuItem("Quit", "Alt+F4"))
      {

      }

      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit"))
    {
      if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
      if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
      ImGui::Separator();
      if (ImGui::MenuItem("Cut", "CTRL+X")) {}
      if (ImGui::MenuItem("Copy", "CTRL+C")) {}
      if (ImGui::MenuItem("Paste", "CTRL+V")) {}
      ImGui::EndMenu();
    }

    menuHeight = (int)ImGui::GetWindowSize().y;

    ImGui::EndMainMenuBar();
  }

  return menuHeight;
}

bool Render(RenderingState &renderingState, vaultContainer &vContainer, RenderingData &renderingData)
{
  static bool showScene = true;
  static bool show_scene5 = true;
  static bool show_scene6 = true;

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  vaultError err;

  bool result = false;

  int menuHeight = (renderingState.currentState == PS_Login) ? 0 : vcMainMenuGui();

  if (ImGui::GetIO().DisplaySize.y > 0) {
    ////////////////////////////////////////////////////
    // Setup root docking window                      //
    // taking into account menu height and status bar //
    ////////////////////////////////////////////////////
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

  if (renderingState.currentState == PS_Login)
  {
    if (ImGui::Begin("Login"))
    {
      ImGui::InputText("ServerURL", renderingState.pServerURL, 1024);
      ImGui::InputText("Username", renderingState.pUsername, 1024);
      ImGui::InputText("Password", renderingState.pPassword, 1024);

      if (ImGui::Button("Login!"))
      {
        err = vaultContext_Connect(&vContainer.pContext, renderingState.pServerURL, "ClientSample");
        if (err != vE_Success)
          goto epilogue;

        err = vaultContext_Login(vContainer.pContext, renderingState.pUsername, renderingState.pPassword);
        if (err != vE_Success)
          goto epilogue;

        err = vaultContext_GetLicense(vContainer.pContext, vaultLT_Basic);
        if (err != vE_Success)
          goto epilogue;

        err = vaultUDRenderer_Create(vContainer.pContext, &vContainer.pRenderer);
        if (err != vE_Success)
          goto epilogue;

        err = vaultUDRenderView_Create(vContainer.pContext, &vContainer.pRenderView, vContainer.pRenderer, renderingState.resolution.x, renderingState.resolution.y);
        if (err != vE_Success)
          goto epilogue;

        renderingState.currentState = (ProgramState)(renderingState.currentState + 1);
      }

      if (ImGui::Button("Guest Login!"))
      {
        err = vaultContext_Connect(&vContainer.pContext, renderingState.pServerURL, "ClientSample");
        if (err != vE_Success)
          goto epilogue;

        err = vaultContext_GetLicense(vContainer.pContext, vaultLT_Basic);
        if (err != vE_Success)
          goto epilogue;

        err = vaultUDRenderer_Create(vContainer.pContext, &vContainer.pRenderer);
        if (err != vE_Success)
          goto epilogue;

        err = vaultUDRenderView_Create(vContainer.pContext, &vContainer.pRenderView, vContainer.pRenderer, renderingState.resolution.x, renderingState.resolution.y);
        if (err != vE_Success)
          goto epilogue;

        renderingState.currentState = (ProgramState)(renderingState.currentState + 1);
      }
    }

    ImGui::End();
  }
  else
  {
    if (ImGui::BeginDock("Scene", &showScene, ImGuiWindowFlags_NoScrollbar))
      vcRenderScene(renderingState, vContainer, renderingData);
    ImGui::EndDock();

    if (ImGui::BeginDock("Dummy3", &show_scene5))
      ImGui::Text("Placeholder!");
    ImGui::EndDock();

    if (ImGui::BeginDock("StyleEditor", &show_scene6))
      ImGui::ShowStyleEditor();
    ImGui::EndDock();

    //Load Model
    if (ImGui::BeginDock("Load Model"))
    {
      ImGui::InputText("Model Path", renderingState.pModelPath, 1024);

      if (ImGui::Button("Load Model!"))
      {
        renderingState.currentState = (ProgramState)(renderingState.currentState + 1);
        err = vaultUDModel_Load(vContainer.pContext, &vContainer.pModel, renderingState.pModelPath);
        if (err != vE_Success)
          goto epilogue;

        err = vaultUDRenderView_SetTargets(vContainer.pContext, vContainer.pRenderView, renderingData.pColorBuffer, 0, renderingData.pDepthBuffer);
        if (err != vE_Success)
          goto epilogue;

        err = vaultUDRenderView_SetMatrix(vContainer.pContext, vContainer.pRenderView, vUDRVM_Camera, renderingState.camMatrix.a);
        if (err != vE_Success)
          goto epilogue;
      }
    }
    ImGui::EndDock();

    if (ImGui::BeginDock("Settings"))
    {
      if (ImGui::Button("Unload Model"))
      {
        renderingState.currentState = PS_LoadModel;
        err = vaultUDModel_Unload(vContainer.pContext, &vContainer.pModel);
        if (err != vE_Success)
          goto epilogue;
      }

      if (ImGui::Button("Logout"))
      {
        err = vaultUDModel_Unload(vContainer.pContext, &vContainer.pModel);
        if (err != vE_Success)
          goto epilogue;

        err = vaultContext_Logout(vContainer.pContext);
        if (err != vE_Success)
          goto epilogue;

        err = vaultContext_GetLicense(vContainer.pContext, vaultLT_Basic);
        if (err != vE_Success)
          goto epilogue;

        renderingState.currentState = PS_Login;
      }

      if (ImGui::RadioButton("PlaneMode", renderingState.planeMode))
        renderingState.planeMode = true;
      if (ImGui::RadioButton("HeliMode", !renderingState.planeMode))
        renderingState.planeMode = false;
    }
    ImGui::EndDock();

  }

  result = true;

epilogue:
  return result;
}
