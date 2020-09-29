#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform type_u_cameraPlaneParams
{
    float s_CameraNearPlane;
    float s_CameraFarPlane;
    float u_clipZNear;
    float u_clipZFar;
} u_cameraPlaneParams;

layout(location = 0) in vec3 in_var_COLOR0;
layout(location = 1) in vec4 in_var_COLOR1;
layout(location = 2) in vec3 in_var_COLOR2;
layout(location = 3) in vec4 in_var_COLOR3;
layout(location = 4) in vec2 in_var_TEXCOORD0;
layout(location = 0) out vec4 out_var_SV_Target0;
layout(location = 1) out vec4 out_var_SV_Target1;

void main()
{
    vec3 _54 = (in_var_COLOR0 * vec3(2.0)) - vec3(1.0);
    float _89;
    switch (0u)
    {
        default:
        {
            if (in_var_TEXCOORD0.y != 0.0)
            {
                _89 = in_var_TEXCOORD0.x / in_var_TEXCOORD0.y;
                break;
            }
            _89 = log2(in_var_TEXCOORD0.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
            break;
        }
    }
    out_var_SV_Target0 = vec4(((in_var_COLOR1.xyz * (0.5 + (dot(in_var_COLOR2, _54) * (-0.5)))) + (vec3(1.0, 1.0, 0.89999997615814208984375) * pow(max(0.0, -dot(-normalize(in_var_COLOR3.xyz / vec3(in_var_COLOR3.w)), _54)), 60.0))) * in_var_COLOR1.w, 1.0);
    out_var_SV_Target1 = vec4(0.0, ((step(0.0, in_var_COLOR0.z) * 2.0) - 1.0) * _89, in_var_COLOR0.xy);
    gl_FragDepth = _89;
}

