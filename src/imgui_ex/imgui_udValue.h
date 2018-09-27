#ifndef imgui_udValue_h__
#define imgui_udValue_h__

class udJSON;

void vcImGuiValueTree(const char *pMemberName, const udJSON *pMember);
void vcImGuiValueTreeObject(const udJSON *pMember);
void vcImGuiValueTreeArray(const udJSON *pMember);

#endif // imgui_udValue_h__
