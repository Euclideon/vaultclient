#include "gl/vcRenderShaders.h"
#include "udPlatformUtil.h"

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR || UDPLATFORM_EMSCRIPTEN || UDPLATFORM_ANDROID
# define FRAG_HEADER "#version 300 es\nprecision highp float;\nlayout (std140) uniform u_cameraPlaneParams { float s_CameraNearPlane; float s_CameraFarPlane; float u_unused1; float u_unused2; };\n"
# define VERT_HEADER "#version 300 es\nlayout (std140) uniform u_cameraPlaneParams { float s_CameraNearPlane; float s_CameraFarPlane; float u_unused1; float u_unused2; };\n"
#else
# define FRAG_HEADER "#version 330 core\n#extension GL_ARB_explicit_attrib_location : enable\nlayout (std140) uniform u_cameraPlaneParams { float s_CameraNearPlane; float s_CameraFarPlane; float u_unused1; float u_unused2; };\n"
# define VERT_HEADER "#version 330 core\n#extension GL_ARB_explicit_attrib_location : enable\nlayout (std140) uniform u_cameraPlaneParams { float s_CameraNearPlane; float s_CameraFarPlane; float u_unused1; float u_unused2; };\n"
#endif

const char *const g_udGPURenderQuadVertexShader = VERT_HEADER R"shader(
  layout(location = 0) in vec4 a_position;
  layout(location = 1) in vec4 a_color;
  layout(location = 2) in vec2 a_corner;

  out vec4 v_colour;

  layout (std140) uniform u_EveryObject
  {
    mat4 u_worldViewProjectionMatrix;
  };

  void main()
  {
    v_colour = a_color.bgra;

    // Points
    vec4 off = vec4(a_position.www * 2.0, 0);
    vec4 pos0 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.www, 1.0);
    vec4 pos1 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.xww, 1.0);
    vec4 pos2 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.xyw, 1.0);
    vec4 pos3 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.wyw, 1.0);
    vec4 pos4 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.wwz, 1.0);
    vec4 pos5 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.xwz, 1.0);
    vec4 pos6 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.xyz, 1.0);
    vec4 pos7 = u_worldViewProjectionMatrix * vec4(a_position.xyz + off.wyz, 1.0);

    vec4 minPos, maxPos;
    minPos = min(pos0, pos1);
    minPos = min(minPos, pos2);
    minPos = min(minPos, pos3);
    minPos = min(minPos, pos4);
    minPos = min(minPos, pos5);
    minPos = min(minPos, pos6);
    minPos = min(minPos, pos7);
    maxPos = max(pos0, pos1);
    maxPos = max(maxPos, pos2);
    maxPos = max(maxPos, pos3);
    maxPos = max(maxPos, pos4);
    maxPos = max(maxPos, pos5);
    maxPos = max(maxPos, pos6);
    maxPos = max(maxPos, pos7);
    gl_Position = (minPos + (maxPos - minPos) * 0.5);

    vec2 pointSize = vec2(maxPos.x - minPos.x, maxPos.y - minPos.y);
    gl_Position.xy += pointSize * a_corner * 0.5;
  }
)shader";

const char *const g_udGPURenderQuadFragmentShader = FRAG_HEADER R"shader(
  in vec4 v_colour;

  out vec4 out_Colour;

  void main()
  {
    out_Colour = v_colour;
  }
)shader";
