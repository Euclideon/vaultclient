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
in vec4 v_colour;
in vec3 v_normal;
in float v_fLogDepth;

//Output Format
out vec4 out_Colour;

uniform sampler2D u_texture;

void main()
{
  vec4 col = texture(u_texture, v_uv);
  vec4 diffuseColour = col * v_colour;

  // some fixed lighting
  vec3 lightDirection = normalize(vec3(0.85, 0.15, 0.5));
  float ndotl = dot(v_normal, lightDirection) * 0.5 + 0.5;
  vec3 diffuse = diffuseColour.xyz * ndotl;

  out_Colour = vec4(diffuse, diffuseColour.a);

  float halfFcoef  = 1.0 / log2(s_CameraFarPlane + 1.0);
  gl_FragDepth = log2(v_fLogDepth) * halfFcoef;
}
