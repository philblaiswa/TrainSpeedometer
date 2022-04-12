//#include <Wire.h>
#include <Adafruit_LEDBackpack.h>
#include "SpeedoStateMachine.h"
#include "IRSensorInterrupts.h"

/******************************/
/* PIN ASSIGNMENTS            */
/******************************/

// Arduino UNO has only 2 pins that support interrupts: 2 and 3
#define PIN_IRSENSOR1 2 // Digital pin that supports interrupts
#define PIN_IRSENSOR2 3 // Digital pin that supports interrupts

#define DISTANCE_BETWEEN_SENSORS 10.0 // Distance between sensors in inches
#define SCALE 160                     // Scale N=160, HO=87

/******************************/
/* FORWARD DECLARATIONS       */
/******************************/

void handleIRSensor1();
void handleIRSensor2();
void displaySpeed();
void displayStateName(int state);

/******************************/
/* GLOBALS                    */
/******************************/

int g_state = SPEEDOSTATE_WAITING_FIRST_SENSOR;
int g_previousState = SPEEDOSTATE_UNKNOWN;
int g_lastTriggeredSensor = -1;
unsigned long g_cooloffStarted = 0;

double g_matrixSpeed = 10000.0; // Invalid, will print dashes

Adafruit_7segment g_matrix = Adafruit_7segment();

IRSensorPinInfo g_sensorPins[2] =
{
	{ PIN_IRSENSOR1, 0, handleIRSensor1 },
	{ PIN_IRSENSOR2, 0, handleIRSensor2 },
};

/******************************/
/* SETUP                      */
/******************************/

void setup()
{
	Serial.begin(9600);

  Serial.println("Welcome to the model train Speedometer");
  Serial.print("Configured scale: 1:");
  Serial.println(SCALE);
  Serial.print("Configured distance betwen sensors (inches):");
  Serial.println(DISTANCE_BETWEEN_SENSORS);
  Serial.println();

  // This starts the LED 7 segment LED matrix
  g_matrix.begin(0x70);
	Serial.println("Attached LED matrix");

  // Setup interrupt handlers for IR sensors
	for (int i = 0; i < 2; i++)
	{
		g_sensorPins[i].interruptTime = 0;
		pinMode(g_sensorPins[i].digitalPin, INPUT_PULLUP);
		digitalWrite(g_sensorPins[i].digitalPin, HIGH); // turn on the pullup
		attachInterrupt(digitalPinToInterrupt(g_sensorPins[i].digitalPin), g_sensorPins[i].interruptFunction, CHANGE);
		Serial.print("Attached IR SENSOR ");
		Serial.print(i);
    Serial.print(" on PIN ");
    Serial.println(g_sensorPins[i].digitalPin);
	}

  Serial.println("Ready to detect train traffic");
  Serial.println();
}

/******************************/
/* LOOP                       */
/******************************/

void loop()
{
	if (g_previousState != g_state || g_state == SPEEDOSTATE_COOLOFF) {
    if (g_previousState != g_state) {
		  Serial.print("Old state: ");
		  displayStateName(g_previousState);
		  Serial.print(" New state: ");
		  displayStateName(g_state);
		  Serial.println();
    }
		g_previousState = g_state;

		switch (g_state)
		{
		case SPEEDOSTATE_WAITING_FIRST_SENSOR:
      g_matrix.printFloat(10000.0, 2, DEC); // Invalid number shows dashes
      g_matrix.writeDisplay();
			break;

		case SPEEDOSTATE_WAITING_SECOND_SENSOR:
			g_matrix.writeDigitNum(0, 0);
			g_matrix.writeDigitNum(1, 0);
			g_matrix.writeDigitNum(3, 0);
			g_matrix.writeDigitNum(4, 0);
			g_matrix.writeDisplay();
			break;

		case SPEEDOSTATE_SHOW_SPEED:
			displaySpeed();
			g_lastTriggeredSensor = -1;
      g_cooloffStarted = millis();
			g_state = SPEEDOSTATE_COOLOFF;
			break;

    case SPEEDOSTATE_COOLOFF:
      if ((millis() - g_cooloffStarted)> 15000) {
        g_cooloffStarted = 0;
        g_matrixSpeed = 10000.0; // Invalid speed
        g_state = SPEEDOSTATE_WAITING_FIRST_SENSOR;
      }
      break;
		}
	}
}


