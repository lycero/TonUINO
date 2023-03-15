#include "loopModifier.h"
#include "tonuino.hpp"
#include "mp3.hpp"
#include "chip_card_v3.hpp"
#include "settings.hpp"
#include "constants.hpp"
#include "state_machine.hpp"

using namespace LoopModifier;

Timer _delayTimer{};

// Active
void Active::Loop()
{
	tonuino.runActiveLoop();
}

void Active::EndCycle(unsigned long startCycle)
{
	unsigned long stop_cycle = millis();
	if (stop_cycle - startCycle < cycleTime)
		delay(cycleTime - (stop_cycle - startCycle));
}

// KeyRead

void KeyRead::Init()
{
	_lastCommand = commandRaw::none;
	_delayTimer.start(keyReadTimerDuration);
}

void KeyRead::Loop()
{
	_lastCommand = commands.getCommandRaw();
	SM_tonuino::dispatch(command_e(_lastCommand));
}

void KeyRead::UpdateTimer(unsigned long timeout) 
{
	_delayTimer.updateMillis(timeout); 
}

LoopModifierId KeyRead::GetTransition()
{
	if(_lastCommand == commandRaw::none && !_delayTimer.isExpired())
		return LoopModifierId::None;

	if(_lastCommand == commandRaw::pause)
		return LoopModifierId::CardRead;

	if (digitalRead(buttonPausePin))
		return LoopModifierId::None;

	if (digitalRead(buttonUpPin))
		return LoopModifierId::None;

	if (digitalRead(buttonDownPin))
		return LoopModifierId::None;

	if (_delayTimer.isExpired())
		return LoopModifierId::LightSleep;

	return LoopModifierId::None;
}

void KeyRead::EndCycle(unsigned long startCycle)
{
	unsigned long stop_cycle = millis();
	if (stop_cycle - startCycle < cycleTime)
		delay(cycleTime - (stop_cycle - startCycle));
}

// CardRead
void CardRead::Init()
{
	tonuino.SetSleepTimeout(cardReadSleepTime);
	_delayTimer.start(cardReadTimerDuration);
	//TODO change watchdog to 1s
	digitalWrite(cardPowerDownPin, HIGH);
	card.initCard();
}

void CardRead::Loop()
{
	mp3.loop();

	SM_tonuino::dispatch(command_e(commandRaw::none));

	if (!cardSleepTimer.isExpired())
			return;

	card.enableRFField();
	SM_tonuino::dispatch(card_e(card.getCardEvent()));
	card.disableRFField();

	cardSleepTimer.start(cardSleep);
}

void CardRead::UpdateTimer(unsigned long timeout) 
{
	cardSleepTimer.updateMillis(timeout);
	_delayTimer.updateMillis(timeout);
}

void CardRead::HandleModifierChange(LoopModifierId newModifier)
{
	digitalWrite(cardPowerDownPin, LOW);
}

LoopModifierId CardRead::GetTransition()
{
	if (SM_tonuino::is_in_state<StartPlay>())
		return LoopModifierId::BeginPlay;

	if (_delayTimer.isExpired())
		return LoopModifierId::DeepSleep;

	return LoopModifierId::None;
}

void CardRead::EndCycle(unsigned long startCycle)
{
	tonuino.executeSleep();
}

//LigthSleep
void LightSleep::Init()
{
	_delayTimer.start(lightSleepTimerDuration);
	tonuino.SetSleepTimeout(sleepCycleTime);
}

void LightSleep::Loop()
{

	mp3.loop();
	SM_tonuino::dispatch(command_e(commandRaw::none));
	if (mp3.isPlaying())
		_delayTimer.start(lightSleepTimerDuration);

	digitalWrite(dfPlayer_ampPin, !digitalRead(dfPlayer_headPhonePin));
}

void LightSleep::UpdateTimer(unsigned long timeout) 
{ 
	_delayTimer.updateMillis(timeout); 
}

LoopModifierId LightSleep::GetTransition()
{
	if (_delayTimer.isExpired())
		return LoopModifierId::DeepSleep;
	return LoopModifierId::None;
}

void LightSleep::EndCycle(unsigned long start_cycle)
{
	unsigned long stop_cycle = millis();
	if (stop_cycle - start_cycle < cycleTime)
		delay(cycleTime - (stop_cycle - start_cycle));
	tonuino.executeSleep();
}

//DeepSleep
void DeepSleep::Init()
{
	digitalWrite(dfPlayer_ampPin, 1);
	tonuino.SetSleepTimeout(deepSleepCycleTime);
	_delayTimer.start(deepSleepTimerDuration);
}

void DeepSleep::Loop()
{
	SM_tonuino::dispatch(command_e(commandRaw::none));
	if (mp3.isPausing() || mp3.isPlaying())
		_delayTimer.start(deepSleepTimerDuration);
}

void DeepSleep::UpdateTimer(unsigned long timeout) { _delayTimer.updateMillis(timeout); }

LoopModifierId DeepSleep::GetTransition()
{
	if (mp3.isPlaying())
		return LoopModifierId::LightSleep;
	if (!mp3.isPausing() && _delayTimer.isExpired())
		return LoopModifierId::VeryDeepSleep;
	return LoopModifierId::None;
}

void DeepSleep::HandleModifierChange(LoopModifierId newModifier)
{
	if (newModifier == LoopModifierId::VeryDeepSleep)
		return;

	digitalWrite(dfPlayer_ampPin, 0);
}

void DeepSleep::EndCycle(unsigned long startCycle)
{
	tonuino.executeSleep();
}

//VeryDeepSleep
void VeryDeepSleep::Init()
{
	tonuino.ChangeTriggerMode(true);
	tonuino.Mp3ShutDown();
}

void VeryDeepSleep::Loop()
{
}

void VeryDeepSleep::HandleModifierChange(LoopModifierId newModifier)
{
	wdt_reset();
	tonuino.ChangeTriggerMode(false);
	tonuino.Mp3Init();
	SM_tonuino::dispatch(command_e(commandRaw::start));
}

void VeryDeepSleep::EndCycle(unsigned long startCycle)
{
	tonuino.executeSleep(true);
}

LoopModifierId VeryDeepSleep::GetTransition()
{
	return LoopModifierId::None;
}

// BeginPlay

void BeginPlay::Loop()
{
	mp3.loop();

	SM_tonuino::dispatch(command_e(commandRaw::none));
}

LoopModifierId BeginPlay::GetTransition()
{
	if (SM_tonuino::is_in_state<Play>())
		return LoopModifierId::LightSleep;

	return LoopModifierId::None;
}

void BeginPlay::EndCycle(unsigned long startCycle)
{
	unsigned long stop_cycle = millis();
	if (stop_cycle - startCycle < cycleTime)
		delay(cycleTime - (stop_cycle - startCycle));
}
