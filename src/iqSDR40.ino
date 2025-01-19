/*
 * iqSDR40 Version 0.9.240
 *
 * Copyright 2025 Ian Mitchell VK7IAN
 * Licenced under the GNU GPL Version 3
 *
 * libraries
 *
 * https://github.com/etherkit/Si5351Arduino
 * https://github.com/brianlow/Rotary
 * https://github.com/datacute/Tiny4kOLED
 *
 * Build:
 *  Board: Pi Pico 2
 *  Flash 4MB (Sketch: 4032KB, FS: 64KB)
 *  CPU Speed: 240Mhz
 *  Optimize: -O2
 *  USB Stack: No USB
 */

#include <si5351.h>
#include <Tiny4kOLED.h>
#include <I2S.h>
#include <EEPROM.h>
#include "Rotary.h"
#include "filter.h"
#include "dsp.h"
#include "menu.h"
#include "cw.h"
#include "spectrum.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/vreg.h"

//#define YOUR_CALL "VK7IAN"

#define VERSION_STRING         " V0.9."
#define SPECTRUM_OFF           0u
#define SPECTRUM_RX            1u
#define SPECTRUM_TX            2u
#define SPECTRUM_RXTX          3u
#define CW_TIMEOUT             800u
#define CW_SIDETONE            700u
#define MENU_TIMEOUT           5000u
#define DEFAULT_FREQUENCY      7105000ul
#define DEFAULT_MODE           MODE_LSB
#define DEFAULT_STEP           1000ul
#define DEFAULT_JNR            0u
#define DEFAULT_SPECTRUM       SPECTRUM_OFF
#define BUTTON_LONG_PRESS_TIME 800ul
#define SHOW_STEP_TIMEOUT      8000ul
#define TCXO_FREQ              26000000ul
#define BAND_40M_LO            7000000ul
#define BAND_40M_HI            7300000ul
#define QUADRATURE_DIVISOR     88ul
#define SAMPLE_RATE            31250
#define MAX_ADC_SAMPLES        2048
#define MUTE                   LOW
#define UNMUTE                 HIGH

#define PIN_PTT      0 // Mic PTT (active low) and CW Paddle A
#define PIN_UNUSED1  1 // free pin
#define PIN_UNUSED2  2 // free pin
#define PIN_UNUSED3  3 // free pin
#define PIN_SDA      4 // I2C
#define PIN_SCL      5 // I2C
#define PIN_TX000    6 // TX PWM
#define PIN_TX180    7 // TX PWM
#define PIN_TX090    8 // TX PWM
#define PIN_TX270    9 // TX PWM
#define PIN_DOUT    10 // I2S
#define PIN_BCLK    11 // I2S
#define PIN_LRCL    12 // I2S
#define PIN_MCLK    13 // I2S
#define PIN_PADB    14 // CW Paddle B
#define PIN_MUTE    15 // Low to mute
#define PIN_AUPWML  16 // audio PWM low
#define PIN_AUPWMH  17 // audio PWM high
#define PIN_ENCA    18 // rotary
#define PIN_ENCB    19 // rotary
#define PIN_ENCBUT  20 // rotary
#define PIN_TXBIAS  21 // enable TX bias
#define PIN_RXN     22 // Enable RX mixer (active low)
#define PIN_REG     23 // Pico regulator
#define PIN_TXN     26 // Enable TX mixer (active low)
#define PIN_UNUSED4 27 // free pin
#define PIN_MIC     28 // analog MIC

#if PIN_MIC == 26U
#define MIC_MUX 0U
#elif PIN_MIC == 27U
#define MIC_MUX 1U
#elif PIN_MIC == 28U
#define MIC_MUX 2U
#elif PIN_MIC == 29U
#define MIC_MUX 3U
#endif

enum radio_mode_t
{
  MODE_NONE,
  MODE_LSB,
  MODE_USB,
  MODE_CWL,
  MODE_CWU
};

enum radio_setmode_t
{
  SETMODE_AUTO,
  SETMODE_LSB,
  SETMODE_USB,
  SETMODE_CWL,
  SETMODE_CWU
};

// the audio out PWM simulates a 12 bit DAC using two channels
// resistors on the pin pairs have a ratio of 64:1 (eg 100k and 1.5k)
// assuming a pin resistance (on average) of 50 ohms (100,000/1550 ~ 64)
// the PWM frequency will be processor clock / 64 (eg 240Mhz/64 = 3.75MHz)
// v is signed, channel A is the high 6 bits, channel B is the low 6 bits
// need to convert to unsigned and extract the high and low 6 bits
// PWM A is Pin 2 (high bits - 1k5 resistor)
// PWM B is Pin 3 (low bits - 100k resistor)

volatile static struct
{
  int32_t tune;
  uint32_t step;
  uint32_t frequency;
  radio_mode_t mode;
  radio_setmode_t set_mode;
  uint8_t jnr;
  uint8_t spectrum;
  bool tx_enable;
  bool keydown;
}
radio =
{
  0,
  DEFAULT_STEP,
  DEFAULT_FREQUENCY,
  DEFAULT_MODE,
  SETMODE_AUTO,
  DEFAULT_JNR,
  DEFAULT_SPECTRUM,
  false,
  false
};

static uint8_t magnitude[1024] = {0};

