#version 330 core
#extension GL_ARB_explicit_attrib_location : enable
layout (std140) uniform u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

uniform sampler2D u_texture;

//Input Format
in vec2 v_uv;
in vec4 v_tintColour;

//Output Format
out vec4 out_Colour;

void main()
{
  vec4 colour = texture(u_texture, v_uv).rgba;
  float effectiveAlpha = min(colour.a, v_tintColour.a);
  out_Colour = vec4((colour.rgb * effectiveAlpha) + (v_tintColour.rgb * (1.0 - effectiveAlpha)), 1);
}
