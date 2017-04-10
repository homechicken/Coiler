/* Arduino code for Coiling Machine
By A Jacob Cord
*/

#include <Wire.h> // for the I2C protocol

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DEBUG

// pin definitions
#define LEFT_HOME_SWITCH 2
#define RIGHT_HOME_SWITCH 4
#define COIL_DIR 8
#define COIL_STEP 9
#define CARRIAGE_DIR 10
#define CARRIAGE_STEP 11
#define MOTOR_ENABLE 12

// coil motor directions, reverse if necessary
#define COIL_COIL 1
#define COIL_UNCOIL 0

// carriage directions (reverse if carriage stepper rotates differently)
#define CARRIAGE_RIGHT 1
#define CARRIAGE_LEFT 0

// OLED definitions
// what does this do, why is it defined if it's not connected to a pin??
#define OLED_RESET 4

// Check to make sure the library is correct
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h, define SSD_1306_128_64!");
#endif

Adafruit_SSD1306 display(OLED_RESET);

// Menus
// 0-greeting/motor test, 2-switch test, 3 22/900 coil
unsigned char currentMenu=0;

// stepper pulse control in uSeconds. 20/250 = 360 degrees in 1.728 seconds
#define PULSE_WIDTH 20
#define PULSE_SPACE 250

// Coil:Carriage move ratio. 22AWG should be 60:1
// *** cannot be greater than 255 without modifying the makeCoil loop data type!

/* Remington Magnet Wire definitions and ratios at 1/32 microstep
  18 AWG 1.087mm, 0.0428" diameter. 36.79:1
  20 AWG 0.871mm, 0.0343" diameter. 45.92:1
  22 AWG 0.668mm, 0.0263" diameter. 59.88:1
  23 AWG 0.599mm, 0.0235" diameter. 66.77:1
  24 AWG 0.561mm, 0.0221" diameter. 71.30:1
  26 AWG 0.426mm, 0.0168" diameter. 86.58:1
  30 AWG 0.274mm, 0.0108" diameter. 145.98:1
  32 AWG 0.236mm, 0.0093" diameter. 169.49:1
  34 AWG 0.175mm, 0.0069" diameter. 228.57:1
  36 AWG 0.139mm, 0.0055" diameter. 287.76:1 // update char!
  38 AWG 0.111mm, 0.0044" diameter. 360.36:1 // update char!
  40 AWG 0.086mm, 0.0034" diameter. 465.11:1 // update char!
*/

// should be 200 to 6400 depending on microstep settings
#define STEPPER_STEPS 6400

//
// Initialization
//
void setup() {
  Serial.begin(9600);
  
#ifdef DEBUG
  Serial.println(F("Setting Arduino pin modes.."));
#endif

  pinMode(LEFT_HOME_SWITCH, INPUT_PULLUP);
  pinMode(RIGHT_HOME_SWITCH, INPUT_PULLUP);

  pinMode(COIL_DIR, OUTPUT);
  pinMode(COIL_STEP, OUTPUT);
  pinMode(CARRIAGE_DIR, OUTPUT);
  pinMode(CARRIAGE_STEP, OUTPUT);
  pinMode(MOTOR_ENABLE, OUTPUT);

#ifdef DEBUG
  Serial.println(F("Initializing display.."));
#endif

  display.begin(SSD1306_SWITCHCAPVCC, 0X3C);
  display.display(); // show splashscreen
  delay(1000);
  
  menuGreeting();
}

void menuGreeting() {
#ifdef DEBUG
  Serial.println(F("Displaying Greeting menu"));
#endif
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(F("Coilatron 9000"));
  display.setCursor(0, 16);
  display.println(F("Left: Menus"));
  display.println(F("\nRight: Test motors"));
  display.display();
}

void menuSwitches() {
#ifdef DEBUG
  Serial.println(F("Displaying Switch menu"));
#endif
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Coilatron 9000"));
  display.setCursor(0, 16);
  display.println(F("Left: Menus"));
  display.println(F("\nRight: Switch test"));
  display.display();
}

void menuCoil() {
#ifdef DEBUG
  Serial.println(F("Displaying coil menu"));
#endif
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Coilatron 9000"));
  display.setCursor(0, 16);
  display.println(F("Left: Menus"));
  display.println(F("\nRight: 22-900 coil"));
  display.display();
}

//
// main loop
//
void loop() {
  if(!digitalRead(RIGHT_HOME_SWITCH)) {
    switch(currentMenu) {
      case 0: testMotors(); break;
      case 1: testSwitches(); break;
      case 2: coil22900(); break;
      default: // do nothing
    }
  } else if(!digitalRead(LEFT_HOME_SWITCH)) {
    if(++currentMenu > 2) {
      currentMenu = 0;
    }

    switch(currentMenu) {
      case 0: menuGreeting(); break;
      case 1: menuSwitches(); break;
      case 2: menuCoil(); break;
      default: menuGreeting();
    }
  }

  // impose short delay between loops
  delay(100);
}

