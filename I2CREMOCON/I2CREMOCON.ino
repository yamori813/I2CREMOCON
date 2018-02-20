
/* I2C IR remocon code for ATTiny85
 * IR signal out put pin 6
 * 2018 Hiroki Mori
 */

/**
 * Pin notes by Suovula, see also http://hlt.media.mit.edu/?p=1229
 *
 * DIP and SOIC have same pinout, however the SOIC chips are much cheaper, especially if you buy more than 5 at a time
 * For nice breakout boards see https://github.com/rambo/attiny_boards
 *
 * Basically the arduino pin numbers map directly to the PORTB bit numbers.
 *
 * // I2C
 * arduino pin 0 = not(OC1A) = PORTB <- _BV(0) = SOIC pin 5 (I2C SDA, PWM)
 * arduino pin 2 =           = PORTB <- _BV(2) = SOIC pin 7 (I2C SCL, Analog 1)
 * // Timer1 -> PWM
 * arduino pin 1 =     OC1A  = PORTB <- _BV(1) = SOIC pin 6 (PWM)
 * arduino pin 3 = not(OC1B) = PORTB <- _BV(3) = SOIC pin 2 (Analog 3)
 * arduino pin 4 =     OC1B  = PORTB <- _BV(4) = SOIC pin 3 (Analog 2)
 */

#include <avr/sleep.h>

#define I2C_SLAVE_ADDRESS 0x4 // the 7-bit address (remember to change this when adapting this example)

// Get this from https://github.com/rambo/TinyWire
#include <TinyWireS.h>

#define TWI_RX_BUFFER_SIZE  16

// effective plus 1 value
#define IR_REPEAT 3

#define AEHA  1
#define NEC   2
#define SONY  3

int count;
int nextedge;
int curpos;

int irtype;
int irlen;
int repeat;
int gosleep;

volatile uint8_t ir_data[TWI_RX_BUFFER_SIZE];

// TODO: Either update this to use something smarter for timing or remove it alltogether
void blinkn(uint8_t blinks)
{
  digitalWrite(3, HIGH);
  while(blinks--)
  {
    digitalWrite(3, LOW);
    tws_delay(50);
    digitalWrite(3, HIGH);
    tws_delay(100);
  }
}

void requestEvent()
{  
  TinyWireS.send(0);
}

int getbit(int pos)
{
  int byte;
  int bit;

  byte = pos / 8;
  bit = pos % 8;

  return (ir_data[byte+1] >> (7 - bit)

  ) & 0x01;
}

int sony_calcnext()
{
  int result;
  int val;
  if(curpos == 0)
    result = 4;
  else if(curpos == (irlen + 1) * 2 - 1)
    result = 50;
  else {
    val = getbit((curpos - 1) / 2);
    if((curpos % 2) == 0 && val) {
      result = 2;
    } 
    else {
      result = 1;
    }
  }
  ++curpos;
  return result;
}

int nec_calcnext()
{
  int result;
  int val;
  if(repeat == 0){
    if(curpos == 0)
      result = 16;
    else if(curpos == 1)
      result = 8;
    else if(curpos == (irlen + 2) * 2 - 1)
      result = 192 - count;
    else {
      val = getbit((curpos - 2) / 2);
      if((curpos % 2) == 1 && val) {
        result = 3;
      } 
      else {
        result = 1;
      }
    }
  } 
  else {
    if(curpos == 0)
      result = 16;
    else if(curpos == 1)
      result = 4;
    else if(curpos == 2)
      result = 1;
    else if(curpos == 3)
      result = 192 - count;
  }
  ++curpos;
  return result;
}

int aeha_calcnext()
{
  int result;
  int val;
  if(curpos == 0)
    result = 8;
  else if(curpos == 1)
    result = 4;
  else if(curpos == (irlen + 2) * 2 - 1)
    result = 100;
  else {
    val = getbit((curpos - 2) / 2);
    if((curpos % 2) == 1 && val) {
      result = 3;
    } 
    else {
      result = 1;
    }
  }
  ++curpos;
  return result;
}

