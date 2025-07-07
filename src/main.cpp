/* <----------------------------| INCLUDES  |----------------------------> */

#include <Arduino.h>
#include <LiquidCrystal.h>

/* <----------------------------| DEFINES  |----------------------------> */

// LCD pins
#define LCD_CA 13
#define LCD_RS 27
#define LCD_EN 26
#define LCD_D4 25
#define LCD_D5 24
#define LCD_D6 23
#define LCD_D7 22

// Countdown buzzer pin
#define BUZZER 12

// Servo pin
#define SERVO 6

#define RESET_PIN 49
#define RESET_SWITCH 48

// LED pins
#define LED_STATIC_RED 5
#define LED_STATIC_YELLOW 4
#define LED_STATIC_GREEN 3
#define LED_STATIC_BLUE 2
#define LED_DYNAMIC_RED 8
#define LED_DYNAMIC_GREEN 9
#define LED_DYNAMIC_BLUE 10

// Potentiometer pin
/*
 * ORANGE = data
 * RED = ground
 * BROWN = power
 */
#define POTENTIOMETER 54

// Button pins
#define BUTTON_RED 53
#define BUTTON_YELLOW 52
#define BUTTON_GREEN 51
#define BUTTON_BLUE 50

// Wire pins
#define PUZZLE_WIRE_BROWN 45
#define PUZZLE_WIRE_ORANGE 43
#define PUZZLE_WIRE_BLUE 41
#define PUZZLE_WIRE_GREEN 39

/* <----------------------------| FUNCTIONS  |----------------------------> */

int countDigits(int num);
bool arraysAreEquivalent(int array1[], int array2[], int arrayLength);
bool valueWithinTargetError(int value, int target, int error);
void printNumberWithLeadingZeros(int num, int width);
void setDynamicLED(int redValue, int greenValue, int blueValue);
void resetUserSequence();
void(* resetFunc) (void) = 0; //declare reset function at address 0

/* <----------------------------| CONSTANTS |----------------------------> */

// LCD constants
const int LCD_CONTRAST = 100;
const int LCD_COLUMNS = 16, LCD_ROWS = 2;

// Countdown constants
const int COUNTDOWN_DURATION_SECONDS = 1;
const long STARTING_BUZZER_DELAY_MILLISECONDS = 10000;

// Potentiometer constants
const int DIAL_SOLUTION_ANGLE = 433;
const int DIAL_SOLUTION_ERROR = 1;

// Button constants
const int NUM_BUTTONS = 4;
const int BUTTON_PINS[NUM_BUTTONS] = {BUTTON_RED, BUTTON_YELLOW, BUTTON_GREEN, BUTTON_BLUE};
const int MASTER_SEQUENCE[] = {BUTTON_RED, BUTTON_YELLOW, BUTTON_RED};
const int SEQUENCE_LENGTH = (int) (sizeof(MASTER_SEQUENCE) / sizeof(*MASTER_SEQUENCE));

// Wire constants
const int CUT_COUNT_THRESHOLD = 20;

/* <----------------------------| VARIABLES |----------------------------> */

