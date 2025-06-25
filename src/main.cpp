/* <----------------------------| INCLUDES  |----------------------------> */

#include <Arduino.h>
#include <LiquidCrystal.h>

/* <----------------------------| DEFINES  |----------------------------> */

#define SPEAKER 12
#define LCD_CONTRAST 13

#define STATIC_LED_GREEN 2
#define DYNAMIC_LED_RED 8
#define DYNAMIC_LED_GREEN 9
#define DYNAMIC_LED_BLUE 10

#define POTENTIOMETER 54

// TODO: numbers are temporary until wiring is finalized
#define BUTTON_RED 53
#define BUTTON_YELLOW 52
#define BUTTON_GREEN 51
#define BUTTON_BLUE 50

#define TESTING_LENGTH 3

/* <----------------------------| FUNCTIONS  |----------------------------> */

int countDigits(int num);
int length(int array[]);
bool arraysAreEquivalent(int array1[], int array2[]);
void printNumberWithLeadingZeros(int num, int width);
void setLEDColor(int redValue, int greenValue, int blueValue);

/* <----------------------------| CONSTANTS |----------------------------> */

const int rs = 27, en = 26, d4 = 25, d5 = 24, d6 = 23, d7 = 22;
const int LCD_COLUMNS = 16, LCD_ROWS = 2;
const int MIN_DIAL_ANGLE = 150, MAX_DIAL_ANGLE = 170;
const int countdownDurationSeconds = 10;

/* <----------------------------| VARIABLES |----------------------------> */

int countdownElapsedSeconds;
unsigned long startTimeMs, endTimeMs, deltaTimeMs;
int potentiometerAngle;
int userButtonSequence[TESTING_LENGTH], masterButtonSequence[TESTING_LENGTH] = {BUTTON_RED, BUTTON_RED, BUTTON_RED}; // TODO: abstract array length; figure out way to do this not stupidly 
int userButtonSequenceIndex;
bool potentiometerSolved, buttonSolved, bombDefused;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

/* <----------------------------| MAIN FUNCTIONS |----------------------------> */

void setup() {
    Serial.begin(9600);
    
    // Initialize input pins
    pinMode(SPEAKER, OUTPUT);
    pinMode(LCD_CONTRAST, OUTPUT);
    pinMode(STATIC_LED_GREEN, OUTPUT);
    pinMode(DYNAMIC_LED_RED, OUTPUT);
    pinMode(DYNAMIC_LED_GREEN, OUTPUT);
    pinMode(DYNAMIC_LED_BLUE, OUTPUT);
    pinMode(POTENTIOMETER, INPUT);
    pinMode(BUTTON_RED, INPUT);
    pinMode(BUTTON_YELLOW, INPUT);
    pinMode(BUTTON_GREEN, INPUT);
    pinMode(BUTTON_BLUE, INPUT);

    // Initialize LCD screen
    lcd.begin(LCD_COLUMNS, LCD_ROWS);
    lcd.setCursor(0, 0);
    analogWrite(LCD_CONTRAST, 100);

    // Initialize LEDs
    digitalWrite(STATIC_LED_GREEN, LOW);
    analogWrite(DYNAMIC_LED_RED, 255);
    analogWrite(DYNAMIC_LED_GREEN, 0);
    analogWrite(DYNAMIC_LED_BLUE, 0);

    // Initialize variables
    potentiometerSolved = false, buttonSolved = false, bombDefused = false;
    userButtonSequenceIndex = 0;
    countdownElapsedSeconds = 0;
    startTimeMs = millis(), endTimeMs = millis();
}

void loop() {
    /* ---------- BOMB DEFUSED/DETONATED ---------- */

    // Check if all puzzles have been solved
    bombDefused = potentiometerSolved && buttonSolved;
    bool countdownComplete = countdownElapsedSeconds >= countdownDurationSeconds;

    // Terminate countdown via defusal or detonation
    if (countdownComplete) {
        lcd.setCursor(0, 1);
        lcd.print("BOOM!");

        // Long speaker firing to indicate bomb detonation
        analogWrite(SPEAKER, 1);
        delay(3000);
        analogWrite(SPEAKER, 0);

        exit(0);
    }
    else if (bombDefused) {
        lcd.setCursor(0, 1);
        lcd.print("BOMB DEFUSED");

        exit(0);
    }

    /* ---------- COUNTDOWN SEQUENCE ---------- */

    // Countdown update
    deltaTimeMs = endTimeMs - startTimeMs;
    if (deltaTimeMs >= 1000) {
        // Update LCD Screen
        countdownElapsedSeconds++;
        lcd.home();
        printNumberWithLeadingZeros((countdownElapsedSeconds - countdownDurationSeconds) * -1, 2);

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
        potentiometerSolved = true;
    }
    else {
        int redVal = deltaTimeMs % (MAX_DIAL_ANGLE - potentiometerAngle) >= 60 ? 255 : 0;
        setLEDColor(redVal, 0, 0);
        potentiometerSolved = false;
    }

    /* ---------- BUTTON PUZZLE ---------- */

    // TODO: this shit doesnt work. probably an issue with button debouncing
    // Keep track of user button presses
    userButtonSequenceIndex = userButtonSequenceIndex < TESTING_LENGTH ? userButtonSequenceIndex : 0;
    if (digitalRead(BUTTON_RED)    == 1) { userButtonSequence[userButtonSequenceIndex++] = BUTTON_RED;    }
    if (digitalRead(BUTTON_YELLOW) == 1) { userButtonSequence[userButtonSequenceIndex++] = BUTTON_GREEN;  }
    if (digitalRead(BUTTON_BLUE)   == 1) { userButtonSequence[userButtonSequenceIndex++] = BUTTON_YELLOW; }
    if (digitalRead(BUTTON_GREEN)  == 1) { userButtonSequence[userButtonSequenceIndex++] = BUTTON_BLUE;   }

    // Turn Green LED on if red button pressed four times in a row
    if (arraysAreEquivalent(userButtonSequence, masterButtonSequence)) {
        digitalWrite(STATIC_LED_GREEN, HIGH);
        buttonSolved = true;
    }
    else {
        digitalWrite(STATIC_LED_GREEN, LOW);
        buttonSolved = false;
    }

    /* ---------- CLOCK ---------- */

    // Keep track of how much time has passed since last second
    endTimeMs = millis();
}

/* <----------------------------| HELPER METHODS |----------------------------> */

// Helper function to count the number of digits in a given number for formatting
int countDigits(int num) {
    int digits = 0;
    do {
        digits++;
        num /= 10;
    } while (num != 0);
    return digits;
}

int length(int array[]) {
    return (int) (sizeof(array) / sizeof(array[0])); // TODO why is this a warning?
}

// Determine if two arrays contain the same elements
bool arraysAreEquivalent(int array1[], int array2[]) {
    for (int i = 0; i < length(array1); i++) {
        if (array1[i] != array2[i]) { return false; }
    }
    return true;
}

// Helper function to print numbers to an LCD screen as a formatted string with leading zeros 
void printNumberWithLeadingZeros(int num, int width) {
    int currentDigits = countDigits(num);
    for (int i = 0; i < width - currentDigits; i++) {
        lcd.print("0");
    }
    lcd.print(num);
}

// Sets color of common cathod RGB LED on breadboard
void setLEDColor(int redValue, int greenValue, int blueValue) {
    analogWrite(DYNAMIC_LED_RED, redValue);
    analogWrite(DYNAMIC_LED_GREEN, greenValue);
    analogWrite(DYNAMIC_LED_BLUE, blueValue);
}