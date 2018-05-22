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

#endif//vcRenderShaders_h__
