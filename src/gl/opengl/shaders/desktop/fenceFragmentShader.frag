#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform type_u_cameraPlaneParams
{
    float s_CameraNearPlane;
    float s_CameraFarPlane;
    float u_clipZNear;
    float u_clipZFar;
} u_cameraPlaneParams;

uniform sampler2D SPIRV_Cross_CombinedcolourTexturecolourSampler;

layout(location = 0) in vec4 in_var_COLOR0;
layout(location = 1) in vec2 in_var_TEXCOORD0;
layout(location = 2) in vec2 in_var_TEXCOORD1;
layout(location = 0) out vec4 out_var_SV_Target0;
layout(location = 1) out vec4 out_var_SV_Target1;

void main()
{
    vec4 _42 = texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, in_var_TEXCOORD0);
    float _60 = log2(in_var_TEXCOORD1.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
    out_var_SV_Target0 = vec4(_42.xyz * in_var_COLOR0.xyz, _42.w * in_var_COLOR0.w);
    out_var_SV_Target1 = vec4(0.0, 0.0, 0.0, _60);
    gl_FragDepth = _60;
}

