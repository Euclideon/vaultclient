#include "vcSceneTool.h"

vcSceneTool *vcSceneTool::tools[vcActiveTool_Count];

vcSceneTool::vcSceneTool(vcActiveTool tool)
{
  vcSceneTool::tools[tool] = this;
}
