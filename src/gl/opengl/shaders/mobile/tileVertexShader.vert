#version 300 es
layout (std140) uniform u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

//Input format
layout(location = 0) in vec3 a_uv;

//Output Format
out vec4 v_colour;
out vec2 v_uv;
out float v_fLogDepth;

// This should match CPU struct size
#define VERTEX_COUNT 3

layout (std140) uniform u_EveryObject
{
  mat4 u_projection;
  vec4 u_eyePositions[VERTEX_COUNT * VERTEX_COUNT];
  vec4 u_colour;
  vec4 u_uvOffsetScale;
};

// this could be used instead instead of writing to depth directly,
// for highly tesselated geometry (hopefully tiles in the future)
//float CalcuteLogDepth(vec4 clipPos)
//{
//  float Fcoef  = 2.0 / log2(s_CameraFarPlane + 1.0);
//  return log2(max(1e-6, 1.0 + clipPos.w)) * Fcoef - 1.0;
//}

void main()
{
  // TODO: could have precision issues on some devices
  vec4 finalClipPos = u_projection * u_eyePositions[int(a_uv.z)];

  v_uv = u_uvOffsetScale.xy + u_uvOffsetScale.zw * a_uv.xy;
  v_colour = u_colour;
  gl_Position = finalClipPos;
  v_fLogDepth = 1.0 + gl_Position.w;

  //gl_Position.z = CalcuteLogDepth(gl_Position);
}
