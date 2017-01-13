#include <Wire.h>
#include <Adafruit_LEDBackpack.h>
#include "SpeedoStateMachine.h"
#include "IRSensorInterrupts.h"

/******************************/
/* PIN ASSIGNMENTS            */
/******************************/

#define PIN_IRSENSOR1 2 // Digital pin that supports interrupts
#define PIN_IRSENSOR2 3 // Digital pin that supports interrupts

/******************************/
/* FORWARD DECLARATIONS       */
/******************************/

void handleIRSensor1();
void handleIRSensor2();
void displaySpeed();
void displayStateName(SpeedoState state);

/******************************/
/* GLOBALS                    */
/******************************/

volatile SpeedoState g_state = waitingForFirstSensor;
volatile SpeedoState g_previousState = Unknown;
volatile int g_lastTriggeredSensor = -1;

volatile double g_matrixSpeed = 10000.0; // Invalid, will print dashes

Adafruit_7segment g_matrix = Adafruit_7segment();

volatile IRSensorPinInfo g_sensorPins[2] =
{
	{ PIN_IRSENSOR1, 0, 0, handleIRSensor1 },
	{ PIN_IRSENSOR2, 0, 0, handleIRSensor2 },
};

/******************************/
/* SETUP                      */
/******************************/

void setup()
{
	Serial.begin(9600);

	g_matrix.begin(0x70);
	Serial.println("Attached LED g_matrix");

	for (int i = 0; i < 2; i++)
	{
		g_sensorPins[i].interruptTime = 0;
		pinMode(g_sensorPins[i].digitalPin, INPUT_PULLUP);
		digitalWrite(g_sensorPins[i].digitalPin, HIGH); // turn on the pullup
		attachInterrupt(digitalPinToInterrupt(g_sensorPins[i].digitalPin), g_sensorPins[i].interruptFunction, CHANGE);
		Serial.print("Attached IR SENSOR ");
		Serial.println(i);
	}
}

/******************************/
/* LOOP                       */
/******************************/

void loop()
{
	if (g_previousState != g_state)
	{
		Serial.print("Old state: ");
		displayStateName(g_previousState);
		Serial.print(" New state: ");
		displayStateName(g_state);
		Serial.println();

		g_previousState = g_state;

		switch (g_state)
		{
		case waitingForFirstSensor:
			Serial.println(g_matrixSpeed);
			g_matrix.printFloat(g_matrixSpeed, 2, DEC); // Invalid, prints dashes
			g_matrix.writeDisplay();
			break;

		case waitingForSecondSensor:
			g_matrix.writeDigitNum(0, 0);
			g_matrix.writeDigitNum(1, 0);
			g_matrix.writeDigitNum(3, 0);
			g_matrix.writeDigitNum(4, 0);
			g_matrix.writeDisplay();
			break;

		case showSpeed:
			displaySpeed();
			g_lastTriggeredSensor = -1;
			g_state = waitingForFirstSensor;
			break;
		}
	}
}


void handleIRSensor(int sensorPin)
{
	unsigned long currentTime = millis();
	unsigned long timeSinceLastBreak = currentTime - g_sensorPins[sensorPin].lastInterruptTime;

	if ((timeSinceLastBreak > 5000) &&
		(g_lastTriggeredSensor != sensorPin) &&
		((g_state == waitingForFirstSensor) || (g_state == waitingForSecondSensor))) {
		handleIRSensor(sensorPin);
		g_lastTriggeredSensor = sensorPin;

		g_sensorPins[sensorPin].interruptTime = millis();
		switch (g_state)
		{
		case waitingForFirstSensor:
			g_state = waitingForSecondSensor;
			break;

		case waitingForSecondSensor:
			g_state = showSpeed;
			break;
		}
	}

	g_sensorPins[sensorPin].lastInterruptTime = currentTime;
}

void handleIRSensor1()
{
	if (digitalRead(PIN_IRSENSOR1) == LOW) {
		handleIRSensor(PIN_IRSENSOR1);
	}
}

void handleIRSensor2()
{
	if (digitalRead(PIN_IRSENSOR1) == LOW) {
		handleIRSensor(PIN_IRSENSOR2);
	}
}

void displaySpeed()
{
	unsigned long startTime = 0;
	unsigned long endTime = 0;
	if (g_sensorPins[IRSENSOR1].interruptTime < g_sensorPins[IRSENSOR2].interruptTime)
	{
		startTime = g_sensorPins[IRSENSOR1].interruptTime;
		endTime = g_sensorPins[IRSENSOR2].interruptTime;
	}
	else
	{
		startTime = g_sensorPins[IRSENSOR2].interruptTime;
		endTime = g_sensorPins[IRSENSOR1].interruptTime;
	}

	double speedInchPerSecond = 10.0 / (((endTime - startTime) * 1.0) / 1000);
	double speedMilesPerHour = speedInchPerSecond * 0.0568181818;
	double speedNScale = speedMilesPerHour * 160.0;

	g_matrixSpeed = speedNScale;

	Serial.println("*******************************************");
	Serial.print("Start time: ");
	Serial.print(startTime);
	Serial.print(" End time: ");
	Serial.println(endTime);
	Serial.print("Inches/Second: ");
	Serial.println(speedInchPerSecond);
	Serial.print("Miles/Hour: ");
	Serial.println(speedMilesPerHour);
	Serial.print("Miles/Hour: ");
	Serial.print(speedNScale);
	Serial.println(" (N Scale)");
	Serial.println("*******************************************");
}

void displayStateName(SpeedoState state)
{
  switch (state)
  {
    case Unknown:
      Serial.print("Unknown");
      break;
    case waitingForFirstSensor:
      Serial.print("waitingForFirstSensor");
      break;
    case waitingForSecondSensor:
      Serial.print("waitingForSecondSensor");
      break;
    case showSpeed:
      Serial.print("showSpeed");
      break;

    default:
	  Serial.print("???");
	  break;
  }

  Serial.print(" (");
  Serial.print(state);
  Serial.print(")");
}
