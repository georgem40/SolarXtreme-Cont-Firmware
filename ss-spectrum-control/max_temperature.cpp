#include "menu-system.h"
#include "lighting-program.h"
#include "serial-comm.h"

enum edit_list {
   NOT_EDITING,
   E_TEMP,
   E_LO,
   E_HI
};

static enum edit_list am_editing;

static uint8_t last_second;
static uint8_t LoLimit2;
static uint8_t HiLimit2;

static WLabel temp_label(10,    100, 10);
static WLabel lo_limit_label(10,    100, 70);
static WLabel hi_limit_label(10,    100, 130);

static void update_widget_for(enum edit_list n, uint16_t c)
{
   switch (n) {
      case NOT_EDITING:
         break;
      case E_TEMP:
         temp_label.paint_four_digits(lp.currentTemp, c, ILI9341_WHITE);
         break;
      case E_LO:
         if (lp.LoLimit > lp.HiLimit){
            lp.LoLimit = lp.HiLimit;
         }
         lo_limit_label.paint_four_digits(lp.LoLimit, c, ILI9341_WHITE);
         break;
      case E_HI:
         if (lp.LoLimit > lp.HiLimit){
            lp.HiLimit = lp.LoLimit;
         }
         hi_limit_label.paint_four_digits(lp.HiLimit, c, ILI9341_WHITE);
         break;
   }
}

static uint8_t *knob_for(enum edit_list n, uint8_t *max)
{
   switch (n) {
      case NOT_EDITING: return nullptr;
      case E_LO:   *max = 150; return &lp.LoLimit;
      case E_HI: *max = 150; return &lp.HiLimit;
   }
   return nullptr;
}

static void start_editing(enum edit_list n)
{
   if (am_editing != NOT_EDITING)
      update_widget_for(am_editing, ILI9341_BLACK);

   am_editing = n;

   if (am_editing != NOT_EDITING)
      update_widget_for(am_editing, ILI9341_RED);
}

void WEditTemp::paint()
{
   update_widget_for(E_TEMP, ILI9341_BLACK);
   update_widget_for(E_LO, ILI9341_BLACK);
   update_widget_for(E_HI, ILI9341_BLACK);

   WLabel::paint(F("TEMP"),      40, 10, ILI9341_GREEN, ILI9341_BLACK, 2, 0);
   WLabel::paint(F("LO"),      50, 70, ILI9341_GREEN, ILI9341_BLACK, 2, 0);
   WLabel::paint(F("HI"),     50, 130, ILI9341_GREEN, ILI9341_BLACK, 2, 0);

   menu.paintChangeControls();
   start_editing(NOT_EDITING);
   
   last_second = 0xff;

   tick();
}

static void edit_value(int chg)
{
   uint8_t max = 0;
   uint8_t *knob = knob_for(am_editing, &max);
   int result = (int)(*knob) + chg;
   if (result < 1)
      result = 1;
   if (result > max)
      result = max;

   *knob = result;
   update_widget_for(am_editing, ILI9341_RED);
}

void WEditTemp::touch(uint16_t x, uint16_t y)
{
    if (save_button.hit(x, y)) {
      lp.saveLoLimit( lp.LoLimit );
      lp.saveHiLimit( lp.HiLimit );
      lp.restart();
      menu.setMenu(setup_menu);
    }
    else if (up_slow.hit(x, y) && (am_editing != NOT_EDITING))
      edit_value(1);
    else if (up_fast.hit(x, y) && (am_editing != NOT_EDITING))
      edit_value(10);
    else if (down_fast.hit(x, y) && (am_editing != NOT_EDITING))
      edit_value(-10);
    else if (down_slow.hit(x, y) && (am_editing != NOT_EDITING))
      edit_value(-1);

   else if (lo_limit_label.hit(x, y)) start_editing(E_LO);
   else if (hi_limit_label.hit(x, y)) start_editing(E_HI);
   else
      start_editing(NOT_EDITING);
}

void WEditTemp::tick()
{ 
   // Update current temperature
   if (lp.now_second() != last_second) {
      last_second = lp.now_second();
      temp_label.paint_four_digits(lp.currentTemp, ILI9341_BLACK, ILI9341_WHITE);
   }
}