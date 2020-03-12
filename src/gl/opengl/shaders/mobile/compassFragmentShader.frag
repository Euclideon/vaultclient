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
in vec4 v_colour;
in vec3 v_normal;
in vec4 v_fragClipPosition;
in vec3 v_sunDirection;
in float v_fLogDepth;

//Output Format
out vec4 out_Colour;

void main()
{
  // fake a reflection vector
  vec3 fakeEyeVector = normalize(v_fragClipPosition.xyz / v_fragClipPosition.w);
  vec3 worldNormal = normalize(v_normal * vec3(2.0) - vec3(1.0));
  float ndotl = 0.5 + 0.5 * -dot(v_sunDirection, worldNormal);
  float edotr = max(0.0, -dot(-fakeEyeVector, worldNormal));
  edotr = pow(edotr, 60.0);
  vec3 sheenColour = vec3(1.0, 1.0, 0.9);
  out_Colour = vec4(v_colour.a * (ndotl * v_colour.xyz + edotr * sheenColour), 1.0);

  float halfFcoef  = 1.0 / log2(s_CameraFarPlane + 1.0);
  gl_FragDepth = log2(v_fLogDepth) * halfFcoef;
}
