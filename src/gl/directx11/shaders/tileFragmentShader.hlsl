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
  float2 normalUV : TEXCOORD3;
  float3 normal: TEXCOORD4;
  float3 bitangent: TEXCOORD5;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
  float4 Normal : SV_Target1;
};

sampler colourSampler;
Texture2D colourTexture;

sampler normalSampler;
Texture2D normalTexture;

float4 packNormal(float3 normal, float objectId, float depth)
{
  float zSign = step(0, normal.z) * 2 - 1; // signed 0
  return float4(objectId, zSign * depth, normal.x, normal.y);
}

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  float4 col = colourTexture.Sample(colourSampler, input.uv);

  float3 normal = normalTexture.Sample(normalSampler, input.normalUV).xyz;
  normal = normal * float3(2.0, 2.0, 2.0) - float3(1.0, 1.0, 1.0);
  
  float3 tangent = normalize(cross(input.bitangent, input.normal));
  float3x3 tbn = float3x3(tangent, input.bitangent, input.normal);
  normal.y *= -1; // TODO: Investigate this flip
  normal = normalize(mul(normal, tbn));
  
  output.Color0 = float4(col.xyz * input.colour.xyz, input.colour.w);
  
  float scale = 1.0 / (u_clipZFar - u_clipZNear);
  float bias = -(u_clipZNear * 0.5);
  float depth = (input.depth.x / input.depth.y) * scale + bias; // depth packed here
  
  output.Normal = packNormal(normal, input.objectInfo.x, depth);   
  return output;
}
