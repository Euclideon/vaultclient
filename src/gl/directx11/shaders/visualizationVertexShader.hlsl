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
};

cbuffer u_vertParams : register(b1)
{
  float4 u_outlineStepSize; // outlineStepSize.xy (in uv space), (unused), (unused)
}

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;
  output.pos = float4(input.pos.xy, 0.f, 1.f);
  output.uv = input.uv;

  float3 sampleOffsets = float3(u_outlineStepSize.xy, 0.0);
  output.edgeSampleUV0 = output.uv + sampleOffsets.xz;
  output.edgeSampleUV1 = output.uv - sampleOffsets.xz;
  output.edgeSampleUV2 = output.uv + sampleOffsets.zy;
  output.edgeSampleUV3 = output.uv - sampleOffsets.zy;

  return output;
}
