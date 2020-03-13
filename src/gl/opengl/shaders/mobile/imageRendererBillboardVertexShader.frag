#version 300 es
layout (std140) uniform u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

//Input Format
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;

//Output Format
out vec2 v_uv;
out vec4 v_colour;
out float v_fLogDepth;

layout (std140) uniform u_EveryObject
{
  mat4 u_worldViewProjectionMatrix;
  vec4 u_colour;
  vec4 u_screenSize;
};

void main()
{
  gl_Position = u_worldViewProjectionMatrix * vec4(a_pos, 1.0);
  gl_Position.xy += u_screenSize.z * gl_Position.w * u_screenSize.xy * vec2(a_uv.x * 2.0 - 1.0, a_uv.y * 2.0 - 1.0); // expand billboard

  v_uv = vec2(a_uv.x, 1.0 - a_uv.y);
  v_colour = u_colour;
  v_fLogDepth = 1.0 + gl_Position.w;
}
