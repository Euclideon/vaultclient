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
layout(location = 0) out vec4 out_var_SV_Target;

float _33;

void main()
{
    float _56 = log2(in_var_TEXCOORD1.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
    vec4 _57 = vec4(texture(SPIRV_Cross_CombinedcolourTexturecolourSampler, in_var_TEXCOORD0).xyz * in_var_COLOR0.xyz, _33);
    _57.w = _56;
    out_var_SV_Target = _57;
    gl_FragDepth = _56;
}

