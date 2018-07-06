#include "gl/vcRenderShaders.h"
#include "udPlatform/udPlatformUtil.h"

const char* const g_udFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
  };

  struct PS_OUTPUT {
    float4 Color0 : SV_Target;
    float Depth0 : SV_Depth;
  };

  sampler sampler0;
  Texture2D texture0;

  sampler sampler1;
  Texture2D texture1;

  PS_OUTPUT main(PS_INPUT input)
  {
    PS_OUTPUT output;

    output.Color0 = texture0.Sample(sampler0, input.uv);
    output.Depth0 = texture1.Sample(sampler1, input.uv).x * 2.0 - 1;

    return output;
  }
)shader";

const char* const g_udVertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;
    output.pos = float4(input.pos.xy, 0.f, 1.f);
    output.uv  = input.uv;
    return output;
  }
)shader";


const char* const g_terrainTileFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 colour : COLOR0;
    float2 uv : TEXCOORD0;
  };

  sampler sampler0;
  Texture2D texture0;

  float4 main(PS_INPUT input) : SV_Target
  {
    return texture0.Sample(sampler0, input.uv) * input.colour;
  }
)shader";

const char* const g_terrainTileVertexShader = R"shader(
  struct VS_INPUT
  {
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 colour : COLOR0;
    float2 uv : TEXCOORD0;
  };

  cbuffer u_EveryObject : register(b0)
  {
    float4x4 u_worldViewProjection0;
    float4x4 u_worldViewProjection1;
    float4x4 u_worldViewProjection2;
    float4x4 u_worldViewProjection3;
    float4 u_colour;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;

    float4 finalClipPos = float4(0.f, 0.f, 0.f, 0.f);

    // corner id is stored in the x component of the position attribute
    // note: could have precision issues on some devices

    if (input.pos.x == 0.f)
      finalClipPos = mul(u_worldViewProjection0, float4(0.f, 0.f, 0.f, 1.f));
    else if (input.pos.x == 1.f)
      finalClipPos = mul(u_worldViewProjection1, float4(0.f, 0.f, 0.f, 1.f));
    else if (input.pos.x == 2.f)
      finalClipPos = mul(u_worldViewProjection2, float4(0.f, 0.f, 0.f, 1.f));
    else if (input.pos.x == 4.f)
      finalClipPos = mul(u_worldViewProjection3, float4(0.f, 0.f, 0.f, 1.f));

    output.colour = u_colour;
    output.uv = input.uv;
    output.pos = finalClipPos;
    return output;
  }
)shader";

const char* const g_vcSkyboxFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
  };

  cbuffer u_EveryFrame : register(b0)
  {
    float4x4 u_inverseViewProjection;
  };

  sampler sampler0;
  Texture2DArray u_texture;

  float4 main(PS_INPUT input) : SV_Target
  {
    float2 uv = float2(input.uv.x, 1.0 - input.uv.y);

    // work out 3D point
    float4 point3D = mul(u_inverseViewProjection, float4(uv * float2(2.0, 2.0) - float2(1.0, 1.0), 1.0, 1.0));
    point3D.xyz = normalize(point3D.xyz / point3D.w);

    float3 absolutes = float3(abs(point3D.x), abs(point3D.y), abs(point3D.z));

    float3 coords = float3(0.f, 0.f, 0.f);

    if( absolutes.x > absolutes.y && absolutes.x > absolutes.z)
    {
      if(point3D.x > 0)
      {
        coords.z = 0.f;
        coords.x = -point3D.z / absolutes.x;
        coords.y = point3D.y / absolutes.x; // correct
      }
      else
      {
        coords.z = 1.f;
        coords.x = point3D.z / absolutes.x;
        coords.y = point3D.y / absolutes.x; // correct
      }
    }
    else if(absolutes.y > absolutes.x && absolutes.y > absolutes.z)
    {
      if(point3D.y > 0)
      {
        coords.z = 3.f;
        coords.x = point3D.x / absolutes.y;
        coords.y = -point3D.z / absolutes.y;
      }
      else
      {
        coords.z = 2.f;
        coords.x = point3D.x / absolutes.y;
        coords.y = point3D.z / absolutes.y;
      }
    }
    else
    {
      if(point3D.z > 0)
      {
        coords.z = 4.f;
        coords.xy = point3D.xy / absolutes.z;
      }
      else
      {
        coords.z = 5.f;
        coords.x = -point3D.x / absolutes.z;
        coords.y = point3D.y / absolutes.z;
      }
    }

    return u_texture.Sample(sampler0, float3((coords.xy + 1.f) / 2.f, coords.z));
  }
)shader";

const char* const g_ImGuiVertexShader = R"vert(
  cbuffer u_EveryFrame : register(b0)
  {
    float4x4 ProjectionMatrix;
  };

  struct VS_INPUT
  {
    float2 pos : POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
  };

  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;
    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
    output.col = input.col;
    output.uv  = input.uv;
    return output;
  }
)vert";

const char* const g_ImGuiFragmentShader = R"frag(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
  };

  sampler sampler0;
  Texture2D texture0;

  float4 main(PS_INPUT input) : SV_Target
  {
    float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
    return out_col;
  }
)frag";
