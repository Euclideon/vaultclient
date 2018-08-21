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

  cbuffer u_params : register(b0)
  {
    float4 u_screenParams;  // sampleStepX, sampleStepSizeY, near plane, far plane
    float4x4 u_inverseViewProjection;
    
    // outlining
    float4 u_outlineColour;
    float4 u_outlineParams;   // outlineWidth, threshold, (unused), (unused)
    
    // colour by height
    float4 u_colourizeHeightColourMin;
    float4 u_colourizeHeightColourMax;
    float4 u_colourizeHeightParams; // min world height, max world height, (unused), (unused)
    
    // colour by depth
    float4 u_colourizeDepthColour;
    float4 u_colourizeDepthParams; // min distance, max distance, (unused), (unused)

    // contours
    float4 u_contourColour;
    float4 u_contourParams; // contour distance, contour band height, (unused), (unused)
  };

  sampler sampler0;
  Texture2D texture0;

  sampler sampler1;
  Texture2D texture1;

  float linearizeDepth(float depth)
  {
    float nearPlane = u_screenParams.z;
    float farPlane = u_screenParams.w;
    return (2.0 * nearPlane) / (farPlane + nearPlane - depth * (farPlane - nearPlane));
  }

  float getNormalizedPosition(float v, float min, float max)
  {
    return clamp((v - min) / (max - min), 0.0, 1.0);
  }

  float3 edgeHighlight(float3 col, float2 uv, float depth)
  {
    float3 sampleOffsets = float3(u_screenParams.xy, 0.0);
    float edgeOutlineThreshold = u_outlineParams.y;

    float ld0 = linearizeDepth(depth);
    float ld1 = linearizeDepth(texture1.Sample(sampler1, uv + sampleOffsets.xz).x);
    float ld2 = linearizeDepth(texture1.Sample(sampler1, uv - sampleOffsets.xz).x);
    float ld3 = linearizeDepth(texture1.Sample(sampler1, uv + sampleOffsets.zy).x);
    float ld4 = linearizeDepth(texture1.Sample(sampler1, uv - sampleOffsets.zy).x);
    
    float isEdge = 1.0 - step(ld0 - ld1, edgeOutlineThreshold) * step(ld0 - ld2, edgeOutlineThreshold) * step(ld0 - ld3, edgeOutlineThreshold) * step(ld0 - ld4, edgeOutlineThreshold);

    float3 edgeColour = lerp(col.xyz, u_outlineColour.xyz, u_outlineColour.w);
    return lerp(col.xyz, edgeColour, isEdge);
  }

  float3 contourColour(float3 col, float3 fragWorldPosition)
  {
    float contourDistance = u_contourParams.x;
    float contourBandHeight = u_contourParams.y;
  
    float isCountour = step(contourBandHeight, fmod(fragWorldPosition.z, contourDistance));
    float3 contourColour = lerp(col.xyz, u_contourColour.xyz, u_contourColour.w);
    return lerp(contourColour, col.xyz, isCountour);
  }

  float3 colourizeByHeight(float3 col, float3 fragWorldPosition)
  { 
    float2 worldColourMinMax = u_colourizeHeightParams.xy;

    float minMaxColourStrength = getNormalizedPosition(fragWorldPosition.z, worldColourMinMax.x, worldColourMinMax.y);
    
    float3 minColour = lerp(col.xyz, u_colourizeHeightColourMin.xyz, u_colourizeHeightColourMin.w);
    float3 maxColour = lerp( col.xyz, u_colourizeHeightColourMax.xyz,u_colourizeHeightColourMax.w);
    return lerp(minColour, maxColour, minMaxColourStrength);
  }

  float3 colourizeByDepth(float3 col, float depth)
  {
    float farPlane = u_screenParams.w;
    float linearDepth = linearizeDepth(depth) * farPlane;   
    float2 depthColourMinMax = u_colourizeDepthParams.xy;

    float depthColourStrength = getNormalizedPosition(linearDepth, depthColourMinMax.x, depthColourMinMax.y);

    return lerp(col.xyz, u_colourizeDepthColour.xyz, depthColourStrength * u_colourizeDepthColour.w);
  }

  PS_OUTPUT main(PS_INPUT input)
  {
    PS_OUTPUT output;

    float4 col = texture0.Sample(sampler0, input.uv);
    float depth = texture1.Sample(sampler1, input.uv).x;

    float4 fragWorldPosition = mul(u_inverseViewProjection, float4(input.uv.x * 2.0 - 1.0, (1.0 - input.uv.y) * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0));
    fragWorldPosition /= fragWorldPosition.w;

    col.xyz = colourizeByHeight(col.xyz, fragWorldPosition.xyz);
    col.xyz = colourizeByDepth(col.xyz, depth);
   
    float edgeOutlineWidth = u_outlineParams.x;
    if (edgeOutlineWidth > 0.0)
      col.xyz = edgeHighlight(col.xyz, input.uv, depth);

    col.xyz = contourColour(col.xyz, fragWorldPosition.xyz);

    output.Color0 = col; 
    output.Depth0 = depth * 2.0 - 1;
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

  // This should match CPU struct size
  #define VERTEX_COUNT 3

  cbuffer u_EveryObject : register(b0)
  {
    float4x4 u_projection;
    float4 u_eyePositions[VERTEX_COUNT * VERTEX_COUNT];
    float4 u_colour;
  };

  PS_INPUT main(VS_INPUT input)
  {
    PS_INPUT output;

    // note: could have precision issues on some devices
    float4 finalClipPos = mul(u_projection, u_eyePositions[int(input.pos.x)]);

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
