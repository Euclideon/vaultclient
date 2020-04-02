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
  float2 fLogDepth : TEXCOORD1;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target;
  float Depth0 : SV_Depth;
};

sampler albedoSampler;
Texture2D albedoTexture;

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  float4 col = albedoTexture.Sample(albedoSampler, input.uv);
  float4 diffuseColour = col * input.colour;

  // some fixed lighting
  float3 lightDirection = normalize(float3(0.85, 0.15, 0.5));
  float ndotl = dot(input.normal, lightDirection) * 0.5 + 0.5;
  float3 diffuse = diffuseColour.xyz * ndotl;

  output.Color0 = float4(diffuse, diffuseColour.a);

  float halfFcoef = 1.0 / log2(s_CameraFarPlane + 1.0);
  output.Depth0 = log2(input.fLogDepth.x) * halfFcoef;

  return output;
}
