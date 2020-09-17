#include "vcTesting.h"
#include "vcEndian.h"

TEST(vcEndianTests, EndianConversion)
{
  EXPECT_TRUE(vcEndian_Endianness() == vcEn_Big || vcEndian_Endianness() == vcEn_Little);

  uint8_t u8 = 0x42;
  uint16_t u16 = 0x0102;
  uint32_t u32 = 0x01020304UL;
  uint64_t u64 = 0x0102030405060708ULL;

  uint8_t u8r = 0x42;
  uint16_t u16r = 0x0201;
  uint32_t u32r = 0x04030201UL;
  uint64_t u64r = 0x0807060504030201ULL;

  EXPECT_EQ(vcEndian_ReverseBytes(u8), u8r);
  EXPECT_EQ(vcEndian_ReverseBytes(u16), u16r);
  EXPECT_EQ(vcEndian_ReverseBytes(u32), u32r);
  EXPECT_EQ(vcEndian_ReverseBytes(u64), u64r);
  
  {
    vcEndian_Converter converter(vcEn_Big, vcEn_Big);
    EXPECT_EQ(converter.Convert(u8), u8);
    EXPECT_EQ(converter.Convert(u16), u16);
    EXPECT_EQ(converter.Convert(u32), u32);
    EXPECT_EQ(converter.Convert(u64), u64);
  }

  {
    vcEndian_Converter converter(vcEn_Little, vcEn_Little);
    EXPECT_EQ(converter.Convert(u8), u8);
    EXPECT_EQ(converter.Convert(u16), u16);
    EXPECT_EQ(converter.Convert(u32), u32);
    EXPECT_EQ(converter.Convert(u64), u64);
  }

  {
    vcEndian_Converter converter(vcEn_Big, vcEn_Little);
    EXPECT_EQ(converter.Convert(u8), u8r);
    EXPECT_EQ(converter.Convert(u16), u16r);
    EXPECT_EQ(converter.Convert(u32), u32r);
    EXPECT_EQ(converter.Convert(u64), u64r);
  }

  {
    vcEndian_Converter converter(vcEn_Little, vcEn_Big);
    EXPECT_EQ(converter.Convert(u8), u8r);
    EXPECT_EQ(converter.Convert(u16), u16r);
    EXPECT_EQ(converter.Convert(u32), u32r);
    EXPECT_EQ(converter.Convert(u64), u64r);
  }
}
