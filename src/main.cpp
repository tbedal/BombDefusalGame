/* <----------------------------| INCLUDES  |----------------------------> */

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Servo.h>

/* <----------------------------| DEFINES  |----------------------------> */

// Initalize UX pins
#define LCD_CONTRAST 13
#define SPEAKER 12
#define SERVO 6

// LED pins
#define STATIC_LED_GREEN 2
#define DYNAMIC_LED_RED 8
#define DYNAMIC_LED_GREEN 9
#define DYNAMIC_LED_BLUE 10

// Potentiometer pin
#define POTENTIOMETER 54

// Button pins
#define BUTTON_RED 53
#define BUTTON_YELLOW 52
#define BUTTON_GREEN 51
#define BUTTON_BLUE 50

// Wire pins
#define PUZZLE_WIRE_GREEN 40
#define PUZZLE_WIRE_RED 41

/* <----------------------------| FUNCTIONS  |----------------------------> */

int countDigits(int num);
bool arraysAreEquivalent(int array1[], int array2[], int arrayLength);
void printNumberWithLeadingZeros(int num, int width);
void setLEDColor(int redValue, int greenValue, int blueValue);
void setServo(Servo s, int angle, int speed);
void resetUserSequence();

/* <----------------------------| CONSTANTS |----------------------------> */

// LCD constants
const int rs = 27, en = 26, d4 = 25, d5 = 24, d6 = 23, d7 = 22;
const int LCD_COLUMNS = 16, LCD_ROWS = 2;

// Countdown constants
const int COUNTDOWN_DURATION = 500;

// Potentiometer constants
const int MIN_DIAL_ANGLE = 150, MAX_DIAL_ANGLE = 170;

// Button constants
const int NUM_BUTTONS = 4;
const int BUTTON_PINS[NUM_BUTTONS] = {BUTTON_RED, BUTTON_YELLOW, BUTTON_GREEN, BUTTON_BLUE};
const int MASTER_SEQUENCE[] = {BUTTON_RED, BUTTON_YELLOW, BUTTON_RED};
const int SEQUENCE_LENGTH = (int) (sizeof(MASTER_SEQUENCE) / sizeof(*MASTER_SEQUENCE));

// Wire constants
const int CUT_COUNT_THRESHOLD = 20;

/* <----------------------------| VARIABLES |----------------------------> */

// LCD variables
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Servo variables
Servo servo;
int servoPosition;

// Countdown variables
int countdownElapsedSeconds;
unsigned long startTimeMs, endTimeMs, deltaTimeMs;
bool potentiometerIsSolved, buttonIsSolved, wireIsSolved;
bool countdownIsComplete, bombIsDefused;

// Potentiometer variables
int potentiometerAngle;

// Button variables
int userSequence[SEQUENCE_LENGTH];
int userSequenceIndex;
int buttonState[NUM_BUTTONS];
int lastButtonState[NUM_BUTTONS];

// Wire variables
int greenWireCutCount, redWireCutCount;


/* <----------------------------| MAIN FUNCTIONS |----------------------------> */

void setup() {
    Serial.begin(9600);
    
    // Initialize UX pins
    pinMode(SPEAKER, OUTPUT);
    pinMode(LCD_CONTRAST, OUTPUT);

    // Initialize LED pins
    pinMode(STATIC_LED_GREEN, OUTPUT);
    pinMode(DYNAMIC_LED_RED, OUTPUT);
    pinMode(DYNAMIC_LED_GREEN, OUTPUT);
    pinMode(DYNAMIC_LED_BLUE, OUTPUT);

    // Initialize puzzle pins
    pinMode(POTENTIOMETER, INPUT);
    pinMode(BUTTON_RED, INPUT);
    pinMode(BUTTON_YELLOW, INPUT);
    pinMode(BUTTON_GREEN, INPUT);
    pinMode(BUTTON_BLUE, INPUT);

    // Initialize LCD screen
    lcd.begin(LCD_COLUMNS, LCD_ROWS);
    lcd.setCursor(0, 0);
    analogWrite(LCD_CONTRAST, 100);
    
    // Initialize Servo
    servo.attach(SERVO);
    servoPosition = servo.read();

    // Initialize LEDs
    digitalWrite(STATIC_LED_GREEN, LOW);
    analogWrite(DYNAMIC_LED_RED, 255);
    analogWrite(DYNAMIC_LED_GREEN, 0);
    analogWrite(DYNAMIC_LED_BLUE, 0);

    // Initialize variables
    for (int i = 0; i < SEQUENCE_LENGTH; i++) { userSequence[i] = -1; }
    potentiometerIsSolved = false, buttonIsSolved = false, wireIsSolved = false, bombIsDefused = false;
    userSequenceIndex = 0;
    countdownElapsedSeconds = 0;
    greenWireCutCount = 0;
    startTimeMs = millis(), endTimeMs = millis();
}

