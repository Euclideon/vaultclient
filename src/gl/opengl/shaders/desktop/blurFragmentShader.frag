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
in vec2 v_uv0;
in vec2 v_uv1;
in vec2 v_uv2;

//Output Format
out vec4 out_Colour;

uniform sampler2D u_texture;

vec4 kernel[3] = vec4[](vec4(0.0, 0.0, 0.0, 0.27901),
                        vec4(1.0, 1.0, 1.0, 0.44198),
                        vec4(0.0, 0.0, 0.0, 0.27901));

void main()
{
  vec4 colour = vec4(0);

  colour += kernel[0] * texture(u_texture, v_uv0);
  colour += kernel[1] * texture(u_texture, v_uv1);
  colour += kernel[2] * texture(u_texture, v_uv2);

  out_Colour = colour;
}
