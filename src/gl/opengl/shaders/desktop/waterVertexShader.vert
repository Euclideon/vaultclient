#version 330 core
#extension GL_ARB_explicit_attrib_location : enable
layout (std140) uniform u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

layout(location = 0) in vec2 a_position;

out vec2 v_uv0;
out vec2 v_uv1;
out vec4 v_fragEyePos;
out vec3 v_colour;

layout (std140) uniform u_EveryFrameVert
{
  vec4 u_time;
};

layout (std140) uniform u_EveryObject
{
  vec4 u_colourAndSize;
  mat4 u_worldViewMatrix;
  mat4 u_worldViewProjectionMatrix;
};

void main()
{
  float uvScaleBodySize = u_colourAndSize.w; // packed here

  // scale the uvs with time
  float uvOffset = u_time.x * 0.0625;
  v_uv0 = uvScaleBodySize * a_position.xy * vec2(0.25) - vec2(uvOffset, uvOffset);
  v_uv1 = uvScaleBodySize * a_position.yx * vec2(0.50) - vec2(uvOffset, uvOffset * 0.75);

  v_fragEyePos = u_worldViewMatrix * vec4(a_position, 0.0, 1.0);
  v_colour = u_colourAndSize.xyz;

  gl_Position = u_worldViewProjectionMatrix * vec4(a_position, 0.0, 1.0);
}
