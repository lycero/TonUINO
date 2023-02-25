#include "ChangeCounter.h"
#include "tonuino.hpp"

ChangeCounter::ChangeCounter(uint8_t pin)
	: riseCounter(0)
	, fallCounter(0)
	, inputPin(pin)
{
}

void ChangeCounter::begin() {
	pinMode(inputPin, INPUT);
	riseCounter = 0;
	fallCounter = 0;
	attachPCInterrupt(digitalPinToPCINT(inputPin));
}

void ChangeCounter::stop() {
	detachPCInterrupt(digitalPinToPCINT(inputPin));
}

bool ChangeCounter::getRise() {
	bool res = riseCounter > 0;
	riseCounter = 0;
	return res;
}

bool ChangeCounter::getFall() {
	bool res = fallCounter > 0;
	fallCounter = 0;
	return res;
}

// Overwrite handlePCInterrupt() method
void ChangeCounter::handlePCInterrupt(int8_t interruptNum, bool value) {
	if (digitalPinToPCINT(inputPin) != interruptNum)
		return;

	if (value)
		++riseCounter;
	else
		++fallCounter;

	Tonuino::getTonuino().keepAwake();
}