void testMotors() {
#ifdef DEBUG
  Serial.println(F("Start motor test"));
#endif
  currentDirection = CARRIAGE_RIGHT;

  digitalWrite(MOTOR_ENABLE, HIGH);
  digitalWrite(COIL_DIR, COIL_COIL);
   
  for(char i=0; i<2; ++i) {  
    for(int j=0; j<6400; ++j) {
      digitalWrite(COIL_STEP, HIGH);
      delayMicroseconds(PULSE_WIDTH);
      digitalWrite(COIL_STEP, LOW);
      delayMicroseconds(PULSE_SPACE);
    }
    digitalWrite(COIL_DIR, COIL_UNCOIL);
  }

  digitalWrite(CARRIAGE_DIR, currentDirection); 
  
  for(char i=0; i<2; ++i) {
    for(int j=0; j<6400; ++j) {
      digitalWrite(CARRIAGE_STEP, HIGH);
      delayMicroseconds(PULSE_WIDTH);
      digitalWrite(CARRIAGE_STEP, LOW);
      delayMicroseconds(PULSE_SPACE);

      checkLimitSwitches();
    }
  }

  digitalWrite(COIL_DIR, LOW);
  digitalWrite(CARRIAGE_DIR, LOW);
  digitalWrite(MOTOR_ENABLE, LOW);
  
#ifdef DEBUG
  Serial.println(F("End motor test"));
#endif
}

void testSwitches() {
#ifdef DEBUG
  Serial.println(F("Starting limit switch test"));
#endif

  digitalWrite(MOTOR_ENABLE, HIGH);
  char currentDirection = CARRIAGE_RIGHT;
  
  digitalWrite(CARRIAGE_DIR, currentDirection);

  for(int i=0; i<10; ++i) {
    for(int j=0; j<6400; ++j) {
      digitalWrite(CARRIAGE_STEP, HIGH);
      delayMicroseconds(PULSE_WIDTH);
      digitalWrite(CARRIAGE_STEP, LOW);
      delayMicroseconds(PULSE_SPACE);

      checkLimitSwitches();
    } // step loop
  } // count loop
  
  digitalWrite(CARRIAGE_DIR, LOW);
  digitalWrite(MOTOR_ENABLE, LOW);
}

void checkLimitSwitches() {
  if(currentDirection == CARRIAGE_RIGHT && digitalRead(RIGHT_HOME_SWITCH) == 0) {
#ifdef DEBUG
  Serial.println(F("Right limit hit, changing direction!"));
#endif
    currentDirection = CARRIAGE_LEFT;
    digitalWrite(CARRIAGE_DIR, currentDirection);
  } else if(currentDirection == CARRIAGE_LEFT && digitalRead(LEFT_HOME_SWITCH) == 0) {
#ifdef DEBUG
  Serial.println(F("Left limit hit, changing direction!"));
#endif
    currentDirection = CARRIAGE_RIGHT;
    digitalWrite(CARRIAGE_DIR, currentDirection);
  }
}

void updateDisplay(unsigned int coils, unsigned int total) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.println(F("Coiling..."));
  display.setCursor(0, 20);
  display.setTextSize(3);
  display.print(coils);
  display.setTextSize(2);
  display.setCursor(50, 35);
  display.print(F("/"));
  display.println(total);
  display.display();
}

void coil22900() {
  char currentDirection = CARRIAGE_RIGHT;

  unsigned char ratio = 0;
  const unsigned char moveRatio = 60;

  const unsigned int totalCoils = 900;

  updateDisplay(0, totalCoils);

  digitalWrite(MOTOR_ENABLE, HIGH);
  
  for(unsigned int coil=0; coil<totalCoils; ++coil) {
    for(unsigned int i=0; i<STEPPER_STEPS; ++i) {
      if(ratio++ >= moveRatio) {
        digitalWrite(CARRIAGE_STEP, HIGH);
        delayMicroseconds(PULSE_WIDTH);
        digitalWrite(CARRIAGE_STEP, LOW);
        
        ratio = 0;

        checkLimitSwitches();        
      } // moveRatio exceeded
        
      digitalWrite(COIL_STEP, HIGH);
      delayMicroseconds(PULSE_WIDTH);
      digitalWrite(COIL_STEP, LOW);
      delayMicroseconds(PULSE_SPACE);
    } // STEPPER_STEPS, a single rotation
        
      updateDisplay(coil, totalCoils);
  } // total coils

  digitalWrite(MOTOR_ENABLE, LOW);
}

