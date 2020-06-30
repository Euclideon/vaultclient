#include "vcLabelRenderer.h"
#include "udPlatformUtil.h"
#include "vcMesh.h"
#include "vcInternalModels.h"

#include "imgui.h"

static struct vcLabelShader
{
  vcShader *pShader;
  vcShaderConstantBuffer *pEveryObjectConstantBuffer;

  vcShaderSampler *pDiffuseSampler;

  struct
  {
    udFloat4x4 u_worldViewProjectionMatrix;
    udFloat4 u_screenSize; // objectId.w
  } everyObject;
} gShader;

vcMesh *g_LabelMesh = nullptr;

static int gRefCount = 0;
udResult vcLabelRenderer_Init(udWorkerPool *pWorkerPool)
{
  udResult result;
  ++gRefCount;
  vcLabelShader *pShader = nullptr;

  UD_ERROR_IF(gRefCount != 1, udR_Success);

  pShader = &gShader;
  UD_ERROR_IF(!vcShader_CreateFromFileAsync(&gShader.pShader, pWorkerPool, "asset://assets/shaders/imgui3DVertexShader", "asset://assets/shaders/imgui3DFragmentShader", vcImGuiVertexLayout,
    [pShader](void *)
    {
      vcShader_Bind(pShader->pShader);
      vcShader_GetConstantBuffer(&pShader->pEveryObjectConstantBuffer, pShader->pShader, "u_EveryObject", sizeof(pShader->everyObject));
      vcShader_GetSamplerIndex(&pShader->pDiffuseSampler, pShader->pShader, "Texture");
    }
  ), udR_InternalError);

  if (g_LabelMesh == nullptr)
  {
    const vcImGuiVertex labelQuadVertices[4]{ { { 0.f, 0.f }, { 0, 0 }, 0 },{ { 0.f, 0.f }, { 0, 1 }, 0 },{ { 0.f, 0.f }, { 1, 1 }, 0 },{ { 0.f, 0.f }, { 1, 0 }, 0 } };
    const uint16_t labelQuadIndices[6] = { 0, 1, 2, 0, 2, 3 };
    UD_ERROR_CHECK(vcMesh_Create(&g_LabelMesh, vcImGuiVertexLayout, (int)udLengthOf(vcImGuiVertexLayout), labelQuadVertices, 4, labelQuadIndices, 6, vcMF_Dynamic));
  }
  result = udR_Success;

epilogue:
  if (result != udR_Success)
    vcLabelRenderer_Destroy();

  return result;
}

udResult vcLabelRenderer_Destroy()
{
  udResult result;
  --gRefCount;

  UD_ERROR_IF(gRefCount != 0, udR_Success);

  vcShader_ReleaseConstantBuffer(gShader.pShader, gShader.pEveryObjectConstantBuffer);
  vcShader_DestroyShader(&gShader.pShader);
  vcMesh_Destroy(&g_LabelMesh);

  result = udR_Success;

epilogue:
  return result;
}


bool vcLabelRenderer_Render(ImDrawList *drawList, vcLabelInfo *pLabelRenderer, float encodedObjectId, const udDouble4x4 &viewProjectionMatrix, const udUInt2 &screenSize)
{
  if (!drawList || !pLabelRenderer->pText)
    return false;

  // These values were picked by visual inspection
  float fontScales[] =
  {
    1.0f, // medium
    0.87f, // small
    1.2f // large
  };
  UDCOMPILEASSERT(udLengthOf(fontScales) == vcLFS_Count, "Font sizes count doesn't match");

  ImGui::SetWindowFontScale(fontScales[vcLFS_Medium]);

  ImVec2 labelSize = ImGui::CalcTextSize(pLabelRenderer->pText);
  ImVec2 halfLabelSize = ImVec2(labelSize.x * 0.5f, labelSize.y * 0.5f);
  ImVec2 offset = ImVec2();
  offset.y = -labelSize.y; // label is 'above' the actual point

  udDouble4x4 mvp = viewProjectionMatrix * udDouble4x4::translation(pLabelRenderer->worldPosition);
  vcShader_Bind(gShader.pShader);

  gShader.everyObject.u_worldViewProjectionMatrix = udFloat4x4::create(mvp);
  gShader.everyObject.u_screenSize = udFloat4::create(2.0f / screenSize.x, 2.0f / screenSize.y, 0.0f, encodedObjectId);

#if !GRAPHICS_API_OPENGL
  // ImGui vertices are built with origin in bottom left corner
  gShader.everyObject.u_screenSize.y *= -1;
#endif

  ImDrawCmd *pcmd = &drawList->CmdBuffer.back();
  int vtx = drawList->VtxBuffer.Size;
  int idx = drawList->IdxBuffer.Size;

  // Force write clip_rect of the window for ImDrawList::AddText.
  ImVec4 clip_rect = drawList->_ClipRectStack.back();
  ImVec4& currRect = drawList->_ClipRectStack.back();
  currRect.x = -halfLabelSize.x + offset.x - 1;
  currRect.y = -halfLabelSize.y + offset.y - 1;
  currRect.z = halfLabelSize.x + offset.x + 1;
  currRect.w = halfLabelSize.y + offset.y + 1;
  pcmd->ClipRect.x = -halfLabelSize.x + offset.x - 1;
  pcmd->ClipRect.y = -halfLabelSize.y + offset.y - 1;
  pcmd->ClipRect.z = halfLabelSize.x + offset.x + 1;
  pcmd->ClipRect.w = halfLabelSize.y + offset.y + 1;

  drawList->AddQuadFilled(ImVec2(-halfLabelSize.x + offset.x, -halfLabelSize.y + offset.y), ImVec2(halfLabelSize.x + offset.x, -halfLabelSize.y + offset.y), ImVec2(halfLabelSize.x + offset.x, halfLabelSize.y + offset.y), ImVec2(-halfLabelSize.x + offset.x, halfLabelSize.y + offset.y), pLabelRenderer->backColourRGBA);
  drawList->AddText(ImVec2(-halfLabelSize.x + offset.x, -halfLabelSize.y + offset.y), pLabelRenderer->textColourRGBA, pLabelRenderer->pText);

  int vtxAdd = drawList->VtxBuffer.Size - vtx;
  int idxAdd = drawList->IdxBuffer.Size - idx;

  // re-offset indices
  for (int i = 0; i < idxAdd; ++i)
    drawList->IdxBuffer.Data[idx + i] -= vtx;

  vcMesh_UploadData(g_LabelMesh, vcImGuiVertexLayout, (int)udLengthOf(vcImGuiVertexLayout), &drawList->VtxBuffer.Data[vtx], vtxAdd, (void *)&drawList->IdxBuffer.Data[idx], idxAdd);
  vcShader_BindConstantBuffer(gShader.pShader, gShader.pEveryObjectConstantBuffer, &gShader.everyObject, sizeof(gShader.everyObject));
  vcShader_BindTexture(gShader.pShader, (vcTexture *)pcmd->TextureId, 0, gShader.pDiffuseSampler);

  vcMesh_Render(g_LabelMesh);
  pcmd->ElemCount = 0;

  ImGui::SetWindowFontScale(1.f);

  currRect.x = clip_rect.x;
  currRect.y = clip_rect.y;
  currRect.z = clip_rect.z;
  currRect.w = clip_rect.w;

  vcShader_Bind(nullptr);

  return true;
}
