#include "imgui_udValue.h"

#include "udJSON.h"
#include "udStringUtil.h"

#include "imgui.h"

void vcImGuiValueTreeArray(const udJSON *pValue)
{
  if (!pValue->IsArray())
    return;

  udJSONArray *pArray = pValue->AsArray();
  char buffer[16];

  for (size_t i = 0; i < pArray->length; ++i)
  {
    udSprintf(buffer, "Item %zu", i);
    vcImGuiValueTree(buffer, pArray->GetElement(i));
  }
}

void vcImGuiValueTreeObject(const udJSON *pValue)
{
  for (size_t i = 0; i < pValue->MemberCount(); ++i)
  {
    vcImGuiValueTree(pValue->GetMemberName(i), pValue->GetMember(i));
  }
}

void vcImGuiValueTree(const char *pMemberName, const udJSON *pMember)
{
  if (pMember->IsArray())
  {
    if (ImGui::TreeNode(pMember, "%s [%zu items]", pMemberName, pMember->ArrayLength()))
    {
      vcImGuiValueTreeArray(pMember);
      ImGui::TreePop();
    }
  }
  else if (pMember->IsObject())
  {
    if (ImGui::TreeNode(pMember, "%s [Object]", pMemberName))
    {
      vcImGuiValueTreeObject(pMember);
      ImGui::TreePop();
    }
  }
  else if (pMember->IsString() || pMember->GetType() == udJSON::Type::T_Bool)
  {
    ImGui::BulletText("[%s] \"%s\"", pMemberName, pMember->AsString());
  }
  else if (pMember->IsIntegral())
  {
    ImGui::BulletText("[%s] %s", pMemberName, udCommaInt(pMember->AsInt64()));
  }
  else if (pMember->IsNumeric())
  {
    ImGui::BulletText("[%s] %f", pMemberName, pMember->AsDouble());
  }
  else
  {
    ImGui::BulletText("[%s] <Unknown Type>", pMemberName);
  }
}