Si5351 si5351;
Rotary r = Rotary(PIN_ENCB,PIN_ENCA);
I2S i2s(INPUT);

auto_init_mutex(rotary_mutex);
volatile static uint32_t audio_pwm = 0;
volatile static uint32_t tx_i_pwm = 0;
volatile static uint32_t tx_q_pwm = 0;
volatile static int32_t dac_h = 0;
volatile static int32_t dac_l = 0;
volatile static int32_t dac_value_i_p = 0;
volatile static int32_t dac_value_i_n = 0;
volatile static int32_t dac_value_q_p = 0;
volatile static int32_t dac_value_q_n = 0;
volatile static int16_t adc_value = 0;
volatile static bool adc_value_ready = false;
volatile static bool setup_complete = false;
volatile static uint32_t adc_sample_p = 0;
volatile static int16_t adc_data_i[MAX_ADC_SAMPLES] = {0};
volatile static int16_t adc_data_q[MAX_ADC_SAMPLES] = {0};
volatile static bool save_settings_now = false;

void __not_in_flash_func(adc_interrupt_handler)(void)
{
  volatile static uint32_t counter = 0;
  volatile static uint32_t adc_raw = 0;
  if (adc_fifo_get_level()<4u)
  {
    return;
  }
  adc_raw += adc_fifo_get();
  adc_raw += adc_fifo_get();
  adc_raw += adc_fifo_get();
  adc_raw += adc_fifo_get();
  if (counter==4)
  {
    pwm_set_both_levels(audio_pwm,dac_l,dac_h);
    pwm_set_both_levels(tx_i_pwm,dac_value_i_p,dac_value_i_n);
    pwm_set_both_levels(tx_q_pwm,dac_value_q_p,dac_value_q_n);
    adc_value = (int16_t)(adc_raw>>4)-2048;
    adc_value_ready = true;
    adc_raw = 0;
    counter = 0;
  }
  counter++;
}

static void init_adc(void)
{
  adc_init();
  adc_gpio_init(PIN_MIC);
  adc_select_input(MIC_MUX);
  adc_fifo_setup(true, false, 4, false, false);
  adc_fifo_drain();
  adc_irq_set_enabled(true);
  irq_set_exclusive_handler(ADC_IRQ_FIFO, adc_interrupt_handler);
  irq_set_priority(ADC_IRQ_FIFO, PICO_HIGHEST_IRQ_PRIORITY);
  irq_set_enabled(ADC_IRQ_FIFO, true);
  adc_run(true);
}

static void init_i2s(void)
{
  i2s.setDATA(PIN_DOUT);
  i2s.setBCLK(PIN_BCLK); // Note: LRCLK = BCLK + 1
  i2s.setMCLK(PIN_MCLK);
  i2s.setBitsPerSample(32);
  i2s.setFrequency(SAMPLE_RATE);
  i2s.setMCLKmult(256);
  i2s.setBuffers(4, 256, 0);
  i2s.begin();
}

static void save_settings(void)
{
  // need to stop I2S to enable EEPROM write to function
  static const uint32_t key = 0x12345678ul;
  digitalWrite(PIN_MUTE,MUTE);
  i2s.end();
  EEPROM.begin(256);
  EEPROM.put(0*sizeof(uint32_t),key);
  EEPROM.put(1*sizeof(uint32_t),(uint32_t)radio.jnr);
  EEPROM.put(2*sizeof(uint32_t),(uint32_t)radio.spectrum);
  EEPROM.end();
  init_i2s();
  digitalWrite(PIN_MUTE,UNMUTE);
}

static void restore_settings(void)
{
  uint32_t key = 0 ;
  EEPROM.begin(256);
  EEPROM.get(0,key);
  if (key==0x12345678)
  {
    uint32_t data32 = 0;
    EEPROM.get(1*sizeof(uint32_t),data32); radio.jnr = (uint8_t)data32;
    EEPROM.get(2*sizeof(uint32_t),data32); radio.spectrum = (uint8_t)data32;
  }
  EEPROM.end();
}