ISR(TIMER0_COMPA_vect)
{
  ++count;
  if(irtype == NEC) {
    if (count == nextedge) {
      if((repeat == 0 && curpos / 2 == irlen + 2) ||
        (repeat != 0 && curpos == 4)) {
        curpos = 0;
        count = 1;
        nextedge = 1;
        ++repeat;
      }
      if(curpos % 2 == 0) {
        // rising edge
        TCCR1 |= (1 << COM1A0);
      }
      else {
        // falling edge
        TCCR1 &= ~(1 << COM1A0);
      } 
      nextedge += nec_calcnext();
    }
    // stop
    if(repeat == IR_REPEAT) {
      TCCR0B = 0x00;
      TCCR1 &= ~(1 << COM1A0);
      gosleep = 1;
    } 
  } 
  else if(irtype == AEHA) {
    if (count == nextedge) {
      if(curpos / 2 == irlen + 2) {
        curpos = 0;
        count = 1;
        nextedge = 1;
        ++repeat;
      }
      if(curpos % 2 == 0) {
        // rising edge
        TCCR1 |= (1 << COM1A0);
      }
      else {
        // falling edge
        TCCR1 &= ~(1 << COM1A0);
      } 
      nextedge += aeha_calcnext();
    }
    // stop
    if(curpos / 2 == irlen + 2) {
      if(repeat == IR_REPEAT) {
        TCCR0B = 0x00;
        TCCR1 &= ~(1 << COM1A0);
        gosleep = 1;
      } 
    }
  }
  else if(irtype == SONY) {
    if (count == nextedge) {
      if(curpos / 2 == irlen + 1) {
        curpos = 0;
        count = 1;
        nextedge = 1;
        ++repeat;
      }
      if(curpos % 2 == 0) {
        // rising edge
        TCCR1 |= (1 << COM1A0);
      }
      else {
        // falling edge
        TCCR1 &= ~(1 << COM1A0);
      } 
      nextedge += sony_calcnext();
    }
    // stop
    if(curpos / 2 == irlen + 1) {
      if(repeat == IR_REPEAT) {
        TCCR0B = 0x00;
        TCCR1 &= ~(1 << COM1A0);
        gosleep = 1;
      } 
    }
  }
}

/**
 * The I2C data received -handler
 *
 * This needs to complete before the next incoming transaction (start, data, restart/stop) on the bus does
 * so be quick, set flags for long running tasks to be called from the mainloop instead of running them directly,
 */
void receiveEvent(uint8_t howMany)
{
  int i;

  if (howMany < 1)
  {
    // Sanity-check
    return;
  }
  if (howMany > TWI_RX_BUFFER_SIZE)
  {
    // Also insane number
    return;
  }

  i = 0;
  while(howMany--)
  {
    ir_data[i] = TinyWireS.receive();
    ++i;
  }
  irtype = ir_data[0] & 0x0f;
  irlen = (ir_data[0] >> 4) * 4;

  count = 0;
  nextedge = 1;
  curpos = 0;
  repeat = 0;

  // http://www.ernstc.dk/arduino/38khz_timer.htm
  // set up 38K carrier by timer1 but not output
  pinMode(1, OUTPUT);
  TCNT1 = 0;
  TCCR1 = 0;
  GTCCR |= (1 << PSR1); //section 13.3.2 reset the prescaler
  TCCR1 |= (1 << CTC1); // section 12.3.1 CTC mode
  //  TCCR1 |= (1 << COM1A0); //togle pin PB1 table 12-4
  TCCR1 |= (1 << CS10); //prescaler 1 table 12-5
  OCR1C = 104;
  OCR1A = 104;

  // set up timer0
  TCCR0A = 1 << 1;  // CTC
  TCCR0B = 0x03;   // 64 div
  TIMSK = (1 << OCIE0A);
  if(irtype == SONY)
    OCR0A = 0x4c;
  else if(irtype == NEC)
    OCR0A = 0x48;
  else if(irtype == AEHA)
    OCR0A = 0x38;

  sei();
}


void setup()
{
  // TODO: Tri-state this and wait for input voltage to stabilize 
  pinMode(3, OUTPUT); // OC1B-, Arduino pin 3, ADC
  digitalWrite(3, LOW); // Note that this makes the led turn on, it's wire this way to allow for the voltage sensing above.

  pinMode(1, OUTPUT); // OC1A, also The only HW-PWM -pin supported by the tiny core analogWrite

  /**
   * Reminder: taking care of pull-ups is the masters job
   */

  TinyWireS.begin(I2C_SLAVE_ADDRESS);
  TinyWireS.onReceive(receiveEvent);
  TinyWireS.onRequest(requestEvent);

  // Whatever other setup routines ?

  digitalWrite(3, HIGH);

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  gosleep = 1;
}

void loop()
{

  /**
   * This is the only way we can detect stop condition (http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&p=984716&sid=82e9dc7299a8243b86cf7969dd41b5b5#984716)
   * it needs to be called in a very tight loop in order not to miss any (REMINDER: Do *not* use delay() anywhere, use tws_delay() instead).
   * It will call the function registered via TinyWireS.onReceive(); if there is data in the buffer on stop.
   */
  if(gosleep)
    sleep_mode();
  gosleep = 0;
  TinyWireS_stop_check();
}







