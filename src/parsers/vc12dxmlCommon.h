#ifndef vc12dxmlCommon_h__
#define vc12dxmlCommon_h__

#include <stdint.h>
#include <vector>
#include <cstdio>
#include "udJSON.h"

struct vcState;
struct udProjectNode;

class vc12DXML_ProjectGlobals
{
public:

  vc12DXML_ProjectGlobals();

  double nullValue;
  uint32_t colour;
};

class vc12DXML_Item
{
public:
  virtual ~vc12DXML_Item();
  virtual udResult Build(udJSON const *, vc12DXML_ProjectGlobals &) = 0;
  virtual void AddToProject(vcState *pProgramState, udProjectNode *pParent) = 0;
  virtual void Print(FILE *, int indent) const = 0;
};

class vc12DXML_SuperString : public vc12DXML_Item
{
public:

  vc12DXML_SuperString();
  ~vc12DXML_SuperString();
  udResult Build(udJSON const *pNode, vc12DXML_ProjectGlobals &globals) override;
  void Print(FILE *pOut, int indent) const override;
  void AddToProject(vcState *pProgramState, udProjectNode *pParent);
private:

  char *m_pName;
  bool m_isClosed;
  double m_weight;
  uint32_t m_colour;
  std::vector<udDouble3> m_points;
};

class vc12DXML_Model : public vc12DXML_Item
{
public:

  vc12DXML_Model();
  ~vc12DXML_Model();
  udResult BuildChildren(udJSON const *pNode, vc12DXML_ProjectGlobals &globals);
  udResult Build(udJSON const *pNode, vc12DXML_ProjectGlobals &globals) override;
  void Print(FILE *pOut, int indent) const override;
  void AddToProject(vcState *pProgramState, udProjectNode *pParent);

private:

  char *m_pName;
  std::vector<vc12DXML_Item *> m_elements;
};

class vc12DXML_Project : public vc12DXML_Item
{
public:

  vc12DXML_Project();
  ~vc12DXML_Project();
  void SetName(const char *pName);
  udResult BeginBuild(udJSON const *pNode);
  udResult Build(udJSON const *pNode, vc12DXML_ProjectGlobals &globals) override;
  void Print(FILE *pOut, int indent = 0) const override;
  void AddToProject(vcState *pProgramState, udProjectNode *pParent) override;

private:

  char *m_pName;
  vc12DXML_ProjectGlobals m_globals; // Every 12dxml file has a set of globals that can change while parsing
  std::vector<vc12DXML_Item *> m_models;
};

#endif
