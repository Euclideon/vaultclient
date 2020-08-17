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
  float2 depth : TEXCOORD1;
  float2 objectInfo : TEXCOORD2;
  float2 normalUV : TEXCOORD3;
  float3 normal: TEXCOORD4;
  float3 bitangent: TEXCOORD5;
};

// This should match CPU struct size
#define CONTROL_POINT_RES 3

cbuffer u_EveryObject : register(b0)
{
  float4x4 u_projection;
  float4x4 u_view;
  float4 u_eyePositions[CONTROL_POINT_RES * CONTROL_POINT_RES];
  float4 u_colour;
  float4 u_objectInfo; // objectId.x, skirtLength.y
  float4 u_uvOffsetScale;
  float4 u_demUVOffsetScale;
  float4 u_worldNormals[CONTROL_POINT_RES * CONTROL_POINT_RES];
  float4 u_worldBitangents[CONTROL_POINT_RES * CONTROL_POINT_RES];
};

sampler demSampler;
Texture2D demTexture;

// this works for highly tesselated geometry
float CalcuteLogDepth(float4 clipPos)
{
  float Fcoef = (u_clipZFar - u_clipZNear) / log2(s_CameraFarPlane + 1.0);
  return (log2(max(1e-6, 1.0 + clipPos.w)) * Fcoef + u_clipZNear) * clipPos.w;
}

float4 BilinearSample(float4 samples[CONTROL_POINT_RES * CONTROL_POINT_RES], float2 uv)
{
  // whole
  float ui = floor(uv.x);
  float vi = floor(uv.y);
  float ui2 = min((CONTROL_POINT_RES - 1.0), ui + 1.0);
  float vi2 = min((CONTROL_POINT_RES - 1.0), vi + 1.0);
  
  // fraction
  float2 uvt = float2(uv.x - ui, uv.y - vi);
  
  float4 p0 = samples[int(vi * CONTROL_POINT_RES + ui)];
  float4 p1 = samples[int(vi * CONTROL_POINT_RES + ui2)];
  float4 p2 = samples[int(vi2 * CONTROL_POINT_RES + ui)];
  float4 p3 = samples[int(vi2 * CONTROL_POINT_RES + ui2)];
  
  // bilinear position
  float4 pu = lerp(p0, p1, uvt.x);
  float4 pv = lerp(p2, p3, uvt.x);
  return lerp(pu, pv, uvt.y);
}

float demHeight(float2 uv)
{
  float2 tileHeightSample = demTexture.SampleLevel( demSampler, uv, 0 ).xy;
  // Reconstruct uint16 in float space and then convert back to int16 in float space
  return ((tileHeightSample.x * 255) + (tileHeightSample.y * 255 * 256)) - 32768.0;
}

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;

  // interpolate between control points to generate a final position for this vertex
  float2 indexUV = input.pos.xy * (CONTROL_POINT_RES - 1.0);
  float3 worldNormal = normalize(BilinearSample(u_worldNormals, indexUV)).xyz;
  float4 eyePos = BilinearSample(u_eyePositions, indexUV);
  float4 eyeNormal = mul(u_view, float4(worldNormal, 0.0));

  float2 demUV = u_demUVOffsetScale.xy + u_demUVOffsetScale.zw * input.pos.xy;
  float tileHeight = demHeight(demUV);
  
  // add a 'skirt' to the tile edge
  tileHeight += input.pos.z * u_objectInfo.y;

  float4 finalClipPos = mul(u_projection, (eyePos + eyeNormal * tileHeight));
  finalClipPos.z = CalcuteLogDepth(finalClipPos);
	
  // note: could have precision issues on some devices
  output.colour = u_colour;
  output.uv = u_uvOffsetScale.xy + u_uvOffsetScale.zw * input.pos.xy;
  output.pos = finalClipPos;
  output.depth = float2(output.pos.z, output.pos.w);
  output.objectInfo.x = u_objectInfo.x;
  output.normalUV = demUV;
  output.normal = worldNormal;
  output.bitangent = normalize(BilinearSample(u_worldBitangents, indexUV)).xyz;
	
  return output;
}
