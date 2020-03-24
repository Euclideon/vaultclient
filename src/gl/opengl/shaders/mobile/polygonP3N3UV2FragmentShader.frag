#version 300 es
precision mediump float;
precision highp int;

layout(std140) uniform type_u_cameraPlaneParams
{
    highp float s_CameraNearPlane;
    highp float s_CameraFarPlane;
    highp float u_clipZNear;
    highp float u_clipZFar;
} u_cameraPlaneParams;

uniform highp sampler2D SPIRV_Cross_Combinedtexture0sampler0;

in highp vec2 in_var_TEXCOORD0;
in highp vec3 in_var_NORMAL;
in highp vec4 in_var_COLOR0;
in highp vec2 in_var_TEXCOORD1;
layout(location = 0) out highp vec4 out_var_SV_Target;

void main()
{
    highp vec4 _48 = texture(SPIRV_Cross_Combinedtexture0sampler0, in_var_TEXCOORD0) * in_var_COLOR0;
    out_var_SV_Target = vec4(_48.xyz * ((dot(in_var_NORMAL, normalize(vec3(0.85000002384185791015625, 0.1500000059604644775390625, 0.5))) * 0.5) + 0.5), _48.w);
    gl_FragDepth = log2(in_var_TEXCOORD1.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
}

