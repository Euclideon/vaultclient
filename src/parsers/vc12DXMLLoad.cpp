#include "vc12DXML.h"
#include "vc12dxmlCommon.h"
#include "udStringUtil.h"
#include "udFile.h"

/*
*  Notes:
*  I am unsure if commands such as <null></null> and <colour></colour> can be placed anywhere in the xml
*  or must be called from inside a string. The spec is difficult to interpret. I have opted for the
*  following rule regarding commands
*     - Commands (null and colour at least) can only be called from within a string and should
*       only be called once. Position is irrelevant. The command will set the global value,
*       and will affect all subsequent strings.
*       WARNING: If udJSON does not preserve order when building arrays of objects, the spec will
*                not be obeyed!
*/

#define LOG_ERROR(...) {}
#define LOG_WARNING(...) {}
#define LOG_INFO(...) {}

// TODO support 'light' and 'dark' variants of all colours
static uint32_t vc12DXML_ToColour(const char *pStr)
{
  struct ColourPair
  {
    const char *pStr;
    uint32_t val;
  };

  static const ColourPair Colours[] =
  {
    {"red",     0xFFFF0000},
    {"green",   0xFF00FF00},
    {"blue",    0xFF0000FF},
    {"yellow",  0xFFFFFF00},
    {"cyan",    0xFF00FFFF},
    {"magenta", 0xFFFF00FF},
    {"black",   0xFF000000},
    {"white",   0xFFFFFFFF},
  };

  for (size_t i = 0; i < udLengthOf(Colours); ++i)
  {
    if (udStrcmpi(pStr, Colours[i].pStr) == 0)
      return Colours[i].val;
  }

  return 0xFFFF00FF;
}

static void SetGlobals(udJSON const *pNode, vc12DXML_ProjectGlobals &globals)
{
  const udJSON *pItem = nullptr;

  pItem = &pNode->Get("null");
  if (!pItem->IsVoid())
    globals.nullValue = pItem->AsDouble();

  pItem = &pNode->Get("colour");
  if (!pItem->IsVoid())
    globals.colour = vc12DXML_ToColour(pItem->AsString());
}

static void vc12DXML_Ammend_data_3d(udJSON const *pNode, std::vector<udDouble3> &pointList, vc12DXML_ProjectGlobals &globals)
{
  if (pNode->IsArray())
  {
    for (size_t i = 0; i < pNode->AsArray()->length; ++i)
      vc12DXML_Ammend_data_3d(pNode->AsArray()->GetElement(i), pointList, globals);
  }
  else
  {
    char str[128] = {};
    char *tokens[3] = {};
    udDouble3 point = {};
    udStrcpy(str, pNode->AsString());
    if (udStrTokenSplit(str, " ", tokens, 3) != 3)
    {
      LOG_WARNING("Malformed data_3d vertex item");
    }
    else
    {
      for (int p = 0; p < 3; ++p)
      {
        if (udStrcmp(tokens[p], "null") == 0)
          point[p] = globals.nullValue;
        else
          point[p] = udStrAtof64(tokens[p]);
      }
      pointList.push_back(point);
    }
  }
}

vc12DXML_ProjectGlobals::vc12DXML_ProjectGlobals()
  : nullValue(-999.0)                 // Default defined in the spec
  , colour(vc12DXML_ToColour("red"))  // Default defined in the spec
{

}

vc12DXML_Item::~vc12DXML_Item()
{

}

vc12DXML_SuperString::vc12DXML_SuperString()
  : m_pName(nullptr)
  , m_isClosed(false)
  , m_weight(1.0)
  , m_colour(0xFFFF00FF)
{

}

vc12DXML_SuperString::~vc12DXML_SuperString()
{
  udFree(m_pName);
}

udResult vc12DXML_SuperString::Build(udJSON const *pNode, vc12DXML_ProjectGlobals &globals)
{
  udResult result;
  const udJSON *pItem = nullptr;
  //bool has_z = false;
  //double z = 0.0;

  UD_ERROR_IF(pNode == nullptr, udR_InvalidParameter_);

  // TODO Some values can and MUST be pulled out before others.
  // <z> for example MUST be pulled out of the super_string block before reading any
  // 3d/2d data, regardless of where <z> sits in the file in relation to the data.
  // From the spec:
  //   - There is exactly 0 or 1 <z> tag per string.
  //   - Its position is irrelevant, it is global to the string
  //pItem = &pNode->Get("z");
  //UD_ERROR_IF(pItem->IsArray(), udR_InvalidConfiguration);
  //
  //if (!pItem->IsVoid())
  //{
  //  has_z = true;
  //  z = pItem->AsDouble();
  //}

  SetGlobals(pNode, globals);

  pItem = &pNode->Get("name");
  if (!pItem->IsVoid())
  {
    udFree(m_pName);
    m_pName = udStrdup(pItem->AsString());
  }

  pItem = &pNode->Get("closed");
  if (!pItem->IsVoid())
    m_isClosed = pItem->AsBool();

  pItem = &pNode->Get("colour");
  if (!pItem->IsVoid())
    m_colour = vc12DXML_ToColour(pItem->AsString());
  else
    m_colour = globals.colour;

  pItem = &pNode->Get("weight");
  if (!pItem->IsVoid())
    m_weight = pItem->AsDouble();

  pItem = &pNode->Get("data_3d");
  if (!pItem->IsVoid())
    vc12DXML_Ammend_data_3d(&pItem->Get("p"), m_points, globals);

  result = udR_Success;
epilogue:
  return result;
}