void setup(void)
{
  // run DSP on core 0
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN,LOW);
  vreg_set_voltage(VREG_VOLTAGE_1_30);
  const uint32_t clksys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
  // frequency_count_khz() isn't accurate!
  if (clksys < (240000ul - 5ul) || clksys > (240000ul + 5ul))
  {
    // trap the wrong system clock
    pinMode(LED_BUILTIN,OUTPUT);
    for (;;)
    {
      digitalWrite(LED_BUILTIN,HIGH);
      delay(30);
      digitalWrite(LED_BUILTIN,LOW);
      delay(100);
    }
  }
  pinMode(LED_BUILTIN,OUTPUT);
  pinMode(PIN_MUTE,OUTPUT);
  pinMode(PIN_TXBIAS,OUTPUT);
  pinMode(PIN_TXN,OUTPUT);
  pinMode(PIN_RXN,OUTPUT);
  pinMode(PIN_PTT,INPUT);
  pinMode(PIN_PADB,INPUT);
  pinMode(PIN_ENCBUT,INPUT_PULLUP);
  pinMode(PIN_UNUSED1,INPUT_PULLUP);
  pinMode(PIN_UNUSED2,INPUT_PULLUP);
  pinMode(PIN_UNUSED3,INPUT_PULLUP);
  pinMode(PIN_UNUSED4,INPUT_PULLUP);
  pinMode(PIN_REG,OUTPUT);

  // pin defaults
  // set pico regulator to low noise
  digitalWrite(PIN_MUTE,MUTE);
  digitalWrite(PIN_REG,HIGH);
  digitalWrite(PIN_TXBIAS,LOW);
  digitalWrite(PIN_TXN,HIGH);
  digitalWrite(PIN_RXN,LOW);
  delay(40);
  digitalWrite(LED_BUILTIN,LOW);

  // set pin function to PWM
  gpio_set_function(PIN_TX000,GPIO_FUNC_PWM); //  6  PWM
  gpio_set_function(PIN_TX180,GPIO_FUNC_PWM); //  8  PWM
  gpio_set_function(PIN_TX090,GPIO_FUNC_PWM); // 10  PWM
  gpio_set_function(PIN_TX270,GPIO_FUNC_PWM); // 12  PWM

  // get PWM slice connected to each pin pair
  tx_i_pwm = pwm_gpio_to_slice_num(PIN_TX000);
  tx_q_pwm = pwm_gpio_to_slice_num(PIN_TX090);
  
  // set period of 1024 cycles
  pwm_set_wrap(tx_i_pwm,1023);
  pwm_set_wrap(tx_q_pwm,1023);
  
  // initialise to zero (low)
  pwm_set_both_levels(tx_i_pwm,0,0);
  pwm_set_both_levels(tx_q_pwm,0,0);

  // set each PWM running
  pwm_set_enabled(tx_i_pwm,true);
  pwm_set_enabled(tx_q_pwm,true);

  // set up audio out PWM
  gpio_set_function(PIN_AUPWMH,GPIO_FUNC_PWM);
  gpio_set_function(PIN_AUPWML,GPIO_FUNC_PWM);
  audio_pwm = pwm_gpio_to_slice_num(PIN_AUPWML);
  pwm_set_wrap(audio_pwm,63); // 240,000,000 / 64 = 3,750,000
  pwm_set_both_levels(audio_pwm,0,31);
  pwm_set_enabled(audio_pwm,true);

  // init rotary
  r.begin();

  // set defaults or restore
  radio.frequency = DEFAULT_FREQUENCY;
  radio.mode = DEFAULT_MODE;
  restore_settings();

#ifdef DEBUG_LED
  pinMode(LED_BUILTIN,OUTPUT);
  for (int i=0;i<2;i++)
  {
    digitalWrite(LED_BUILTIN,HIGH);
    delay(10);
    digitalWrite(LED_BUILTIN,LOW);
    delay(250);
  }
#endif

  Wire.setSDA(PIN_SDA);
  Wire.setSCL(PIN_SCL);
  Wire.setClock(400000ul);
  const bool si5351_found = si5351.init(SI5351_CRYSTAL_LOAD_0PF,TCXO_FREQ,0);
  if (!si5351_found)
  {
    for (;;)
    {
      pinMode(LED_BUILTIN,OUTPUT);
      digitalWrite(LED_BUILTIN,HIGH);
      delay(10);
      digitalWrite(LED_BUILTIN,LOW);
      delay(500);
    }
  }
  si5351.drive_strength(SI5351_CLK0,SI5351_DRIVE_8MA);
  si5351.drive_strength(SI5351_CLK1,SI5351_DRIVE_8MA);
  const uint64_t f = radio.frequency*SI5351_FREQ_MULT;
  const uint64_t p = radio.frequency*QUADRATURE_DIVISOR*SI5351_FREQ_MULT;
  si5351.set_freq_manual(f,p,SI5351_CLK0);
  si5351.set_freq_manual(f,p,SI5351_CLK1);
  si5351.set_phase(SI5351_CLK0,0);
  si5351.set_phase(SI5351_CLK1,QUADRATURE_DIVISOR);
  si5351.pll_reset(SI5351_PLLA);

#ifdef DEBUG_LED
  pinMode(LED_BUILTIN,OUTPUT);
  for (int i=0;i<2;i++)
  {
    digitalWrite(LED_BUILTIN,HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN,LOW);
    delay(250);
  }
#endif

  // set up audio ADC (IQ input)
  init_i2s();

  // set up mic ADC
  init_adc();

  // disable Mic in RX mode
  pinMode(PIN_MIC,OUTPUT);
  digitalWrite(PIN_MIC,LOW);

  // OLED bright and rotated
  // note: include file declares the oled object
  oled.begin(64,32,sizeof(tiny4koled_init_64x32br),tiny4koled_init_64x32br);
  oled.enableChargePump();
  oled.setRotation(0);
  oled.setInternalIref(true);
  oled.clear();
  oled.on();
  oled.switchRenderFrame();

  // intro screen
  oled.clear();
  oled.setFont(FONT6X8);
  oled.setCursor(0,0);
  oled.print(" iqSDR40");
  char sz_clksys[16] = "";
  memset(sz_clksys,0,sizeof(sz_clksys));
  ultoa(clksys,sz_clksys,10);
  sz_clksys[3] = '\0';
  oled.setCursor(0,1);
  oled.print(VERSION_STRING);
  oled.print(sz_clksys);
#ifdef YOUR_CALL
  oled.setFont(FONT8X16);
  oled.setCursor(0,2);
  oled.print(" ");
  oled.print(YOUR_CALL);
