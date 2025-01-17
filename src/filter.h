#ifndef FILTER_H
#define FILTER_H

#define JNR_FILTER_LENGTH 8
#define FIR_LENGTH 256
#define __mac_tap(_h) acc += (_h)*x[i++]
#define __mac_zap(_h) i++

namespace FILTER
{
  static float dcf(const float in)
  {
    // single pole IIR high-pass filter
    //static const float k = 0.004f; // <100Hz
    //static const float k = 0.01f; // ~100Hz
    //static const float k = 0.1f; // ~300Hz
    static const float k = 0.05f; // ~200Hz
    static float s = 0;
    static float x1 = 0;
    static float y1 = 0;
    s -= x1;
    x1 = in;
    s += x1 - y1 * k;
    return (y1 = s);
  }

  static float __not_in_flash_func(dc1f)(const float in)
  {
    static const float k = 0.05f;
    static float s = 0;
    static float x1 = 0;
    static float y1 = 0;
    s -= x1;
    x1 = in;
    s += x1 - y1 * k;
    return (y1 = s);
  }

  static float __not_in_flash_func(dc2f)(const float in)
  {
    static const float k = 0.05f;
    static float s = 0;
    static float x1 = 0;
    static float y1 = 0;
    s -= x1;
    x1 = in;
    s += x1 - y1 * k;
    return (y1 = s);
  }

  static const float __not_in_flash_func(fap1f)(const float s)
  {
    // all pass 84 @ 31250
    // all pass 607 @ 31250
    // all pass 2539 @ 31250
    static const float k1 = 0.98325f;
    static const float k2 = 0.88497f;
    static const float k3 = 0.59331f;
    static float x1 = 0.0f;
    static float y1 = 0.0f;
    static float x2 = 0.0f;
    static float y2 = 0.0f;
    static float x3 = 0.0f;
    static float y3 = 0.0f;
    y1 = (k1 * (s + y1)) - x1;
    x1 = s;
    y2 = (k2 * (y1 + y2)) - x2;
    x2 = y1;
    y3 = (k3 * (y2 + y3)) - x3;
    x3 = y2;
    return y3;
  }

  static const float __not_in_flash_func(fap2f)(const float s)
  {
    // all pass 8628 @ 31250
    // all pass 1200 @ 31250
    // all pass 287 @ 31250
    static const float k1 = 0.07102f;
    static const float k2 = 0.78470f;
    static const float k3 = 0.94391f;
    static float x1 = 0.0f;
    static float y1 = 0.0f;
    static float x2 = 0.0f;
    static float y2 = 0.0f;
    static float x3 = 0.0f;
    static float y3 = 0.0f;
    y1 = (k1 * (s + y1)) - x1;
    x1 = s;
    y2 = (k2 * (y1 + y2)) - x2;
    x2 = y1;
    y3 = (k3 * (y2 + y3)) - x3;
    x3 = y2;
    return y3;
  }

