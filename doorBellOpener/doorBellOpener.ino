#include <avr/sleep.h>

#define PIN_KEY1 2
#define PIN_KEY2 3
#define PIN_KEY3 4  /* PB4 pullup too weak against USB D+ shottky - either use external strong pullup or don't use this key */
#define PIN_KEY4 5
#define PIN_LED 1
#define PIN_OPENER 0

#define DISABLE_KEY3

#define KEY_MASK 0x0F
#ifdef DISABLE_KEY3
#define KEY_MASK 0x0B
#endif

/* define the keys that are expected to be pressed as a bitfield e.g. the key state has to equal the values in the array sequentially */
const uint8_t codeKeys[] = {(1 << 0), (1 << 0) | (1 << 1), (1 << 3), (1 << 1), 0};
/* define the sample points in milliseconds. Sampling begings when the first key is read and then works sequentially expecting codeKeys[n] codeSampleIntervals[n] milliseconds after the previous sample. */
/* sampling is initially triggered by detecting any key action. */
/* To not allow guessing individual digits of the code, sampling time is always fixed to the sum of codeSampleIntervals[0..N-1] */
const uint16_t codeSampleIntervals[] = {1000, 2000, 2000, 2000, 2000};

void setActiveHigh(uint8_t pin, boolean isOn)
{
  if(isOn) {
    digitalWrite(pin, HIGH);
  } else {
    digitalWrite(pin, LOW);
  }
}

void setLed(boolean isOn)
{
  setActiveHigh(PIN_LED, isOn);
}

void setOpener(boolean isOn)
{
  setActiveHigh(PIN_OPENER, isOn);
}

void setup() {
  /* configure all IO pins */
  digitalWrite(PIN_OPENER, LOW);
  pinMode(PIN_OPENER, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_KEY1, INPUT);
  pinMode(PIN_KEY2, INPUT);
  pinMode(PIN_KEY3, INPUT);
  pinMode(PIN_KEY4, INPUT);
  digitalWrite(PIN_KEY1, HIGH);
  digitalWrite(PIN_KEY2, HIGH);
  digitalWrite(PIN_KEY3, HIGH);
  digitalWrite(PIN_KEY4, HIGH);

  PRR = (1 << PRUSI) | (1 << PRADC); /* Turn off ADC and USI to save some power */
  set_sleep_mode(SLEEP_MODE_IDLE);  /* set lightest sleep mode which just turns off CPU but nothing else */
}

enum {ST_IDLE, ST_SAMPLING, ST_OPEN};

void loop() {
  static uint8_t state = ST_IDLE;

  unsigned long currentTime = millis();

  static unsigned long nextTimeLed;
  static uint8_t ledState;
  /* LED management */
  {
    
    if(currentTime > nextTimeLed) {
      ledState = !ledState;
      setLed(ledState);
      if(state == ST_IDLE) {
        if(ledState) {
          nextTimeLed += 100;
        } else {
          nextTimeLed += 2900;
        }
      } else if(state == ST_SAMPLING) {
        if(ledState) {
          nextTimeLed += 1000;
        } else {
          nextTimeLed += 1000;
        }
      } else if(state == ST_OPEN) {
        if(ledState) {
          nextTimeLed += 950;
        } else {
          nextTimeLed += 50;
        }
      }
    }
  }
  /* Key input management */
  {
    static unsigned long nextKeySamplePoint;
    static uint8_t codePotentiallyCorrect;
    static uint8_t expectCodeIndex;
    if(currentTime > nextKeySamplePoint) {
      uint8_t keysPressed = digitalRead(PIN_KEY1) | (digitalRead(PIN_KEY2) << 1) | (digitalRead(PIN_KEY4) << 3);
      #ifndef DISABLE_KEY3
      /* Workaround against dumb hardware design... */
      keysPressed |= (digitalRead(PIN_KEY3) << 2);
      #endif
      keysPressed = (~keysPressed) & KEY_MASK;
      switch(state) {
        case ST_IDLE:
          if(keysPressed) {
            state = ST_SAMPLING;
            expectCodeIndex = 0;
            codePotentiallyCorrect = 1;
            /* turn LED on synchronously to sampling intervals */
            nextTimeLed = currentTime;
            ledState = 0;
            nextKeySamplePoint = currentTime + codeSampleIntervals[0];
          } else {
            nextKeySamplePoint += 100;
          }
          break;
        case ST_SAMPLING:
          if(keysPressed != codeKeys[expectCodeIndex]) {
            /* if a wrong key is pressed, we note this but do not do anything differently (except not opening the door in the end).
             *  This ensures that there is no "side channel attack" in the user interface that might allow guessing the correct code
             *  easier than by full brute force.
             */
            codePotentiallyCorrect = 0;
          }
          expectCodeIndex++;
          if(expectCodeIndex >= sizeof(codeKeys)) {
            /* We are through our code table. either open or not, depending on if there was an error in the entering process */
            if(codePotentiallyCorrect) {
              state = ST_OPEN;
              setOpener(1);
            } else {
              state = ST_IDLE;
            }
            /* we will open (or lock the input) for this time interval */
            nextKeySamplePoint = currentTime + 5000;
          } else {
            nextKeySamplePoint += codeSampleIntervals[expectCodeIndex];
          }
          break;          
        case ST_OPEN:
          /* this path is triggered after the preset interval, so there is not much to do here. Next state will be triggered immediately. */
          setOpener(0);
          state = ST_IDLE;
          break;
      }
    }
  }

  /* go to sleep. Digispark will wakeup approximately once every millisecond by the millis systemtimer.
     I did not check other system library implementations. */
  sleep_mode();
}
