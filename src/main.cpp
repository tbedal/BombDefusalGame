/* <----------------------------| DECLARATIONS  |----------------------------> */

#include <Arduino.h>
#include <LiquidCrystal.h>

#define BUZZER 12
#define LCD_CONTRAST 13
#define POTENTIOMETER 54
#define DYNAMIC_LED_RED 2
#define DYNAMIC_LED_GREEN 3
#define DYNAMIC_LED_BLUE 4

int countDigits(int num);
void printNumberWithLeadingZeros(int num, int width);

/* <----------------------------| CONSTANTS |----------------------------> */

const int rs = 27, en = 26, d4 = 25, d5 = 24, d6 = 23, d7 = 22;
const int LCD_COLUMNS = 16;
const int LCD_ROWS = 2;

/* <----------------------------| VARIABLES |----------------------------> */

int countdownDurationSeconds, countdownElapsedSeconds;
int startTimeMs, endTimeMs;
int potentiometerAngle;
bool bombDefused;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

/* <----------------------------| MAIN FUNCTIONS |----------------------------> */

void setup() {
    // Initialize pins
    pinMode(BUZZER, OUTPUT);
    pinMode(LCD_CONTRAST, OUTPUT);
    pinMode(POTENTIOMETER, INPUT);
    pinMode(DYNAMIC_LED_RED, OUTPUT);
    pinMode(DYNAMIC_LED_GREEN, OUTPUT);
    pinMode(DYNAMIC_LED_BLUE, OUTPUT);

    // Initialize LCD screen
    lcd.begin(LCD_COLUMNS, LCD_ROWS);
    lcd.setCursor(0, 0);
    analogWrite(LCD_CONTRAST, 100);

    // Initialize LEDs
    analogWrite(DYNAMIC_LED_RED, 255);
    analogWrite(DYNAMIC_LED_GREEN, 0);
    analogWrite(DYNAMIC_LED_BLUE, 0);

    // Initialize variables
    bombDefused = false;
    countdownDurationSeconds = 10;
    countdownElapsedSeconds = 0;
    startTimeMs = millis();
    endTimeMs = millis();

    // Display countdown using millis() method
    while (countdownElapsedSeconds < countdownDurationSeconds && !bombDefused) {
        // Update LCD screen and timekeeping after one second has passed, fire speaker
        if (endTimeMs - startTimeMs >= 1000) {
            countdownElapsedSeconds++;
            lcd.home();
            printNumberWithLeadingZeros((countdownElapsedSeconds - countdownDurationSeconds) * -1, 2);

            analogWrite(BUZZER, 1);
            delay(100);
            analogWrite(BUZZER, 0);

            startTimeMs = millis();
        }

        // Turn LED to green if potentiometer within defusal range, otherwise keep red
        potentiometerAngle = analogRead(POTENTIOMETER);
        if (potentiometerAngle >= 150 && potentiometerAngle <= 160) {
            analogWrite(DYNAMIC_LED_RED, 0);
            analogWrite(DYNAMIC_LED_GREEN, 255);
            analogWrite(DYNAMIC_LED_BLUE, 0);
            bombDefused = true;
            continue;
        }
        else {
            analogWrite(DYNAMIC_LED_RED, 255);
            analogWrite(DYNAMIC_LED_GREEN, 0);
            analogWrite(DYNAMIC_LED_BLUE, 0);
        }

        // Keep track of how much time has passed since last second
        endTimeMs = millis();
    }

    // Long speaker firing to indicate bomb detonation
    analogWrite(BUZZER, 1);
    delay(3000);
    analogWrite(BUZZER, 0);
}

void loop() {
    // End of program -> display BOOM! text
    lcd.setCursor(0, 1);
    analogWrite(BUZZER, 0);
    lcd.print("BOOM!");
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

// Helper function to print numbers to an LCD screen as a formatted string with leading zeros 
void printNumberWithLeadingZeros(int num, int width) {
    int currentDigits = countDigits(num);
    for (int i = 0; i < width - currentDigits; i++) {
        lcd.print("0");
    }
    lcd.print(num);
}