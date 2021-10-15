/**
 * arduino-cli compile --fqbn arduino:avr:uno sketch_sep17a
 * arduino-cli upload --fqbn arduino:avr:uno sketch_sep17a
 */

#include <LiquidCrystal.h>
#include <PID.h>
#include <thermistor.h>
#include <EEPROM.h>
#include "Button.h"


// Define PINS
#define THERMISTOR_PIN A0

#define LCD_RS_PIN 7
#define LCD_EN_PIN 6
#define LCD_D4_PIN 5
#define LCD_D5_PIN 4
#define LCD_D6_PIN 3
#define LCD_D7_PIN 2

#define HEATER_PIN 11

#define BTN_PLUS A4
#define BTN_MINUS A3
#define BTN_ENTER A5

// Config to store on eeprom
struct MyConfig {
	int temperature;
	unsigned long motor;
	unsigned long saved;
};
struct MyConfig eepromConfig = (MyConfig){ 35, 0, 0};

// Thermistor
thermistor thermistor1(THERMISTOR_PIN, 60);	// 60 or 11
int temperatureDesired = 35;
int temperatureCurrent = 0;
int heaterValue = 0;
bool heaterOn = false;

arc::PID<double> temperaturePid(1.0, 0.0, 0.0);

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

byte customOff[] = {
	0x04,
	0x0A,
	0x0A,
	0x0A,
	0x0A,
	0x11,
	0x1F,
	0x0E
};

byte customOn[] = {
	0x04,
	0x0E,
	0x0E,
	0x0E,
	0x0E,
	0x1F,
	0x1F,
	0x0E
};

byte customMotorOff[] = {
	0x00,
	0x00,
	0x04,
	0x1F,
	0x11,
	0x11,
	0x1F,
	0x00
};

byte customMotorOn[] = {
	0x00,
	0x00,
	0x04,
	0x1F,
	0x1D,
	0x17,
	0x1F,
	0x00
};

// Button
Button btn_enter(BTN_ENTER, 3000);
Button btn_minus(BTN_MINUS);
Button btn_plus(BTN_PLUS);

int buttonIndicatorLocation = 0;

// Motor
byte directionPin = 10;
byte stepPin = 9;
int numberOfSteps = 100;
unsigned long step_last_pulse = 0;
unsigned long step_speed_pulse = 0;
bool motorOn = false;

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
	// lcd.setCursor(2, 0);
	lcd.setCursor(1, 0);
	lcd.print(" ");
	if(heaterOn) {
		lcd.write(byte(2));
	}
	else {
		lcd.write(byte(3));
	}

	lcd.print(" ");
	lcd.print(temperatureCurrent);
	// if(temperatureCurrent >= 100) 
	// 	lcd.setCursor(5, 0);
	// else 
	// 	lcd.setCursor(4, 0);
	// lcd.write(byte(1));

	// if(temperatureCurrent >= 100) 
	// 	lcd.setCursor(10, 0);
	// else 
	// 	lcd.setCursor(11, 0);
	
	lcd.print("/");
	lcd.print(temperatureDesired);
	lcd.write(byte(1));
	// lcd.setCursor(13, 0);
	// lcd.write(byte(1));

	// On/Off
	// lcd.setCursor(15, 0);
	// if(heaterOn) {
	// 	lcd.write(byte(2));
	// }
	// else {
	// 	lcd.write(byte(3));
	// }



	// Linha 2
	lcd.setCursor(1, 1);
	lcd.print(" ");
	if(motorOn) {
		lcd.write(byte(5));
	}
	else {
		lcd.write(byte(4));
	}
	lcd.print(" ");
	lcd.print(step_speed_pulse);
	

	// Store time as last refresh
	lcd_last_refresh = millis();
}


/**
 * Control temperature PID
 */

unsigned char ledValue = 0;
void temperaturePID()
{
	// Verify if heater is on
	if(!heaterOn) {
		analogWrite(HEATER_PIN, 0);
		return;
	}

	// if(temperatureCurrent < temperatureDesired) {
	// 	digitalWrite(HEATER_PIN, HIGH);
	// }
	// else {
	// 	digitalWrite(HEATER_PIN, LOW);
	// }


	temperaturePid.setTarget(temperatureDesired);
	temperaturePid.setInput(temperatureCurrent);
	heaterValue = min(255,max(0,heaterValue + temperaturePid.getOutput()));
	analogWrite(HEATER_PIN, heaterValue);
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
	lcd.createChar(2, customOn);
	lcd.createChar(3, customOff);
	lcd.createChar(4, customMotorOff);
	lcd.createChar(5, customMotorOn);
	lcd.clear();

	//
	EEPROM.get(0x0, eepromConfig);
	if(eepromConfig.saved == 1) {
		step_speed_pulse = eepromConfig.motor;
		temperatureDesired = eepromConfig.temperature;
	}
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
	else if (btn1 == -1) {
		if(buttonIndicatorLocation == 1) {
			motorOn = !motorOn;
		}
		else if(buttonIndicatorLocation == 0) {
			heaterOn = !heaterOn;
		}
		
		eepromConfig.motor = step_speed_pulse;
		eepromConfig.temperature = temperatureDesired;
		eepromConfig.saved = 1;

		EEPROM.put(0x0, eepromConfig);
	}

	// Handle button UP
	short btn2 = btn_plus.checkPress();
	if (btn2 == 1) {
		if(buttonIndicatorLocation == 0) {// Temperature
			temperatureDesired++;
		}
		else if(buttonIndicatorLocation == 1) {// Motor
			step_speed_pulse += 25;
		}
	} 

	// Handle button DOWN
	short btn3 = btn_minus.checkPress();
	if (btn3 == 1) {
		if(buttonIndicatorLocation == 0) {// Temperature
			temperatureDesired--;
		}
		else if(buttonIndicatorLocation == 1) { // Motor
			step_speed_pulse -= 25;
			if(step_speed_pulse < 0) {
				step_speed_pulse = 0;
			}
		}
	}

	// Refresh screen
	if((millis() - lcd_last_refresh) > 100) {
		refreshLCD();
	}
}

/**
 

260


 */
