#version 300 es
layout (std140) uniform u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

//Input format
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texCoord;

//Output Format
out vec2 v_uv0;
out vec2 v_uv1;
out vec2 v_uv2;
out vec2 v_uv3;
out vec2 v_uv4;

vec2 searchKernel[4] = vec2[](vec2(-1, -1), vec2(1, -1), vec2(-1,  1), vec2(1,  1));

layout (std140) uniform u_EveryFrame
{
  vec4 u_stepSizeThickness; // (stepSize.xy, outline thickness, inner overlay strength)
  vec4 u_colour;
};

void main()
{
  gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);

  v_uv0 = vec2(a_texCoord.x, 1.0 - a_texCoord.y);
  v_uv1 = v_uv0 + u_stepSizeThickness.xy * searchKernel[0];
  v_uv2 = v_uv0 + u_stepSizeThickness.xy * searchKernel[1];
  v_uv3 = v_uv0 + u_stepSizeThickness.xy * searchKernel[2];
  v_uv4 = v_uv0 + u_stepSizeThickness.xy * searchKernel[3];
}
