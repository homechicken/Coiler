/* Arduino code for Coiling Machine
Version 0.1

By A Jacob Cord

Version History
0.1 Limit switch detection and console feedback (28 Mar 2017)

The OLED should be connected to the I2C bus, SDA to pin A4 and SCL to pin A5. VCC to 3.3 or 5v. Its address is 0x3D
*/

//#include <Wire.h> // why?

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DEBUG

// pin definitions
#define LEFT_HOME_SWITCH 2
#define RIGHT_HOME_SWITCH 4

// carriage directions
#define CARRIAGE_RIGHT 0
#define CARRIAGE_LEFT 1

// OLED definitions
#define OLED_RESET 4

// Check to make sure the library is correct
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h, define SSD_1306_128_64!");
#endif

// A few global variables
Adafruit_SSD1306 display(OLED_RESET);
int currentDirection = 0;


void setup() {
    Serial.begin(9600);

    // give the display time to start up
    delay(200);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
    display.display(); // show splashscreen?
    delay(2000);
    writeDisplay();

    pinMode(LEFT_HOME_SWITCH, INPUT);
    pinMode(RIGHT_HOME_SWITCH, INPUT);

    currentDirection = CARRIAGE_RIGHT;
}

int limitSwitchTriggered() {
    int limitSwitch = 0;

    if(currentDirection == CARRIAGE_RIGHT) {
        limitSwitch = digitalRead(RIGHT_HOME_SWITCH);
    } else {
        limitSwitch = digitalRead(LEFT_HOME_SWITCH);
    }

#ifdef DEBUG
    Serial.println("Limit switch hit");
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

void writeDisplay() {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);

    display.println("Welcome to");

    display.setTextSize(2);
    display.println("The Machine");

    display.display();
}

void loop() {

    if(limitSwitchTriggered()) {
        reverseCarriageDirection();
    }

    delay(100);
}