vc12DXML_Model::vc12DXML_Model()
  : m_pName(nullptr)
{

}

vc12DXML_Model::~vc12DXML_Model()
{
  for (auto pItem : m_elements)
    delete pItem;

  udFree(m_pName);
}

udResult vc12DXML_Model::BuildChildren(udJSON const *pNode, vc12DXML_ProjectGlobals &globals)
{
  udResult result;
  const udJSON *pItem = nullptr;
  UD_ERROR_IF(pNode == nullptr, udR_InvalidParameter_);

  pItem = &pNode->Get("string_super");
  if (!pItem->IsVoid())
  {
    if (pItem->IsArray())
    {
      for (size_t i = 0; i < pItem->ArrayLength(); ++i)
      {
        vc12DXML_SuperString *pSS = new vc12DXML_SuperString();
        udResult res = pSS->Build(pItem->AsArray()->GetElement(i), globals);
        if (res == udR_Success)
          m_elements.push_back(pSS);
        else
          LOG_ERROR("Failed to load super string.");
      }
    }
    else
    {
      vc12DXML_SuperString *pSS = new vc12DXML_SuperString();
      udResult res = pSS->Build(pItem, globals);
      if (res == udR_Success)
        m_elements.push_back(pSS);
      else
        LOG_ERROR("Failed to load super string.");
    }
  }

  result = udR_Success;
epilogue:
  return result;
}

udResult vc12DXML_Model::Build(udJSON const *pNode, vc12DXML_ProjectGlobals &globals)
{
  udResult result;
  const udJSON *pItem = nullptr;
  UD_ERROR_IF(pNode == nullptr, udR_InvalidParameter_);

  // As per the spec, 'name' MUST be defined for models. But we don't check the format, just assume is correct.
  pItem = &pNode->Get("name");
  if (!pItem->IsVoid())
  {
    udFree(m_pName);
    m_pName = udStrdup(pItem->AsString());
  }

  pItem = &pNode->Get("children");
  if (!pItem->IsVoid())
    UD_ERROR_CHECK(BuildChildren(pItem, globals));

  result = udR_Success;
epilogue:
  return result;
};

vc12DXML_Project::vc12DXML_Project()
  : m_pName(nullptr)
{

}

vc12DXML_Project::~vc12DXML_Project()
{
  for (auto pItem : m_models)
    delete pItem;

  udFree(m_pName);
}

void vc12DXML_Project::SetName(const char *pName)
{
  udFree(m_pName);
  m_pName = udStrdup(pName);
}

udResult vc12DXML_Project::BeginBuild(udJSON const *pNode)
{
  return Build(pNode, m_globals);
}

udResult vc12DXML_Project::Build(udJSON const *pNode, vc12DXML_ProjectGlobals &globals)
{
  udResult result;
  const udJSON *pRoot = nullptr;
  const udJSON *pItem = nullptr;

  UD_ERROR_IF(pNode == nullptr, udR_InvalidParameter_);

  pRoot = &pNode->Get("xml12d");

  // Models
  pItem = &pRoot->Get("model");
  if (!pItem->IsVoid())
  {
    if (pItem->IsArray())
    {
      for (size_t i = 0; i < pItem->ArrayLength(); ++i)
      {
        vc12DXML_Model *pModel = new vc12DXML_Model();
        udResult res = pModel->Build(pItem->AsArray()->GetElement(i), globals);
        if (res == udR_Success)
          m_models.push_back(pModel);
        else
          LOG_ERROR("Failed to load model.");
      }
    }
    else
    {
      vc12DXML_Model *pModel = new vc12DXML_Model();
      udResult res = pModel->Build(pItem, globals);
      if (res == udR_Success)
        m_models.push_back(pModel);
      else
        LOG_ERROR("Failed to load model.");
    }
  }

  result = udR_Success;
epilogue:
  return result;
}

udResult vc12DXML_LoadProject(vc12DXML_Project &project, const char *pFilePath)
{
  udResult result;
  udJSON xml;
  const char *pFileContents = nullptr;
  udJSON models;
  udFilename fileName;
  char projectName[256] = {};

  UD_ERROR_NULL(pFilePath, udR_InvalidParameter_);

  fileName.SetFromFullPath("%s", pFilePath);
  fileName.ExtractFilenameOnly(projectName, 256);
  project.SetName(projectName);

  UD_ERROR_CHECK(udFile_Load(pFilePath, (void **)&pFileContents));
  UD_ERROR_CHECK(xml.Parse(pFileContents));

  UD_ERROR_CHECK(project.BeginBuild(&xml));

epilogue:
  udFree(pFileContents);
  return result;
}

void vc12DXML_Load(vcState *pProgramState, const char *pFilename)
{
  vc12DXML_Project project;
  udResult result = vc12DXML_LoadProject(project, pFilename);
  if (result == udR_Success)
  {
    udProjectNode *pParentNode = pProgramState->sceneExplorer.clickedItem.pItem != nullptr ? pProgramState->sceneExplorer.clickedItem.pItem : pProgramState->activeProject.pRoot;
    project.AddToProject(pProgramState, pParentNode);
  }
}
