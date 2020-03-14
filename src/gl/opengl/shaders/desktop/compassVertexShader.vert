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
layout(location = 1) in vec3 a_normal;

//Output Format
out vec4 v_colour;
out vec3 v_normal;
out vec4 v_fragClipPosition;
out vec3 v_sunDirection;
out float v_fLogDepth;

layout (std140) uniform u_EveryObject
{
  mat4 u_worldViewProjectionMatrix;
  vec4 u_colour;
  vec3 u_sunDirection;
  float _padding;
};

void main()
{
  gl_Position = u_worldViewProjectionMatrix * vec4(a_pos, 1.0);

  v_normal = ((a_normal * 0.5) + 0.5);
  v_colour = u_colour;
  v_sunDirection = u_sunDirection;
  v_fragClipPosition = gl_Position;

  v_fLogDepth = 1.0 + gl_Position.w;
}
