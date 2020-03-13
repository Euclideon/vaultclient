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
in vec4 v_fragEyePos;
in vec3 v_colour;

//Output Format
out vec4 out_Colour;

layout (std140) uniform u_EveryFrameFrag
{
  vec4 u_specularDir;
  mat4 u_eyeNormalMatrix;
  mat4 u_inverseViewMatrix;
};

uniform sampler2D u_normalMap;
uniform sampler2D u_skybox;

#define PI 3.14159265359

vec2 directionToLatLong(vec3 dir)
{
  vec2 longlat = vec2(atan(dir.x, dir.y) + PI, acos(dir.z));
  return longlat / vec2(2.0 * PI, PI);
}

void main()
{
  vec3 specularDir = normalize(u_specularDir.xyz);

  vec3 normal0 = texture(u_normalMap, v_uv0).xyz * vec3(2.0) - vec3(1.0);
  vec3 normal1 = texture(u_normalMap, v_uv1).xyz * vec3(2.0) - vec3(1.0);
  vec3 normal = normalize((normal0.xyz + normal1.xyz));

  vec3 eyeToFrag = normalize(v_fragEyePos.xyz);
  vec3 eyeSpecularDir = normalize((u_eyeNormalMatrix * vec4(specularDir, 0.0)).xyz);
  vec3 eyeNormal = normalize((u_eyeNormalMatrix * vec4(normal, 0.0)).xyz);
  vec3 eyeReflectionDir = normalize(reflect(eyeToFrag, eyeNormal));

  float nDotS = abs(dot(eyeReflectionDir, eyeSpecularDir));
  float nDotL = -dot(eyeNormal, eyeToFrag);
  float fresnel = nDotL * 0.5 + 0.5;

  float specular = pow(nDotS, 50.0) * 0.5;

  vec3 deepFactor = vec3(0.35, 0.35, 0.35);
  vec3 shallowFactor = vec3(1.0, 1.0, 0.6);

  float waterDepth = pow(max(0.0, dot(normal, vec3(0.0, 0.0, 1.0))), 5.0); // guess 'depth' based on normal direction
  vec3 refractionColour = v_colour.xyz * mix(shallowFactor, deepFactor, waterDepth);

  // reflection
  vec4 worldFragPos = u_inverseViewMatrix * vec4(eyeReflectionDir, 0.0);
  vec4 skybox = texture(u_skybox, directionToLatLong(normalize(worldFragPos.xyz)));
  vec3 reflectionColour = skybox.xyz;

  vec3 finalColour = mix(reflectionColour, refractionColour, fresnel * 0.75) + vec3(specular);
  out_Colour = vec4(finalColour, 1.0);
}
