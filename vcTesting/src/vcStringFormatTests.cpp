#include "vcTesting.h"

#include "vcStringFormat.h"

TEST(vcStringFormatTests, BufferredTest)
{
  const char *Strings[] = { "one", "2", "{3}" };

  // Basic 'correct' test
  { 
    const char *pResult = vcStringFormat("Testing {0}, Testing {1}, Testing {2}", Strings, 3);
    EXPECT_STRCASEEQ(pResult, "Testing one, Testing 2, Testing {3}");
    udFree(pResult);
  }

  // Basic 'off by one' test
  {
    const char *pResult = vcStringFormat("Testing {1}, Testing {2}, Testing {3}", Strings, 3);
    EXPECT_STRCASEEQ(pResult, "Testing 2, Testing {3}, Testing {3}");
    udFree(pResult);
  }

  // Single String 'correct' test
  {
    const char *pResult = vcStringFormat("Testing {0}, Testing {1}, Testing {2}", Strings[0]);
    EXPECT_STRCASEEQ(pResult, "Testing one, Testing {1}, Testing {2}");
    udFree(pResult);
  }

  // Buffered Single String 'correct' test
  {
    char buffer[1024];
    const char *pResult = vcStringFormat(buffer, udLengthOf(buffer), "Testing {0}, Testing {1}, Testing {2}", Strings[1]);
    EXPECT_STRCASEEQ(pResult, "Testing 2, Testing {1}, Testing {2}");
    EXPECT_EQ(pResult, buffer);
  }

  // Buffered Single String 'way out of bounds' test
  {
    char buffer[1024];
    const char *pResult = vcStringFormat(buffer, udLengthOf(buffer), "Testing {56}, Testing {5}, Testing {-1}", Strings[1]);
    EXPECT_STRCASEEQ(pResult, "Testing {56}, Testing {5}, Testing {-1}");
    EXPECT_EQ(pResult, buffer);
  }

  // Null-In, Null-Out
  {
    const char *pResult = vcStringFormat(nullptr, Strings[1]);
    EXPECT_EQ(nullptr, pResult);
  }

  // Single String 'nullptr' test
  {
    const char *pResult = vcStringFormat("Testing {0}, Testing {1}, Testing {2}", nullptr);
    EXPECT_STRCASEEQ(pResult, "Testing {0}, Testing {1}, Testing {2}");
    udFree(pResult);
  }

  // Buffer too small
  {
    char buffer[8];
    memset(buffer, 0xC0, sizeof(buffer));
    size_t length;

    const char *pResult = vcStringFormat(buffer, udLengthOf(buffer), "Test{0}, Testing {1}, Testing {2}", Strings, 3, &length);
    EXPECT_STRCASEEQ(pResult, "Test{0}\0");
    EXPECT_EQ(length, 32);
    EXPECT_EQ(pResult, buffer);
  }
}
