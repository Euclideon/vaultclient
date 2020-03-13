#version 330 core
#extension GL_ARB_explicit_attrib_location : enable
layout (std140) uniform u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

//Input Format
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal; //unused
layout(location = 2) in vec2 a_uv;

//Output Format
out vec2 v_uv;
out vec4 v_colour;
out float v_fLogDepth;

layout (std140) uniform u_EveryObject
{
  mat4 u_worldViewProjectionMatrix;
  vec4 u_colour;
  vec4 u_screenSize; // unused
};

void main()
{
  gl_Position = u_worldViewProjectionMatrix * vec4(a_pos, 1.0);

  v_uv = a_uv;
  v_colour = u_colour;
  v_fLogDepth = 1.0 + gl_Position.w;
}