#else
  oled.setFont(FONT8X16);
  oled.setCursor(0,2);
  oled.print(" VK7IAN");
#endif
  oled.switchFrame();

  // splash delay
  delay(4000);

  // unmute
  digitalWrite(PIN_MUTE,UNMUTE);

  setup_complete = true;
}

void setup1(void)
{
  // run UI on core 1
  // only go to loop1 when setup() has completed
  while (!setup_complete)
  {
    tight_loop_contents();
  }

#ifdef DEBUG_LED
  pinMode(LED_BUILTIN,OUTPUT);
  for (int i=0;i<5;i++)
  {
    digitalWrite(LED_BUILTIN,HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN,LOW);
    delay(250);
  }
#endif
}

static void show_spectrum(void)
{
  static const uint8_t row1[16] =
  {
    0x00u,
    0x00u,
    0x00u,
    0x00u,
    0x00u,
    0x00u,
    0x00u,
    0x00u,
    0x80u,
    0xc0u,
    0xe0u,
    0xf0u,
    0xf8u,
    0xfcu,
    0xfeu,
    0xffu
  };
  static const uint8_t row2[16] =
  {
    0x80u,
    0xc0u,
    0xe0u,
    0xf0u,
    0xf8u,
    0xfcu,
    0xfeu,
    0xffu,
    0xffu,
    0xffu,
    0xffu,
    0xffu,
    0xffu,
    0xffu,
    0xffu,
    0xffu
  };
  uint8_t spectrum[128] = {0};

  // max of 16 adjacent magnitudes
  for (uint32_t i=0;i<64;i++)
  {
    uint8_t max_value = 0;
    for (uint32_t j=0;j<16;j++)
    {
      const uint8_t m = magnitude[i*16+j];
      if (m>max_value) max_value = m;
    }
    if (max_value>15u) max_value = 15u;
    spectrum[i] = row1[max_value];
    spectrum[i+64] = row2[max_value];
  }
  spectrum[0] = 0;
  spectrum[63] = 0;
  spectrum[64] = 0;
  spectrum[127] = 0;
  oled.bitmap(0, 2, 64, 4, spectrum);
}

void update_display(const uint32_t signal_level = 0u,const bool show_step = false)
{
    char sz_frequency[16] = "";
    memset(sz_frequency,0,sizeof(sz_frequency));
    ultoa(radio.frequency,sz_frequency,10);
    // 7 digits
    // 7123450 -> 7.123.450
    // 0123456    012345678
    sz_frequency[8] = sz_frequency[6];
    sz_frequency[7] = sz_frequency[5];
    sz_frequency[6] = sz_frequency[4];
    sz_frequency[5] = '.';
    sz_frequency[4] = sz_frequency[3];
    sz_frequency[3] = sz_frequency[2];
    sz_frequency[2] = sz_frequency[1];
    sz_frequency[1] = '.';
    oled.clear();
    oled.setFont(FONT6X8);
    oled.setCursor(3,0);
    if (show_step)
    {
      oled.write(sz_frequency[0]); // 1 MHz
      oled.write(sz_frequency[1]); // Dot
      oled.write(sz_frequency[2]); // 100 KHz
      oled.write(sz_frequency[3]); // 10 Khz
      if (radio.step==1000) oled.invertOutput(true);
      oled.write(sz_frequency[4]); // 1 Khz
      if (radio.step==1000) oled.invertOutput(false);
      oled.write(sz_frequency[5]); // Dot
      if (radio.step==100) oled.invertOutput(true);
      oled.write(sz_frequency[6]); // 100 Hz
      if (radio.step==100) oled.invertOutput(false);
      if (radio.step==10) oled.invertOutput(true);
      oled.write(sz_frequency[7]); // 10 Hz
      if (radio.step==10) oled.invertOutput(false);
      oled.write(sz_frequency[8]); // 1 Hz
    }
    else
    {
      oled.print(sz_frequency);
    }
    oled.setCursor(0,1);
    oled.print("Mode:");
    if (radio.tx_enable)
    {
      const char *sz_tx = "TX";
      oled.invertOutput(true);
      oled.print(sz_tx);
      oled.invertOutput(false);
    }
    else
    {
      oled.print("  ");
    }
    switch (radio.mode)
    {
      case MODE_LSB: oled.print("LSB"); break;
      case MODE_USB: oled.print("USB"); break;
      case MODE_CWL: oled.print("CWL"); break;
      case MODE_CWU: oled.print("CWU"); break;
    }
    oled.invertOutput(false);
    if (radio.tx_enable)
    {
      // TX
      if (radio.spectrum==SPECTRUM_TX || radio.spectrum==SPECTRUM_RXTX)
      {
        show_spectrum();
      }
      else
      {
        oled.setCursor(0,2);
        oled.print("-25-50-75-");
        const uint8_t sig = min(signal_level,63);
        oled.bitmap(0, 3, sig, 4, DSP::meter);
      }
    }
    else
    {
      // RX
      if (radio.spectrum==SPECTRUM_RX || radio.spectrum==SPECTRUM_RXTX)
      {
        show_spectrum();
      }
      else
      {
        oled.setCursor(0,2);
        oled.print("-3-5-7-9-+");
        const uint8_t sig = min(signal_level,63);
        oled.bitmap(0, 3, sig, 4, DSP::meter);
      }
    }
    oled.switchFrame();
}

