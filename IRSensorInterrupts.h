#pragma once

typedef void(*InterruptFunction)();

typedef struct
{
	int digitalPin;
	unsigned long interruptTime;
	InterruptFunction interruptFunction;
} IRSensorPinInfo;

#define IRSENSOR1 0
#define IRSENSOR2 1
#define IRSENSOR3 1
#define IRSENSOR4 1
