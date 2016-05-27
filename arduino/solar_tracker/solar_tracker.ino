// ATMEL ATTINY84 / ARDUINO
//
//                           +-\/-+
//                     VCC  1|    |14  GND
//             (D 10)  PB0  2|    |13  AREF (D  0)
//             (D  9)  PB1  3|    |12  PA1  (D  1)
//                     PB3  4|    |11  PA2  (D  2)
//  PWM  INT0  (D  8)  PB2  5|    |10  PA3  (D  3)
//  PWM        (D  7)  PA7  6|    |9   PA4  (D  4)
//  PWM        (D  6)  PA6  7|    |8   PA5  (D  5)        PWM
//                           +----+

//////////////////
// SERIAL DEBUG //
//////////////////

// uncomment '#define DEBUG' to enable serial debug
//#define DEBUG

#ifdef DEBUG
  #include <SoftwareSerial.h>
  SoftwareSerial mySerial(4, 5);
  
  #define DEBUG_BEGIN(x) mySerial.begin(x)
  #define DEBUG_PRINT(x)     mySerial.print (x)
  #define DEBUG_PRINTLN(x)  mySerial.println (x)
  #define DEBUG_DELAY(x) delay(x)
  #else
  #define DEBUG_BEGIN(x)
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_DELAY(x)
#endif

////////////////////
// WATCHDOG RESET //
////////////////////

#include <avr/sleep.h>
#include <avr/wdt.h>

ISR(WDT_vect)
{
  wdt_disable();  // disable watchdog
}

//////////////////////
// GLOBAL VARIABLES //
//////////////////////

#define LDR_LEFT A2
#define LDR_RIGHT A1

#define MOTOR_OPEN 10
#define MOTOR_CLOSE 9

#define SWITCH_OPEN 8
#define SWITCH_CLOSE 7

#define LEFT_FUDGE 0 // LeftSensor adjustment e.g. '0.11' adds 11%
#define TOTAL_FUDGE 2 // Add some slop to the motor centered figure.
#define NIGHT_VALUE 35

#define SLEEP_TIME 15 // 15 = 2min (8s * 15)

enum State {
  MAIN,
  GO_HOME,
  TRACK,
  SLEEP
};

State currentState = MAIN;

int ldrLeft, ldrRight;

////////////
// SETUP  //
////////////

void setup() {

  DEBUG_BEGIN(9600);

  pinMode(LDR_LEFT, INPUT);
  pinMode(LDR_RIGHT, INPUT);

  pinMode(MOTOR_OPEN, OUTPUT);
  pinMode(MOTOR_CLOSE, OUTPUT);

  pinMode(SWITCH_OPEN, INPUT);
  pinMode(SWITCH_CLOSE, INPUT);

}

///////////////
// MAIN LOOP //
///////////////

void loop() {

  DEBUG_DELAY(1000);

  switch ( currentState ) {

    /**
       [MAIN]
       Determines which state to enter based on ambient light
    */
    case MAIN:
      DEBUG_PRINTLN("State: MAIN");

      if ( isDaytime() ) {
        currentState = TRACK;
      } else if ( !isDaytime() ) {
        currentState = GO_HOME;
      }
      break;

    /**
       [TRACK]
       Looks for the sun and controls the motor allowing TOTAL_FUDGE slack
       in readings. Stops motor on contact with corresponding end stop.
    */
    case TRACK:
      DEBUG_PRINTLN("State: TRACK");

      readLDRs();

      if (ldrRight >= ldrLeft - TOTAL_FUDGE && ldrRight <= ldrLeft + TOTAL_FUDGE) {
        DEBUG_PRINTLN("Sweet Spot");
        stopMotor();
        currentState = MAIN;
        break;
      }
      if ( ldrRight > ldrLeft) {
        if ( !digitalRead(SWITCH_OPEN) ) {
          DEBUG_PRINTLN("Open End Stop");
          stopMotor();
          currentState = MAIN;
          break;
        }
        goRight();
      }
      else if ( ldrRight < ldrLeft ) {

        if ( !digitalRead(SWITCH_CLOSE) ) {
          DEBUG_PRINTLN("Closed End Stop");
          stopMotor();
          currentState = MAIN;
          break;
        }
        goLeft();
      }

      currentState = MAIN;
      break;

    /**
       [GO_HOME]
       Returns the panel to western configuration until
       it hits the end stop and goes to sleep.
    */
    case GO_HOME:
      DEBUG_PRINTLN("State: GO_HOME");
      if (!digitalRead(SWITCH_CLOSE)) {
        stopMotor();
        currentState = SLEEP;
        break;
      }
      goLeft();
      currentState = MAIN;
      break;

    /**
       [SLEEP]
        Power down for 8 seconds * SLEEP_TIME.
        http://forum.arduino.cc/index.php?topic=173850.msg1291338#msg1291338
    */
    case SLEEP:
      DEBUG_PRINTLN("State: SLEEP");

      for ( int i = 0; i < SLEEP_TIME; i++ ) {
        MCUSR = 0;                          // reset various flags
        WDTCSR |= 0b00011000;               // see docs, set WDCE, WDE
        WDTCSR =  0b01000000 | 0b100001;    // set WDIE, and 8s delay

        wdt_reset();
        set_sleep_mode (SLEEP_MODE_PWR_DOWN);
        sleep_mode();            // now goes to Sleep and waits for the interrupt
      }

      DEBUG_PRINTLN("Waking!");
      currentState = MAIN;
      break;

    default:
      DEBUG_PRINTLN("CODE FAILURE");
      stopMotor();
      currentState = MAIN;
      break;
  }
}

///////////////
// UTILITIES //
///////////////

void readLDRs () {
  ldrLeft = analogRead(LDR_LEFT);
  ldrRight = analogRead(LDR_RIGHT);

  float ldrLeftFudge = ldrLeft * LEFT_FUDGE;  // Calculate LEFT_FUDGE%
  ldrLeft = ldrLeft + (int)ldrLeftFudge;

  DEBUG_PRINT("LDR L : ");
  DEBUG_PRINT(ldrLeft);
  DEBUG_PRINT(" LDR L FUDGE: ");
  DEBUG_PRINT(ldrLeftFudge);
  DEBUG_PRINT(" LDR R: ");
  DEBUG_PRINTLN(ldrRight);

}

bool isDaytime()
{
  readLDRs();

  if ( ldrLeft > NIGHT_VALUE || ldrRight > NIGHT_VALUE ) {
    return true;
  }

  return false;
}

////////////////////
// MOTOR CONTROLS //
////////////////////

void goRight()
{
  DEBUG_PRINTLN("Going Right");
  digitalWrite(MOTOR_OPEN, HIGH);
  digitalWrite(MOTOR_CLOSE, LOW);
}

void goLeft()
{
  DEBUG_PRINTLN("Going Left");
  digitalWrite(MOTOR_OPEN, LOW);
  digitalWrite(MOTOR_CLOSE, HIGH);
}

void stopMotor()
{
  DEBUG_PRINTLN("Stopping");
  digitalWrite(MOTOR_OPEN, LOW);
  digitalWrite(MOTOR_CLOSE, LOW);
}
