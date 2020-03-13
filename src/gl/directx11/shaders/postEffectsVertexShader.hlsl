cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

struct VS_INPUT
{
  float3 pos : POSITION;
  float2 uv  : TEXCOORD0;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float2 edgeSampleUV0 : TEXCOORD1;
  float2 edgeSampleUV1 : TEXCOORD2;
  float2 edgeSampleUV2 : TEXCOORD3;
  float2 edgeSampleUV3 : TEXCOORD4;
  float2 sampleStepSize : TEXCOORD5;
  float saturation : TEXCOORD6;
};

cbuffer u_params : register(b0)
{
  float4 u_screenParams;  // sampleStepSizex, sampleStepSizeY, (unused), (unused)
  float4 u_saturation; // saturation, (unused), (unused), (unused)
}

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;
  output.pos = float4(input.pos.xy, 0.f, 1.f);
  output.uv = input.uv;
  output.sampleStepSize = u_screenParams.xy;

  // sample corners
  output.edgeSampleUV0 = output.uv + u_screenParams.xy;
  output.edgeSampleUV1 = output.uv - u_screenParams.xy;
  output.edgeSampleUV2 = output.uv + float2(u_screenParams.x, -u_screenParams.y);
  output.edgeSampleUV3 = output.uv + float2(-u_screenParams.x, u_screenParams.y);

  output.saturation = u_saturation.x;

  return output;
}
