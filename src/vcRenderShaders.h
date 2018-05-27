#ifndef vcRenderShaders_h__
#define vcRenderShaders_h__

#include "vcRenderUtils.h"

#if UDPLATFORM_IOS || UDPLATFORM_IOS_SIMULATOR
# define FRAG_HEADER "#version 300 es\nprecision mediump float;\n"
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
in vec3 v_colour;
in vec2 v_uv;

//Output Format
out vec4 out_Colour;

uniform sampler2D u_texture;

void main()
{
  vec4 col = texture(u_texture, v_uv);
  out_Colour = vec4(col.xyz * v_colour.xyz, 1.0);
}
)shader";

const GLchar* const g_terrainTileVertexShader = VERT_HEADER R"shader(
//Input format
layout(location = 0) in vec3 a_position;

//Output Format
out vec3 v_colour;
out vec2 v_uv;

uniform mat4 u_viewProjection;
uniform mat4 u_world;
uniform vec3 u_debugColour;

uniform sampler2D u_texture; // temporary height map

void main()
{
  v_uv = vec2(a_position.x, 1.0 - a_position.y);
  vec4 col = texture(u_texture, v_uv); // todo: this may be a dependent read on device. Todo: pass in uvs as attribute

  vec4 worldPosition = u_world * vec4(a_position, 1.0);
  //worldPosition.z = 10.0 * length(col.xyz) / 3.0f;
  gl_Position = u_viewProjection * worldPosition;
  v_colour = u_debugColour;

}
)shader";

const GLchar* const g_vcSkyboxShader = R"shader(#version 330 core

#extension GL_NV_shadow_samplers_cube : enable

uniform samplerCube CubemapSampler;
uniform mat4x4 invSkyboxMatrix;

//Input Format
in vec2 v_texCoord;

//Output Format
out vec4 out_Colour;

void main()
{
  vec2 uv = vec2(v_texCoord.x, 1 - v_texCoord.y);

  // work out 3D point
  vec4 point3D = invSkyboxMatrix * vec4(uv * 2 - 1, 1, 1);
  point3D.xyz = normalize(point3D.xyz / point3D.w);
  vec4 c1 = texture(CubemapSampler, point3D.xyz);

  out_Colour = c1;
  //out_Colour = vec4(1,0,0,1);
}
)shader";

#endif//vcRenderShaders_h__
