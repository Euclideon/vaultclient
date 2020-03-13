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
in vec2 v_uv;

//Output Format
out vec4 out_Colour;

uniform sampler2D u_texture;
uniform sampler2D u_depth;

void main()
{
  vec4 col = texture(u_texture, v_uv);
  float depth = texture(u_depth, v_uv).x;

  out_Colour = vec4(col.xyz, 1.0); // UD always opaque
  gl_FragDepth = depth;
}
