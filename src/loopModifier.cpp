#include "loopModifier.h"
#include "tonuino.hpp"
#include "mp3.hpp"
#include "chip_card_v3.hpp"
#include "settings.hpp"
#include "constants.hpp"
#include "state_machine.hpp"

using namespace LoopModifier;

// Active
void Active::Loop(unsigned long timeout)
{
}

void Active::EndCycle(unsigned long startCycle)
{
	unsigned long stop_cycle = millis();
	if (stop_cycle - startCycle < cycleTime)
		delay(cycleTime - (stop_cycle - startCycle));
}

// KeyRead
void KeyRead::Loop(unsigned long timeout)
{
}

LoopModifierId KeyRead::GetTransition()
{
	return LoopModifierId::None;
}

void KeyRead::EndCycle(unsigned long startCycle)
{
	unsigned long stop_cycle = millis();
	if (stop_cycle - startCycle < cycleTime)
		delay(cycleTime - (stop_cycle - startCycle));
}

// CardRead
void CardRead::Loop(unsigned long timeout)
{
	cardSleepTimer.updateMillis(timeout);
	if (!cardSleepTimer.isExpired())
			return;

	card.wakeCard();
	SM_tonuino::dispatch(card_e(card.getCardEvent()));
	card.sleepCard();

	cardSleepTimer.start(cardSleep);
}

LoopModifierId CardRead::GetTransition()
{
	if (SM_tonuino::is_in_state<Play>())
		return LoopModifierId::LightSleep;
	return LoopModifierId::None;
}

void CardRead::EndCycle(unsigned long startCycle)
{
	tonuino.executeSleep();
}

//LigthSleep
void LightSleep::Loop(unsigned long timeout)
{
	SM_tonuino::dispatch(command_e(commandRaw::none));
}

LoopModifierId LightSleep::GetTransition()
{
	return LoopModifierId::None;
}

void LightSleep::EndCycle(unsigned long startCycle)
{
	tonuino.executeSleep();
}

//DeepSleep
LoopModifierId DeepSleep::GetTransition()
{
	return LoopModifierId::None;
}

void DeepSleep::HandleModifierChange(LoopModifierId newModifier)
{
}

void DeepSleep::EndCycle(unsigned long startCycle)
{
	tonuino.executeSleep();
}

//VeryDeepSleep
void VeryDeepSleep::Loop(unsigned long timeout)
{
}

void VeryDeepSleep::HandleModifierChange(LoopModifierId newModifier)
{
}

void VeryDeepSleep::EndCycle(unsigned long startCycle)
{
	tonuino.executeSleep();
}