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
  float2 uv0  : TEXCOORD0;
  float2 uv1  : TEXCOORD1;
  float2 uv2  : TEXCOORD2;
  float2 uv3  : TEXCOORD3;
  float2 uv4  : TEXCOORD4;
  float4 colour : COLOR0;
  float4 stepSizeThickness : COLOR1;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
  float4 Normal : SV_Target1;
};

sampler colourSampler;
Texture2D colourTexture;

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  output.Normal = float4(0, 0, 0, 0);
	
  float4 middle = colourTexture.Sample(colourSampler, input.uv0);

  // 'outside' the geometry, just use the blurred 'distance'
  if (middle.x == 0.0)
  {
    output.Color0 = float4(input.colour.xyz, min(1.0, middle.y * input.stepSizeThickness.z * input.colour.w));
    return output;
  }
  
  float result = 1.0 - middle.y;
  
  // look for an edge, setting to full colour if found
  float softenEdge = 0.15 * input.colour.w;
  result += softenEdge * step(colourTexture.Sample(colourSampler, input.uv1).x - middle.x, -0.00001);
  result += softenEdge * step(colourTexture.Sample(colourSampler, input.uv2).x - middle.x, -0.00001);
  result += softenEdge * step(colourTexture.Sample(colourSampler, input.uv3).x - middle.x, -0.00001);
  result += softenEdge * step(colourTexture.Sample(colourSampler, input.uv4).x - middle.x, -0.00001);
  
  result = max(input.stepSizeThickness.w, result) * input.colour.w; // overlay colour
  output.Color0 = float4(input.colour.xyz, result);
  
  return output;
}