static void display_menu(const uint8_t window,const uint8_t selection)
{
  oled.clear();
  oled.setFont(FONT6X8);
  for (uint8_t i=window,j=0;i<NUM_MENU_ITEMS && j<4;i++,j++)
  {
    char sz_menu_name[16] = "";
    memset(sz_menu_name,' ',sizeof(sz_menu_name));
    sz_menu_name[10] = '\0';
    for (int j=0;j<10;j++)
    {
      const char c = menu_options[i].menu_name[j];
      if (c=='\0') break;
      sz_menu_name[j] = c;
    }
    oled.setCursor(0,j);
    if (i==selection) oled.invertOutput(true);
    oled.print(sz_menu_name);
    if (i==selection) oled.invertOutput(false);
  }
  oled.switchFrame();
}

static void display_menu_option(const uint8_t menu_id,const uint8_t option_id,const uint8_t selection)
{
  oled.clear();
  if (menu_id<NUM_MENU_ITEMS)
  {
    const uint8_t num_options = menu_options[menu_id].num_options;
    if (option_id<num_options && selection<num_options)
    {
      oled.setFont(FONT6X8);
      oled.setCursor(0,0);
      oled.print(menu_options[menu_id].menu_name);
      for (uint8_t i=option_id,j=1;i<num_options && j<4;i++,j++)
      {
        char sz_option_name[16] = "";
        memset(sz_option_name,' ',sizeof(sz_option_name));
        sz_option_name[10] = '\0';
        for (int j=1;j<10;j++)
        {
          const char c = menu_options[menu_id].options[i].option_name[j-1];
          if (c=='\0') break;
          sz_option_name[j] = c;
        }
        oled.setCursor(0,j);
        if (i==selection) oled.invertOutput(true);
        oled.print(sz_option_name);
        if (i==selection) oled.invertOutput(false);
      }
    }
  }
  oled.switchFrame();
}

static const option_value_t process_menu(void)
{
  option_value_t option = OPTION_EXIT;
  uint8_t menu_window = 0;
  uint8_t menu_current = 0;
  display_menu(menu_window,menu_current);
  if (digitalRead(PIN_ENCBUT)==LOW)
  {
    delay(50);
    while (digitalRead(PIN_ENCBUT)==LOW)
    {
      delay(50);
    }
    delay(50);
  }
  uint32_t menu_timeout = millis()+MENU_TIMEOUT;
  for (;;)
  {
    mutex_enter_blocking(&rotary_mutex);
    const int32_t rotary_delta = radio.tune;
    radio.tune = 0;
    mutex_exit(&rotary_mutex);
    if (rotary_delta>0)
    {
      menu_timeout = millis()+MENU_TIMEOUT;
      if (menu_current<NUM_MENU_ITEMS-1)
      {
        menu_current++;
        if (menu_current>menu_window+3)
        {
          if (menu_window<NUM_MENU_ITEMS-1)
          {
            menu_window++;
          }
        }
        display_menu(menu_window,menu_current);
      }
    }
    else if (rotary_delta<0)
    {
      menu_timeout = millis()+MENU_TIMEOUT;
      if (menu_current>0)
      {
        menu_current--;
        if (menu_current<menu_window)
        {
          if (menu_window>0)
          {
            menu_window--;
          }          
        }
        display_menu(menu_window,menu_current);
      }
    }
    // button press?
    if (digitalRead(PIN_ENCBUT)==LOW)
    {
      delay(50);
      while (digitalRead(PIN_ENCBUT)==LOW)
      {
        delay(50);
      }
      delay(50);
      if (menu_options[menu_current].menu_value==MENU_EXIT)
      {
        return option;
      }
      // process menu options
      menu_timeout = millis()+MENU_TIMEOUT;
      const uint8_t num_options = menu_options[menu_current].num_options;
      uint8_t option_window = 0;
      uint8_t option_current = 0;
      display_menu_option(menu_current,option_window,option_current);
      for (;;)
      {
        mutex_enter_blocking(&rotary_mutex);
        const int32_t rotary_delta = radio.tune;
        radio.tune = 0;
        mutex_exit(&rotary_mutex);
        if (rotary_delta>0)
        {
          menu_timeout = millis()+MENU_TIMEOUT;
          if (option_current<num_options-1)
          {
            option_current++;
            if (option_current>option_window+2)
            {
              if (option_window<num_options-1)
              {
                option_window++;
              }
            }
            display_menu_option(menu_current,option_window,option_current);
          }
        }
        else if (rotary_delta<0)
        {
          menu_timeout = millis()+MENU_TIMEOUT;
          if (option_current>0)
          {
            option_current--;
            if (option_current<option_window)
            {
              if (option_window>0)
              {
                option_window--;
              }          
            }
            display_menu_option(menu_current,option_window,option_current);
          }
        }
        // button press?
        if (digitalRead(PIN_ENCBUT)==LOW)
        {
          delay(50);
          while (digitalRead(PIN_ENCBUT)==LOW)
          {
            delay(50);
          }
          delay(50);
          option = menu_options[menu_current].options[option_current].option_value;
          return option;
        }
        if (millis()>menu_timeout)
        {
          return OPTION_EXIT;
        }
      }
    }
    if (millis()>menu_timeout)
    {
      return OPTION_EXIT;
    }
  }
  return option;
}

