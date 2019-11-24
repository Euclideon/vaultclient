#include "vcTesting.h"

#include "udSocket.h"
#include "udFile.h"
#include "udPlatformUtil.h"
#include "udStringUtil.h"

#if UDPLATFORM_WINDOWS && !defined(NDEBUG)
#  define _CRT_SECURE_NO_WARNINGS
#  define _CRTDBG_MAP_ALLOC
#  include <crtdbg.h>
#  include <stdio.h>
#endif

vdkContext *g_pSharedTestContext = nullptr;

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);

#if UDPLATFORM_WINDOWS && !defined(NDEBUG)
  _CrtMemState m1, m2, diff;
  _CrtMemCheckpoint(&m1);
#endif //UDPLATFORM_WINDOWS && !defined(NDEBUG)

  int testResult = testing::UnitTest::GetInstance()->Run();

#if UDPLATFORM_WINDOWS && !defined(NDEBUG)
  _CrtMemCheckpoint(&m2);
  if (_CrtMemDifference(&diff, &m1, &m2) && diff.lCounts[_NORMAL_BLOCK] > 1) // gtest leaks 1 allocation, let's ignore it!
  {
    _CrtMemDumpAllObjectsSince(&m1);
    printf("%s\n", "Memory leaks in test found");
    testResult = -1;
  }
#endif //UDPLATFORM_WINDOWS && !defined(NDEBUG)

  return testResult;
}
