cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_clipZNear;
  float u_clipZFar;
};

struct VS_INPUT
{
  float3 pos : POSITION;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 colour : COLOR0;
  float2 uv : TEXCOORD0;
  float2 fLogDepth : TEXCOORD1;
};

// This should match CPU struct size
#define CONTROL_POINT_RES 3

cbuffer u_EveryObject : register(b0)
{
  float4x4 u_projection;
  float4x4 u_view;
  float4 u_eyePositions[CONTROL_POINT_RES * CONTROL_POINT_RES];
  float4 u_colour;
  float4 u_uvOffsetScale;
  float4 u_demUVOffsetScale;
  float4 u_tileNormal;
};

sampler sampler1;
Texture2D texture1;

// this could be used instead instead of writing to depth directly,
// for highly tesselated geometry (hopefully tiles in the future)
//float CalcuteLogDepth(float4 clipPos)
//{
//  float Fcoef  = 2.0 / log2(s_CameraFarPlane + 1.0);
//  return log2(max(1e-6, 1.0 + clipPos.w)) * Fcoef - 1.0;
//}

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;

  float4 eyePos = float4(0, 0, 0, 0);

  // interpolate between control points to generate a final position for this vertex
  {
    float2 indexUV = input.pos.xy * (CONTROL_POINT_RES - 1.0);

    float ui = floor(indexUV.x);
    float vi = floor(indexUV.y);
    float ui2 = min((CONTROL_POINT_RES - 1.0), ui + 1.0);
    float vi2 = min((CONTROL_POINT_RES - 1.0), vi + 1.0);
    float2 uvt = float2(indexUV.x - ui, indexUV.y - vi);

    // bilinear position
    float4 p0 = u_eyePositions[int(vi * CONTROL_POINT_RES + ui)];
    float4 p1 = u_eyePositions[int(vi * CONTROL_POINT_RES + ui2)];
    float4 p2 = u_eyePositions[int(vi2 * CONTROL_POINT_RES + ui)];
    float4 p3 = u_eyePositions[int(vi2 * CONTROL_POINT_RES + ui2)];

    float4 pu = (p0 + (p1 - p0) * uvt.x);
    float4 pv = (p2 + (p3 - p2) * uvt.x);
    eyePos = (pu + (pv - pu) * uvt.y);
  }

  float2 demUV = u_demUVOffsetScale.xy + u_demUVOffsetScale.zw * input.pos.xy;
  float tileHeight = texture1.SampleLevel( sampler1, demUV, 0 ).x * 32768.0;
  float3 heightOffset = u_tileNormal.xyz * tileHeight;
  float4 h = mul(u_view, float4(heightOffset, 1.0));
  float4 baseH = mul(u_view, float4(0, 0, 0, 1.0));
  float4 diff = (h - baseH);

  float4 finalClipPos = mul(u_projection, (eyePos + diff));
  
  // note: could have precision issues on some devices
  output.colour = u_colour;
  output.uv = u_uvOffsetScale.xy + u_uvOffsetScale.zw * input.pos.xy;
  output.pos = finalClipPos;
  output.fLogDepth.x = 1.0 + output.pos.w;

  return output;
}
