#version 330 core
#extension GL_ARB_explicit_attrib_location : enable
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

layout (std140) uniform u_EveryFrame
{
  vec4 u_stepSize; // remember: requires 16 byte alignment
};

void main()
{
  gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);

  // sample on edges, taking advantage of bilinear sampling
  vec2 sampleOffset = 1.42 * u_stepSize.xy;
  vec2 uv = vec2(a_texCoord.x, 1.0 - a_texCoord.y);
  v_uv0 = uv - sampleOffset;
  v_uv1 = uv;
  v_uv2 = uv + sampleOffset;
}
