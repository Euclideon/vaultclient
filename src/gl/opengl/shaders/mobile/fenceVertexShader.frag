#version 300 es
layout (std140) uniform u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_ribbonInfo; // xyz: expand vector; z: pair id (0 or 1)

out vec2 v_uv;
out vec4 v_colour;
out float v_fLogDepth;

layout (std140) uniform u_EveryFrame
{
  vec4 u_bottomColour;
  vec4 u_topColour;

  float u_orientation;
  float u_width;
  float u_textureRepeatScale;
  float u_textureScrollSpeed;
  float u_time;

  vec3 _padding;
};

layout (std140) uniform u_EveryObject
{
  mat4 u_worldViewProjectionMatrix;
};

void main()
{
  // fence horizontal UV pos packed into Y channel
  v_uv = vec2(mix(a_uv.y, a_uv.x, u_orientation) * u_textureRepeatScale - u_time * u_textureScrollSpeed, a_ribbonInfo.w);
  v_colour = mix(u_bottomColour, u_topColour, a_ribbonInfo.w);

  // fence or flat
  vec3 worldPosition = a_position + mix(vec3(0, 0, a_ribbonInfo.w) * u_width, a_ribbonInfo.xyz, u_orientation);

  gl_Position = u_worldViewProjectionMatrix * vec4(worldPosition, 1.0);
  v_fLogDepth = 1.0 + gl_Position.w;
}
