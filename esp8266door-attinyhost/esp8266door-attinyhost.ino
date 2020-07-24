// ATTinyCore, attiny25/45/85, attiny45, 8mhz

//                        +-\/-+
//                  PB5  1|    |8   VCC
//  client power    PB3  2|    |7   PB2 ((SIGNAL INPUT))
//  client callback PB4  3|    |6   PB1 status led
//                  GND  4|    |5   PB0
//                        +----+

// ATMEL ATTINY45 / ARDUINO
//
//                           +-\/-+
//  Ain0       (D  5)  PB5  1|    |8   VCC
//  Ain3       (D  3)  PB3  2|    |7   PB2  (D  2)  INT0  Ain1
//  PWM  Ain2  (D  4)  PB4  3|    |6   PB1  (D  1)        PWM (OC0B)
//  OC1B               GND  4|    |5   PB0  (D  0)        PWM (OC0A)
//                           +----+

#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <util/delay.h>

// Routines to set and claer bits (used in the sleep code)
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

const byte pinStatus = PIN_B1;
const byte pinInput = PIN_B2;

const byte pinClientPower = PIN_B3;
const byte pinClientCallback = PIN_B4;

volatile byte applicationState = 1;
volatile boolean pinClientCallbackValue = 0;
volatile boolean pinClientCallbackPoweredOffValue = 0;
volatile boolean interruptTriggered = false;
volatile boolean watchdogTriggered = false;

void blink(int count);

ISR(WDT_vect) {
  blink(1);
  watchdogTriggered = true;
}

ISR(PCINT0_vect) {
  interruptTriggered = true;
}

void setup_watchdog(int ii) {
  /*
    #define WDTO_15MS   0    // 16ms
    #define WDTO_30MS   1    // 32ms
    #define WDTO_60MS   2    // 64ms
    #define WDTO_120MS  3    // 128ms
    #define WDTO_250MS  4    // 250ms
    #define WDTO_500MS  5    // 500ms
    #define WDTO_1S     6    // 1s
    #define WDTO_2S     7    // 2s
    #define WDTO_4S     8    // 4s
    #define WDTO_8S     9    // 8s
   */
  watchdogTriggered = false;
  byte bb;
  int ww;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);
  ww=bb;

  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCR = bb;
  WDTCR |= _BV(WDIE);
}

void enterSleep(void) {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  //sbi(MCUCR,PUD); //Disables All Internal Pullup Resistors
  cbi(ADCSRA,ADEN); // switch Analog to Digitalconverter OFF
  cbi(ACSR,ACD); // disable analog comparator
  power_all_disable();
  sleep_bod_disable();
  
  sei(); // enable interrupts
  sleep_mode();
  cli(); //disable interrupts
  
  sbi(ADCSRA,ADEN); // switch Analog to Digitalconverter ON
  sbi(ACSR,ACD); // enable analog comparator
}

void enterSleep(int ii) {
  wdt_disable();
  setup_watchdog(ii);
  enterSleep();
}

void initInterrupt(void) {
  cli(); // Disable interrupts during setup
  GIMSK |= (1 << PCIE);   // pin change interrupt enable
  GIFR  |= (1 << PCIF);   // clear any outstanding interrupts
  PCMSK |= (1 << pinInput); // pin change interrupt enabled for PCINT4
}

void setup() {
  wdt_disable();
  
  pinMode(pinInput, INPUT);
  pinMode(pinStatus, OUTPUT);
  pinMode(pinClientPower, OUTPUT);
  digitalWrite(pinClientPower, LOW);
  pinMode(pinClientCallback, INPUT);
  
  initInterrupt();
}

void blink(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(pinStatus, HIGH);
    _delay_ms(5);
    digitalWrite(pinStatus, LOW);
    if (i < count - 1) {
      _delay_ms(250);
    }
  }
}

void loop() {
  wdt_reset();
  wdt_disable();

  if (applicationState < 1) {
    applicationState == 1;
    return;
  }
  
  if (applicationState == 1) {
    if (interruptTriggered) {
      interruptTriggered = false;
      _delay_ms(500); // debounce
      applicationState = 2;
      return;
    }
  }
  
  if (applicationState == 2) {
    pinClientCallbackPoweredOffValue = digitalRead(pinClientCallback);
    digitalWrite(pinClientPower, HIGH);
    applicationState = 3;
    enterSleep(WDTO_2S); return;
  }
  
  if (applicationState == 3) {
    boolean newPinClientCallbackValue = digitalRead(pinClientCallback);
    if (newPinClientCallbackValue != pinClientCallbackPoweredOffValue) {
      pinClientCallbackValue = newPinClientCallbackValue;
      applicationState = 4;
    }
    enterSleep(WDTO_1S); return;
  }
  
  if (applicationState == 4) {
    boolean newPinClientCallbackValue = digitalRead(pinClientCallback);
    if (newPinClientCallbackValue != pinClientCallbackValue) {
      applicationState = 5; return;
    }
    applicationState = 4;
    enterSleep(WDTO_1S); return;
  }
  
  if (applicationState == 5) {
    digitalWrite(pinClientPower, LOW);
    applicationState = 1;
  }

  enterSleep();
}
