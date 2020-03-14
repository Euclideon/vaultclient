#version 330 core
#extension GL_ARB_explicit_attrib_location : enable
layout (std140) uniform u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

//Input format
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texCoord;

//Output Format
out vec2 v_uv;
out vec2 v_edgeSampleUV0;
out vec2 v_edgeSampleUV1;
out vec2 v_edgeSampleUV2;
out vec2 v_edgeSampleUV3;
out vec2 v_sampleStepSize;
out float v_saturation;

layout (std140) uniform u_params
{
  vec4 u_screenParams;  // sampleStepSizex, sampleStepSizeY, (unused), (unused)
  vec4 u_saturation; // saturation, (unused), (unused), (unused)
};

void main()
{
  gl_Position = vec4(a_position.xy, 0.0, 1.0);
  v_uv = vec2(a_texCoord.x, 1.0 - a_texCoord.y);
  v_sampleStepSize = u_screenParams.xy;

  // sample corners
  v_edgeSampleUV0 = v_uv + u_screenParams.xy;
  v_edgeSampleUV1 = v_uv - u_screenParams.xy;
  v_edgeSampleUV2 = v_uv + vec2(u_screenParams.x, -u_screenParams.y);
  v_edgeSampleUV3 = v_uv + vec2(-u_screenParams.x, u_screenParams.y);

  v_saturation = u_saturation.x;
}
