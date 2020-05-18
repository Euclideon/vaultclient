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
  float4 partner : COLOR0;
  float4 neighbour : COLOR1;
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
  float4 u_nearPlane;
  float4x4 u_worldViewProjectionMatrix;
};

static const int g_clipNone = 0;
static const int g_clipCurrent = 1;
static const int g_clipPartner = 2;
static const int g_clipBoth = 3;

//Return code specifies which point has been clipped.
int ClipSegment(float3 planeNormal, float planeOffset, inout float3 current, inout float3 partner)
{
  float dc = dot(current, planeNormal) - planeOffset;
  float dp = dot(partner, planeNormal) - planeOffset;

  int result = 0;

  if (dc > 0.0)
  {
    if (dp >= 0.0)
      result = g_clipNone;
    else
      result = g_clipPartner;
  }
  else if (dc < 0.0)
  {
    if (dp <= 0.0)
      result = g_clipBoth;
    else
      result = g_clipCurrent;
  }
  else //dc == 0.0
  {
    if (dp > 0.0)
      result = g_clipNone;
    else
      result = g_clipBoth;
  }

  if (result == g_clipCurrent)
  {
    float frac = -dc / (dp - dc);   
    current = current + frac * (partner - current);
  }
  else if (result == g_clipPartner)
  {
    float frac = -dp / (dc - dp);   
    partner = partner + frac * (current - partner);
  }

  return result;
}

float2 FindDirectionVector(in float2 to, in float2 from)
{
  float2 v = to - from;
  float vLen = length(v);
  if (vLen == 0.0)
    return float2(1.0, 0.0);
  return v / vLen;
}

PS_INPUT main(VS_INPUT input)
{
  PS_INPUT output;

  float3 current = input.pos.xyz;
  float3 partner = input.partner.xyz;

  int result = ClipSegment(u_nearPlane.xyz, u_nearPlane.w, current, partner);

  if (result == g_clipBoth)
  {
    output.pos = float4(0.0, 0.0, 0.0, 0.0);
    output.fLogDepth.x = 0.0;
    return output;
  }

  float4 current4 = float4(current, 1.0);
  float4 partner4 = float4(partner, 1.0);
  current4 = mul(u_worldViewProjectionMatrix, current4);
  partner4 = mul(u_worldViewProjectionMatrix, partner4);
  float2 screenCurrent = (current4.xy / current4.w);
  float2 screenPartner = (partner4.xy / partner4.w);

  float2 Vpc = FindDirectionVector(screenCurrent, screenPartner);

  float2 expand = float2(-Vpc.y, Vpc.x) * input.pos.w;

  //------------------------------------------------------------------------------------
  //Uncomment below for mitre joins...

  //if (result == g_clipCurrent)
  //{
  //  //We don't need the neighbour to find the extend vector
  //  expand = float2(-Vpc.y, Vpc.x) * input.pos.w;
  //}
  //else if (input.neighbour.w == 0.0) //endpoint
  //{
  //  expand = float2(-Vpc.y, Vpc.x) * input.pos.w;
  //}
  //else
  //{
  //  //We need the neighbour to find the extend vector
  //  float3 neighbour = input.neighbour.xyz;

  //  ClipSegment(u_nearPlane.xyz, u_nearPlane.w, current, neighbour);

  //  float4 neighbour4 = float4(neighbour, 1.0);
  //  neighbour4 = mul(u_worldViewProjectionMatrix, neighbour4);
  //  float2 screenNeighbour = (neighbour4.xy / neighbour4.w);

  //  float2 Vcn = FindDirectionVector(screenNeighbour, screenCurrent);

  //  float2 t = normalize(Vcn + Vpc);
  //  expand = float2(-t.y, t.x);

  //  float denom = dot(expand, float2(-Vcn.y, Vcn.x));
  //  denom = max(denom, 0.10);

  //  expand = expand * input.pos.w / denom;
  //}
  //------------------------------------------------------------------------------------

  float aspect = u_thickness.y;
  expand.x /= aspect;
  expand *= u_thickness.x;
  float2 outputPos = screenCurrent + expand;

  output.pos = float4(outputPos.x * current4.w, outputPos.y * current4.w, current4.z, current4.w);
  output.colour = u_colour;
  output.fLogDepth.x = 1.0 + current4.w;

  return output;
}
