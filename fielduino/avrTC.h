/**************
Oct. 2014 Rui Azevedo - ruihfazevedo(@rrob@)gmail.com
creative commons license 3.0: Attribution-ShareAlike CC BY-SA
This software is furnished "as is", without technical support, and with no
warranty, express or implied, as to its usefulness for any purpose.

Thread Safe: No
Extendable: Yes

AVR timer/counter generalization
please do not allocate variables on the classes
this is intended to be JUST an hardware description
with some attached code
.. add code as you like
***/

#include <Arduino.h>

/**********************
to be considered in future:
  clock sources
  input capture
  output compare
  compare match output
  wave gen modes
  interrupts
  Output Compare Modulator?

8 bits: 0, 2
16 bits 1,3,4,5

TC2 prescales:
  0,1,8,32,64,128,256,1024
Timer/Counter 0,1, 3, 4, and 5 Prescaler
  0,1,8,64,256,1024,ext



TC0 CS1 8 bits
TC1 CS1 16 bits
TC2 CS2 8 bits
*/

// this macros alllow for usage of '_r' as a reserved bit or bitfield (only 1 per line)
#define _CONCAT_(x,y) x ## y
#define CONCAT(x,y) _CONCAT_(x,y)
#define _r CONCAT(_R,__LINE__)

#define DUTTY_MAX 100
#define DEFAULT_DUTTY (DUTTY_MAX/2)

struct tifr {
  byte tov:1;
  byte ocfA:1;
  byte ocfB:1;
  byte ocfC:1;
  byte _na0:1;
  byte icf:1;
  byte _na1:2;
};

struct timsk {
	byte tioe:1;
	byte ocieA:1;
	byte ocieB:1;
	byte ocieC:1;
	byte _na:1;
	byte ocie:1;
  byte _na1:2;
};

class _tc8bits {
public:
	enum waveModes {Normal=0,PWMPhase,CTC,FastPWM,PWMPhace_OC=5,FastPWM_OC=7};
	enum outModes {None=0,Toggle,Clear,Set};
  union {
  	byte crA;
  	struct {
  		byte wgm0:1;
  		byte wgm1:1;
  		byte _r:2;
  		byte comB:2;
  		byte comA:2;
  	};
  };
  union {
	  byte crB;
	  struct {
	  	byte cs:3;//clock source selector (prescale)
	  	byte wgm2:1;
	  	byte _r:2;
	  	byte focB:1;
	  	byte focA:1;
	  };
	};
  volatile byte cnt;
  byte ocA, ocB;
  inline void setWaveMode(int n) {
  	crA&=~0b11;
		crA|=(n&0b11);
		wgm2=n>>2;
  }
  inline void setOutMode_A(int n) {comA=n;}
  inline void setOutMode_B(int n) {comB=n;}
  inline void setClockSource(int n) {cs=n;}
  void on(uint16_t fr,int scale=1,int dutty=DEFAULT_DUTTY) {
    //crC=0b11000000;
    //crA=0b10000001;
    //crB=0b00001000;
    cnt=0;
    ocA=fr;//map(dutty,0,DUTTY_MAX,0,fr);
    setClockSource(scale);
    //crB = (crB & ~ 0b111) | scale;
  }
  inline void setDutty(int dutty) {}
  void off() {}
};

class _tc16bits {
public:
  union {
  	byte crA;
  	struct {
  		byte wgm0:1;
  		byte wgm1:1;
  		byte _r:2;
  		byte comB:2;
  		byte comA:2;
  	};
  };
  union {
  	byte crB;
  	struct {
  		byte cs:3;
  		byte wgm2:1;
  		byte wgm3:1;
  		byte _r:1;
  		byte ices:1;
  		byte icnc:1;
  	};
  };
  union {
  	byte crC;
  	struct {
  		byte _r:6;
  		byte focB:1;
  		byte focA:1;
  	};
  };
  byte reserved;
  volatile int cnt;
  int ic;
  int ocA,ocB,ocC;

  inline void setOutMode_A(int n) {comA=n;}
  inline void setOutMode_B(int n) {comB=n;}
  inline void setClockSource(int n) {cs=n;}
  void on(uint16_t fr,int scale=1,int dutty=DEFAULT_DUTTY) {
    crC=0b11000000;
    crA=0b10000000;
    crB=0b00010000;
    cnt=0;
    ic=fr;
    //if (fr<=10) Serial.println("ALERT: dutty granularity<=10%");
    ocA=map(dutty,0,DUTTY_MAX,0,fr);
    crB = (crB & ~ 0b111) | scale;
  }

  //TODO: make this generic!
  inline void setDutty(int dutty) {ocA=map(dutty,0,DUTTY_MAX,0,ic);}

  void off() {
    crB = (crB & ~ 0b111);
    crC=0b00000000;
    crA = 0b0000000;
    delay(100);//TOS: what's this?
    cnt=0;
  }
};

class _tc_CS1 {
public:
	enum avrCS1 {stop=0,clk_1,clk_8,clk_64,clk_256,clk_1024,eFall,eRise};
	//const static int CS1Values[8] PROGMEM;