  static const float __not_in_flash_func(lpf_2400f_rx)(const float sample)
  {
    // 31250
    // att: 60dB
    static float x[FIR_LENGTH] = { 0.0f };
    static uint8_t sample_index = 0;
    uint8_t i = sample_index;
    x[sample_index--] = sample;
    float acc = 0;
    __mac_tap(-0.000059f);
    __mac_tap(-0.000046f);
    __mac_tap(-0.000015f);
    __mac_tap(0.000032f);
    __mac_tap(0.000084f);
    __mac_tap(0.000127f);
    __mac_tap(0.000147f);
    __mac_tap(0.000131f);
    __mac_tap(0.000075f);
    __mac_tap(-0.000015f);
    __mac_tap(-0.000121f);
    __mac_tap(-0.000219f);
    __mac_tap(-0.00028f);
    __mac_tap(-0.000279f);
    __mac_tap(-0.000205f);
    __mac_tap(-0.000064f);
    __mac_tap(0.00012f);
    __mac_tap(0.000307f);
    __mac_tap(0.000448f);
    __mac_tap(0.000498f);
    __mac_tap(0.00043f);
    __mac_tap(0.000241f);
    __mac_tap(-0.000038f);
    __mac_tap(-0.000352f);
    __mac_tap(-0.000624f);
    __mac_tap(-0.00078f);
    __mac_tap(-0.000764f);
    __mac_tap(-0.000554f);
    __mac_tap(-0.000176f);
    __mac_tap(0.000297f);
    __mac_tap(0.00076f);
    __mac_tap(0.001096f);
    __mac_tap(0.001205f);
    __mac_tap(0.001029f);
    __mac_tap(0.000577f);
    __mac_tap(-0.000073f);
    __mac_tap(-0.000785f);
    __mac_tap(-0.001389f);
    __mac_tap(-0.001724f);
    __mac_tap(-0.001676f);
    __mac_tap(-0.001211f);
    __mac_tap(-0.000396f);
    __mac_tap(0.000607f);
    __mac_tap(0.001573f);
    __mac_tap(0.002262f);
    __mac_tap(0.002475f);
    __mac_tap(0.002108f);
    __mac_tap(0.001188f);
    __mac_tap(-0.000117f);
    __mac_tap(-0.001531f);
    __mac_tap(-0.00272f);
    __mac_tap(-0.003372f);
    __mac_tap(-0.003272f);
    __mac_tap(-0.002369f);
    __mac_tap(-0.0008f);
    __mac_tap(0.00112f);
    __mac_tap(0.002961f);
    __mac_tap(0.004269f);
    __mac_tap(0.004675f);
    __mac_tap(0.00399f);
    __mac_tap(0.002272f);
    __mac_tap(-0.000164f);
    __mac_tap(-0.002802f);
    __mac_tap(-0.005024f);
    __mac_tap(-0.006252f);
    __mac_tap(-0.006091f);
    __mac_tap(-0.004442f);
    __mac_tap(-0.001554f);
    __mac_tap(0.002002f);
    __mac_tap(0.005435f);
    __mac_tap(0.007906f);
    __mac_tap(0.008721f);
    __mac_tap(0.007509f);
    __mac_tap(0.00435f);
    __mac_tap(-0.000206f);
    __mac_tap(-0.005214f);
    __mac_tap(-0.009518f);
    __mac_tap(-0.012001f);
    __mac_tap(-0.011855f);
    __mac_tap(-0.008802f);
    __mac_tap(-0.003219f);
    __mac_tap(0.00387f);
    __mac_tap(0.010954f);
    __mac_tap(0.016329f);
    __mac_tap(0.018459f);
    __mac_tap(0.016348f);
    __mac_tap(0.009838f);
    __mac_tap(-0.000236f);
    __mac_tap(-0.012089f);
    __mac_tap(-0.023221f);
    __mac_tap(-0.030826f);
    __mac_tap(-0.032303f);
    __mac_tap(-0.025773f);
    __mac_tap(-0.01051f);
    __mac_tap(0.012816f);
    __mac_tap(0.042129f);
    __mac_tap(0.074188f);
    __mac_tap(0.105057f);
    __mac_tap(0.130708f);
    __mac_tap(0.14767f);
    __mac_tap(0.1536f);
    __mac_tap(0.14767f);
    __mac_tap(0.130708f);
    __mac_tap(0.105057f);
    __mac_tap(0.074188f);
    __mac_tap(0.042129f);
    __mac_tap(0.012816f);
    __mac_tap(-0.01051f);
    __mac_tap(-0.025773f);
    __mac_tap(-0.032303f);
    __mac_tap(-0.030826f);
    __mac_tap(-0.023221f);
    __mac_tap(-0.012089f);
    __mac_tap(-0.000236f);
    __mac_tap(0.009838f);
    __mac_tap(0.016348f);
    __mac_tap(0.018459f);
    __mac_tap(0.016329f);
    __mac_tap(0.010954f);
    __mac_tap(0.00387f);
    __mac_tap(-0.003219f);
    __mac_tap(-0.008802f);
    __mac_tap(-0.011855f);
    __mac_tap(-0.012001f);
    __mac_tap(-0.009518f);
    __mac_tap(-0.005214f);
    __mac_tap(-0.000206f);
    __mac_tap(0.00435f);
    __mac_tap(0.007509f);
    __mac_tap(0.008721f);
    __mac_tap(0.007906f);
    __mac_tap(0.005435f);
    __mac_tap(0.002002f);
    __mac_tap(-0.001554f);
    __mac_tap(-0.004442f);
    __mac_tap(-0.006091f);
    __mac_tap(-0.006252f);
    __mac_tap(-0.005024f);
    __mac_tap(-0.002802f);
    __mac_tap(-0.000164f);
    __mac_tap(0.002272f);
    __mac_tap(0.00399f);
    __mac_tap(0.004675f);
    __mac_tap(0.004269f);
    __mac_tap(0.002961f);
    __mac_tap(0.00112f);
    __mac_tap(-0.0008f);
    __mac_tap(-0.002369f);
    __mac_tap(-0.003272f);
    __mac_tap(-0.003372f);
    __mac_tap(-0.00272f);
    __mac_tap(-0.001531f);
    __mac_tap(-0.000117f);
    __mac_tap(0.001188f);
    __mac_tap(0.002108f);
    __mac_tap(0.002475f);
    __mac_tap(0.002262f);
    __mac_tap(0.001573f);
    __mac_tap(0.000607f);
    __mac_tap(-0.000396f);
    __mac_tap(-0.001211f);
    __mac_tap(-0.001676f);
    __mac_tap(-0.001724f);
    __mac_tap(-0.001389f);
    __mac_tap(-0.000785f);
    __mac_tap(-0.000073f);
    __mac_tap(0.000577f);
    __mac_tap(0.001029f);
    __mac_tap(0.001205f);
    __mac_tap(0.001096f);
    __mac_tap(0.00076f);
    __mac_tap(0.000297f);
    __mac_tap(-0.000176f);
    __mac_tap(-0.000554f);
    __mac_tap(-0.000764f);
    __mac_tap(-0.00078f);
    __mac_tap(-0.000624f);
    __mac_tap(-0.000352f);
    __mac_tap(-0.000038f);
    __mac_tap(0.000241f);
    __mac_tap(0.00043f);
    __mac_tap(0.000498f);
    __mac_tap(0.000448f);
    __mac_tap(0.000307f);
    __mac_tap(0.00012f);
    __mac_tap(-0.000064f);
    __mac_tap(-0.000205f);
    __mac_tap(-0.000279f);
    __mac_tap(-0.00028f);
    __mac_tap(-0.000219f);
    __mac_tap(-0.000121f);
    __mac_tap(-0.000015f);
    __mac_tap(0.000075f);
    __mac_tap(0.000131f);
    __mac_tap(0.000147f);
    __mac_tap(0.000127f);
    __mac_tap(0.000084f);
    __mac_tap(0.000032f);
    __mac_tap(-0.000015f);
    __mac_tap(-0.000046f);
    __mac_tap(-0.000059f);
    return acc;
  }

