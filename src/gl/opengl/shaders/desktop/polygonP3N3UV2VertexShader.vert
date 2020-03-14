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
layout(location = 2) in vec2 a_uv;
//layout(location = 3) in vec4 a_colour;

//Output Format
out vec2 v_uv;
out vec4 v_colour;
out vec3 v_normal;
out float v_fLogDepth;

layout (std140) uniform u_EveryObject
{
  mat4 u_worldViewProjectionMatrix;
  mat4 u_worldMatrix;
  vec4 u_colour;
};

void main()
{
  // making the assumption that the model matrix won't contain non-uniform scale
  vec3 worldNormal = normalize((u_worldMatrix * vec4(a_normal, 0.0)).xyz);

  gl_Position = u_worldViewProjectionMatrix * vec4(a_pos, 1.0);
  v_fLogDepth = 1.0 + gl_Position.w;

  v_uv = a_uv;
  v_normal = worldNormal;
  v_colour = u_colour;// * a_colour;
}