void __not_in_flash_func(loop)(void)
{
  // run DSP on core 0
  static bool tx = false;
  if (tx)
  {
    // TX, check if changed to RX
    if (radio.tx_enable)
    {
      if (adc_value_ready)
      {
        adc_value_ready = false;
        int16_t tx_i = 0;
        int16_t tx_q = 0;
        switch (radio.mode)
        {
          case MODE_LSB: DSP::process_mic(adc_value,tx_q,tx_i);   break;
          case MODE_USB: DSP::process_mic(adc_value,tx_i,tx_q);   break;
          case MODE_CWL: CW::process_cw(radio.keydown,tx_i,tx_q); break;
          case MODE_CWU: CW::process_cw(radio.keydown,tx_i,tx_q); break;
        }
        tx_i = constrain(tx_i,-512,+511);
        tx_q = constrain(tx_q,-512,+511);
        dac_value_i_p = 512+tx_i;
        dac_value_i_n = 511-tx_i;
        dac_value_q_p = 512+tx_q;
        dac_value_q_n = 511-tx_q;
        adc_data_i[adc_sample_p] = tx_i<<5;
        adc_data_q[adc_sample_p] = tx_q<<5;
        adc_sample_p++;
        adc_sample_p &= (MAX_ADC_SAMPLES-1);
      }
    }
    else
    {
      // switched to RX
      pinMode(PIN_MIC,OUTPUT);
      digitalWrite(PIN_MIC,LOW);
      // set TX output to zero in receive mode
      dac_value_i_p = 0;
      dac_value_i_n = 0;
      dac_value_q_p = 0;
      dac_value_q_n = 0;
      tx = false;
    }
  }
  else
  {
    // RX, check if changed to TX
    if (radio.tx_enable)
    {
      // switch to TX
      adc_gpio_init(PIN_MIC);
      adc_select_input(MIC_MUX);
      tx = true;
    }
    else
    {
      if (adc_value_ready)
      {
        adc_value_ready = false;
        if (i2s.available())
        {
          int32_t ii = 0;
          int32_t qq = 0;
          i2s.read32(&ii, &qq);
          ii >>= 16;
          qq >>= 16;
          adc_data_i[adc_sample_p] = (int16_t)ii;
          adc_data_q[adc_sample_p] = (int16_t)qq;
          adc_sample_p++;
          adc_sample_p &= (MAX_ADC_SAMPLES-1);
          int32_t dac_audio = 0;
          switch (radio.mode)
          {
            case MODE_LSB: dac_audio = DSP::process_ssb(qq,ii,radio.jnr); break;
            case MODE_USB: dac_audio = DSP::process_ssb(ii,qq,radio.jnr); break;
            case MODE_CWL: dac_audio = DSP::process_cw(qq,ii);  break;
            case MODE_CWU: dac_audio = DSP::process_cw(ii,qq);  break;
          }
          dac_audio = constrain(dac_audio,-2048l,+2047l);
          dac_audio += 2048l;
          dac_h = dac_audio >> 6;
          dac_l = dac_audio & 0x3f;
        }
        // only process rotary in receive mode
        static int32_t rotary = 0l;
        switch (r.process())
        {
          case DIR_CW:  rotary++; break;
          case DIR_CCW: rotary--; break;
        }
        if (rotary!=0)
        {
          // don't hang around if we can't own the mutex immediately
          // rotary will record any rotations
          if (mutex_try_enter(&rotary_mutex,0ul))
          {
            radio.tune += rotary;
            rotary = 0;
            mutex_exit(&rotary_mutex);
          }
        }
        if (save_settings_now)
        {
          save_settings_now = false;
          save_settings();
        }
      }
    }
  }
}

static void process_spectrum(void)
{
  int16_t data_re[1024] = {0};
  int16_t data_im[1024] = {0};
  const uint32_t sample_p = adc_sample_p;
  if (sample_p<800)
  {
    // first half is in use, get the second half
    for (uint32_t i=0;i<1024;i++)
    {
      data_re[i] = adc_data_i[i+1024];
      data_im[i] = adc_data_q[i+1024];
    }
    spectrum::process(data_re,data_im,magnitude);
  }
  else if (sample_p>1023 && sample_p<1800)
  {
    // second half is in use, get the first half
    for (uint32_t i=0;i<1024;i++)
    {
      data_re[i] = adc_data_i[i];
      data_im[i] = adc_data_q[i];
    }
    spectrum::process(data_re,data_im,magnitude);
  }
}

