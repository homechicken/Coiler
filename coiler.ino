/* Arduino code for Coiling Machine
Version 0.3

By A Jacob Cord

Version History
0.1 Limit switch detection and console feedback (28 Mar 2017)
0.2 Stepper library added (30 Mar 2017)
0.3 Display and test functions added (31 Mar 2017);

The OLED should be connected to the I2C bus, SDA to pin A4 and SCL to pin A5. VCC to 3.3 or 5v. Its address is 0x3C
*/

#include <Wire.h> // for the I2C protocol

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//#include <Arduino.h> // why?
#include "BasicStepperDriver.h"

#define DEBUG

// pin definitions
#define LEFT_HOME_SWITCH 2
#define RIGHT_HOME_SWITCH 4
#define COIL_DIR 8
#define COIL_STEP 9
#define CARRIAGE_DIR 10
#define CARRIAGE_STEP 11

// carriage directions
#define CARRIAGE_RIGHT 1
#define CARRIAGE_LEFT -1

// OLED definitions
// what does this do, why is it defined if it's not connected to a pin??
#define OLED_RESET 4
int displayTestTextSize = 0;

// Stepper motor defines
#define MOTOR_STEPS 200
#define MICROSTEPS 1 // 1=full step, 2=half step, etc
#define COIL_RPM 120
#define CARRIAGE_RPM 120

/*
  Carriage moves 0.111(repeating) millimeters per degree of rotation or 0.2mm per step
  Belt pitch 2mm (GT2 belt)
  Pulley teeth 20
  One rotation: 40mm
  40/360 = 0.111111
  40/200 = 0.2
  Using degrees will yield more accuracy most of the time, since 360>200
  * Using degrees very likely requires microstepping!
*/

// Remington Magnet Wire definitions
/*
  18 AWG 1.087mm, 0.0428" diameter. 5.4 steps, 9.79 degrees
  20 AWG 0.871mm, 0.0343" diameter. 4.3 steps, 7.84 degrees
  22 AWG 0.668mm, 0.0263" diameter. 3.34 steps, 6.01 degrees
  24 AWG 0.561mm, 0.0221" diameter. 2.8 steps, 5.05 degrees
  26 AWG 0.426mm, 0.0168" diameter. 2.1 steps, 3.83 degrees
  30 AWG 0.274mm, 0.0108" diameter. 1.37 steps, 2.46 degrees
  32 AWG 0.236mm, 0.0093" diameter. 1.18 steps, 2.12 degrees
  34 AWG 0.175mm, 0.0069" diameter. 0.87 steps, 1.57 degrees
  36 AWG 0.139mm, 0.0055" diameter. 0.69 steps, 1.25 degrees
  38 AWG 0.111mm, 0.0044" diameter. 0.55 steps, 1 degree
  40 AWG 0.086mm, 0.0034" diameter. 0.43 steps, 0.77 degrees
*/
#define REM22_DEGREES 6

// Check to make sure the library is correct
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h, define SSD_1306_128_64!");
#endif

// A few global variables
Adafruit_SSD1306 display(OLED_RESET);
int currentDirection = 0;

/* BasicStepperDriver.move(int steps)
   BasicStepperDriver.rotate(int degrees)
   negative for reverse direction
*/
BasicStepperDriver sCoil(MOTOR_STEPS, COIL_DIR, COIL_STEP);
BasicStepperDriver sCarriage(MOTOR_STEPS, CARRIAGE_DIR, CARRIAGE_STEP);

// Bitmap Logo
static unsigned char PROGMEM const logo_bmp[] = 
{ B11111111, B11111111,
  B10000111, B00000001,
  B10011000, B01111101,
  B10101000, B01000101,
  B10001111, B01000101,
  B10111000, B01111101,
  B10011100, B00000001,
  B10101010, B00111001,
  B10101010, B00010001,
  B10101001, B00010001,
  B10101001, B01111101,
  B10001000, B00010001,
  B10101000, B00010001,
  B10011000, B01111101,
  B10000000, B00000001,
  B11111111, B11111111 };

//
// Initialization
//
void setup() {
    Serial.begin(9600);

    // give the display time to start up
    delay(200);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.display(); // show splashscreen?
    delay(2000);

    pinMode(LEFT_HOME_SWITCH, INPUT);
    pinMode(RIGHT_HOME_SWITCH, INPUT);

    pinMode(COIL_DIR, OUTPUT);
    pinMode(COIL_STEP, OUTPUT);
    pinMode(CARRIAGE_DIR, OUTPUT);
    pinMode(CARRIAGE_STEP, OUTPUT);

    currentDirection = CARRIAGE_RIGHT;

    sCoil.setRPM(COIL_RPM)
    sCarriage.setRPM(CARRIAGE_RPM);

    testStepperMotors();
}

