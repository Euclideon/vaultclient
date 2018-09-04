#ifndef imgui_udValue_h__
#define imgui_udValue_h__

class udValue;

void vcImGuiValueTree(const char *pMemberName, const udValue *pMember);
void vcImGuiValueTreeObject(const udValue *pMember);
void vcImGuiValueTreeArray(const udValue *pMember);

#endif // imgui_udValue_h__