// LCD variables
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Countdown variables
int countdownElapsedSeconds, countdownSecondsLeft;
double percentTimeLeft;
unsigned long startTimeMs, endTimeMs, deltaTimeMs;
long buzzOnStart, timeSinceLastBuzz, buzzDelay;
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

    pinMode(PUZZLE_WIRE_BROWN, INPUT);
    pinMode(PUZZLE_WIRE_ORANGE, INPUT);
    pinMode(PUZZLE_WIRE_BLUE, INPUT);
    pinMode(PUZZLE_WIRE_GREEN, INPUT);
    
    // Initialize UX pins
    pinMode(BUZZER, OUTPUT);
    pinMode(LCD_CA, OUTPUT);
    pinMode(SERVO, OUTPUT);
    pinMode(RESET_PIN, INPUT);
    pinMode(RESET_SWITCH, INPUT);

    // Initialize LED pins
    pinMode(LED_STATIC_RED, OUTPUT);
    pinMode(LED_STATIC_YELLOW, OUTPUT);
    pinMode(LED_STATIC_GREEN, OUTPUT);
    pinMode(LED_STATIC_BLUE, OUTPUT);
    pinMode(LED_DYNAMIC_RED, OUTPUT);
    pinMode(LED_DYNAMIC_GREEN, OUTPUT);
    pinMode(LED_DYNAMIC_BLUE, OUTPUT);

    // Initialize puzzle pins
    pinMode(POTENTIOMETER, INPUT);
    pinMode(BUTTON_RED, INPUT);
    pinMode(BUTTON_YELLOW, INPUT);
    pinMode(BUTTON_GREEN, INPUT);
    pinMode(BUTTON_BLUE, INPUT);

    // Initialize LCD screen
    lcd.begin(LCD_COLUMNS, LCD_ROWS);
    lcd.setCursor(0, 0);
    analogWrite(LCD_CA, LCD_CONTRAST);
    
    // Initialize Servo
    analogWrite(SERVO, 0);

    // Initialize LEDs
    digitalWrite(LED_STATIC_RED, HIGH);
    digitalWrite(LED_STATIC_YELLOW, LOW);
    digitalWrite(LED_STATIC_GREEN, LOW);
    digitalWrite(LED_STATIC_BLUE, LOW);
    analogWrite(LED_DYNAMIC_RED, 255);
    analogWrite(LED_DYNAMIC_GREEN, 0);
    analogWrite(LED_DYNAMIC_BLUE, 0);

    // Initialize variables
    for (int i = 0; i < SEQUENCE_LENGTH; i++) { userSequence[i] = -1; }
    potentiometerIsSolved = false, buttonIsSolved = false, wireIsSolved = false, bombIsDefused = false;
    userSequenceIndex = 0;
    countdownElapsedSeconds = 0;
    greenWireCutCount = 0;
    startTimeMs = millis(), endTimeMs = millis();

    /* TODO:
     * So basically my idea is, at startup, check if the reset switch is at 1. If it is enter the secret menu and
     * let the user be able to change what the button sequence, timer, and wire for the puzzle should be. This is OPTIONAL!!!!!!
     * So get the thing working AND A MANUAL FIRST, AND THEN try this little doohicky out.
     */
}