  static const float __not_in_flash_func(lpf_2400f_tx)(const float sample)
  {
    // 31250
    // att: 60dB
    static float x[FIR_LENGTH] = { 0.0f };
    static uint8_t sample_index = 0;
    uint8_t i = sample_index;
    x[sample_index--] = sample;
    float acc = 0;
    __mac_tap(-0.000059f);
    __mac_tap(-0.000046f);
    __mac_tap(-0.000015f);
    __mac_tap(0.000032f);
    __mac_tap(0.000084f);
    __mac_tap(0.000127f);
    __mac_tap(0.000147f);
    __mac_tap(0.000131f);
    __mac_tap(0.000075f);
    __mac_tap(-0.000015f);
    __mac_tap(-0.000121f);
    __mac_tap(-0.000219f);
    __mac_tap(-0.00028f);
    __mac_tap(-0.000279f);
    __mac_tap(-0.000205f);
    __mac_tap(-0.000064f);
    __mac_tap(0.00012f);
    __mac_tap(0.000307f);
    __mac_tap(0.000448f);
    __mac_tap(0.000498f);
    __mac_tap(0.00043f);
    __mac_tap(0.000241f);
    __mac_tap(-0.000038f);
    __mac_tap(-0.000352f);
    __mac_tap(-0.000624f);
    __mac_tap(-0.00078f);
    __mac_tap(-0.000764f);
    __mac_tap(-0.000554f);
    __mac_tap(-0.000176f);
    __mac_tap(0.000297f);
    __mac_tap(0.00076f);
    __mac_tap(0.001096f);
    __mac_tap(0.001205f);
    __mac_tap(0.001029f);
    __mac_tap(0.000577f);
    __mac_tap(-0.000073f);
    __mac_tap(-0.000785f);
    __mac_tap(-0.001389f);
    __mac_tap(-0.001724f);
    __mac_tap(-0.001676f);
    __mac_tap(-0.001211f);
    __mac_tap(-0.000396f);
    __mac_tap(0.000607f);
    __mac_tap(0.001573f);
    __mac_tap(0.002262f);
    __mac_tap(0.002475f);
    __mac_tap(0.002108f);
    __mac_tap(0.001188f);
    __mac_tap(-0.000117f);
    __mac_tap(-0.001531f);
    __mac_tap(-0.00272f);
    __mac_tap(-0.003372f);
    __mac_tap(-0.003272f);
    __mac_tap(-0.002369f);
    __mac_tap(-0.0008f);
    __mac_tap(0.00112f);
    __mac_tap(0.002961f);
    __mac_tap(0.004269f);
    __mac_tap(0.004675f);
    __mac_tap(0.00399f);
    __mac_tap(0.002272f);
    __mac_tap(-0.000164f);
    __mac_tap(-0.002802f);
    __mac_tap(-0.005024f);
    __mac_tap(-0.006252f);
    __mac_tap(-0.006091f);
    __mac_tap(-0.004442f);
    __mac_tap(-0.001554f);
    __mac_tap(0.002002f);
    __mac_tap(0.005435f);
    __mac_tap(0.007906f);
    __mac_tap(0.008721f);
    __mac_tap(0.007509f);
    __mac_tap(0.00435f);
    __mac_tap(-0.000206f);
    __mac_tap(-0.005214f);
    __mac_tap(-0.009518f);
    __mac_tap(-0.012001f);
    __mac_tap(-0.011855f);
    __mac_tap(-0.008802f);
    __mac_tap(-0.003219f);
    __mac_tap(0.00387f);
    __mac_tap(0.010954f);
    __mac_tap(0.016329f);
    __mac_tap(0.018459f);
    __mac_tap(0.016348f);
    __mac_tap(0.009838f);
    __mac_tap(-0.000236f);
    __mac_tap(-0.012089f);
    __mac_tap(-0.023221f);
    __mac_tap(-0.030826f);
    __mac_tap(-0.032303f);
    __mac_tap(-0.025773f);
    __mac_tap(-0.01051f);
    __mac_tap(0.012816f);
    __mac_tap(0.042129f);
    __mac_tap(0.074188f);
    __mac_tap(0.105057f);
    __mac_tap(0.130708f);
    __mac_tap(0.14767f);
    __mac_tap(0.1536f);
    __mac_tap(0.14767f);
    __mac_tap(0.130708f);
    __mac_tap(0.105057f);
    __mac_tap(0.074188f);
    __mac_tap(0.042129f);
    __mac_tap(0.012816f);
    __mac_tap(-0.01051f);
    __mac_tap(-0.025773f);
    __mac_tap(-0.032303f);
    __mac_tap(-0.030826f);
    __mac_tap(-0.023221f);
    __mac_tap(-0.012089f);
    __mac_tap(-0.000236f);
    __mac_tap(0.009838f);
    __mac_tap(0.016348f);
    __mac_tap(0.018459f);
    __mac_tap(0.016329f);
    __mac_tap(0.010954f);
    __mac_tap(0.00387f);
    __mac_tap(-0.003219f);
    __mac_tap(-0.008802f);
    __mac_tap(-0.011855f);
    __mac_tap(-0.012001f);
    __mac_tap(-0.009518f);
    __mac_tap(-0.005214f);
    __mac_tap(-0.000206f);
    __mac_tap(0.00435f);
    __mac_tap(0.007509f);
    __mac_tap(0.008721f);
    __mac_tap(0.007906f);
    __mac_tap(0.005435f);
    __mac_tap(0.002002f);
    __mac_tap(-0.001554f);
    __mac_tap(-0.004442f);
    __mac_tap(-0.006091f);
    __mac_tap(-0.006252f);
    __mac_tap(-0.005024f);
    __mac_tap(-0.002802f);
    __mac_tap(-0.000164f);
    __mac_tap(0.002272f);
    __mac_tap(0.00399f);
    __mac_tap(0.004675f);
    __mac_tap(0.004269f);
    __mac_tap(0.002961f);
    __mac_tap(0.00112f);
    __mac_tap(-0.0008f);
    __mac_tap(-0.002369f);
    __mac_tap(-0.003272f);
    __mac_tap(-0.003372f);
    __mac_tap(-0.00272f);
    __mac_tap(-0.001531f);
    __mac_tap(-0.000117f);
    __mac_tap(0.001188f);
    __mac_tap(0.002108f);
    __mac_tap(0.002475f);
    __mac_tap(0.002262f);
    __mac_tap(0.001573f);
    __mac_tap(0.000607f);
    __mac_tap(-0.000396f);
    __mac_tap(-0.001211f);
    __mac_tap(-0.001676f);
    __mac_tap(-0.001724f);
    __mac_tap(-0.001389f);
    __mac_tap(-0.000785f);
    __mac_tap(-0.000073f);
    __mac_tap(0.000577f);
    __mac_tap(0.001029f);
    __mac_tap(0.001205f);
    __mac_tap(0.001096f);
    __mac_tap(0.00076f);
    __mac_tap(0.000297f);
    __mac_tap(-0.000176f);
    __mac_tap(-0.000554f);
    __mac_tap(-0.000764f);
    __mac_tap(-0.00078f);
    __mac_tap(-0.000624f);
    __mac_tap(-0.000352f);
    __mac_tap(-0.000038f);
    __mac_tap(0.000241f);
    __mac_tap(0.00043f);
    __mac_tap(0.000498f);
    __mac_tap(0.000448f);
    __mac_tap(0.000307f);
    __mac_tap(0.00012f);
    __mac_tap(-0.000064f);
    __mac_tap(-0.000205f);
    __mac_tap(-0.000279f);
    __mac_tap(-0.00028f);
    __mac_tap(-0.000219f);
    __mac_tap(-0.000121f);
    __mac_tap(-0.000015f);
    __mac_tap(0.000075f);
    __mac_tap(0.000131f);
    __mac_tap(0.000147f);
    __mac_tap(0.000127f);
    __mac_tap(0.000084f);
    __mac_tap(0.000032f);
    __mac_tap(-0.000015f);
    __mac_tap(-0.000046f);
    __mac_tap(-0.000059f);
    return acc;
  }

