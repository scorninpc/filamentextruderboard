/**
 * arduino-cli compile --fqbn arduino:avr:uno code
 * arduino-cli upload -p /dev/ttyUSB0  --fqbn arduino:avr:uno code
 */

#include <LiquidCrystal.h>
#include <PID.h>
#include <EEPROM.h>
#include "Button.h"
#include <AccelStepper.h>


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
int temperatureDesired = 35;
int temperatureCurrent = 0;
int heaterValue = 0;
bool heaterOn = false;

arc::PID<double> temperaturePid(1.0, 0.05, 0.25);

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
Button btn_enter(BTN_ENTER, 1000);
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

int motorSpeed = 1000;
AccelStepper stepper = AccelStepper(1, stepPin, directionPin);

/**
 * Refresh LCD screen
 */
void refreshLCD()
{
	lcd.clear();

	// Set indicator
	lcd.setCursor(0, buttonIndicatorLocation);
	lcd.write(byte(0));

	// Line 1
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

	lcd.print("/");
	lcd.print(temperatureDesired);
	lcd.write(byte(1));

	// Line 2
	lcd.setCursor(1, 1);
	lcd.print(" ");
	if(motorOn) {
		lcd.write(byte(5));
	}
	else {
		lcd.write(byte(4));
	}
	lcd.print(" ");
	// lcd.print(step_speed_pulse);
	lcd.print(motorSpeed);
	

	// Store time as last refresh
	lcd_last_refresh = millis();
}


/**
 * Control temperature PID
 */

unsigned char ledValue = 0;
void temperaturePID()
{
	// Control PID
	temperaturePid.setTarget(temperatureDesired);
	temperaturePid.setInput(temperatureCurrent);
	heaterValue = min(255,max(0,heaterValue + temperaturePid.getOutput()));
	
	// Verify if heater is on
	if(!heaterOn) {
		analogWrite(HEATER_PIN, 0);
		return;
	}
	else {
		analogWrite(HEATER_PIN, heaterValue);
	}
}

/**
 * Setup
 */
void setup()
{
	

	// Heater
	pinMode(HEATER_PIN, OUTPUT);
	digitalWrite(HEATER_PIN, LOW);

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
		motorSpeed = eepromConfig.motor;
		temperatureDesired = eepromConfig.temperature;
	}


	// Step motor
	stepper.setMaxSpeed(1500);
}


#define NUMTEMPS 20
short temptable[NUMTEMPS][2] = {
   {1, 841},
   {54, 255},
   {107, 209},
   {160, 184},
   {213, 166},
   {266, 153},
   {319, 142},
   {372, 132},
   {425, 124},
   {478, 116},
   {531, 108},
   {584, 101},
   {637, 93},
   {690, 86},
   {743, 78},
   {796, 70},
   {849, 61},
   {902, 50},
   {955, 34},
   {1008, 3}
};
int rawtemp = 0;
int current_celsius = 0;
byte i = 0;
unsigned long temperature_last_fetch = 0;



#define SERIESRESISTOR 4700   

/**
 * Loop
 */
void loop()
{
	// Store current temperature and Control temp PID
	rawtemp = rawtemp + analogRead(THERMISTOR_PIN) / 2;
	if((millis() - temperature_last_fetch) > 50) {
		i++;
		if(i<NUMTEMPS) {
			rawtemp = analogRead(THERMISTOR_PIN);
			if (temptable[i][0] > rawtemp) {
				temperatureCurrent  = (int)temptable[i-1][1] + (rawtemp - temptable[i-1][0]) * (temptable[i][1] - temptable[i-1][1]) / (temptable[i][0] - temptable[i-1][0]);
				i=0;
				rawtemp = 0;
			}
		}
		else {
			i=0;
		}

		// Store time as last refresh
		temperature_last_fetch = millis();
	}
	temperaturePID();


	// Pulse motor
	if(motorOn) {
		stepper.runSpeed();
	}

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
			stepper.setSpeed(motorSpeed);
		}
		else if(buttonIndicatorLocation == 0) {
			heaterOn = !heaterOn;
			stepper.setSpeed(motorSpeed);
		}
		
		eepromConfig.motor = motorSpeed;
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
			motorSpeed+=50;
			stepper.setSpeed(motorSpeed);
		}
	} 

	// Handle button DOWN
	short btn3 = btn_minus.checkPress();
	if (btn3 == 1) {
		if(buttonIndicatorLocation == 0) {// Temperature
			temperatureDesired--;
		}
		else if(buttonIndicatorLocation == 1) { // Motor
			motorSpeed-=50;
			if(motorSpeed < 0) {
				motorSpeed = 0;
			}

			stepper.setSpeed(motorSpeed);
		}
	}

	// Refresh screen
	if((millis() - lcd_last_refresh) > 100) {
		refreshLCD();
	}
}