void loop() {
    /* ---------- SUPER SECRET MENU ---------- */
    // Serial.println(digitalRead(48));
    // if (potentiometerAngle == 777 && digitalRead(BUTTON_RED) == 1 && digitalRead(BUTTON_BLUE) == 1 && digitalRead(PUZZLE_WIRE_GREEN) == 0) {
    //     if (digitalRead(RESET_PIN) == 1) {
    //         resetFunc();
    //     }
    // } 
    
    Serial.print(potentiometerAngle);
    Serial.print("  |  ");
    Serial.print(buttonState[0]);
    Serial.print(" ");
    Serial.print(buttonState[1]);
    Serial.print(" ");
    Serial.print(buttonState[2]);
    Serial.print(" ");
    Serial.print(buttonState[3]);
    Serial.print("  |  ");
    Serial.print(digitalRead(PUZZLE_WIRE_BROWN));
    Serial.print(" ");
    Serial.print(digitalRead(PUZZLE_WIRE_ORANGE));
    Serial.print(" ");
    Serial.print(digitalRead(PUZZLE_WIRE_BLUE));
    Serial.print(" ");

    Serial.print(digitalRead(PUZZLE_WIRE_GREEN));
    Serial.print("\n");
    
    /* ---------- BOMB DEFUSED/DETONATED ---------- */

    // Check if all puzzles have been solved
    bombIsDefused = potentiometerIsSolved && buttonIsSolved && wireIsSolved;
    countdownIsComplete = countdownElapsedSeconds >= COUNTDOWN_DURATION_SECONDS;

    // Terminate countdown via defusal or detonation
    if (countdownIsComplete) {
        // Announce bomb detonation 
        lcd.setCursor(0, 1);
        lcd.print("DETONATING...");

        // Long buzzer firing to indicate bomb detonation
        analogWrite(BUZZER, 1);
        delay(3000);
        analogWrite(BUZZER, 0);

        // Move pin out of way to let chemicals mix

        /* For some godforsaken reason...
         * - 0, and 183 to 194 is STOPPED 
         * - 195 to 249 is MINIMUM to MAXIMUM CLOCKWISE
         * - 182 to 110 is MINIMUM to MAXIMUM COUNTER-CLOCKWISE
         */
        analogWrite(SERVO, 220);
        delay(600);
        analogWrite(SERVO, 0);


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
    countdownSecondsLeft = COUNTDOWN_DURATION_SECONDS - countdownElapsedSeconds;
    percentTimeLeft = (double)(countdownSecondsLeft) / (double)(COUNTDOWN_DURATION_SECONDS);
    buzzDelay = STARTING_BUZZER_DELAY_MILLISECONDS * percentTimeLeft;
    
    // Restart seconds counter
    if (deltaTimeMs >= 1000) {
        countdownElapsedSeconds++;
        startTimeMs = millis();
    }

    if (countdownElapsedSeconds % 2 == 0) {
        digitalWrite(LED_STATIC_RED, HIGH);
        digitalWrite(LED_STATIC_YELLOW, HIGH);
        digitalWrite(LED_STATIC_GREEN, HIGH);
        digitalWrite(LED_STATIC_BLUE, HIGH);
    }
    else {
        digitalWrite(LED_STATIC_RED, LOW);
        digitalWrite(LED_STATIC_YELLOW, LOW);
        digitalWrite(LED_STATIC_GREEN, LOW);
        digitalWrite(LED_STATIC_BLUE, LOW);
    }

    // Determine if buzzer should be turned on or off
    timeSinceLastBuzz = millis() - buzzOnStart;
    if (timeSinceLastBuzz >= buzzDelay) {
        buzzOnStart = millis();
        analogWrite(BUZZER, 1);
    }
    else if (timeSinceLastBuzz >= 100) {
        analogWrite(BUZZER, 0);
    }

    // Calculate raw elapsed time in units
    int centiseconds = (999 - deltaTimeMs) / 10;
    int seconds = COUNTDOWN_DURATION_SECONDS - countdownElapsedSeconds;
    int minutes = seconds / 60;
    int hours = minutes / 60;

    // Calculate units to display on LCD
    int displayCentiseconds = centiseconds < 0 ? 0 : centiseconds;
    int displaySeconds = seconds % 60;
    int displayMinutes = minutes % 60;
    int displayHours = hours;

    // Display time on LCD
    lcd.home();
    printNumberWithLeadingZeros(displayHours, 2);
    lcd.print(":");
    printNumberWithLeadingZeros(displayMinutes, 2);
    lcd.print(":");
    printNumberWithLeadingZeros(displaySeconds, 2);
    lcd.print(":");
    printNumberWithLeadingZeros(displayCentiseconds, 2);

    /* ---------- POTENTIOMETER PUZZLE ---------- */

    // Turn LED to green if potentiometer within defusal range, otherwise keep red
    potentiometerAngle = (analogRead(POTENTIOMETER) / 1023.0) * 999.0;
    if (valueWithinTargetError(potentiometerAngle, DIAL_SOLUTION_ANGLE, DIAL_SOLUTION_ERROR)) {
        setDynamicLED(0, 255, 0);
        potentiometerIsSolved = true;
    }
    else {
        setDynamicLED(0, 0, 0);
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

    // Turn Green LED on if button sequence is entered by user, otherwise, reset the sequence register
    if (userSequenceIndex != 0 && userSequence[userSequenceIndex - 1] != MASTER_SEQUENCE[userSequenceIndex - 1]) {
        resetUserSequence();
    }
    else if (userSequenceIndex == SEQUENCE_LENGTH) {
        digitalWrite(LED_STATIC_GREEN, HIGH);
        buttonIsSolved = true;
    }

    /* ---------- WIRE PUZZLE ---------- */

    // Wait for pinout on indicated wire to read 20 zeros in a row to wire is cut, then defuse or detonate accordingly
    greenWireCutCount = digitalRead(PUZZLE_WIRE_GREEN) == 0 ? greenWireCutCount + 1 : 0;
    // redWireCutCount = digitalRead(PUZZLE_WIRE_RED) == 0 ? redWireCutCount + 1 : 0;
    if (greenWireCutCount >= 20) {
        wireIsSolved = true;
    }
    // else if (redWireCutCount >= 20) {
    //     countdownElapsedSeconds = COUNTDOWN_DURATION_SECONDS;
    // }

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

// Return true if integer value within specified margin of error of target, false otherwise
bool valueWithinTargetError(int value, int target, int error) {
    return (value <= target + error) && (value >= target - error);
}

// Print numbers to an LCD screen as a formatted string with leading zeros 
void printNumberWithLeadingZeros(int num, int width) {
    for (int i = 0; i < width - countDigits(num); i++) {
        lcd.print("0");
    }
    lcd.print(num);
}

// Set color of common cathode RGB LED on breadboard
void setDynamicLED(int redValue, int greenValue, int blueValue) {
    analogWrite(LED_DYNAMIC_RED, redValue);
    analogWrite(LED_DYNAMIC_GREEN, greenValue);
    analogWrite(LED_DYNAMIC_BLUE, blueValue);
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