#ifndef vcRenderShaders_h__
#define vcRenderShaders_h__

#include "vcRenderUtils.h"

const GLchar* const g_udFragmentShader = R"shader(#version 330 core

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

const GLchar* const g_udVertexShader = R"shader(#version 330 core

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


const GLchar* const g_terrainTileFragmentShader = R"shader(#version 330 core

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

const GLchar* const g_terrainTileVertexShader = R"shader(#version 330 core

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

#endif//vcRenderShaders_h__
