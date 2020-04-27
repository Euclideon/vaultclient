cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_clipZNear;
  float u_clipZFar;
};

struct VS_INPUT
{
  float4 pos : POSITION;
  float4 previous : COLOR0;
  float4 next : COLOR1;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 colour : COLOR0;
  float2 fLogDepth : TEXCOORD0;
};

cbuffer u_EveryObject : register(b0)
{
  float4 u_colour;
  float4 u_thickness;
  float4x4 u_worldViewProjectionMatrix;
};

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;

  float aspect = u_thickness.y;
  
  float4 clipPos = mul(u_worldViewProjectionMatrix, float4(input.pos.xyz, 1.0));
  float2 currentScreen = (clipPos.xy / clipPos.w) * float2(0.5, 0.5) + float2(0.5, 0.5);
  
  float4 previousScreen = mul(u_worldViewProjectionMatrix, float4(input.previous.xyz, 1.0));
  previousScreen.xy = (previousScreen.xy / previousScreen.w)* float2(0.5, 0.5) + float2(0.5, 0.5);
  float4 nextScreen = mul(u_worldViewProjectionMatrix, float4(input.next.xyz, 1.0));
  nextScreen.xy = (nextScreen.xy / nextScreen.w)* float2(0.5, 0.5) + float2(0.5, 0.5);

  // handle clipping
  if (sign(previousScreen.w) != sign(nextScreen.w))
  {
    // TODO: Handle me
  }
  
  // correct aspect ratio
  currentScreen.x *= aspect;
  nextScreen.x *= aspect;
  previousScreen.x *= aspect;

  float2 dirPrev = normalize(currentScreen.xy - previousScreen.xy);
  float2 dirNext = normalize(nextScreen.xy - currentScreen.xy);
  float2 dir = normalize(dirPrev + dirNext);
  float2 normal = float2(-dir.y, dir.x);
  
  // artificially make corners thicker to try keep line width consistent
  float cornerThicken = (1.0 - (dot(dirPrev, dirNext) * 0.5 + 0.5));
  // extrude from center
  normal *= u_thickness.x / 2.0 * (1.0 + pow(cornerThicken, 7.0) * 7.0);
  normal.x /= aspect;

  output.pos = clipPos + float4(normal * input.pos.w * clipPos.w, 0.0, 0.0);
  output.fLogDepth.x = 1.0 + clipPos.w;
  
  output.colour = u_colour;
  return output;
}
