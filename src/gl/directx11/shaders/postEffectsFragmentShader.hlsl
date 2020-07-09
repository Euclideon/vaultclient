cbuffer u_cameraPlaneParams
{
  float s_CameraNearPlane;
  float s_CameraFarPlane;
  float u_clipZNear;
  float u_clipZFar;
};

/*
============================================================================
                    NVIDIA FXAA 3.11 by TIMOTHY LOTTES
------------------------------------------------------------------------------
COPYRIGHT (C) 2010, 2011 NVIDIA CORPORATION. ALL RIGHTS RESERVED.
------------------------------------------------------------------------------
TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED
*AS IS* AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL NVIDIA
OR ITS SUPPLIERS BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR
LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION,
OR ANY OTHER PECUNIARY LOSS) ARISING OUT OF THE USE OF OR INABILITY TO USE
THIS SOFTWARE, EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
DAMAGES.
*/

#define FXAA_GREEN_AS_LUMA 1
#define FXAA_DISCARD 0
#define FXAA_FAST_PIXEL_OFFSET 0
#define FXAA_GATHER4_ALPHA 0

#define FXAA_QUALITY__PS 5
#define FXAA_QUALITY__P0 1.0
#define FXAA_QUALITY__P1 1.5
#define FXAA_QUALITY__P2 2.0
#define FXAA_QUALITY__P3 4.0
#define FXAA_QUALITY__P4 12.0

#define FxaaBool bool
#define FxaaDiscard clip(-1)
#define FxaaFloat float
#define FxaaFloat2 float2
#define FxaaFloat3 float3
#define FxaaFloat4 float4
#define FxaaHalf half
#define FxaaHalf2 half2
#define FxaaHalf3 half3
#define FxaaHalf4 half4
#define FxaaSat(x) saturate(x)

#define FxaaInt2 int2
struct FxaaTex { SamplerState smpl; Texture2D tex; };
#define FxaaTexTop(t, p) t.tex.SampleLevel(t.smpl, p, 0.0)
#define FxaaTexOff(t, p, o, r) t.tex.SampleLevel(t.smpl, p, 0.0, o)
#define FxaaTexAlpha4(t, p) t.tex.GatherAlpha(t.smpl, p)
#define FxaaTexOffAlpha4(t, p, o) t.tex.GatherAlpha(t.smpl, p, o)
#define FxaaTexGreen4(t, p) t.tex.GatherGreen(t.smpl, p)
#define FxaaTexOffGreen4(t, p, o) t.tex.GatherGreen(t.smpl, p, o)

FxaaFloat FxaaLuma(FxaaFloat4 rgba) { return rgba.y; }

FxaaFloat4 FxaaPixelShader(
    FxaaFloat2 pos,
    FxaaFloat4 fxaaConsolePosPos,
    FxaaTex tex,
    FxaaTex fxaaConsole360TexExpBiasNegOne,
    FxaaTex fxaaConsole360TexExpBiasNegTwo,
    FxaaFloat2 fxaaQualityRcpFrame,
    FxaaFloat4 fxaaConsoleRcpFrameOpt,
    FxaaFloat4 fxaaConsoleRcpFrameOpt2,
    FxaaFloat4 fxaaConsole360RcpFrameOpt2,
    FxaaFloat fxaaQualitySubpix,
    FxaaFloat fxaaQualityEdgeThreshold,
    FxaaFloat fxaaQualityEdgeThresholdMin,
    FxaaFloat fxaaConsoleEdgeSharpness,
    FxaaFloat fxaaConsoleEdgeThreshold,
    FxaaFloat4 fxaaConsole360ConstDir
) {
  FxaaFloat2 posM;
  posM.x = pos.x;
  posM.y = pos.y;

  FxaaFloat4 rgbyM = FxaaTexTop(tex, posM);

#define lumaM rgbyM.y

  FxaaFloat lumaS = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(0, 1), fxaaQualityRcpFrame.xy));
  FxaaFloat lumaE = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(1, 0), fxaaQualityRcpFrame.xy));
  FxaaFloat lumaN = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(0, -1), fxaaQualityRcpFrame.xy));
  FxaaFloat lumaW = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(-1, 0), fxaaQualityRcpFrame.xy));

  FxaaFloat maxSM = max(lumaS, lumaM);
  FxaaFloat minSM = min(lumaS, lumaM);
  FxaaFloat maxESM = max(lumaE, maxSM);
  FxaaFloat minESM = min(lumaE, minSM);
  FxaaFloat maxWN = max(lumaN, lumaW);
  FxaaFloat minWN = min(lumaN, lumaW);
  FxaaFloat rangeMax = max(maxWN, maxESM);
  FxaaFloat rangeMin = min(minWN, minESM);
  FxaaFloat rangeMaxScaled = rangeMax * fxaaQualityEdgeThreshold;
  FxaaFloat range = rangeMax - rangeMin;
  FxaaFloat rangeMaxClamped = max(fxaaQualityEdgeThresholdMin, rangeMaxScaled);
  FxaaBool earlyExit = range < rangeMaxClamped;
  if (earlyExit)
