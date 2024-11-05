#include <SPI.h>
#include "Adafruit_ILI9341.h"
#include "Adafruit_FT6206.h"
#include "RTClib.h"
#include "lighting-program.h"
#include "serial-comm.h"
#include "menu-system.h"
#include "touch-debounce.h"
#include "Adafruit_SHT31.h"
#include <stdio.h>
#define HWSERIAL Serial1 //for Teensy Only
//#define ROTATE_DISPLAY

static WLabel debug_display1( 2, 201, 60  );
static WLabel debug_display2( 2, 20,  60  );
static WLabel debug_display3( 2, 201, 100 );
static WLabel debug_display4( 2, 20,  100 );

// The display also uses hardware SPI, plus #9 & #10
#define TFT_CS 10
#define TFT_DC 9
Adafruit_ILI9341 tft(TFT_CS, TFT_DC);

// The FT6206 uses hardware I2C (SCL/SDA)
Adafruit_FT6206 ctp = Adafruit_FT6206();

// Debouncing for the touch screen controller
TouchDebounce debouncer;

// Our lighting programs
LightingProgram lp;

// The menu controller
MenuSystem menu;

// Last second
static uint8_t last_second;

Adafruit_SHT31 sht31 = Adafruit_SHT31();
const unsigned int MAX_MESSAGE_LENGTH = 16;

/* needed only on the teensy
 * the rtc is not reset properly, and if we reset during a read
 * the i2c state machine in the rtc hangs.. waiting for more clocks
 * so just feed it some clocks
 */
static void whack_i2c_with_clocks()
{
   pinMode(19, OUTPUT);

   for (int i = 0; i < 10; i++)
   {
      digitalWrite(19, LOW);
      delayMicroseconds(3);
      digitalWrite(19, HIGH);
      delayMicroseconds(3);
   }
   pinMode(19, INPUT);
}

void setup(void)
{
   whack_i2c_with_clocks();
   Serial.begin(1200);
   HWSERIAL.begin(1200);
   initSerialComms();
   tft.begin();
   debouncer.begin();

   lp.begin();

   if (! ctp.begin(40) )
   {
      Serial.println(F("ERR: unable to start touchscreen."));
   } 

   // origin = left,top landscape (USB left upper)
   tft.setRotation(3); 
   menu.begin();
}

void loop()
{
   lp.tick();
   if ( lp.now_second() != last_second )
   {
      menu.tick();
      last_second = lp.now_second();
      serialCommTick();
   }

   if (ctp.touched())
   {
      TS_Point p1 = ctp.getPoint();
      delay( 10 );
      TS_Point p2 = ctp.getPoint();

      int16_t x1 = p1.x;
      int16_t y1 = p1.y;
      int16_t x2 = p2.x;
      int16_t y2 = p2.y;
      int16_t temp;

      // map to the display configuration
      temp = map(y1, 0, 320, 320, 0);
      y1   = map(x1, 0, 240, 0, 240);
      x1   = temp;

      temp = map(y2, 0, 320, 320, 0);
      y2   = map(x2, 0, 240, 0, 240);
      x2   = temp;

#ifdef ROTATE_DISPLAY
      // map back to the Adafruit FT6206 display configuration
      x1 = map(x1, 0, 320, 320, 0);
      y1 = map(y1, 0, 240, 240, 0);

      x2 = map(x2, 0, 320, 320, 0);
      y2 = map(y2, 0, 240, 240, 0);
#endif

      debouncer.hit(x1, y1, x2, y2);
   } else {
      debouncer.nohit();
   }

   if (debouncer.touched()) {
      menu.touch(debouncer.x(), debouncer.y());
   }

   repeatedUpdatePoll();

   serial_poll();

   //Check to see if anything is available in the serial receive buffer
   while (HWSERIAL.available() > 0)
   {
      //Create a place to hold the incoming message
      static char message[MAX_MESSAGE_LENGTH];
      static unsigned int message_pos = 0;

      //Read the next available byte in the serial receive buffer
      char inByte = HWSERIAL.read();

      //Message coming in (check not terminating character) and guard for over message size
      if ( inByte != '\n' && (message_pos < MAX_MESSAGE_LENGTH - 1) )
      {
         //Add the incoming byte to our message
         message[message_pos] = inByte;
         message_pos++;
      }
      //Full message received...
      else
      { 
         //Add null character to string
         message[message_pos] = '\0';
         
         // truncate message
         int messageLength = sizeof(message)/sizeof(message[0]);
         char messageNum[messageLength - 10];
         for (int i = 10; i < messageLength - 1; i++){
            messageNum[i - 10] = message[i];
         }

         //Print the message (or do other things)
         Serial.println(message);

         // Set currentTemp to number in message
         lp.currentTemp = atoi(messageNum);

         //Reset for the next message
         message_pos = 0;
      }
   }
}
