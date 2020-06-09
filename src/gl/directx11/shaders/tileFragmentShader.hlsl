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
  float3x3 tbn : TEXCOORD4;
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
  return float4(normal.x, normal.y, objectId, depth);
}

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  float4 col = colourTexture.Sample(colourSampler, input.uv);
  
  float4 normal = normalTexture.Sample(normalSampler, input.normalUV);
  normal.xyz = normal.xyz * float3(2.0, 2.0, 2.0) - float3(1.0, 1.0, 1.0);
  
  //normal.xy = normal.yx;
  
  normal.xyz = normalize(mul(input.tbn, normal.xyz));
  //normal.xz = -normal.xz;
  
  // vertex normals
  //normal.xyz = input.colour.xyz; 
  
  output.Color0 = float4(col.xyz, input.colour.w);// * 0.000001 + float4(normal.xyz, 1.0);
  
  float scale = 1.0 / (u_clipZFar - u_clipZNear);
  float bias = -(u_clipZNear * 0.5);
  float depth = (input.depth.x / input.depth.y) * scale + bias; // depth packed here
  
  output.Normal = packNormal(normal.xyz, input.objectInfo.x, depth); 
  
  return output;
}
