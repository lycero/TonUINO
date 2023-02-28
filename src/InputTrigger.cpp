#include "InputTrigger.h"
#include "constants.hpp"
#include "tonuino.hpp"

int8_t digitalPinToPCINT(int8_t p) {
	return digitalPinToPCICR(p) ? ((8 * digitalPinToPCICRbit(p)) + digitalPinToPCMSKbit(p)) : NOT_AN_INTERRUPT;
}

InputTrigger::InputTrigger() : _state(1)
{
}

void InputTrigger::begin()
{
	//pinMode(dfPlayer_busyPin, INPUT_PULLUP);
	//digitalWrite(dfPlayer_busyPin, 1);
	//pinMode(buttonPausePin, INPUT);
	//pinMode(buttonUpPin, INPUT);
	//pinMode(buttonDownPin, INPUT);
	 
	//digitalWrite(buttonPausePin, 0); 
	//digitalWrite(buttonUpPin, 0);
	//digitalWrite(buttonDownPin, 0);

	PCICR |= B00000100;// enable interrupt for the group
	PCMSK2 |= B11110000;
}

void InputTrigger::stop()
{
	//byte pcIntNum = digitalPinToPCINT(buttonPausePin);
	//byte port = pcIntNum / 8;
	//byte bits = 0x01111000;
	//noInterrupts();

	//// Disable pins
	//portToPCMSK(port) &= ~(bits);
	//PCICR &= ~(1 << portToPCICRbit(port));

	//interrupts();

	PCICR &= ~B00000100;// disable interrupt for the group
	//PCMSK2 = B00000000;

}

TriggerEvent InputTrigger::GetEvent()
{
	if (TriggerOnLow(dfPlayer_busyPin, B0001))
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
//#if defined(PCINT0_vect)
//    ISR(PCINT0_vect) { changeInterrupt(0, PCINT_INPUT_PORT0); }
//#endif
//#if defined(PCINT1_vect)
//ISR(PCINT1_vect) {
//	Serial.println("PCI1");
//}
//#endif
//#if defined(PCINT2_vect)
//ISR(PCINT2_vect) {
//	Serial.println("PCI2");
//}
//#endif
//#if defined(PCINT3_vect)
//ISR(PCINT3_vect) { changeInterrupt(3, PCINT_INPUT_PORT3); }
//#endif
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
