#pragma once

#include "PinChangeInterruptHandler/PinChangeInterruptHandler.h"

class ChangeCounter : PinChangeInterruptHandler
{

public:
	ChangeCounter(byte pin);
	void begin();
	void stop();
	bool getRise();
	bool getFall();
	// Overwrite handlePCInterrupt() method
	virtual void handlePCInterrupt(int8_t interruptNum, bool value);
private:
	byte inputPin;
	byte inputMode;
	unsigned long riseCounter;
	unsigned long fallCounter;
};

