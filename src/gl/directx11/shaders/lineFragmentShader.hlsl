cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_clipZNear;
  float u_clipZFar;
};

//Input Format
struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 colour : COLOR0;
  float2 depthInfo : TEXCOORD0;
};

//Output Format
struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
  float4 Normal : SV_Target1;
  float Depth0 : SV_Depth;
};

float4 packNormal(float3 normal, float objectId, float depth)
{
  float zSign = step(0, normal.z) * 2 - 1; // signed 0
  return float4(objectId, zSign * depth, normal.x, normal.y);
}

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

  output.Color0 = input.colour;

  output.Depth0 = CalculateDepth(input.depthInfo);

  output.Normal = packNormal(float3(0.0, 0.0, 0.0), 0.0, output.Depth0);
  return output;
}
