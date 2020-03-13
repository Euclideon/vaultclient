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
in vec2 v_uv0;
in vec2 v_uv1;
in vec2 v_uv2;
in vec2 v_uv3;
in vec2 v_uv4;

//Output Format
out vec4 out_Colour;

uniform sampler2D u_texture;
layout (std140) uniform u_EveryFrame
{
  vec4 u_stepSizeThickness; // (stepSize.xy, outline thickness, inner overlay strength)
  vec4 u_colour;
};

void main()
{
  vec4 middle = texture(u_texture, v_uv0);
  float result = middle.w;

  // 'outside' the geometry, just use the blurred 'distance'
  if (middle.x == 0.0)
  {
    out_Colour = vec4(u_colour.xyz, result * u_stepSizeThickness.z * u_colour.a);
    return;
  }

  result = 1.0 - result;

  // look for an edge, setting to full colour if found
  float softenEdge = 0.15 * u_colour.a;
  result += softenEdge * step(texture(u_texture, v_uv1).x - middle.x, -0.00001);
  result += softenEdge * step(texture(u_texture, v_uv2).x - middle.x, -0.00001);
  result += softenEdge * step(texture(u_texture, v_uv3).x - middle.x, -0.00001);
  result += softenEdge * step(texture(u_texture, v_uv4).x - middle.x, -0.00001);

  result = max(u_stepSizeThickness.w, result) * u_colour.w; // overlay colour
  out_Colour = vec4(u_colour.xyz, result);
}
