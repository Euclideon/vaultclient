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
layout(location = 0) out vec4 out_var_SV_Target;

void main()
{
    vec3 _50 = (in_var_COLOR0 * vec3(2.0)) - vec3(1.0);
    float _76 = log2(in_var_TEXCOORD0.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
    vec4 _77 = vec4(((in_var_COLOR1.xyz * (0.5 + (dot(in_var_COLOR2, _50) * (-0.5)))) + (vec3(1.0, 1.0, 0.89999997615814208984375) * pow(max(0.0, -dot(-normalize(in_var_COLOR3.xyz / vec3(in_var_COLOR3.w)), _50)), 60.0))) * in_var_COLOR1.w, 1.0);
    _77.w = _76;
    out_var_SV_Target = _77;
    gl_FragDepth = _76;
}

