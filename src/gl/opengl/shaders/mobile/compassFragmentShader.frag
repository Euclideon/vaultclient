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

in highp vec3 varying_COLOR0;
in highp vec4 varying_COLOR1;
in highp vec3 varying_COLOR2;
in highp vec4 varying_COLOR3;
in highp vec2 varying_TEXCOORD0;
layout(location = 0) out highp vec4 out_var_SV_Target0;
layout(location = 1) out highp vec4 out_var_SV_Target1;

void main()
{
    highp vec3 _51 = (varying_COLOR0 * vec3(2.0)) - vec3(1.0);
    highp float _77 = log2(varying_TEXCOORD0.x) * (1.0 / log2(u_cameraPlaneParams.s_CameraFarPlane + 1.0));
    out_var_SV_Target0 = vec4(((varying_COLOR1.xyz * (0.5 + (dot(varying_COLOR2, _51) * (-0.5)))) + (vec3(1.0, 1.0, 0.89999997615814208984375) * pow(max(0.0, -dot(-normalize(varying_COLOR3.xyz / vec3(varying_COLOR3.w)), _51)), 60.0))) * varying_COLOR1.w, 1.0);
    out_var_SV_Target1 = vec4(0.0, ((step(0.0, varying_COLOR0.z) * 2.0) - 1.0) * _77, varying_COLOR0.xy);
    gl_FragDepth = _77;
}

