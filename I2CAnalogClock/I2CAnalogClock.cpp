#include "I2CAnalogClock.h"


volatile uint16_t position; // This is the position that we believe the clock is in.
volatile uint16_t adjustment;   // This is the adjustment to be made.
volatile uint16_t tp_duration;
volatile uint16_t tp_count;
volatile uint16_t ap_duration;
volatile uint16_t ap_count;
volatile uint16_t ap_delay;     // delay in ms between ticks during adjustment

volatile uint8_t control;       // This is our control "register".
volatile uint8_t status;        // status register

volatile uint8_t command;       // This is which "register" to be read/written.

volatile unsigned int receives;
volatile unsigned int requests;
volatile unsigned int errors;

// i2c receive handler
void i2creceive(int size)
{
  ++receives;
  command = WireRead();
  --size;
  // check for a write command
  if (size > 0)
  {
    switch (command)
    {
    case CMD_POSITION:
      position = WireRead() | WireRead() << 8;
      break;
    case CMD_ADJUSTMENT:
      adjustment = WireRead() | WireRead() << 8;
      break;
    case CMD_TP_DURATION:
      tp_duration = WireRead() | WireRead() << 8;
      break;
    case CMD_TP_COUNT:
      tp_count = WireRead() | WireRead() << 8;
      break;
    case CMD_AP_DURATION:
      ap_duration = WireRead() | WireRead() << 8;
      break;
    case CMD_AP_COUNT:
      ap_count = WireRead() | WireRead() << 8;
      break;
    case CMD_AP_DELAY:
      ap_delay = WireRead() | WireRead() << 8;
      break;
    case CMD_CONTROL:
      control = WireRead();
      break;
    case CMD_STATUS:
      status = WireRead();
      break;
    default:
      ++errors;
    }
  }
}

// i2c request handler
void i2crequest()
{
  ++requests;
  uint16_t value;
  switch (command)
  {
  case CMD_POSITION:
    value = position;
    WireWrite(value & 0xff);
    WireWrite(value >> 8);
    break;
  case CMD_ADJUSTMENT:
    value = adjustment;
    WireWrite(value & 0xff);
    WireWrite(value >> 8);
    break;
  case CMD_TP_DURATION:
    value = tp_duration;
    WireWrite(value & 0xff);
    WireWrite(value >> 8);
    break;
  case CMD_TP_COUNT:
    value = tp_count;
    WireWrite(value & 0xff);
    WireWrite(value >> 8);
    break;
  case CMD_AP_DURATION:
    value = ap_duration;
    WireWrite(value & 0xff);
    WireWrite(value >> 8);
    break;
  case CMD_AP_COUNT:
    value = ap_count;
    WireWrite(value & 0xff);
    WireWrite(value >> 8);
    break;
  case CMD_AP_DELAY:
   	value = ap_delay;
    WireWrite(value & 0xff);
    WireWrite(value >> 8);
    break;
  case CMD_CONTROL:
	WireWrite(control);
    break;
  case CMD_STATUS:
    WireWrite(status);
    break;
  default:
    ++errors;
  }
}

void tickDelay(uint16_t duration, uint16_t count)
{
  if (count == 0)
  {
    count = 1;
  }

  for (uint16_t i = 0; i < count; ++i)
  {
    delayMicroseconds(duration);
  }
}

//
//  Advance the clock by one second.
//

void advanceClock(uint16_t duration, uint16_t count)
{
  position += 1;
  if (position >= MAX_SECONDS)
  {
    position = 0;
  }
  // placeholder for advance clock - for now just toggle LED
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));

  // toggle the pins.
  digitalWrite(A_PIN, !digitalRead(A_PIN));
  tickDelay(duration, count);
  digitalWrite(B_PIN, !digitalRead(B_PIN));
  /*
   if (isTick())
   {
   // tick
   digitalWrite (A_PIN, TICK_ON);
   tickDelay(duration, count);
   digitalWrite (A_PIN, TICK_OFF);
   }
   else
   {
   // tock
   digitalWrite (B_PIN, TICK_ON);
   tickDelay(duration, count);
   digitalWrite (B_PIN, TICK_OFF);
   }
   toggleTick();
   */
}

//
// ISR for 1hz interrupt
//
void tick()
{
  if (isEnabled())
  {

    if (adjustment > 0)
    {
      ++adjustment;
    }
    else
    {
      advanceClock(tp_duration, tp_count);
    }
  }
}

void setup()
{
#ifdef DEBUG_I2CAC
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Startup!");
#endif

  tp_duration = DEFAULT_TP_DURATION;
  tp_count = DEFAULT_TP_COUNT;
  ap_duration = DEFAULT_AP_DURATION;
  ap_count = DEFAULT_AP_COUNT;
  ap_delay = DEFAULT_AP_DELAY;

  WireBegin(I2C_ADDRESS);
  WireOnReceive(&i2creceive);
  WireOnRequest(&i2crequest);

  digitalWrite(B_PIN, TICK_OFF);
  digitalWrite(A_PIN, TICK_OFF);
  pinMode(A_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);
  pinMode(INT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), &tick, FALLING);
}

#ifdef DEBUG_I2CAC
unsigned long last_print;
uint16_t last_pos = -1;
#endif

void loop()
{
  if (adjustment > 0)
  {
    advanceClock(ap_duration, ap_count);
    delay(ap_delay);
    --adjustment;
  }

#ifdef DEBUG_I2CAC
  unsigned long now = millis();

  if (last_pos != position && (now - last_print) > 60000)
  {
    last_pos = position;
    last_print = now;
    Serial.print("position:");
    Serial.print(position);
    Serial.print(" adjustment:");
    Serial.print(adjustment);
    Serial.print(" control:");
    Serial.print(control);
//    Serial.print(" tp_duraation:");
//    Serial.print(tp_duration);
//    Serial.print(" tp_count:");
//    Serial.print(tp_count);
//    Serial.print(" ap_duraation:");
//    Serial.print(ap_duration);
//    Serial.print(" ap_count:");
//    Serial.print(ap_count);
//    Serial.print(" ap_delay:");
//    Serial.print(ap_delay);
//    Serial.print(" receives: ");
//    Serial.print(receives);
//    Serial.print(" requests: ");
//    Serial.print(requests);
//    Serial.print(" errors: ");
//    Serial.print(errors);
    Serial.print(" seconds: ");
    Serial.print(position % 60);
    Serial.println();
  }
#endif
  delay(1);
}
