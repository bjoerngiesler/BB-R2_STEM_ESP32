#include <Servo.h>
#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <DabbleESP32.h>
#include <DFRobotDFPlayerMini.h>

// This struct describes properties for a servo.
typedef struct {
  unsigned int pin;              // The hardware pin the servo is connected to
  
  unsigned int goal;             // The goal for the servo -- this gets set in loop()
  int current;                   // The current servo position -- this gets updated every loop to move towards goal
  unsigned int min, max;
  unsigned int speed;            // The speed to move the servo's current position to its goal
} ServoState;

typedef enum {
  LEFT  = 0,
  RIGHT = 1,
  DOME  = 2
} ServoIndex;

// Here the servo parameters for our three servos are set. The order is given by ServoIndex above.
ServoState servoStates[3] = {{2, 90, 90, 40, 150, 1},  // LEFT
                             {3, 90, 90, 40, 150, 1},  // RIGHT
                             {4, 90, 90, 40, 150, 1}}; // DOME

static const bool MOVE = true;           // If true, the droid's drive motors will move
static const bool JOYSTICK_MODE = false; // If true, the gamepad can be used in joystick mode (otherwise, gamepad mode).
                                         // Unfortunately Dabble does not let you test for the model programmatically.
                                         // Please note that joystick mode, when used as a regular thumbstick, sometimes "buffers"
                                         // data, meaning if you lift your thumb it doesn't immediately stop. Therefore it is currently
                                         // not recommended to use joystick mode.

// States for the sound buttons. We want to remember what buttons are pressed, so that we do not trigger sounds multiple times.
bool triangleBtn, crossBtn, startBtn, selectBtn;

// Object to talk to the servos
Servo servo;
// Object to talk to the sound player
DFRobotDFPlayerMini dfp;
// Serial connection to talk to the sound player
HardwareSerial dfpSerial(0);

void setup() {
  Serial.begin(2000000);
  Serial.println("BB-R2 ESP32, 2023 Bjoern Giesler");

  Dabble.begin("BB-R2");

  // Initialize sound module
  dfpSerial.begin(9600, SERIAL_8N1, RX, TX);
  if(!dfp.begin(dfpSerial)) {
    Serial.println("DFPlayer could not be initialized!");
  } else {
    dfp.play(1);
  }

  // Initialize servos
  for(auto& s: servoStates) {
    servo.write(s.pin, s.goal);
  }
}

void moveServos() {
  for(auto& s: servoStates) {
    // Move servo current towards servo goal...
    if(s.current < s.goal) {
      s.current += s.speed;
    } else if(s.current > s.goal) {
      s.current -= s.speed;
    }

    if(s.current < s.min) {
      s.current = s.min;
    }
    if(s.current > s.max) {
      s.current = s.max;
    }

    // ...and write to the servo.
    Serial.print(String("Servo ") + s.pin + ": G" + s.goal + " C" + s.current + " ");
    servo.write(s.pin, s.current);
  }
  Serial.println();
}

void loop() {
  Dabble.processInput();

  // Move the dome using the square / circle buttons
  if(GamePad.isSquarePressed()) {
    servoStates[DOME].goal = 0;
  } else if(GamePad.isCirclePressed()) {
    servoStates[DOME].goal = 180;
  } else {
    servoStates[DOME].goal = 90;
  }

  // Move the drive motors using joystick or gamepad
  float x, y;
  if(JOYSTICK_MODE) {
    y = GamePad.getYaxisData();
    x = GamePad.getXaxisData();
  } else {
    if(GamePad.isLeftPressed()) {
      x = -7.0f;
    } else if(GamePad.isRightPressed()) {
      x = 7.0f;
    } else {
      x = 0.0f;
    }

    if(GamePad.isUpPressed()) {
      y = 7.0f;
    } else if(GamePad.isDownPressed()) {
      y = -7.0f;
    } else {
      y = 0.0f;
    }
  }

  float speed = (x*90.0f)/7.0f;
  float curvature = (y*90.0f)/7.0f;
  if(MOVE == true) {
    servoStates[LEFT].goal = 90 + speed + curvature;
    servoStates[RIGHT].goal = 90 + speed - curvature;
  }

  // Make the servos actually move
  moveServos();

  // Trigger sounds
  if(triangleBtn == false && GamePad.isTrianglePressed()) {
    dfp.play(1);
  } else if(crossBtn == false && GamePad.isCrossPressed()) {
    dfp.play(2);
  } else if(startBtn == false && GamePad.isStartPressed()) {
    dfp.play(3);
  } else if(selectBtn == false && GamePad.isSelectPressed()) {
    dfp.next();
  }

  // Remember what buttons are pressed, so that we do not trigger sounds multiple times
  triangleBtn = GamePad.isTrianglePressed();
  crossBtn = GamePad.isCrossPressed();
  startBtn = GamePad.isStartPressed();
  selectBtn = GamePad.isSelectPressed();

  delay(10);
}