void loop() {
    /* ---------- BOMB DEFUSED/DETONATED ---------- */

    // Check if all puzzles have been solved
    bombIsDefused = potentiometerIsSolved && buttonIsSolved && wireIsSolved;
    countdownIsComplete = countdownElapsedSeconds >= COUNTDOWN_DURATION;

    // Terminate countdown via defusal or detonation
    if (countdownIsComplete) {
        // Announce bomb detonation 
        lcd.setCursor(0, 1);
        lcd.print("DETONATING...");

        // Long speaker firing to indicate bomb detonation
        analogWrite(SPEAKER, 1);
        delay(3000);
        analogWrite(SPEAKER, 0);

        // Move pin out of way to let chemicals mix
        setServo(servo, 180, 15);

        // Terminate program
        exit(0);
    }
    else if (bombIsDefused) {
        // Announce successful bomb defusal
        lcd.setCursor(0, 1);
        lcd.print("BOMB DEFUSED");

        // Terminate program
        exit(0);
    }

    /* ---------- COUNTDOWN SEQUENCE ---------- */

    // Countdown update
    deltaTimeMs = endTimeMs - startTimeMs;
    if (deltaTimeMs >= 1000) {
        // Update LCD Screen
        countdownElapsedSeconds++;
        lcd.home();
        printNumberWithLeadingZeros((countdownElapsedSeconds - COUNTDOWN_DURATION) * -1, 2);

        // Buzz speaker
        analogWrite(SPEAKER, 1);
        delay(100);
        analogWrite(SPEAKER, 0);

        // Restart seconds counter
        startTimeMs = millis();
    }

    /* ---------- POTENTIOMETER PUZZLE ---------- */

    // Turn LED to green if potentiometer within defusal range, otherwise keep red
    potentiometerAngle = analogRead(POTENTIOMETER);
    if (potentiometerAngle >= MIN_DIAL_ANGLE && potentiometerAngle <= MAX_DIAL_ANGLE) {
        setLEDColor(0, 255, 0);
        potentiometerIsSolved = true;
    }
    else {
        int redVal = deltaTimeMs % (MAX_DIAL_ANGLE - potentiometerAngle) >= 60 ? 255 : 0;
        setLEDColor(redVal, 0, 0);
        potentiometerIsSolved = false;
    }

    /* ---------- BUTTON PUZZLE ---------- */

    // Poll all buttons in RGYB order, with positive-edge-triggered updates to user's button sequence
    for (int i = 0; i < NUM_BUTTONS; i++) {
        int currentButton = BUTTON_PINS[i];
        buttonState[i] = digitalRead(currentButton);
        if (lastButtonState[i] == 1 && buttonState[i] == 0) {
            userSequence[userSequenceIndex++] = currentButton;
        }
        lastButtonState[i] = buttonState[i];
    }

    // Turn Green LED on if red button pressed four times in a row, otherwise, clear user sequence
    if (userSequenceIndex != 0 && userSequence[userSequenceIndex - 1] != MASTER_SEQUENCE[userSequenceIndex - 1]) {
        resetUserSequence();
    }
    else if (userSequenceIndex == SEQUENCE_LENGTH) {
        digitalWrite(STATIC_LED_GREEN, HIGH);
        buttonIsSolved = true;
    }


    /* ---------- WIRE PUZZLE ---------- */

    greenWireCutCount = digitalRead(PUZZLE_WIRE_GREEN) == 0 ? greenWireCutCount + 1 : 0;
    redWireCutCount = digitalRead(PUZZLE_WIRE_RED) == 0 ? redWireCutCount + 1 : 0;
    if (greenWireCutCount >= 20) {
        wireIsSolved = true;
    }
    else if (redWireCutCount >= 20) {
        countdownElapsedSeconds = COUNTDOWN_DURATION;
    }

    /* ---------- CLOCK ---------- */

    // Keep track of how much time has passed since last second
    endTimeMs = millis();
}

/* <----------------------------| HELPER METHODS |----------------------------> */

// Count the number of digits in a given number for formatting
int countDigits(int num) {
    int digits = 0;
    do {
        digits++;
        num /= 10;
    } while (num != 0);
    return digits;
}

// Determine if two arrays contain the same elements
bool arraysAreEquivalent(int array1[], int array2[], int arrayLength) {
    for (int i = 0; i < arrayLength; i++) {
        if (array1[i] != array2[i]) { return false; }
    }
    return true;
}

// Print numbers to an LCD screen as a formatted string with leading zeros 
void printNumberWithLeadingZeros(int num, int width) {
    int currentDigits = countDigits(num);
    for (int i = 0; i < width - currentDigits; i++) {
        lcd.print("0");
    }
    lcd.print(num);
}

// Set color of common cathode RGB LED on breadboard
void setLEDColor(int redValue, int greenValue, int blueValue) {
    analogWrite(DYNAMIC_LED_RED, redValue);
    analogWrite(DYNAMIC_LED_GREEN, greenValue);
    analogWrite(DYNAMIC_LED_BLUE, blueValue);
}

// Turn servo to specified angle, with a delay of speed between each degree turn
void setServo(Servo s, int angle, int speed) {
    while (s.read() < angle) {
        s.write(s.read() + 1);
        delay(speed);
    }
}

// Clears the user sequence array with null pin
void resetUserSequence() {
    int lastButtonPressed = userSequence[userSequenceIndex];
    for (int i = 0; i < SEQUENCE_LENGTH; i++) {
        userSequence[i] = -1; 
    }
    userSequenceIndex = 0;
    userSequence[0] = lastButtonPressed;
}