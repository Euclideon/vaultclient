#ifndef vcEndian_h__
#define vcEndian_h__

#include <stdint.h>
#include <type_traits>

// We won't worry about anything exotic (middle-endian for example)
enum vcEndian
{
  vcEn_Unknown,
  vcEn_Big,
  vcEn_Little
};

// Determine Endianness of this machine during runtime.
vcEndian vcEndian_Endianness();

// Reverse the bytes of a type, but should only be used for fundamental types
template<typename T>
T vcEndian_ReverseBytes(T const &a_val)
{
  T result(static_cast<T>(0));
  for (size_t i = 0; i < sizeof(T); i++)
  {
    reinterpret_cast<uint8_t *>(&result)[sizeof(T) - i - 1] =
      reinterpret_cast<uint8_t const *>(&a_val)[i];
  }
  return result;
}

// Helper class for endian conversion
class vcEndian_Converter
{
public:

  // The default converter will not reverse bytes
  vcEndian_Converter();
  vcEndian_Converter(vcEndian from, vcEndian to);

  void SetConversion(vcEndian from, vcEndian to);

  // Assumes we are working with 8-bit bytes
  template<typename T>
  T Convert(T const & a_val) const
  {
    if (!std::is_fundamental<T>::value || !m_reverse)
      return a_val;

    return vcEndian_ReverseBytes<T>(a_val);
  }

private:

  bool m_reverse;
};

#endif
