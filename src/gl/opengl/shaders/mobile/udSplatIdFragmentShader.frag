#version 300 es
precision highp float;
layout (std140) uniform u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

//Input Format
in vec2 v_uv;

//Output Format
out vec4 out_Colour;

layout (std140) uniform u_params
{
  vec4 u_idOverride;
};

uniform sampler2D u_texture;
uniform sampler2D u_depth;

bool floatEquals(float a, float b)
{
  return abs(a - b) <= 0.0015f;
}

void main()
{
  gl_FragDepth = texture(u_depth, v_uv).x;
  out_Colour = vec4(0.0);

  vec4 col = texture(u_texture, v_uv);
  if (u_idOverride.w == 0.0 || floatEquals(u_idOverride.w, col.w))
  {
    out_Colour = vec4(col.w, 0, 0, 1.0);
  }
}
