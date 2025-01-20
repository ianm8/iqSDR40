#ifndef MENU_H
#define MENU_H

/*

 Step
  10
  100
  500
  1000
  Exit

 Mode
   LSB
   USB
   CWL
   CWU
   Auto
   Exit

  JNR
    Level 1
    Level 2
    Level 3
    Off
    Exit

  Exit
    Exit

*/

#define NUM_MENU_ITEMS 7U

enum menu_top_t
{
  MENU_STEP,
  MENU_MODE,
  MENU_JNR,
  MENU_SPECTRUM,
  MENU_CW_MODE,
  MENU_CW_SPEED,
  MENU_EXIT
};

enum option_value_t
{
  OPTION_STEP_10,
  OPTION_STEP_100,
  OPTION_STEP_500,
  OPTION_STEP_1000,
  OPTION_MODE_USB,
  OPTION_MODE_LSB,
  OPTION_MODE_CWL,
  OPTION_MODE_CWU,
  OPTION_MODE_AUTO,
  OPTION_SPECTRUM_RX,
  OPTION_SPECTRUM_TX,
  OPTION_SPECTRUM_RXTX,
  OPTION_SPECTRUM_OFF,
  OPTION_JNR_1,
  OPTION_JNR_2,
  OPTION_JNR_3,
  OPTION_JNR_OFF,
  OPTION_CW_STRAIGHT,
  OPTION_CW_PADDLE,
  OPTION_CW_SPEED_10,
  OPTION_CW_SPEED_15,
  OPTION_CW_SPEED_20,
  OPTION_CW_SPEED_25,
  OPTION_EXIT,
  OPTION_NULL
};

struct options_t
{
  option_value_t option_value;
  const char* option_name;
};

volatile static struct
{
  const menu_top_t menu_value;
  const char *menu_name;
  const uint8_t num_options;
  const options_t options[6];
}
menu_options[] =
{
  {
    MENU_MODE,
    "Mode",
    6U,
    {
      {OPTION_MODE_LSB,"LSB"},
      {OPTION_MODE_USB,"USB"},
      {OPTION_MODE_CWL,"CWL"},
      {OPTION_MODE_CWU,"CWU"},
      {OPTION_MODE_AUTO,"AUTO"},
      {OPTION_EXIT,"Exit"}
    }
  },
  {
    MENU_JNR,
    "JNR",
    5U,
    {
      {OPTION_JNR_1,"Level 1"},
      {OPTION_JNR_2,"Level 2"},
      {OPTION_JNR_3,"Level 3"},
      {OPTION_JNR_OFF,"Off"},
      {OPTION_EXIT,"Exit"},
      {OPTION_NULL,"NULL"}
    }
  },
  {
    MENU_SPECTRUM,
    "Spectrum",
    5U,
    {
      {OPTION_SPECTRUM_RX,"RX Only"},
      {OPTION_SPECTRUM_TX,"TX Only"},
      {OPTION_SPECTRUM_RXTX,"RX/TX On"},
      {OPTION_SPECTRUM_OFF,"Off"},
      {OPTION_EXIT,"Exit"},
      {OPTION_NULL,"NULL"}
    }
  },
  {
    MENU_CW_MODE,
    "CW Mode",
    3U,
    {
      {OPTION_CW_STRAIGHT,"Straight"},
      {OPTION_CW_PADDLE,"Paddle"},
      {OPTION_EXIT,"Exit"},
      {OPTION_NULL,"NULL"},
      {OPTION_NULL,"NULL"},
      {OPTION_NULL,"NULL"}
    }
  },
  {
    MENU_CW_SPEED,
    "CW Speed",
    5U,
    {
      {OPTION_CW_SPEED_10,"10 WPM"},
      {OPTION_CW_SPEED_15,"15 WPM"},
      {OPTION_CW_SPEED_20,"20 WPM"},
      {OPTION_CW_SPEED_25,"25 WPM"},
      {OPTION_EXIT,"Exit"},
      {OPTION_NULL,"NULL"}
    }
  },
  {
    MENU_STEP,
    "Step",
    5U,
    {
      {OPTION_STEP_10,"10"},
      {OPTION_STEP_100,"100"},
      {OPTION_STEP_500,"500"},
      {OPTION_STEP_1000,"1000"},
      {OPTION_EXIT,"Exit"},
      {OPTION_NULL,"NULL"}
    }
  },
  {
    MENU_EXIT,
    "Exit",
    1U,
    {
      {OPTION_EXIT,"Exit"},
      {OPTION_NULL,"NULL"},
      {OPTION_NULL,"NULL"},
      {OPTION_NULL,"NULL"},
      {OPTION_NULL,"NULL"},
      {OPTION_NULL,"NULL"}
    }
  }
};

#endif