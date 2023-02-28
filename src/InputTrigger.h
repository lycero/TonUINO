#pragma once

#include <Arduino.h>

enum class TriggerEvent 
{
	None,
	MP3,
	Pause,
	Volume
};

class InputTrigger
{

public:
	InputTrigger();
	void begin();
	void stop();
	TriggerEvent GetEvent();
private:
	bool IsTriggerChanged(uint8_t pin, int stateMask);
	void ChangeState(int stateMask);
	bool IsSet(int stateMask);
	bool TriggerOnHigh(uint8_t pin, int stateMaskt);
	bool TriggerOnLow(uint8_t pin, int stateMask);
	uint8_t _state { 0 };
};

