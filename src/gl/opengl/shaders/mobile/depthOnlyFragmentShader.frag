#version 300 es
precision highp float;
layout (std140) uniform u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

// Input format
in float v_fLogDepth;

//Output Format
out vec4 out_Colour;

void main()
{
  out_Colour = vec4(0.0);

  float halfFcoef  = 1.0 / log2(s_CameraFarPlane + 1.0);
  gl_FragDepth = log2(v_fLogDepth) * halfFcoef;
}