  static const float __not_in_flash_func(bpf_700f)(const float sample)
  {
    // 31250
    // att: 60dB
    // Lo: 600
    // Hi: 800
    static float x[FIR_LENGTH] = { 0.0f };
    static uint8_t sample_index = 0;
    uint8_t i = sample_index;
    x[sample_index--] = sample;
    float acc = 0;
    __mac_tap(0.000032f);
    __mac_tap(0.000029f);
    __mac_tap(0.000024f);
    __mac_tap(0.000015f);
    __mac_tap(0.000003f);
    __mac_tap(-0.000013f);
    __mac_tap(-0.000032f);
    __mac_tap(-0.000056f);
    __mac_tap(-0.000084f);
    __mac_tap(-0.000116f);
    __mac_tap(-0.00015f);
    __mac_tap(-0.000187f);
    __mac_tap(-0.000225f);
    __mac_tap(-0.000264f);
    __mac_tap(-0.000301f);
    __mac_tap(-0.000336f);
    __mac_tap(-0.000367f);
    __mac_tap(-0.000391f);
    __mac_tap(-0.000408f);
    __mac_tap(-0.000414f);
    __mac_tap(-0.000409f);
    __mac_tap(-0.000391f);
    __mac_tap(-0.000358f);
    __mac_tap(-0.00031f);
    __mac_tap(-0.000244f);
    __mac_tap(-0.000162f);
    __mac_tap(-0.000062f);
    __mac_tap(0.000054f);
    __mac_tap(0.000185f);
    __mac_tap(0.000329f);
    __mac_tap(0.000485f);
    __mac_tap(0.000648f);
    __mac_tap(0.000816f);
    __mac_tap(0.000984f);
    __mac_tap(0.001148f);
    __mac_tap(0.001302f);
    __mac_tap(0.001441f);
    __mac_tap(0.00156f);
    __mac_tap(0.001654f);
    __mac_tap(0.001717f);
    __mac_tap(0.001744f);
    __mac_tap(0.001731f);
    __mac_tap(0.001675f);
    __mac_tap(0.001572f);
    __mac_tap(0.00142f);
    __mac_tap(0.001219f);
    __mac_tap(0.000969f);
    __mac_tap(0.000672f);
    __mac_tap(0.000331f);
    __mac_tap(-0.000049f);
    __mac_tap(-0.000463f);
    __mac_tap(-0.000902f);
    __mac_tap(-0.001358f);
    __mac_tap(-0.001822f);
    __mac_tap(-0.002282f);
    __mac_tap(-0.002727f);
    __mac_tap(-0.003146f);
    __mac_tap(-0.003526f);
    __mac_tap(-0.003855f);
    __mac_tap(-0.004122f);
    __mac_tap(-0.004315f);
    __mac_tap(-0.004426f);
    __mac_tap(-0.004446f);
    __mac_tap(-0.004368f);
    __mac_tap(-0.004187f);
    __mac_tap(-0.003901f);
    __mac_tap(-0.003511f);
    __mac_tap(-0.003019f);
    __mac_tap(-0.002429f);
    __mac_tap(-0.00175f);
    __mac_tap(-0.000992f);
    __mac_tap(-0.000168f);
    __mac_tap(0.000706f);
    __mac_tap(0.001615f);
    __mac_tap(0.002538f);
    __mac_tap(0.003456f);
    __mac_tap(0.004347f);
    __mac_tap(0.005191f);
    __mac_tap(0.005965f);
    __mac_tap(0.006649f);
    __mac_tap(0.007224f);
    __mac_tap(0.007672f);
    __mac_tap(0.007977f);
    __mac_tap(0.008126f);
    __mac_tap(0.008109f);
    __mac_tap(0.00792f);
    __mac_tap(0.007556f);
    __mac_tap(0.007018f);
    __mac_tap(0.006311f);
    __mac_tap(0.005446f);
    __mac_tap(0.004435f);
    __mac_tap(0.003295f);
    __mac_tap(0.002047f);
    __mac_tap(0.000715f);
    __mac_tap(-0.000676f);
    __mac_tap(-0.002096f);
    __mac_tap(-0.003515f);
    __mac_tap(-0.004902f);
    __mac_tap(-0.006227f);
    __mac_tap(-0.00746f);
    __mac_tap(-0.00857f);
    __mac_tap(-0.009531f);
    __mac_tap(-0.010319f);
    __mac_tap(-0.010912f);
    __mac_tap(-0.011294f);
    __mac_tap(-0.011452f);
    __mac_tap(-0.011378f);
    __mac_tap(-0.011068f);
    __mac_tap(-0.010526f);
    __mac_tap(-0.009759f);
    __mac_tap(-0.008779f);
    __mac_tap(-0.007604f);
    __mac_tap(-0.006257f);
    __mac_tap(-0.004763f);
    __mac_tap(-0.003153f);
    __mac_tap(-0.00146f);
    __mac_tap(0.000282f);
    __mac_tap(0.002035f);
    __mac_tap(0.003763f);
    __mac_tap(0.005429f);
    __mac_tap(0.006996f);
    __mac_tap(0.008432f);
    __mac_tap(0.009704f);
    __mac_tap(0.010785f);
    __mac_tap(0.011652f);
    __mac_tap(0.012285f);
    __mac_tap(0.012671f);
    __mac_tap(0.0128f);
    __mac_tap(0.012671f);
    __mac_tap(0.012285f);
    __mac_tap(0.011652f);
    __mac_tap(0.010785f);
    __mac_tap(0.009704f);
    __mac_tap(0.008432f);
    __mac_tap(0.006996f);
    __mac_tap(0.005429f);
    __mac_tap(0.003763f);
    __mac_tap(0.002035f);
    __mac_tap(0.000282f);
    __mac_tap(-0.00146f);
    __mac_tap(-0.003153f);
    __mac_tap(-0.004763f);
    __mac_tap(-0.006257f);
    __mac_tap(-0.007604f);
    __mac_tap(-0.008779f);
    __mac_tap(-0.009759f);
    __mac_tap(-0.010526f);
    __mac_tap(-0.011068f);
    __mac_tap(-0.011378f);
    __mac_tap(-0.011452f);
    __mac_tap(-0.011294f);
    __mac_tap(-0.010912f);
    __mac_tap(-0.010319f);
    __mac_tap(-0.009531f);
    __mac_tap(-0.00857f);
    __mac_tap(-0.00746f);
    __mac_tap(-0.006227f);
    __mac_tap(-0.004902f);
    __mac_tap(-0.003515f);
    __mac_tap(-0.002096f);
    __mac_tap(-0.000676f);
    __mac_tap(0.000715f);
    __mac_tap(0.002047f);
    __mac_tap(0.003295f);
    __mac_tap(0.004435f);
    __mac_tap(0.005446f);
    __mac_tap(0.006311f);
    __mac_tap(0.007018f);
    __mac_tap(0.007556f);
    __mac_tap(0.00792f);
    __mac_tap(0.008109f);
    __mac_tap(0.008126f);
    __mac_tap(0.007977f);
    __mac_tap(0.007672f);
    __mac_tap(0.007224f);
    __mac_tap(0.006649f);
    __mac_tap(0.005965f);
    __mac_tap(0.005191f);
    __mac_tap(0.004347f);
    __mac_tap(0.003456f);
    __mac_tap(0.002538f);
    __mac_tap(0.001615f);
    __mac_tap(0.000706f);
    __mac_tap(-0.000168f);
    __mac_tap(-0.000992f);
    __mac_tap(-0.00175f);
    __mac_tap(-0.002429f);
    __mac_tap(-0.003019f);
    __mac_tap(-0.003511f);
    __mac_tap(-0.003901f);
    __mac_tap(-0.004187f);
    __mac_tap(-0.004368f);
    __mac_tap(-0.004446f);
    __mac_tap(-0.004426f);
    __mac_tap(-0.004315f);
    __mac_tap(-0.004122f);
    __mac_tap(-0.003855f);
    __mac_tap(-0.003526f);
    __mac_tap(-0.003146f);
    __mac_tap(-0.002727f);
    __mac_tap(-0.002282f);
    __mac_tap(-0.001822f);
    __mac_tap(-0.001358f);
    __mac_tap(-0.000902f);
    __mac_tap(-0.000463f);
    __mac_tap(-0.000049f);
    __mac_tap(0.000331f);
    __mac_tap(0.000672f);
    __mac_tap(0.000969f);
    __mac_tap(0.001219f);
    __mac_tap(0.00142f);
    __mac_tap(0.001572f);
    __mac_tap(0.001675f);
    __mac_tap(0.001731f);
    __mac_tap(0.001744f);
    __mac_tap(0.001717f);
    __mac_tap(0.001654f);
    __mac_tap(0.00156f);
    __mac_tap(0.001441f);
    __mac_tap(0.001302f);
    __mac_tap(0.001148f);
    __mac_tap(0.000984f);
    __mac_tap(0.000816f);
    __mac_tap(0.000648f);
    __mac_tap(0.000485f);
    __mac_tap(0.000329f);
    __mac_tap(0.000185f);
    __mac_tap(0.000054f);
    __mac_tap(-0.000062f);
    __mac_tap(-0.000162f);
    __mac_tap(-0.000244f);
    __mac_tap(-0.00031f);
    __mac_tap(-0.000358f);
    __mac_tap(-0.000391f);
    __mac_tap(-0.000409f);
    __mac_tap(-0.000414f);
    __mac_tap(-0.000408f);
    __mac_tap(-0.000391f);
    __mac_tap(-0.000367f);
    __mac_tap(-0.000336f);
    __mac_tap(-0.000301f);
    __mac_tap(-0.000264f);
    __mac_tap(-0.000225f);
    __mac_tap(-0.000187f);
    __mac_tap(-0.00015f);
    __mac_tap(-0.000116f);
    __mac_tap(-0.000084f);
    __mac_tap(-0.000056f);
    __mac_tap(-0.000032f);
    __mac_tap(-0.000013f);
    __mac_tap(0.000003f);
    __mac_tap(0.000015f);
    __mac_tap(0.000024f);
    __mac_tap(0.000029f);
    __mac_tap(0.000032f);
    return acc;
  }

