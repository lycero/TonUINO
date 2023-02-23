#pragma once

#include <Arduino.h>

#include "chip_card_v3.hpp"
#include "logger.hpp"
#include "timer.hpp"
#include "commands.hpp"

class Tonuino;
class Mp3;
class Settings;
class Chip_card;

namespace LoopModifier {

	enum class LoopModifierId {
		None,
		Active,
		KeyRead,
		CardRead,
		LightSleep,
		DeepSleep,
		VeryDeepSleep
	};

	class LoopModifier {
	public:
		LoopModifier(Tonuino& tonuino, const Settings& settings) : tonuino(tonuino), settings(settings) {}
		virtual void Loop() {}
		virtual LoopModifierId GetTransition() { return LoopModifierId::None; }
		virtual LoopModifierId GetModifierId() = 0;
		virtual void EndCycle(unsigned long startCycle) { }
		virtual void HandleModifierChange(LoopModifierId newModifier) { }
		virtual void Init() {}
		virtual void UpdateTimer(unsigned long timeout) {}
		LoopModifier& operator=(const LoopModifier&) = delete;
	protected:
		Tonuino& tonuino;
		const Settings& settings;
	};

	class Active : public LoopModifier {
	public:
		Active(Tonuino& tonuino, Mp3& mp3, const Settings& settings) : LoopModifier(tonuino, settings), instance(tonuino) {}
		LoopModifierId GetModifierId() final { return LoopModifierId::Active; }
		void Loop() final;
		void EndCycle(unsigned long startCycle) final;

	private:
		Tonuino& instance;
	};

	class KeyRead : public LoopModifier {
	public:
		KeyRead(Tonuino& tonuino, Commands& commands, const Settings& settings) : LoopModifier(tonuino, settings), commands(commands) {}
		void Init() final;
		LoopModifierId GetModifierId() final { return LoopModifierId::KeyRead; }
		void Loop() final;
		void UpdateTimer(unsigned long timeout) final;;
		LoopModifierId GetTransition() final;
		void EndCycle(unsigned long startCycle) final;

	private:
		Commands& commands;
		commandRaw _lastCommand{ commandRaw::none };		
	};

	class CardRead : public LoopModifier {
	public:
		CardRead(Tonuino& tonuino, Chip_card& card, const Settings& settings) : LoopModifier(tonuino, settings), card(card) {}
		LoopModifierId GetModifierId() final { return LoopModifierId::CardRead; }
		void Init() final;
		void Loop() final;
		void UpdateTimer(unsigned long timeout) final;;
		LoopModifierId GetTransition() final;
		void EndCycle(unsigned long startCycle) final;
	private:
		Chip_card& card;
		Timer cardSleepTimer{};
	};

	class LightSleep : public LoopModifier {
	public:
		LightSleep(Tonuino& tonuino, Mp3& mp3, const Settings& settings) : LoopModifier(tonuino, settings), mp3(mp3) {}
		LoopModifierId GetModifierId() final { return LoopModifierId::LightSleep; }
		void Init() final;
		void Loop() final;
		void UpdateTimer(unsigned long timeout) final;;
		LoopModifierId GetTransition() final;
		void EndCycle(unsigned long startCycle) final;
	private:
		Mp3& mp3;
	};

	class DeepSleep : public LoopModifier {
	public:
		DeepSleep(Tonuino& tonuino, Mp3& mp3, const Settings& settings) : LoopModifier(tonuino, settings), mp3(mp3) {}
		LoopModifierId GetModifierId() final { return LoopModifierId::DeepSleep; }
		void Init() final;
		void Loop() final;
		void UpdateTimer(unsigned long timeout) final;;
		LoopModifierId GetTransition() final;
		void HandleModifierChange(LoopModifierId newModifier) final;
		void EndCycle(unsigned long startCycle) final;
	private:
		Mp3& mp3;
	};

	class VeryDeepSleep : public LoopModifier {
	public:
		VeryDeepSleep(Tonuino& tonuino, Mp3& mp3, const Settings& settings) : LoopModifier(tonuino, settings), mp3(mp3) {}
		LoopModifierId GetModifierId() final { return LoopModifierId::VeryDeepSleep; }
		void Init() final;
		void Loop() final;
		void HandleModifierChange(LoopModifierId newModifier) final;
		void EndCycle(unsigned long startCycle) final;
	private:
		Mp3& mp3;
	};



}