cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_unused1;
  float u_unused2;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv0  : TEXCOORD0;
  float2 uv1  : TEXCOORD1;
  float2 uv2  : TEXCOORD2;
  float2 uv3  : TEXCOORD3;
  float2 uv4  : TEXCOORD4;
  float4 colour : COLOR0;
  float4 stepSizeThickness : COLOR1;
};

sampler sampler0;
Texture2D u_texture;

float4 main(PS_INPUT input) : SV_Target
{
  float4 middle = u_texture.Sample(sampler0, input.uv0);
  float result = middle.w;

  // 'outside' the geometry, just use the blurred 'distance'
  if (middle.x == 0.0)
    return float4(input.colour.xyz, result * input.stepSizeThickness.z * input.colour.a);

  result = 1.0 - result;

  // look for an edge, setting to full colour if found
  float softenEdge = 0.15 * input.colour.a;
  result += softenEdge * step(u_texture.Sample(sampler0, input.uv1).x - middle.x, -0.00001);
  result += softenEdge * step(u_texture.Sample(sampler0, input.uv2).x - middle.x, -0.00001);
  result += softenEdge * step(u_texture.Sample(sampler0, input.uv3).x - middle.x, -0.00001);
  result += softenEdge * step(u_texture.Sample(sampler0, input.uv4).x - middle.x, -0.00001);

  result = max(input.stepSizeThickness.w, result) * input.colour.w; // overlay colour
  return float4(input.colour.xyz, result);
}
