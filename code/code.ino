/**
 * arduino-cli compile --fqbn arduino:avr:uno sketch_sep17a
 * arduino-cli upload --fqbn arduino:avr:uno sketch_sep17a
 */

#include <LiquidCrystal.h>

#include "Button.h"

#include <thermistor.h>

/**

DEIXAR AS TRILHAS MAIS DISTANTES ENTRE ELAS
DEIXAR OS PADS MAIORES

 */

// Define PINS
#define THERMISTOR_PIN A0

#define LCD_RS_PIN 7
#define LCD_EN_PIN 6
#define LCD_D4_PIN 5
#define LCD_D5_PIN 4
#define LCD_D6_PIN 3
#define LCD_D7_PIN 2

#define HEATER_PIN 8

#define BTN_PLUS A4
#define BTN_MINUS A3
#define BTN_ENTER A5


// Thermistor
thermistor thermistor1(THERMISTOR_PIN, 60);	// 60 or 11
int temperatureDesired = 35;
int temperatureCurrent = 0;

// LCD
LiquidCrystal lcd(LCD_RS_PIN, LCD_EN_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);
unsigned long lcd_last_refresh = 0;

byte customArrow[] = {
	0x00,
	0x08,
	0x0C,
	0x0E,
	0x0E,
	0x0C,
	0x08,
	0x00
};

byte customGraus[] = {
	0x00,
	0x06,
	0x09,
	0x09,
	0x06,
	0x00,
	0x00,
	0x00
};

// Button
Button btn_enter(BTN_ENTER);
Button btn_minus(BTN_MINUS);
Button btn_plus(BTN_PLUS);

int buttonIndicatorLocation = 0;

// Motor
byte directionPin = 10;
byte stepPin = 9;
int numberOfSteps = 100;
unsigned long step_last_pulse = 0;
unsigned long step_speed_pulse = 0;

/**
 * Refresh LCD screen
 */
void refreshLCD()
{
	lcd.clear();

	// Set indicator
	lcd.setCursor(0, buttonIndicatorLocation);
	lcd.write(byte(0));

	// Temperature
	lcd.setCursor(2, 0);
	lcd.print(temperatureCurrent);
	if(temperatureCurrent >= 100) 
		lcd.setCursor(5, 0);
	else 
		lcd.setCursor(4, 0);
	lcd.write(byte(1));

	lcd.setCursor(10, 0);
	lcd.print(temperatureDesired);
	lcd.setCursor(13, 0);
	lcd.write(byte(1));
	

	// Store time as last refresh
	lcd_last_refresh = millis();
}


/**
 * Control temperature PID
 */

unsigned char ledValue = 0;
void temperaturePID()
{
	if(temperatureCurrent < temperatureDesired) {
		digitalWrite(HEATER_PIN, HIGH);
	}
	else {
		digitalWrite(HEATER_PIN, LOW);
	}
}


/**
 * Setup
 */
void setup()
{
	// Step motor
	pinMode(directionPin, OUTPUT);
	pinMode(stepPin, OUTPUT);

	// Heater
	pinMode(HEATER_PIN, OUTPUT);
	// digitalWrite(HEATER_PIN, LOW);

	// Start LCD and custom chars
	lcd.begin(16, 2);
	lcd.createChar(0, customArrow);
	lcd.createChar(1, customGraus);
	lcd.clear();
}



/**
 * Loop
 */
void loop()
{
	// 
	if(step_speed_pulse > 0) {
		if((millis() - step_last_pulse) > step_speed_pulse) {
			digitalWrite(directionPin, HIGH);
			digitalWrite(stepPin, HIGH);
			digitalWrite(stepPin, LOW);
			digitalWrite(directionPin, LOW);

			step_last_pulse = millis();
		}
	}

	// Store current temperature
	temperatureCurrent = (int)thermistor1.analog2temp();

	// Control temp PID
	temperaturePID();

	// Handle button ENTER
	short btn1 = btn_enter.checkPress();
	if (btn1 == 1) {
		if(buttonIndicatorLocation == 1) {
			buttonIndicatorLocation = 0;
		}
		else if(buttonIndicatorLocation == 0) {
			buttonIndicatorLocation = 1;
		}
	} 

	// Handle button UP
	short btn2 = btn_plus.checkPress();
	if (btn2 == 1) {
		if(buttonIndicatorLocation == 0) // Temperature
			temperatureDesired++;
		else if(buttonIndicatorLocation == 1) // Motor
			step_speed_pulse += 25;
	} 

	// Handle button DOWN
	short btn3 = btn_minus.checkPress();
	if (btn3 == 1) {
		if(buttonIndicatorLocation == 0) // Temperature
			temperatureDesired--;
		else if(buttonIndicatorLocation == 1) // Motor
			step_speed_pulse -= 25;
	}

	// Refresh screen
	if((millis() - lcd_last_refresh) > 100) {
		refreshLCD();
	}
}

/**
 

260


 */
