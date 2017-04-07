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

// carriage directions (reverse if carriage stepper rotates differently)
#define CARRIAGE_RIGHT 0
#define CARRIAGE_LEFT 1

// OLED definitions
// what does this do, why is it defined if it's not connected to a pin??
#define OLED_RESET 4

// Check to make sure the library is correct
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h, define SSD_1306_128_64!");
#endif

Adafruit_SSD1306 display(OLED_RESET);

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
#define MOVE_RATIO 60

// MAKING THE COILS
#define COIL_NUMBER 900

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
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(F("Jacob's Coiler"));
  display.setCursor(0, 16);
  display.println(F("Left: Display Check"));
  display.println(F("\nRight: Carriage Check"));
  display.display();
}

//
// main loop
//
void loop() {
  if(!digitalRead(RIGHT_HOME_SWITCH)) {
    testMotor();
  } else if(!digitalRead(LEFT_HOME_SWITCH)) {
    testDisplay();
  }

  delay(100);
}

void testMotor() {
#ifdef DEBUG
  Serial.println(F("Start motor test"));
#endif

  for(char i=0; i<2; ++i) {  
    for(int j=0; j<6400; ++j) {
      digitalWrite(COIL_STEP, HIGH);
      delayMicroseconds(PULSE_WIDTH);
      digitalWrite(COIL_STEP, LOW);
      delayMicroseconds(PULSE_SPACE);
    }
    digitalWrite(COIL_DIR, HIGH);
  }

  digitalWrite(COIL_DIR, LOW);

  for(char i=0; i<2; ++i) {
    for(int j=0; j<6400; ++j) {
      digitalWrite(CARRIAGE_STEP, HIGH);
      delayMicroseconds(PULSE_WIDTH);
      digitalWrite(CARRIAGE_STEP, LOW);
      delayMicroseconds(PULSE_SPACE);
    }
    digitalWrite(CARRIAGE_DIR, HIGH);
  }
  digitalWrite(CARRIAGE_DIR, LOW);i
  
#ifdef DEBUG
  Serial.println(F("End motor test"));
#endif
}

void testDisplay() {
  unsigned int coil = 0;

  updateDisplay(coil);

  while(digitalRead(RIGHT_HOME_SWITCH) == 1) {
    if(digitalRead(LEFT_HOME_SWITCH) == 0) {
      updateDisplay(coil++);
    }
    delay(100);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("Jacob's Coiler"));
  display.setCursor(0, 16);
  display.println(F("Left: Display Check"));
  display.println(F("\nRight: Carriage Check"));
  display.display();
}

void updateDisplay(unsigned int coils) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.println(F("Coiling..."));
  display.setCursor(0, 20);
  display.setTextSize(3);
  display.print(coils);
  display.setTextSize(2);
  display.setCursor(50, 35);
  display.println(F("/900"));
  display.display();
}

void makeCoil() {
  char currentDirection = CARRIAGE_RIGHT;
  unsigned char ratio = 0;
  
  updateDisplay(0);
  
  for(unsigned int coil=0; coil<COIL_NUMBER; ++coil) {
    for(unsigned int i=0; i<STEPPER_STEPS; ++i) {
      if(ratio++ >= MOVE_RATIO) {
        digitalWrite(CARRIAGE_STEP, HIGH);
        delayMicroseconds(PULSE_WIDTH);
        digitalWrite(CARRIAGE_STEP, LOW);
        
        ratio = 0;
        
        // check for direction change
        if(currentDirection == CARRIAGE_RIGHT && !digitalRead(RIGHT_HOME_SWITCH)) {
          digitalWrite(CARRIAGE_DIR, CARRIAGE_LEFT);
          currentDirection = CARRIAGE_LEFT;
        } else if(currentDirection == CARRIAGE_LEFT && !digitalRead(LEFT_HOME_SWITCH)) {
          digitalWrite(CARRIAGE_DIR, CARRIAGE_RIGHT);
          currentDirection = CARRIAGE_RIGHT;
        }
      } // MOVE_RATIO
        
        digitalWrite(COIL_STEP, HIGH);
        delayMicroseconds(PULSE_WIDTH);
        digitalWrite(COIL_STEP, LOW);
        delayMicroseconds(PULSE_SPACE);
    } // STEPPER_STEPS, a single rotation
        
        updateDisplay(coil);
    } // COIL_NUMBER
}