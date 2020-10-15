#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform type_u_cameraPlaneParams
{
    float s_CameraNearPlane;
    float s_CameraFarPlane;
    float u_clipZNear;
    float u_clipZFar;
} u_cameraPlaneParams;

uniform sampler2D SPIRV_Cross_CombinedalbedoTexturealbedoSampler;

layout(location = 0) in vec2 in_var_TEXCOORD0;
layout(location = 1) in vec4 in_var_COLOR0;
layout(location = 2) in vec2 in_var_TEXCOORD1;
layout(location = 3) in vec2 in_var_TEXCOORD2;
layout(location = 0) out vec4 out_var_SV_Target0;
layout(location = 1) out vec4 out_var_SV_Target1;

void main()
{
    vec4 _45 = texture(SPIRV_Cross_CombinedalbedoTexturealbedoSampler, in_var_TEXCOORD0);
    float _63;
    switch (0u)
    {
        default:
        {
            if (in_var_TEXCOORD1.y != 0.0)
            {
                _63 = in_var_TEXCOORD1.x / in_var_TEXCOORD1.y;
                break;
            }
            _63 = log2(in_var_TEXCOORD1.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
            break;
        }
    }
    vec4 _70 = vec4(in_var_TEXCOORD2.x, ((step(0.0, 0.0) * 2.0) - 1.0) * _63, 0.0, 0.0);
    _70.w = 1.0;
    out_var_SV_Target0 = _45 * in_var_COLOR0;
    out_var_SV_Target1 = _70;
}

