#pragma once

#include "PinChangeInterruptHandler/PinChangeInterruptHandler.h"

class ChangeCounter : PinChangeInterruptHandler
{

public:
	ChangeCounter(uint8_t pin);
	void begin();
	void stop();
	bool getRise();
	bool getFall();
	// Overwrite handlePCInterrupt() method
	virtual void handlePCInterrupt(int8_t interruptNum, bool value);
private:
	uint8_t inputPin;
	uint8_t riseCounter;
	uint8_t fallCounter;
};