#if (FXAA_DISCARD == 1)
    FxaaDiscard;
#else
    return rgbyM;
#endif

  FxaaFloat lumaNW = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(-1, -1), fxaaQualityRcpFrame.xy));
  FxaaFloat lumaSE = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(1, 1), fxaaQualityRcpFrame.xy));
  FxaaFloat lumaNE = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(1, -1), fxaaQualityRcpFrame.xy));
  FxaaFloat lumaSW = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(-1, 1), fxaaQualityRcpFrame.xy));

  FxaaFloat lumaNS = lumaN + lumaS;
  FxaaFloat lumaWE = lumaW + lumaE;
  FxaaFloat subpixRcpRange = 1.0 / range;
  FxaaFloat subpixNSWE = lumaNS + lumaWE;
  FxaaFloat edgeHorz1 = (-2.0 * lumaM) + lumaNS;
  FxaaFloat edgeVert1 = (-2.0 * lumaM) + lumaWE;
  FxaaFloat lumaNESE = lumaNE + lumaSE;
  FxaaFloat lumaNWNE = lumaNW + lumaNE;
  FxaaFloat edgeHorz2 = (-2.0 * lumaE) + lumaNESE;
  FxaaFloat edgeVert2 = (-2.0 * lumaN) + lumaNWNE;
  FxaaFloat lumaNWSW = lumaNW + lumaSW;
  FxaaFloat lumaSWSE = lumaSW + lumaSE;
  FxaaFloat edgeHorz4 = (abs(edgeHorz1) * 2.0) + abs(edgeHorz2);
  FxaaFloat edgeVert4 = (abs(edgeVert1) * 2.0) + abs(edgeVert2);
  FxaaFloat edgeHorz3 = (-2.0 * lumaW) + lumaNWSW;
  FxaaFloat edgeVert3 = (-2.0 * lumaS) + lumaSWSE;
  FxaaFloat edgeHorz = abs(edgeHorz3) + edgeHorz4;
  FxaaFloat edgeVert = abs(edgeVert3) + edgeVert4;
  FxaaFloat subpixNWSWNESE = lumaNWSW + lumaNESE;
  FxaaFloat lengthSign = fxaaQualityRcpFrame.x;
  FxaaBool horzSpan = edgeHorz >= edgeVert;
  FxaaFloat subpixA = subpixNSWE * 2.0 + subpixNWSWNESE;
  if (!horzSpan) lumaN = lumaW;
  if (!horzSpan) lumaS = lumaE;
  if (horzSpan) lengthSign = fxaaQualityRcpFrame.y;
  FxaaFloat subpixB = (subpixA * (1.0 / 12.0)) - lumaM;
  FxaaFloat gradientN = lumaN - lumaM;
  FxaaFloat gradientS = lumaS - lumaM;
  FxaaFloat lumaNN = lumaN + lumaM;
  FxaaFloat lumaSS = lumaS + lumaM;
  FxaaBool pairN = abs(gradientN) >= abs(gradientS);
  FxaaFloat gradient = max(abs(gradientN), abs(gradientS));
  if (pairN) lengthSign = -lengthSign;
  FxaaFloat subpixC = FxaaSat(abs(subpixB) * subpixRcpRange);
  FxaaFloat2 posB;
  posB.x = posM.x;
  posB.y = posM.y;
  FxaaFloat2 offNP;
  offNP.x = (!horzSpan) ? 0.0 : fxaaQualityRcpFrame.x;
  offNP.y = (horzSpan) ? 0.0 : fxaaQualityRcpFrame.y;
  if (!horzSpan) posB.x += lengthSign * 0.5;
  if (horzSpan) posB.y += lengthSign * 0.5;
  FxaaFloat2 posN;
  posN.x = posB.x - offNP.x * FXAA_QUALITY__P0;
  posN.y = posB.y - offNP.y * FXAA_QUALITY__P0;
  FxaaFloat2 posP;
  posP.x = posB.x + offNP.x * FXAA_QUALITY__P0;
  posP.y = posB.y + offNP.y * FXAA_QUALITY__P0;
  FxaaFloat subpixD = ((-2.0) * subpixC) + 3.0;
  FxaaFloat lumaEndN = FxaaLuma(FxaaTexTop(tex, posN));
  FxaaFloat subpixE = subpixC * subpixC;
  FxaaFloat lumaEndP = FxaaLuma(FxaaTexTop(tex, posP));
  if (!pairN) lumaNN = lumaSS;
  FxaaFloat gradientScaled = gradient * 1.0 / 4.0;
  FxaaFloat lumaMM = lumaM - lumaNN * 0.5;
  FxaaFloat subpixF = subpixD * subpixE;
  FxaaBool lumaMLTZero = lumaMM < 0.0;
  lumaEndN -= lumaNN * 0.5;
  lumaEndP -= lumaNN * 0.5;
  FxaaBool doneN = abs(lumaEndN) >= gradientScaled;
  FxaaBool doneP = abs(lumaEndP) >= gradientScaled;
  if (!doneN) posN.x -= offNP.x * FXAA_QUALITY__P1;
  if (!doneN) posN.y -= offNP.y * FXAA_QUALITY__P1;
  FxaaBool doneNP = (!doneN) || (!doneP);
  if (!doneP) posP.x += offNP.x * FXAA_QUALITY__P1;
  if (!doneP) posP.y += offNP.y * FXAA_QUALITY__P1;
  if (doneNP) {
    if (!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
    if (!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
    if (!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
    if (!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
    doneN = abs(lumaEndN) >= gradientScaled;
    doneP = abs(lumaEndP) >= gradientScaled;
    if (!doneN) posN.x -= offNP.x * FXAA_QUALITY__P2;
    if (!doneN) posN.y -= offNP.y * FXAA_QUALITY__P2;
    doneNP = (!doneN) || (!doneP);
    if (!doneP) posP.x += offNP.x * FXAA_QUALITY__P2;
    if (!doneP) posP.y += offNP.y * FXAA_QUALITY__P2;
    if (doneNP) {
      if (!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
      if (!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
      if (!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
      if (!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
      doneN = abs(lumaEndN) >= gradientScaled;
      doneP = abs(lumaEndP) >= gradientScaled;
      if (!doneN) posN.x -= offNP.x * FXAA_QUALITY__P3;
      if (!doneN) posN.y -= offNP.y * FXAA_QUALITY__P3;
      doneNP = (!doneN) || (!doneP);
      if (!doneP) posP.x += offNP.x * FXAA_QUALITY__P3;
      if (!doneP) posP.y += offNP.y * FXAA_QUALITY__P3;
      if (doneNP) {
        if (!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
        if (!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
        if (!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
        if (!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
        doneN = abs(lumaEndN) >= gradientScaled;
        doneP = abs(lumaEndP) >= gradientScaled;
        if (!doneN) posN.x -= offNP.x * FXAA_QUALITY__P4;
        if (!doneN) posN.y -= offNP.y * FXAA_QUALITY__P4;
        doneNP = (!doneN) || (!doneP);
        if (!doneP) posP.x += offNP.x * FXAA_QUALITY__P4;
        if (!doneP) posP.y += offNP.y * FXAA_QUALITY__P4;

      }
    }
  }
  FxaaFloat dstN = posM.x - posN.x;
  FxaaFloat dstP = posP.x - posM.x;
  if (!horzSpan) dstN = posM.y - posN.y;
  if (!horzSpan) dstP = posP.y - posM.y;
  FxaaBool goodSpanN = (lumaEndN < 0.0) != lumaMLTZero;
  FxaaFloat spanLength = (dstP + dstN);
  FxaaBool goodSpanP = (lumaEndP < 0.0) != lumaMLTZero;
  FxaaFloat spanLengthRcp = 1.0 / spanLength;
  FxaaBool directionN = dstN < dstP;
  FxaaFloat dst = min(dstN, dstP);
  FxaaBool goodSpan = directionN ? goodSpanN : goodSpanP;
  FxaaFloat subpixG = subpixF * subpixF;
  FxaaFloat pixelOffset = (dst * (-spanLengthRcp)) + 0.5;
  FxaaFloat subpixH = subpixG * fxaaQualitySubpix;
  FxaaFloat pixelOffsetGood = goodSpan ? pixelOffset : 0.0;
  FxaaFloat pixelOffsetSubpix = max(pixelOffsetGood, subpixH);
  if (!horzSpan) posM.x += pixelOffsetSubpix * lengthSign;
  if (horzSpan) posM.y += pixelOffsetSubpix * lengthSign;
  return FxaaFloat4(FxaaTexTop(tex, posM).xyz, lumaM);
}

float3 saturation(float3 rgb, float adjustment)
{
  const float3 W = float3(0.2125, 0.7154, 0.0721);
  float intensity = dot(rgb, W);
  return lerp(float3(intensity, intensity, intensity), rgb, adjustment);
}

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float2 edgeSampleUV0 : TEXCOORD1;
  float2 edgeSampleUV1 : TEXCOORD2;
  float2 edgeSampleUV2 : TEXCOORD3;
  float2 edgeSampleUV3 : TEXCOORD4;
  float2 sampleStepSize : TEXCOORD5;
  float saturation : TEXCOORD6;
};

struct PS_OUTPUT
{
  float4 Color0 : SV_Target0;
};

sampler sceneColourSampler;
Texture2D sceneColourTexture;

sampler sceneDepthSampler;
Texture2D sceneDepthTexture;

PS_OUTPUT main(PS_INPUT input)
{
  PS_OUTPUT output;
  float4 colour = float4(0.0, 0.0, 0.0, 0.0);
  float depth = sceneDepthTexture.Sample(sceneDepthSampler, input.uv).x;
  
  FxaaTex samplerInfo;
  samplerInfo.smpl = sceneColourSampler;
  samplerInfo.tex = sceneColourTexture;

  colour = FxaaPixelShader(input.uv, float4(0, 0, 0, 0), samplerInfo, samplerInfo, samplerInfo, input.sampleStepSize,
                                 float4(0, 0, 0, 0), float4(0, 0, 0, 0), float4(0, 0, 0, 0),
                                 0.75,  //fxaaQualitySubpix
                                 0.125, // fxaaQualityEdgeThreshold
                                 0.0, // fxaaQualityEdgeThresholdMin
                                 0, 0, float4(0, 0, 0, 0));

  output.Color0 = float4(saturation(colour.xyz, input.saturation), 1.0);
   
  return output;
}
