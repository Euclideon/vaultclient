#include "vc12DXML.h"

#include "udFile.h"
#include "udJSON.h"
#include "udStringUtil.h"

void vc12DXML_Load(vcState *pProgramState, const char *pFilename)
{
  udResult result;
  udJSON xml;
  const char *pXMLText = nullptr;

  udUnused(pProgramState);

  UD_ERROR_CHECK(udFile_Load(pFilename, (void**)&pXMLText));
  UD_ERROR_CHECK(xml.Parse(pXMLText));

  if (xml.Get("DataBlock").IsObject() && udStrEqual(xml.Get("DataBlock.Name").AsString(), "ProjectData"))
  {
    const udJSON &dataEntries = xml.Get("DataBlock.DataEntry");
    for (size_t i = 0; i < dataEntries.ArrayLength(); ++i)
    {
    }

    const udJSON &dataBlocks = xml.Get("DataBlock.DataBlock");
    for (size_t i = 0; i < dataBlocks.ArrayLength(); ++i)
    {
      const udJSON &groupDataBlock = dataBlocks.Get("[%zu]", i);

      udUnused(groupDataBlock);
    }

  }

epilogue:
  udFree(pXMLText);
}