static void process_ssb_tx(void)
{
  // 1. mute the receiver
  // 2. set TX/RX relay to TX
  // 3. enable MIC (DSP)
  // 4. enable TX bias

  // indicate PTT pressed
  digitalWrite(LED_BUILTIN,HIGH);

  // mute the receiver
  analogWrite(PIN_MUTE,MUTE);
  delay(10);

  // disable QSD
  digitalWrite(PIN_RXN,HIGH);

  // enable MIC processing
  radio.tx_enable = true;
  update_display();
  delay(10);

  // enable QSE and TX bias
  digitalWrite(PIN_TXN,LOW);
  digitalWrite(PIN_TXBIAS,HIGH);
  delay(50);

  // wait for PTT release
  uint32_t mic_peak_level = 0;
  uint32_t mic_level_update = 0;
  uint32_t tx_display_update = 0;
  uint32_t mic_hangtime_update = 0;
  while (digitalRead(PIN_PTT)==LOW)
  {
    const uint32_t now = millis();
    if (radio.spectrum==SPECTRUM_TX || radio.spectrum==SPECTRUM_RXTX)
    {
      // spectrum data is mic input
      process_spectrum();
      if (now>tx_display_update)
      {
        update_display();
        tx_display_update = now + 50ul;
      }
    }
    else
    {
      static const uint32_t MIC_LEVEL_DECAY_RATE = 50ul;
      static const uint32_t MIC_LEVEL_HANG_TIME = 1000ul;
      const uint32_t mic_level = abs(adc_value)>>5;
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
      if (now>tx_display_update)
      {
        update_display(mic_peak_level);
        tx_display_update = now + 50ul;
      }
      else
      {
        delayMicroseconds(32u);
      }
    }
  }
}

static void process_key(void)
{
  // mute the receiver
  analogWrite(PIN_MUTE,MUTE);

  // disable QSD
  digitalWrite(PIN_RXN,HIGH);
  delay(10);

  // enable TX processing
  radio.keydown = false;
  radio.tx_enable = true;

  // enable QSE and TX bias
  digitalWrite(PIN_TXN,LOW);
  digitalWrite(PIN_TXBIAS,HIGH);
  delay(10);

  // stay here until timeout after key up (PTT released)
  uint32_t cw_timeout = millis() + CW_TIMEOUT;
  for (;;)
  {
    if (digitalRead(PIN_PTT)==LOW)
    {
      // indicate PTT pressed
      digitalWrite(LED_BUILTIN,HIGH);
      radio.keydown = true;
      cw_timeout = millis() + CW_TIMEOUT;
      update_display(63u);
      delay(20);
    }
    else
    {
      // indicate PTT released
      digitalWrite(LED_BUILTIN,LOW);
      radio.keydown = false;
      update_display(0u);
      delay(20);
      if (millis()>cw_timeout)
      {
        break;
      }
    }
  }
}