  int bestPrescale(unsigned long f) {
    unsigned long r=F_CPU/f;
    if (r<=0xFFFF) return 1;
    if (r>>3<=0xFFFF && r>>3>0) return 8;
    if (r>>6<=0xFFFF && r>>6>0) return 64;
    if (r>>8<=0xFFFF && r>>8>0) return 256;
    if (r>>10<0xFFFF && r>>10>0) return 1024;
  }

  int get_prescale_code(int p) {
    if (p==1024) return 0b101;
    else if (p==256) return 0b100;
    else if (p==64) return 0b011;
    else if (p==8) return 0b010;
    else if (p==1) return 1;
    return 0;
  }
};

class _tc_CS2 {
public:
	//enum avrCS1 {stop=0,clk_1,clk_8,clk_64,clk_256,clk_1024,eFall,eRise};
	enum avrCS2 {stop=0,clk_1,clk_8,clk_32,clk_64,clk_128,clk_256,clk_1024};
	//const static int CS2Values[8] PROGMEM;

  int bestPrescale(unsigned long f) {
    unsigned long r=F_CPU/f;
    if (r<=0xFF) return 1;
    if (r>>3<=0xFF && r>>3>0) return 8;
    if (r>>5<=0xFF && r>>6>0) return 32;
    if (r>>6<=0xFF && r>>6>0) return 64;
    if (r>>7<0xFF && r>>7>0) return 128;
    if (r>>8<0xFF && r>>8>0) return 256;
    if (r>>10<0xFF && r>>10>0) return 1024;
  }

  int get_prescale_code(int p) {
    if (p==1024) return 0b101;
    else if (p==256) return 0b111;
    else if (p==128) return 0b110;
    else if (p==64) return 0b101;
    else if (p==32) return 0b011;
    else if (p==8) return 0b010;
    else if (p==1) return 1;
    return 0;
  }
};

//const int _tc_CS1::CS1Values[8]={0,1,8,64,256,1024,0,0};
//const int _tc_CS2::CS2Values[8]={0,1,8,32,64,128,256,1024};

class _tc8bits_CS1:public _tc8bits, public _tc_CS1 {
public:
  float play(float f,int dutty=DEFAULT_DUTTY) {
    f*=2;
    //int p=prescale(F_CPU/f);
    int p=bestPrescale(f);
    unsigned long r=F_CPU/(p*f);
    if(r>0xFFFF) return 0;
    on(r,get_prescale_code(p),dutty);
    return F_CPU/(2.0*p*r);
  }
};

class _tc16bits_CS1:public _tc16bits, public _tc_CS1 {
public:
  float play(float f,int dutty=DEFAULT_DUTTY) {
    f*=2;
    //int p=prescale(F_CPU/f);
    int p=bestPrescale(f);
    unsigned long r=F_CPU/(p*f);
    if(r>0xFFFF) return 0;
    on(r,get_prescale_code(p),dutty);
    return F_CPU/(2.0*p*r);
  }
};

class _tc8bits_CS2:public _tc8bits, public _tc_CS2 {
public:
  float play(float f,int dutty=DEFAULT_DUTTY) {
    f*=2;
    int p=bestPrescale(f);
    unsigned long r=F_CPU/(p*f);
    if(r>0xFFFF) return 0;
    on(r,get_prescale_code(p),dutty);
    return F_CPU/(2.0*p*r);
  }
};

// meta relation, timers <-> interrupt flags/mask
// flags and masks are on separate memory block
// please keep thing inline
template<byte TIFR,byte TIMSK>
class tc8bits_CS1:public _tc8bits_CS1 {
public:
  inline void intA() {(*(timsk*)TIMSK).ocieA=1;}//on out compare/match
};

template<byte TIFR,byte TIMSK>
class tc16bits_CS1:public _tc16bits_CS1 {
public:
  inline void intA() {(*(timsk*)TIMSK).ocieA=1;}//on out compare/match
};

template<byte TIFR,byte TIMSK>
class tc8bits_CS2:public _tc8bits_CS2 {
public:
  inline void intA() {(*(timsk*)TIMSK).ocieA=1;}//on out compare/match
};


//mapping things to hardware with interrupt flags and masks
//mapped classes must reflect hardware seting like allowed prescalers....
tc8bits_CS1<0x35,0x6E> &tc0=*(tc8bits_CS1<0x35,0x6E>*)0x44;
tc16bits_CS1<0x36,0x6F> &tc1=*(tc16bits_CS1<0x36,0x6F>*)0x80;
tc8bits_CS2<0x37,0x70> &tc2=*(tc8bits_CS2<0x37,0x70>*)0xB0;
#if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) || defined(__AVR_ATmega1284__)
  tc16bits_CS1<0x38,0x71> &tc3=*(tc16bits_CS1<0x38,0x71>*)0x90;
#endif
#if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
  tc16bits_CS1<0x39,0x72> &tc4=*(tc16bits_CS1<0x39,0x72>*)0xA0;
  tc16bits_CS1<0x40,0x73> &tc5=*(tc16bits_CS1<0x40,0x73>*)0x120;
#endif