bool canProcessIRInterrupt(int sensorId)
{
  return g_lastTriggeredSensor != sensorId && (g_state == SPEEDOSTATE_WAITING_FIRST_SENSOR || g_state == SPEEDOSTATE_WAITING_SECOND_SENSOR);

}

void handleIRSensor(int sensorId)
{
	unsigned long currentTime = millis();
	unsigned long timeSinceLastBreak = currentTime - g_sensorPins[sensorId].interruptTime;

  // Hardware interrupts will fire as fast as the board can support
  // To limit this, we use a 5 second coolloff for each specific sensor
	if (timeSinceLastBreak > 5000 && canProcessIRInterrupt(sensorId)) {
		g_lastTriggeredSensor = sensorId;
		g_sensorPins[sensorId].interruptTime = currentTime;
		
    switch (g_state)
		{
		case SPEEDOSTATE_WAITING_FIRST_SENSOR:
      g_state = SPEEDOSTATE_WAITING_SECOND_SENSOR;
      break;
      
    case SPEEDOSTATE_WAITING_SECOND_SENSOR:
      g_state = SPEEDOSTATE_SHOW_SPEED;
			break;
		}
	}
}

void handleIRSensor1()
{
	if (digitalRead(PIN_IRSENSOR1) == LOW) {
		handleIRSensor(0);
	}
}

void handleIRSensor2()
{
	if (digitalRead(PIN_IRSENSOR2) == LOW) {
		handleIRSensor(1);
	}
}

void displaySpeed()
{
	unsigned long startTime = 0;
	unsigned long endTime = 0;
	if (g_sensorPins[0].interruptTime < g_sensorPins[1].interruptTime)
	{
		startTime = g_sensorPins[0].interruptTime;
		endTime = g_sensorPins[1].interruptTime;
	}
	else
	{
		startTime = g_sensorPins[1].interruptTime;
		endTime = g_sensorPins[0].interruptTime;
	}
  Serial.println("*******************************************");
  
  Serial.print("Distance (inches): ");
  Serial.println(DISTANCE_BETWEEN_SENSORS);
  Serial.print("Scale            : 1:");
  Serial.println(SCALE);

  Serial.print("Start time       : ");
  Serial.println(startTime);
  
  Serial.print("End time         : ");
  Serial.println(endTime);

  double elapsed = ((endTime - startTime) * 1.0) / 1000.0;
  Serial.print("Elapsed (s)      : ");
  Serial.println(elapsed);

  double miles = DISTANCE_BETWEEN_SENSORS * 1.0 / 63360.0; // Miles
  double hours = elapsed / 3600.0; // Hours
  double mph = miles / hours;
  Serial.print("Actual MPH       : ");
  Serial.println(mph);

  double scaleMph = mph * SCALE;
  Serial.print("Scale MPH        : ");
  Serial.println(scaleMph);
  
  Serial.println("-------------------------------------------");

  g_matrixSpeed = scaleMph;
  g_matrix.printFloat(g_matrixSpeed, 2, DEC);
  g_matrix.writeDisplay();
}

void displayStateName(int state)
{
  Serial.print("(");
  Serial.print(state);
  Serial.print(") ");

  switch (state)
  {
    case SPEEDOSTATE_UNKNOWN:
      Serial.print("SPEEDOSTATE_UNKNOWN");
      break;
    case SPEEDOSTATE_WAITING_FIRST_SENSOR:
      Serial.print("SPEEDOSTATE_WAITING_FIRST_SENSOR");
      break;
    case SPEEDOSTATE_WAITING_SECOND_SENSOR:
      Serial.print("SPEEDOSTATE_WAITING_SECOND_SENSOR");
      break;
    case SPEEDOSTATE_SHOW_SPEED:
      Serial.print("SPEEDOSTATE_SHOW_SPEED");
      break;
    case SPEEDOSTATE_COOLOFF:
      Serial.print("SPEEDOSTATE_COOLOFF");
      break;

    default:
	    Serial.print("???");
	    break;
  }
}
