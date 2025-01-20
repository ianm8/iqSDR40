#ifndef UTIL_H
#define UTIL_H

#define SAMPLERATE 32150u

namespace UTIL
{
  static const uint32_t __not_in_flash_func(map)(const uint32_t x,const uint32_t in_min, const uint32_t in_max,const uint32_t out_min, const float out_max)
  {
    // unsigned map
    if (x<in_min)
    {
      return out_min;
    }
    if (x>in_max)
    {
      return out_max;
    }
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }
}

#endif