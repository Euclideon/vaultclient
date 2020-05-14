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
  float4 colour : COLOR0;
  float2 uv : TEXCOORD0;
  float2 depth : TEXCOORD1;
  float2 objectInfo : TEXCOORD2;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
  float4 Normal : SV_Target1;
  float Depth0 : SV_Depth;
};

sampler colourSampler;
Texture2D colourTexture;

float4 packNormal(float3 normal, float objectId, float depth)
{
  return float4(normal.x / (1.0 - normal.z), normal.y / (1.0 - normal.z),
                objectId,
	            depth);
}

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  float4 col = colourTexture.Sample(colourSampler, input.uv);

  output.Color0 = float4(col.xyz * input.colour.xyz, input.colour.w);
  
  float halfFcoef = 1.0 / log2(s_CameraFarPlane + 1.0);
  output.Depth0 = log2(input.depth.x) * halfFcoef;
  
  //float scale = 1.0 / (u_clipZFar - u_clipZNear);
  //float bias = -(u_clipZNear * 0.5);
  //float depth = (input.depth.x / input.depth.y) * scale + bias; // depth packed here
  
  output.Normal = packNormal(float3(0, 0, 0), input.objectInfo.x, output.Depth0);//depth); 
  return output;
}