int rightLimitTriggered() {
    return digitalRead(RIGHT_HOME_SWITCH);
}

int leftLimitTriggered() {
    return digitalRead(LEFT_HOME_SWITCH);
}

int limitSwitchTriggered() {
    int limitSwitch = 0;

    if(currentDirection == CARRIAGE_RIGHT) {
        limitSwitch = rightLimitTriggered();
    } else {
        limitSwitch = leftLimitTriggered();
    }

#ifdef DEBUG
    Serial.print("limitSwitchTriggered is returning value ");
    Serial.println(limitSwitch);
#endif

    return limitSwitch;
}

void reverseCarriageDirection() {
    if(currentDirection == CARRIAGE_RIGHT) {
        currentDirection = CARRIAGE_LEFT);
    } else {
        currentDirection = CARRIAGE_RIGHT;
    }

#ifdef DEBUG
    Serial.print("Direction changed to ");

    if(currentDirection == CARRIAGE_RIGHT) {
        Serial.println("right");
    } else {
        Serial.println("left");
    }
#endif
}

/****
** DISPLAY FUNCTIONS
OLED's first 16 rows are Yellow, the rest are blue
Adafruit GFX library fonts are 5x7 pixels, so font 2 is a good top size?
****/
void writeGreeting() {
    display.clearDisplay();

    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0); // OLED mfr says +2 to x axis

    display.println("Welcome to the Machine");

    drawLogo(2,16);

    display.setCursor(20, 20);
    display.println("Left Limit-Display Check");
    display.println("Right Limit-Carriage Check");

    display.display();
}

void writeStepperTest() {
    display.clearDisplay();

    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Stepper Motor");
    display.println("Test Mode");
    display.display();
}

void writeDisplayTest(int size) {
    display.clearDisplay();
    display.setTextSize(size);
    display.setCursor(0, 0);
    display.println("AaBbCcDdEeFfGgHhIiJjKkLlMm");
    display.println("NnOoPpQqRrSsTtUuVvWwXxYyZz");
    display.println("123456789012345678901234567890");
    display.display();
}

void writeToDisplay(char *text, int size=1) {
    display.clearDisplay();
    display.setTextSize(size);
    display.setCursor(0, 0);
    display.println(text);
    display.display();
}

void writeToDisplay(char *text[], int lines, int size=1) {
    display.clearDisplay();
    display.setTextSize(size);
    display.setCursor(0, 0);

    for(int i=0; i<lines; ++i) {
        println(text[i]);
    }
    display.display();
}

void displayHeader() {
    display.clearDisplay();

    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("Jacob's Coiling Machine");

    display.setCursor(0, 16);
    display.setTextSize(6);
    display.print("0/");

    display.setTextSize(3);
    display.print("0");

    display.display();
}

void updateCoilDisplay(unsigned int turns, unsigned int total) {
    display.setCursor(0, 16);
    display.setTextSize(6);
    display.print(turns);
    display.print("/");

    display.setTextSize(3);
    display.print(total)

    display.display();
}

void drawLogo(unsigned int x, unsigned int y) {
    display.drawBitmap(x, y, logo_bmp, 16, 16, 1)
}


/****
** DIAGNOSTIC FUNCTIONS
****/
void testStepperMotors() {
    writeStepperTest();
    delay(500);

    sCoil.setMicrostep(MICROSTEPS)
    sCarriage.setMicrostep(MICROSTEPS);

    sCoil.rotate(360);
    sCarriage.rotate(360);

    sCarriage.rotate(-360);
    sCoil.rotate(-360);

    writeGreeting();
}

void carriageTest() {
    writeToDisplay("Carriage Test", 2);

    sCarriage.setMicrostep(MICROSTEPS);

    while(rightLimitTriggered() == 0) {
        sCarriage.rotate(6);
    }

    while(leftLimitTriggered() == 0) {
        sCarriage.rotate(-6);
    }

    writeGreeting();
}

/****
** COILING FUNCTIONS
****/

void makeCoil(unsigned int turns, unsigned int degreesPerTurn) {
    displayHeader();

    sCoil.setMicrostep(MICROSTEPS);
    sCarriage.setMicrostep(MICROSTEPS);

    for(unsigned int i=0; i<turns; ++i) {
        sCoil.rotate(360);
        updateCoilDisplay(i, turns);

        sCarriage.rotate(currentDirection * REM22_DEGREES);

        if(limitSwitchTriggered()) {
            reverseCarriageDirection();
        }
    }
}

void makeFlipperCoil() {
    makeCoil(900, REM22_DEGREES);
}

//
// main loop
//
void loop() {
    if(leftLimitTriggered()) {
        writeDisplayTest(++displayTestTextSize);
    }

    if(rightLimitTriggered()) {
        carriageTest();
        displayTestTextSize = 0; // reset display test text size in case it's weird
    }

    delay(100);
}

