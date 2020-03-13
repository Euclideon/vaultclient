#version 330 core
#extension GL_ARB_explicit_attrib_location : enable
layout (std140) uniform u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

layout(location = 0) in vec2 Position;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec4 Color;

out vec2 Frag_UV;
out vec4 Frag_Color;

layout (std140) uniform u_EveryObject
{
  mat4 u_worldViewProjectionMatrix;
  vec4 u_screenSize;
};

void main()
{
  gl_Position = u_worldViewProjectionMatrix * vec4(0.0, 0.0, 0.0, 1.0);
  gl_Position.xy += u_screenSize.xy * vec2(Position.x, -Position.y) * gl_Position.w;

  Frag_UV = UV;
  Frag_Color = Color;
}
