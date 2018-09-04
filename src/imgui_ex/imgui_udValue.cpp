#include "imgui_udValue.h"

#include "udPlatform/udValue.h"
#include "imgui.h"

void vcImGuiValueTreeArray(const udValue *pValue)
{
  if (!pValue->IsArray())
    return;

  udValueArray *pArray = pValue->AsArray();
  char buffer[16];

  for (size_t i = 0; i < pArray->length; ++i)
  {
    udSprintf(buffer, udLengthOf(buffer), "Item %llu", i);
    vcImGuiValueTree(buffer, pArray->GetElement(i));
  }
}

void vcImGuiValueTreeObject(const udValue *pValue)
{
  for (size_t i = 0; i < pValue->MemberCount(); ++i)
  {
    vcImGuiValueTree(pValue->GetMemberName(i), pValue->GetMember(i));
  }
}

void vcImGuiValueTree(const char *pMemberName, const udValue *pMember)
{
  if (pMember->IsArray())
  {
    if (ImGui::TreeNode(pMember, "%s [%llu items]", pMemberName, pMember->ArrayLength()))
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
  else if (pMember->IsString() || pMember->GetType() == udValue::Type::T_Bool)
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
