#include "InputTrigger.h"
#include "constants.hpp"
#include "tonuino.hpp"

#define INITAL_STATE B1111

int8_t digitalPinToPCINT(int8_t p) {
	return digitalPinToPCICR(p) ? ((8 * digitalPinToPCICRbit(p)) + digitalPinToPCMSKbit(p)) : NOT_AN_INTERRUPT;
}

InputTrigger::InputTrigger() : _state(INITAL_STATE)
{
}

void InputTrigger::begin()
{
	PCICR |= B00000100;// enable interrupt for the group
	PCMSK2 |= B11110000;
	//_state = INITAL_STATE;
	//get current state
	TriggerOnHigh(dfPlayer_busyPin, B0001);
	TriggerOnLow(buttonPausePin, B0010);
	//TriggerOnLow(buttonUpPin, B0100);
	//TriggerOnLow(buttonDownPin, B1000);
}

void InputTrigger::stop()
{
	PCICR &= ~B00000100;// disable interrupt for the group
}

TriggerEvent InputTrigger::GetEvent()
{
	if (TriggerOnHigh(dfPlayer_busyPin, B0001))
		return TriggerEvent::MP3;

	if (TriggerOnLow(buttonPausePin, B0010))
		return TriggerEvent::Pause;

	//if (TriggerOnLow(buttonUpPin, B0100))
	//	return TriggerEvent::Volume;

	//if (TriggerOnLow(buttonDownPin, B1000))
	//	return TriggerEvent::Volume;

	return TriggerEvent::None;
}

bool InputTrigger::IsTriggerChanged(uint8_t pin, int stateMask)
{
	return digitalRead(pin) != IsSet(stateMask);
}

void InputTrigger::ChangeState(int stateMask)
{
	if (IsSet(stateMask))
		_state &= ~stateMask;
	else
		_state |= stateMask;
}

bool InputTrigger::IsSet(int stateMask)
{
	return (_state & stateMask) != 0;
}

bool InputTrigger::TriggerOnHigh(uint8_t pin, int stateMask)
{
	if (!IsTriggerChanged(pin, stateMask))
		return false;
	ChangeState(stateMask);
	return IsSet(stateMask);
}

bool InputTrigger::TriggerOnLow(uint8_t pin, int stateMask)
{
	if (!IsTriggerChanged(pin, stateMask))
		return false;
	ChangeState(stateMask);
	return !IsSet(stateMask);	
}

#ifndef TESTBOARD
ISR(PCINT2_vect)
{
}
#else
// #if defined(PCINT0_vect)
//   ISR(PCINT0_vect) { changeInterrupt(0, PCINT_INPUT_PORT0); }
// #endif
#if defined(PCINT1_vect)
ISR(PCINT1_vect) { changeInterrupt(1, PCINT_INPUT_PORT1); }
#endif
#if defined(PCINT2_vect)
ISR(PCINT2_vect) { changeInterrupt(2, PCINT_INPUT_PORT2); }
#endif
#if defined(PCINT3_vect)
ISR(PCINT3_vect) { changeInterrupt(3, PCINT_INPUT_PORT3); }
#endif
#endif
