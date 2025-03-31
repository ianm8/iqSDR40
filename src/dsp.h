#ifndef DSP_H
#define DSP_H

#include "filter.h"
#include "util.h"

namespace DSP
{
  static const uint8_t __not_in_flash("fast_access_sram") meter[64] =
  {
    0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu,
    0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu,
    0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu,
    0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3cu, 0x3c
  };

  volatile static float agc_peak = 0.0f;

  static const int16_t __not_in_flash_func(agc)(const float in)
  {
    // limit gain to max of 40 (32db)
    static const float max_gain = 40.0f;
    // decay about 10dB per second
    static const float k = 0.99996f;

    const float magnitude = fabsf(in);
    if (magnitude>agc_peak)
    {
      agc_peak = magnitude;
    }
    else
    {
      agc_peak *= k;
    }

    // trap issues with low values
    if (agc_peak<1.0f) return (int16_t)(in * max_gain);

    // set maximum gain possible for 12 bit DAC
    const float m = 2047.0f/agc_peak;
    return (int16_t)(in*fminf(m,max_gain));
  }

  static const uint8_t __not_in_flash_func(smeter)(void)
  {
    // S9 = -73dBm = 141uV PP
    // measured 540 after SSB processing
    // need to return 45 for S9
    static const float S0_sig = 100.0f;
    static const float S9_sig = 540.0f;
    static const float S9p_sig = 8192.0f;
    static const uint32_t S9_from_min = (uint32_t)(log10f(S0_sig) * 1024.0f);
    static const uint32_t S9_from_max = (uint32_t)(log10f(S9_sig) * 1024.0f);
    static const uint32_t S9_min = 0ul;
    static const uint32_t S9_max = 45ul;
    static const uint32_t S9p_from_min = (uint32_t)(log10f(S9_sig) * 1024.0f);
    static const uint32_t S9p_from_max = (uint32_t)(log10f(S9p_sig) * 1024.0f);
    static const uint32_t S9p_min = 46ul;
    static const uint32_t S9p_max = 63ul;
    if (agc_peak<1.0f)
    {
      return 0u;
    }
    const uint32_t log_peak = (uint32_t)(log10f(agc_peak) * 1024.0f);
    if (agc_peak>S9_sig)
    {
      return (uint8_t)UTIL::map(log_peak,S9p_from_min,S9p_from_max,S9p_min,S9p_max);
    }
    return (uint8_t)UTIL::map(log_peak,S9_from_min,S9_from_max,S9_min,S9_max);
  }

  static const int16_t __not_in_flash_func(process_ssb)(const int16_t in_i,const int16_t in_q,const uint32_t jnr_level,const uint8_t bw)
  {
    // remove DC
    const float ii = FILTER::dc1f((float)in_i / 32768.0f);
    const float qq = FILTER::dc2f((float)in_q / 32768.0f);

    // phase shift IQ +/- 45
    const float p45 = FILTER::fap1f(ii);
    const float n45 = FILTER::fap2f(qq);

    // reject image
    const float ssb = p45 - n45;

    // LPF
    const float audio_raw = FILTER::bwf[bw](ssb);

    // JNR
    const float audio_out = FILTER::jnr(audio_raw,jnr_level);

    // AGC returns 12 bit value
    return agc(audio_out * 32768.0f);
  }

  static const int16_t __not_in_flash_func(process_cw)(const int16_t in_i,const int16_t in_q,const uint32_t jnr_level)
  {
    // remove DC
    const float ii = FILTER::dc1f((float)in_i / 32768.0f);
    const float qq = FILTER::dc2f((float)in_q / 32768.0f);

    // phase shift IQ +/- 45
    const float p45 = FILTER::fap1f(ii);
    const float n45 = FILTER::fap2f(qq);

    // reject image
    const float ssb = p45 - n45;

    // BPF for CW
    const float audio_raw = FILTER::bpf_700f(ssb);

    // JNR
    const float audio_out = FILTER::jnr(audio_raw,jnr_level);

    // AGC returns 12 bit value
    return agc(audio_out * 32768.0f);
  }

  static const uint32_t __not_in_flash_func(get_mic_peak_level)(const int16_t mic_in)
  {
    static const uint32_t MIC_LEVEL_DECAY_RATE = 50ul;
    static const uint32_t MIC_LEVEL_HANG_TIME = 500ul;
    static uint32_t mic_peak_level = 0;
    static uint32_t mic_level_update = 0;
    static uint32_t mic_hangtime_update = 0;
    const uint32_t now = millis();
    const uint32_t mic_level = abs(mic_in)>>5;
    if (mic_level>mic_peak_level)
    {
      mic_peak_level = mic_level;
      mic_level_update = now + MIC_LEVEL_DECAY_RATE;
      mic_hangtime_update = now + MIC_LEVEL_HANG_TIME;
    }
    else
    {
      if (now>mic_hangtime_update)
      {
        if (now>mic_level_update)
        {
          if (mic_peak_level) mic_peak_level--;
          mic_level_update = now + MIC_LEVEL_DECAY_RATE;
        }
      }
    }
    return mic_peak_level;
  }

  static void __not_in_flash_func(cessb)(float& ii, float& qq)
  {
    const float mag_raw = sqrtf(ii*ii + qq*qq);
    const float mag_max = fmaxf(mag_raw, 1.0f);
    ii = FILTER::lpf_2400if_tx(ii / mag_max);
    qq = FILTER::lpf_2400qf_tx(qq / mag_max);
  }

  const void __not_in_flash_func(process_mic)(const int16_t s,int16_t &out_i,int16_t &out_q,const float mic_gain,const bool cessb_on)
  {
    // input is 12 bits
    // convert to float
    // remove Mic DC
    // 2400 LPF 
    // phase shift I
    // phase shift Q
    // convert to int
    // output is 10 bits
    const float ac_sig = FILTER::dcf(((float)s)*(1.0f/2048.0f));
    const float mic_sig = FILTER::lpf_2400f_tx(ac_sig);
    const float ii1 = FILTER::fap1f(mic_sig);
    const float qq1 = FILTER::fap2f(mic_sig);
    float ii2 = ii1 * mic_gain;
    float qq2 = qq1 * mic_gain;
    if (cessb_on) cessb(ii2,qq2);
    out_i = (int16_t)(ii2 * 512.0f);
    out_q = (int16_t)(qq2 * 512.0f);
  }
}

#endif