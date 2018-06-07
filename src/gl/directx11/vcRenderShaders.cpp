#include "gl/vcRenderShaders.h"
#include "udPlatform/udPlatformUtil.h"

const char* const g_udFragmentShader = R"shader(
  struct PS_INPUT
  {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
  };

  sampler sampler0;
  Texture2D texture0;

  float4 main(PS_INPUT input) : SV_Target
  {
    float4 out_col = texture0.Sample(sampler0, input.uv);
    return out_col;
    //out_Colour = texture(u_texture, v_texCoord).bgra;
    //gl_FragDepth = texture(u_depth, v_texCoord).x;
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
    float2 uv : TEXCOORD0;
  };

  //Output Format
  out vec4 out_Colour;

  uniform float u_opacity;
  uniform sampler2D u_texture;
  uniform vec3 u_debugColour;

  float4 main(PS_INPUT input) : SV_Target
  {
    float4 col = texture0.Sample(sampler0, input.uv);
    out_Colour = vec4(col.xyz * u_debugColour.xyz, u_opacity);
  }
)shader";

const char* const g_terrainTileVertexShader = R"shader(
  //Input format
  layout(location = 0) in vec3 a_position;
  layout(location = 1) in vec2 a_uv;

  //Output Format
  out vec3 v_colour;
  out vec2 v_uv;

  uniform mat4 u_worldViewProjection0;
  uniform mat4 u_worldViewProjection1;
  uniform mat4 u_worldViewProjection2;
  uniform mat4 u_worldViewProjection3;

  void main()
  {
    vec4 finalClipPos = vec4(0.0);

    // corner id is stored in the x component of the position attribute
    // note: could have precision issues on some devices
    finalClipPos += float(a_position.x == 0.0) * u_worldViewProjection0 * vec4(0.0, 0.0, 0.0, 1.0);
    finalClipPos += float(a_position.x == 1.0) * u_worldViewProjection1 * vec4(0.0, 0.0, 0.0, 1.0);
    finalClipPos += float(a_position.x == 2.0) * u_worldViewProjection2 * vec4(0.0, 0.0, 0.0, 1.0);
    finalClipPos += float(a_position.x == 4.0) * u_worldViewProjection3 * vec4(0.0, 0.0, 0.0, 1.0);

    v_uv = a_uv;
    gl_Position = finalClipPos;
  }
)shader";

const char* const g_vcSkyboxFragmentShader = R"shader(
  uniform samplerCube u_texture;
  uniform mat4 u_inverseViewProjection;

  //Input Format
  in vec2 v_texCoord;

  //Output Format
  out vec4 out_Colour;

  void main()
  {
    vec2 uv = vec2(v_texCoord.x, 1.0 - v_texCoord.y);

    // work out 3D point
    vec4 point3D = u_inverseViewProjection * vec4(uv * vec2(2.0) - vec2(1.0), 1.0, 1.0);
    point3D.xyz = normalize(point3D.xyz / point3D.w);
    vec4 c1 = texture(u_texture, point3D.xyz);

    out_Colour = c1;
  }
)shader";

const char* const g_ImGuiVertexShader = R"vert(
  cbuffer vertexBuffer : register(b0)
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
