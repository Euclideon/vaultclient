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
out vec2 v_uv;
out vec4 v_tintColour;

layout (std140) uniform u_EveryFrame
{
  vec4 u_tintColour; //0 is full colour, 1 is full image
  vec4 u_imageSize; //For purposes of tiling/stretching
};

void main()
{
  gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);
  v_uv = vec2(a_texCoord.x, 1.0 - a_texCoord.y) / u_imageSize.xy;
  v_tintColour = u_tintColour;
}