void loop1(void)
{
  // run UI on core 1
  static uint32_t old_frequency = radio.frequency;
  static mode_t old_mode = radio.mode;
  static uint32_t old_spectrum = radio.spectrum;
  static uint32_t old_jnr = radio.jnr;
  static uint32_t show_step = 0;

  // process button press
  // short press: change tuning step
  // long press: menu options
  enum button_state_t {BUTTON_IDLE,BUTTON_TEST_SHORT,BUTTON_WAIT_RELEASE};
  enum button_action_t {BUTTON_NO_PRESS,BUTTON_SHORT_PRESS,BUTTON_LONG_PRESS};
  static button_state_t button_state = BUTTON_IDLE;
  static uint32_t button_timer = 0;
  button_action_t button_action = BUTTON_NO_PRESS;
  if (!radio.tx_enable)
  {
    switch (button_state)
    {
      case BUTTON_IDLE:
      {
        if (digitalRead(PIN_ENCBUT)==LOW)
        {
          button_state = BUTTON_TEST_SHORT;
          button_timer = millis()+BUTTON_LONG_PRESS_TIME;
          delay(50);
        }
        break;
      }
      case BUTTON_TEST_SHORT:
      {
        const uint32_t now = millis();
        if (digitalRead(PIN_ENCBUT)==HIGH)
        {
          button_state = BUTTON_IDLE;
          if (now<button_timer)
          {
            button_action = BUTTON_SHORT_PRESS;
          }
          delay(50);
          break;
        }
        if (now>button_timer)
        {
          button_state = BUTTON_WAIT_RELEASE;
          button_action = BUTTON_LONG_PRESS;
        }
        break;
      }
      case BUTTON_WAIT_RELEASE:
      {
        if (digitalRead(PIN_ENCBUT)==HIGH)
        {
          button_state = BUTTON_IDLE;
          delay(50);
        }
        break;
      }
    }

    // process button action
    switch (button_action)
    {
      case BUTTON_SHORT_PRESS:
      {
        show_step = millis() + SHOW_STEP_TIMEOUT;
        switch (radio.step)
        {
          case 1000: radio.step = 100;  break;
          case 500:  radio.step = 1000; break;
          case 100:  radio.step = 10;   break;
          case 10:   radio.step = 1000; break;
        }
        break;
      }
      case BUTTON_LONG_PRESS:
      {
        const option_value_t option = process_menu();
        switch (option)
        {
          case OPTION_MODE_LSB:      radio.set_mode = SETMODE_LSB;   break;
          case OPTION_MODE_USB:      radio.set_mode = SETMODE_USB;   break;
          case OPTION_MODE_CWL:      radio.set_mode = SETMODE_CWL;   break;
          case OPTION_MODE_CWU:      radio.set_mode = SETMODE_CWU;   break;
          case OPTION_MODE_AUTO:     radio.set_mode = SETMODE_AUTO;  break;
          case OPTION_STEP_10:       radio.step = 10U;               break;
          case OPTION_STEP_100:      radio.step = 100U;              break;
          case OPTION_STEP_500:      radio.step = 500U;              break;
          case OPTION_STEP_1000:     radio.step = 1000U;             break;
          case OPTION_SPECTRUM_RX:   radio.spectrum = SPECTRUM_RX;   break; 
          case OPTION_SPECTRUM_TX:   radio.spectrum = SPECTRUM_TX;   break; 
          case OPTION_SPECTRUM_RXTX: radio.spectrum = SPECTRUM_RXTX; break; 
          case OPTION_SPECTRUM_OFF:  radio.spectrum = SPECTRUM_OFF;  break; 
          case OPTION_JNR_1:         radio.jnr = 1u;                 break; 
          case OPTION_JNR_2:         radio.jnr = 2u;                 break; 
          case OPTION_JNR_3:         radio.jnr = 3u;                 break; 
          case OPTION_JNR_OFF:       radio.jnr = 0u;                 break; 
        }
        break;
      }
    }

    // when settings change save them
    bool settings_changed = false;
    if (radio.spectrum != old_spectrum)
    {
      old_spectrum = radio.spectrum;
      settings_changed = true;
    }
    if (radio.jnr != old_jnr)
    {
      old_jnr = radio.jnr;
      settings_changed = true;
    }
    if (settings_changed)
    {
      save_settings_now = true;
    }

    // set the mode based on frequency or as selected
    radio_mode_t new_mode = MODE_NONE;
    switch (radio.set_mode)
    {
      case SETMODE_AUTO: new_mode = radio.frequency==7074000ul?MODE_USB:MODE_LSB; break;
      case SETMODE_LSB:  new_mode = MODE_LSB; break;
      case SETMODE_USB:  new_mode = MODE_USB; break;
      case SETMODE_CWL:  new_mode = MODE_CWL; break;
      case SETMODE_CWU:  new_mode = MODE_CWU; break;
    }
    if (new_mode != old_mode)
    {
      radio.mode = new_mode;
    }

    // process main tuning
    mutex_enter_blocking(&rotary_mutex);
    const int32_t tuning_delta = radio.tune;
    radio.tune = 0;
    mutex_exit(&rotary_mutex);
    radio.frequency = radio.frequency+(tuning_delta * (int32_t)radio.step);
    radio.frequency = radio.frequency/radio.step;
    radio.frequency = radio.frequency*radio.step;
    radio.frequency = constrain(radio.frequency,BAND_40M_LO,BAND_40M_HI);
    if (radio.frequency!=old_frequency || radio.mode!=old_mode)
    {
      old_frequency = radio.frequency;
      old_mode = radio.mode;
      const uint32_t correct4cw = radio.mode==MODE_CWL?+CW_SIDETONE:radio.mode==MODE_CWU?-CW_SIDETONE:0u;
      const uint64_t f = SI5351_FREQ_MULT * (radio.frequency+correct4cw);
      const uint64_t p = SI5351_FREQ_MULT * (radio.frequency+correct4cw)*QUADRATURE_DIVISOR;
      si5351.set_freq_manual(f,p,SI5351_CLK0);
      si5351.set_freq_manual(f,p,SI5351_CLK1);
    } // frequency change
  } // receive mode
 
  // process the receive spectrum
  if (radio.spectrum==SPECTRUM_RX || radio.spectrum==SPECTRUM_RXTX)
  {
    process_spectrum();
  }

  // update the display every 50ms
  static uint32_t next_update = 0;
  const uint32_t now = millis();
  if (now>next_update)
  {
    next_update = now + 50ul;
    update_display(DSP::smeter(),now<show_step);
  }

  // check for PTT
  if (digitalRead(PIN_PTT)==LOW)
  {
    const float saved_agc = DSP::agc_peak;
    if (radio.mode==MODE_CWL || radio.mode==MODE_CWU)
    {
      const uint32_t correct4cw_tx = radio.mode==MODE_CWL?+1000u:-1000u;
      const uint64_t f_tx = (radio.frequency+correct4cw_tx)*SI5351_FREQ_MULT;
      const uint64_t p_tx = (radio.frequency+correct4cw_tx)*QUADRATURE_DIVISOR*SI5351_FREQ_MULT;
      si5351.set_freq_manual(f_tx,p_tx,SI5351_CLK0);
      si5351.set_freq_manual(f_tx,p_tx,SI5351_CLK1);
      process_key();
      const uint32_t correct4cw_rx = radio.mode==MODE_CWL?+CW_SIDETONE:radio.mode==MODE_CWU?-CW_SIDETONE:0u;
      const uint64_t f_rx = (radio.frequency+correct4cw_rx)*SI5351_FREQ_MULT;
      const uint64_t p_rx = (radio.frequency+correct4cw_rx)*QUADRATURE_DIVISOR*SI5351_FREQ_MULT;
      si5351.set_freq_manual(f_rx,p_rx,SI5351_CLK0);
      si5351.set_freq_manual(f_rx,p_rx,SI5351_CLK1);
    }
    else
    {
      process_ssb_tx();
    }
    // back to receive
    digitalWrite(PIN_TXBIAS,LOW);
    digitalWrite(PIN_TXN,HIGH);
    radio.tx_enable = false;
    delay(10);
    digitalWrite(PIN_RXN,LOW);
    digitalWrite(LED_BUILTIN,LOW);
    update_display();
    digitalWrite(PIN_MUTE,UNMUTE);
    delay(50);
    DSP::agc_peak = saved_agc;
  }
}