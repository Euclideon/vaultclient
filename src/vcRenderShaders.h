#ifndef vcRenderShaders_h__
#define vcRenderShaders_h__

#include "vcRenderUtils.h"

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
# define FRAG_HEADER "#version 300 es\nprecision highp float;\n"
# define VERT_HEADER "#version 300 es\n"
#else
# define FRAG_HEADER "#version 330 core\n"
# define VERT_HEADER "#version 330 core\n"
#endif

const GLchar* const g_udFragmentShader = FRAG_HEADER R"shader(
uniform sampler2D u_texture;
uniform sampler2D u_depth;

//Input Format
in vec2 v_texCoord;

//Output Format
out vec4 out_Colour;

void main()
{
  out_Colour = texture(u_texture, v_texCoord).bgra;
  gl_FragDepth = texture(u_depth, v_texCoord).x;
}
)shader";

const GLchar* const g_udVertexShader = VERT_HEADER R"shader(
//Input format
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texCoord;

//Output Format
out vec2 v_texCoord;

void main()
{
  gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);
  v_texCoord = a_texCoord;
}
)shader";


const GLchar* const g_terrainTileFragmentShader = FRAG_HEADER R"shader(
//Input Format
in vec2 v_uv;

//Output Format
out vec4 out_Colour;

uniform float u_opacity;
uniform sampler2D u_texture;
uniform vec3 u_debugColour;

void main()
{
  vec4 col = texture(u_texture, v_uv);
  out_Colour = vec4(col.xyz * u_debugColour.xyz, u_opacity);
}
)shader";

const GLchar* const g_terrainTileVertexShader = VERT_HEADER R"shader(
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

const GLchar* const g_vcSkyboxShader = FRAG_HEADER R"shader(

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

#endif//vcRenderShaders_h__
