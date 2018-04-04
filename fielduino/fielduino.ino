/*******************************************
Fielduino

PWM generator with menu to select frequency and dutty %

this is specific to AVR, tested on AtMega328

using hardware timmers

a pratical example of ArduinoMenu library usage

Rui Azevedo (neu-rah)
ruihfazevedo@gmail.com
http://www.r-site.net
*******************************************/

#include <menu.h>
#include <menuIO/serialIO.h>
#include "avrTC.h"

using namespace Menu;

#define PULSEPIN 9

void wave(unsigned long freq,uint8_t dutty=50) {
  //Serial.print("wave:");
  //Serial.println(freq);
  tc1.play(freq,dutty);
}

///////////////////////////////////////////////////
float frequency=480;// Hz
int dutty=50;// %

/////////////////////////////////////////////
// MENU DEF & menu functions

void nothing() {}
//void pulseOff() {tc1.off();}
void updateWave() {wave(frequency,dutty);}

MENU(mainMenu,"Main",doNothing,anyEvent,noStyle
  ,FIELD(frequency,"Freq","Hz",0,16000000,1000,10,updateWave,enterEvent,wrapStyle)
  ,FIELD(dutty,"Duty","%",0,100,10,1,updateWave,enterEvent,wrapStyle)
);

#define MAX_DEPTH 1

MENU_OUTPUTS(out,MAX_DEPTH
  ,SERIAL_OUT(Serial)
  ,NONE//must have 2 items at least
);\

serialIn in(Serial);
NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);

void setup() {
  //adc.ps=avrADC::clk_4;//accelerate ADC converter, setting prescale
  Serial.begin(9600);
  Serial.println("fielduino www.r-site.net");

  pinMode(PULSEPIN,OUTPUT);
  tc2.setWaveMode(2);//fast PWM, freqx2
  tc2.setOutMode_A(0);//no output
  tc2.setOutMode_B(0);//no output
  tc1.play(100);
  delay(1000);
}

void loop() {
  nav.poll();
}
