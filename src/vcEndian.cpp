
#include "vcEndian.h"

vcEndian vcEndian_Endianness()
{
  union
  {
    uint32_t value;
    uint8_t data[sizeof(uint32_t)];
  } number;

  number.data[0] = 0x00;
  number.data[1] = 0x01;
  number.data[2] = 0x02;
  number.data[3] = 0x03;

  switch (number.value)
  {
  case UINT32_C(0x00010203):
    return vcEn_Big;
  case UINT32_C(0x03020100):
    return vcEn_Little;
  default:
    return vcEn_Unknown;
  }
}

vcEndian_Converter::vcEndian_Converter()
  : m_reverse(false)
{

}

vcEndian_Converter::vcEndian_Converter(vcEndian from, vcEndian to)
  : m_reverse(false)
{
  SetConversion(from, to);
}

void vcEndian_Converter::SetConversion(vcEndian from, vcEndian to)
{
  m_reverse = (from + to == 3);
}