  static const float __not_in_flash_func(jnr_maf1)(const float v)
  {
    static float element[JNR_FILTER_LENGTH] = {0.0f};
    static uint8_t p = 0;
    static float sum = 0.0f;
    sum = sum - element[p] + v;
    element[p++] = v;
    p &= (JNR_FILTER_LENGTH-1);
    return sum / ((float)JNR_FILTER_LENGTH);
  }

  static const float __not_in_flash_func(jnr_maf2)(const float v)
  {
    static float element[JNR_FILTER_LENGTH] = {0.0f};
    static uint8_t p = 0;
    static float sum = 0.0f;
    sum = sum - element[p] + v;
    element[p++] = v;
    p &= (JNR_FILTER_LENGTH-1);
    return sum / ((float)JNR_FILTER_LENGTH);
  }

  static const float __not_in_flash_func(jnr_maf3)(const float v)
  {
    static float element[JNR_FILTER_LENGTH] = {0.0f};
    static uint8_t p = 0;
    static float sum = 0.0f;
    sum = sum - element[p] + v;
    element[p++] = v;
    p &= (JNR_FILTER_LENGTH-1);
    return sum / ((float)JNR_FILTER_LENGTH);
  }

  static const float __not_in_flash_func(jnr_maf4)(const float v)
  {
    static float element[JNR_FILTER_LENGTH] = {0.0f};
    static uint8_t p = 0;
    static float sum = 0.0f;
    sum = sum - element[p] + v;
    element[p++] = v;
    p &= (JNR_FILTER_LENGTH-1);
    return sum / ((float)JNR_FILTER_LENGTH);
  }

  static const float __not_in_flash_func(jnr)(const float s,const uint32_t level)
  {
    // just noise reduction (not dynamic)
    switch (level)
    {
      case 1: return jnr_maf1(s);
      case 2: return jnr_maf2(FILTER::jnr_maf1(s));
      case 3: return jnr_maf4(FILTER::jnr_maf3(FILTER::jnr_maf2(FILTER::jnr_maf1(s))));
    }
    return s;
  }

}

#endif