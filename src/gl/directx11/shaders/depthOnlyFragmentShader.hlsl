cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_clipZNear;
  float u_clipZFar;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float3 normal : NORMAL;
  float4 colour : COLOR0;
  float2 depthInfo : TEXCOORD1;
  float2 objectInfo : TEXCOORD2;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
  float Depth0 : SV_Depth;
};

float CalculateDepth(float2 depthInfo)
{
  if (depthInfo.y != 0.0) // orthographic
    return (depthInfo.x / depthInfo.y);
	
  // log-z (perspective)
  float halfFcoef = 1.0 / log2(s_CameraFarPlane + 1.0);
  return log2(depthInfo.x) * halfFcoef;
}

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;

  output.Color0 = float4(0.0, 0.0, 0.0, 0.0);
  
  output.Depth0 = CalculateDepth(input.depthInfo); 
  
  return output;
}
