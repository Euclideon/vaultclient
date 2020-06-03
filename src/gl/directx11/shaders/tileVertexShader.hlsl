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
};

// This should match CPU struct size
#define CONTROL_POINT_RES 3

cbuffer u_EveryObject : register(b0)
{
  float4x4 u_projection;
  float4x4 u_view;
  float4 u_eyePositions[CONTROL_POINT_RES * CONTROL_POINT_RES];
  float4 u_eyeNormals[CONTROL_POINT_RES * CONTROL_POINT_RES];
  float4 u_colour;
  float4 u_objectInfo; // objectId.x
  float4 u_uvOffsetScale;
  float4 u_demUVOffsetScale;
  
  float4 u_NdemUVOffsetScale;
  float4 u_EdemUVOffsetScale;
  float4 u_NEdemUVOffsetScale;
};

sampler demSampler;
Texture2D demTexture;

sampler demNSampler;
Texture2D demNTexture;

sampler demESampler;
Texture2D demETexture;

sampler demNESampler;
Texture2D demNETexture;

//sampler demSSampler;
//Texture2D demSTexture;

//sampler demWSampler;
//Texture2D demWTexture;

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
  float4 pu = (p0 + (p1 - p0) * uvt.x);
  float4 pv = (p2 + (p3 - p2) * uvt.x);
  return (pu + (pv - pu) * uvt.y);
}

float decodeDEM(float2 sample)
{
  return ((sample.x * 255) + (sample.y * 255 * 256)) - 32768.0;
}

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;

  // interpolate between control points to generate a final position for this vertex
  float2 indexUV = input.pos.xy * (CONTROL_POINT_RES - 1.0);
  float4 eyePos = BilinearSample(u_eyePositions, indexUV);
  float4 eyeNormal = BilinearSample(u_eyeNormals, indexUV);

  float2 demUV = u_demUVOffsetScale.xy + u_demUVOffsetScale.zw * input.pos.xy;
  float2 tileHeightSample = demTexture.SampleLevel( demSampler, demUV, 0 ).xy;
  // Reconstruct uint16 in float space and then convert back to int16 in float space
  float tileHeight = decodeDEM(tileHeightSample);
  int count = 1;

  float4 c = u_colour;
  c = float4(demUV, 0, 0);
  if (demUV.y == 0.0 && demUV.x >= (1.0 - (1.0 / 31.0)))
  {
    //float2 NEdemUV = u_NEdemUVOffsetScale.xy + u_NEdemUVOffsetScale.zw * (input.pos.xy * 0.5);
    float2 samplePointNorthEast = float2(0.0, 1.0);
    float2 tileHeightSampleNE = demNETexture.SampleLevel( demNESampler, samplePointNorthEast, 0 ).xy;
	float northEastDem = decodeDEM(tileHeightSampleNE);
    tileHeight = northEastDem;
	
	++count;	
  }
  else if (demUV.y == 0.0)
  {
    float2 EdemUV = u_EdemUVOffsetScale.xy + u_EdemUVOffsetScale.zw * input.pos.xy;
    float2 samplePointNorth = float2(demUV.x, 1.0);
    float2 tileHeightSampleN = demNTexture.SampleLevel( demNSampler, samplePointNorth, 0 ).xy;
	float northDem = decodeDEM(tileHeightSampleN);
    tileHeight = northDem;
	
	++count;
  }
  else if (demUV.x >= (1.0 - (1.0 / 31.0)))
  {
    float2 EdemUV = u_NdemUVOffsetScale.xy + u_NdemUVOffsetScale.zw * (0.5 + input.pos.xy * 0.5);
    float2 samplePointEast = float2(0.0, EdemUV.y);
    float2 tileHeightSampleE = demETexture.SampleLevel( demESampler, samplePointEast, 0 ).xy;
    float eastDEM = decodeDEM(tileHeightSampleE);
	tileHeight = eastDEM;

    c.xy = samplePointEast;
	++count;	
  } 
  
  //tileHeight /= count;
  
  //float2 tileHeightSample = demTexture.SampleLevel( demSampler, demUV, 0 ).xy;
  //float2 tileHeightSample = demTexture.SampleLevel( demSampler, demUV, 0 ).xy;
  //float2 tileHeightSample = demTexture.SampleLevel( demSampler, demUV, 0 ).xy;
		
  float4 finalClipPos = mul(u_projection, (eyePos + eyeNormal * tileHeight));
  finalClipPos.z = CalcuteLogDepth(finalClipPos);
	
  // note: could have precision issues on some devices
  output.colour = c;
  output.uv = u_uvOffsetScale.xy + u_uvOffsetScale.zw * input.pos.xy;
  output.pos = finalClipPos;
  output.depth = float2(output.pos.z, output.pos.w);
  output.objectInfo.x = u_objectInfo.x;
  
  return output;
}
