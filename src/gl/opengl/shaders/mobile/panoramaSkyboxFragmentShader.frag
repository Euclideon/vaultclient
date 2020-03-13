#version 300 es
precision highp float;
layout (std140) uniform u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

uniform sampler2D u_texture;
layout (std140) uniform u_EveryFrame
{
  mat4 u_inverseViewProjection;
};

//Input Format
in vec2 v_uv;

//Output Format
out vec4 out_Colour;

#define PI 3.14159265359

vec2 directionToLatLong(vec3 dir)
{
  vec2 longlat = vec2(atan(dir.x, dir.y) + PI, acos(dir.z));
  return longlat / vec2(2.0 * PI, PI);
}

void main()
{
  // work out 3D point
  vec4 point3D = u_inverseViewProjection * vec4(v_uv * vec2(2.0) - vec2(1.0), 1.0, 1.0);
  point3D.xyz = normalize(point3D.xyz / point3D.w);
  vec4 c1 = texture(u_texture, directionToLatLong(point3D.xyz));

  out_Colour = c1;
}
