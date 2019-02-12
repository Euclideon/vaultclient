#include "vcTime.h"
#include <chrono>

int64_t vcTime_GetEpochSecs()
{
  return std::chrono::system_clock::now().time_since_epoch().count() / std::chrono::system_clock::period::den;
}

double vcTime_GetEpochSecsF()
{
  return (double)std::chrono::system_clock::now().time_since_epoch().count() / std::chrono::system_clock::period::den;
}
