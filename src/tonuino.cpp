#include "tonuino.hpp"

#include <Arduino.h>

#include "array.hpp"
#include "chip_card_v3.hpp"

#include "constants.hpp"
#include "logger.hpp"
#include "state_machine.hpp"

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

namespace
{

	const __FlashStringHelper* str_bis() { return F(" bis "); }

	void ChangeWatchDog(uint8_t time)
	{
	}

	void SetupWatchDog(uint8_t time)
	{
		cli();
		wdt_reset();
		MCUSR &= ~(1 << WDRF);
		WDTCSR = (1 << WDCE) | (1 << WDE);
		WDTCSR = time;
		WDTCSR |= 1 << WDIE | (0 << WDE);
		sei();
	}

} // anonymous namespace

void Tonuino::setup()
{
	SetupWatchDog(sleepCycleTime);

	//pinMode(dfPlayer_ampPin, OUTPUT);
	//digitalWrite(dfPlayer_ampPin, 0);
	// load Settings from EEPROM
	settings.loadSettingsFromFlash();

	// NFC Leser initialisieren
	delay(25);
	//chip_card.initCard();

	digitalWrite(dfPlayer_powerPin, 1);
	delay(25);
	// DFPlayer Mini initialisieren
	mp3.begin();
	// // Zwei Sekunden warten bis der DFPlayer Mini initialisiert ist
	delay(1500);
	digitalWrite(dfPlayer_ampPin, 0);
	mp3.setVolume();
	mp3.setEq(static_cast<DfMp3_Eq>(settings.eq - 1));

	// RESET --- ALLE DREI KNÖPFE BEIM STARTEN GEDRÜCKT HALTEN -> alle EINSTELLUNGEN werden gelöscht
	if (buttons.isReset())
	{
		settings.clearEEPROM();
		settings.loadSettingsFromFlash();
	}

	SM_tonuino::start();
	// Start Shortcut "at Startup" - e.g. Welcome Sound
	SM_tonuino::dispatch(command_e(commandRaw::start));

	sleepStateTimer.start(awakeTime);
	cardSleepTimer.start(cardSleep);

	delay(200);
	mp3.enqueueTrack(6,2);
	mp3.enqueueTrack(6,1);
	mp3.enqueueTrack(6,2);
	mp3.enqueueTrack(6,1);
	mp3.enqueueTrack(6, 2);
	mp3.enqueueTrack(6, 1);
	mp3.enqueueTrack(6,3);
	mp3.enqueueTrack(6, 2);
	mp3.enqueueTrack(6, 1);
	mp3.playCurrent();
}

void Tonuino::loop(WakeupSource source)
{
	ChangePowerState(source);

	unsigned long start_cycle = millis();

	loopMp3();

	checkInputs();

	//checkNfc();

	UpdatePowerState(start_cycle);
}

void Tonuino::playFolder()
{
	LOG(play_log, s_debug, F("playFolder"));
	numTracksInFolder = mp3.getFolderTrackCount(myFolder->folder);
	LOG(play_log, s_info, numTracksInFolder, F(" files in folder "), myFolder->folder);
	numTracksInFolder = min(numTracksInFolder, 0xffu);
	mp3.clearAllQueue();

	switch (myFolder->mode)
	{

	case mode_t::hoerspiel:
		// Hörspielmodus: eine zufällige Datei aus dem Ordner
		myFolder->special = 1;
		myFolder->special2 = numTracksInFolder;
		__attribute__((fallthrough));
		/* no break */
	case mode_t::hoerspiel_vb:
		// Spezialmodus Von-Bin: Hörspiel: eine zufällige Datei aus dem Ordner
		LOG(play_log, s_info, F("Hörspiel"));
		LOG(play_log, s_info, myFolder->special, str_bis(), myFolder->special2);
		mp3.enqueueTrack(myFolder->folder, random(myFolder->special, myFolder->special2 + 1));
		break;

	case mode_t::album:
		// Album Modus: kompletten Ordner spielen
		myFolder->special = 1;
		myFolder->special2 = numTracksInFolder;
		__attribute__((fallthrough));
		/* no break */
	case mode_t::album_vb:
		// Spezialmodus Von-Bis: Album: alle Dateien zwischen Start und Ende spielen
		LOG(play_log, s_info, F("Album"));
		LOG(play_log, s_info, myFolder->special, str_bis(), myFolder->special2);
		mp3.enqueueTrack(myFolder->folder, myFolder->special, myFolder->special2);
		break;

	case mode_t::party:
		// Party Modus: Ordner in zufälliger Reihenfolge
		myFolder->special = 1;
		myFolder->special2 = numTracksInFolder;
		__attribute__((fallthrough));
		/* no break */
	case mode_t::party_vb:
		// Spezialmodus Von-Bis: Party Ordner in zufälliger Reihenfolge
		LOG(play_log, s_info, F("Party"));
		LOG(play_log, s_info, myFolder->special, str_bis(), myFolder->special2);
		mp3.enqueueTrack(myFolder->folder, myFolder->special, myFolder->special2);
		mp3.shuffleQueue();
		break;

	case mode_t::einzel:
		// Einzel Modus: eine Datei aus dem Ordner abspielen
		LOG(play_log, s_info, F("Einzel"));
		mp3.enqueueTrack(myFolder->folder, myFolder->special);
		break;

	case mode_t::hoerbuch:
	case mode_t::hoerbuch_1:
	{
		// Hörbuch Modus: kompletten Ordner spielen und Fortschritt merken (oder nur eine Datei)
		LOG(play_log, s_info, F("Hörbuch"));
		uint16_t startTrack = settings.readFolderSettingFromFlash(myFolder->folder);
		if (startTrack > numTracksInFolder)
			startTrack = 1;
		mp3.enqueueTrack(myFolder->folder, 1, numTracksInFolder, startTrack - 1);
	}
	break;

	default:
		break;
	}
}

void Tonuino::playTrackNumber()
{
	const uint8_t advertTrack = mp3.getCurrentTrack();
	if (advertTrack != 0)
		mp3.playAdvertisement(advertTrack);
}

// Leider kann das Modul selbst keine Queue abspielen, daher müssen wir selbst die Queue verwalten
void Tonuino::nextTrack(bool fromOnPlayFinished)
{
	LOG(play_log, s_info, F("nextTrack"));
	if (activeModifier->handleNext())
		return;
	if (mp3.isPlayingFolder() && (myFolder->mode == mode_t::hoerbuch || myFolder->mode == mode_t::hoerbuch_1))
	{
		const uint8_t trackToSave = (mp3.getCurrentTrack() < numTracksInFolder) ? mp3.getCurrentTrack() + 1 : 1;
		settings.writeFolderSettingToFlash(myFolder->folder, trackToSave);
		if (fromOnPlayFinished && myFolder->mode == mode_t::hoerbuch_1)
			mp3.clearFolderQueue();
	}
	mp3.playNext();
}

void Tonuino::previousTrack()
{
	LOG(play_log, s_info, F("previousTrack"));
	if (mp3.isPlayingFolder() && (myFolder->mode == mode_t::hoerbuch || myFolder->mode == mode_t::hoerbuch_1))
	{
		const uint8_t trackToSave = (mp3.getCurrentTrack() > numTracksInFolder) ? mp3.getCurrentTrack() - 1 : 1;
		settings.writeFolderSettingToFlash(myFolder->folder, trackToSave);
	}
	mp3.playPrevious();
}

void Tonuino::keepAwake() 
{
	wdt_reset();
	myKeepAwake++;
}

void Tonuino::OnPlayFinished(uint16_t track)
{
	mp3.OnPlayFinished(track);
}

bool Tonuino::specialCard(const nfcTagObject& nfcTag)
{
	LOG(card_log, s_debug, F("special card, mode = "), static_cast<uint8_t>(nfcTag.nfcFolderSettings.mode));
	if (activeModifier->getActive() == nfcTag.nfcFolderSettings.mode)
	{
		resetActiveModifier();
		LOG(card_log, s_info, F("modifier removed"));
		mp3.playAdvertisement(advertTracks::t_261_deactivate_mod_card, false /*olnyIfIsPlaying*/);
		return true;
	}
	const Modifier* oldModifier = activeModifier;

	switch (nfcTag.nfcFolderSettings.mode)
	{
	case mode_t::sleep_timer:
		LOG(card_log, s_info, F("act. sleepTimer"));
		mp3.playAdvertisement(advertTracks::t_302_sleep, false /*olnyIfIsPlaying*/);
		activeModifier = &sleepTimer;
		sleepTimer.start(nfcTag.nfcFolderSettings.special);
		break;
	case mode_t::freeze_dance:
		LOG(card_log, s_info, F("act. freezeDance"));
		mp3.playAdvertisement(advertTracks::t_300_freeze_into, false /*olnyIfIsPlaying*/);
		activeModifier = &freezeDance;
		;
		break;
	case mode_t::locked:
		LOG(card_log, s_info, F("act. locked"));
		mp3.playAdvertisement(advertTracks::t_303_locked, false /*olnyIfIsPlaying*/);
		activeModifier = &locked;
		break;
	case mode_t::toddler:
		LOG(card_log, s_info, F("act. toddlerMode"));
		mp3.playAdvertisement(advertTracks::t_304_buttonslocked, false /*olnyIfIsPlaying*/);
		activeModifier = &toddlerMode;
		break;
	case mode_t::kindergarden:
		LOG(card_log, s_info, F("act. kindergardenMode"));
		mp3.playAdvertisement(advertTracks::t_305_kindergarden, false /*olnyIfIsPlaying*/);
		activeModifier = &kindergardenMode;
		break;
	case mode_t::repeat_single:
		LOG(card_log, s_info, F("act. repeatSingleModifier"));
		mp3.playAdvertisement(advertTracks::t_260_activate_mod_card, false /*olnyIfIsPlaying*/);
		activeModifier = &repeatSingleModifier;
		break;
	default:
		return false;
	}
	if (oldModifier != activeModifier)
		activeModifier->init();
	return true;
}

void Tonuino::checkNfc()
{
	if (powerState > PowerState::LightSleep)
		return;

	if (!cardSleepTimer.isExpired())
		return;

	chip_card.wakeCard();
	SM_tonuino::dispatch(card_e(chip_card.getCardEvent()));
	chip_card.sleepCard();

	cardSleepTimer.start(cardSleep);
}

void Tonuino::checkInputs()
{
	if (powerState != PowerState::Active)
		return;

	SM_tonuino::dispatch(command_e(commands.getCommandRaw()));
}

void Tonuino::loopMp3()
{
	if (powerState == PowerState::VeryDeepSleep)
		return;

	mp3.loop();
	activeModifier->loop();
}

void Tonuino::ChangePowerState(WakeupSource source)
{
	//wdt_reset();
	switch (source)
	{
	case WakeupSource::None:
	{
		/* code */
		wdt_reset();
		break;
	}
	case WakeupSource::Watchdog:
	{
		//LOG(powerstate_log, s_debug, F("Watchdog:"), millis());
		auto wdmillis = getWatchDogMillis();
		sleepStateTimer.updateMillis(wdmillis);
		mp3.updateTimer(wdmillis);
		cardSleepTimer.updateMillis(wdmillis);
		break;
	}
	case WakeupSource::KeyInput:
	{
		commands.getCommandRaw();
		HandlePowerStateChange(PowerState::Active, powerState);
		break;
	}
	case WakeupSource::CardReader:
	{
		HandlePowerStateChange(PowerState::Active, powerState);
		break;
	}
	case WakeupSource::Mp3BusyChange:
	{
		wdt_reset();
		myKeepAwake++;

		if (powerState == PowerState::VeryDeepSleep)
		{
			HandlePowerStateChange(PowerState::DeepSleep, PowerState::VeryDeepSleep);
			sleepStateTimer.start(deepSleepTime);
		}

		//if (powerState != PowerState::Active && powerState != PowerState::Mp3Awake) 
		if(true)
		{
			if ((mp3.isPlayingFolder() || mp3.isPlayingMp3()) && !mp3.isPlaying()) 
			{
				if (powerState != PowerState::Active && powerState != PowerState::Mp3Awake)
					sleepStateTimer.start(deepSleepTime);

				mp3.OnPlayFinished(mp3.getLastPlayedTrack());
			}
		}

		if (powerState == PowerState::DeepSleep || powerState == PowerState::VeryDeepSleep)
		{
			sleepStateTimer.start(deepSleepTime);
			//HandlePowerStateChange(PowerState::Mp3Awake, powerState);
		}
		if (powerState == PowerState::Active || powerState == PowerState::LightSleep)
		{
			//HandlePowerStateChange(PowerState::Active, powerState);
		}
		break;
	}
	default:
		break;
	}
}

void Tonuino::UpdatePowerState(unsigned long startCycle)
{
	switch (powerState)
	{
	case PowerState::Active:
	{
		if (sleepStateTimer.isExpired())
		{
			HandlePowerStateChange(PowerState::LightSleep, powerState);
		}
		else
		{
			unsigned long stop_cycle = millis();
			if (stop_cycle - startCycle < cycleTime)
				delay(cycleTime - (stop_cycle - startCycle));
			return;
		}
		break;
	}
	case PowerState::LightSleep:
	{
		if (sleepStateTimer.isExpired())
		{
			HandlePowerStateChange(PowerState::DeepSleep, powerState);
		}
		break;
	}
	case PowerState::Mp3Awake:
	{
		if (sleepStateTimer.isExpired())
		{
			HandlePowerStateChange(PowerState::DeepSleep, powerState);
		}
		else
		{
			unsigned long stop_cycle = millis();
			if (stop_cycle - startCycle < cycleTime)
				delay(cycleTime - (stop_cycle - startCycle));
			return;
		}
		break;
	}
	case PowerState::DeepSleep:
		if (sleepStateTimer.isExpired() && !mp3.isPlaying())
		{
			HandlePowerStateChange(PowerState::VeryDeepSleep, powerState);
		}
		break;
	default:
		break;
	}

	if (isKeepAwake()) 
	{
		wdt_reset();
		unsigned long stop_cycle = millis();
		if (stop_cycle - startCycle < cycleTime)
			delay(cycleTime - (stop_cycle - startCycle));
	}
	else 
	{
		executeSleep();
	}
}

void Tonuino::HandlePowerStateChange(PowerState newState, PowerState oldState)
{
	if (newState == oldState) 
	{
		if (newState == PowerState::Active || newState == PowerState::Mp3Awake)
			sleepStateTimer.start(awakeTime);
		return;
	}
	powerState = newState;

	//wdt_reset();
	logPowerStateChange(newState, oldState);

	if (newState < oldState)
		HandlePowerWakup(newState, oldState);
	else
		HandlePowerGoSleep(newState, oldState);
}

void Tonuino::HandlePowerWakup(PowerState newState, PowerState oldState)
{
	if (oldState == PowerState::VeryDeepSleep)
	{
		// wakup mp3
		if (!mp3.isPausing()) 
		{
			mp3.wakeup();
			delay(10);
		}
		digitalWrite(dfPlayer_ampPin, 0);
	}

	if (newState == PowerState::Active)
	{
		sleepStateTimer.start(awakeTime);
	}
	if (newState == PowerState::Mp3Awake)
	{
		sleepStateTimer.start(awakeTime);
	}
}

void Tonuino::HandlePowerGoSleep(PowerState newState, PowerState oldState)
{
	if (newState == PowerState::VeryDeepSleep)
	{
		digitalWrite(dfPlayer_ampPin, 1);
		ChangeWatchDog(deepSleepCycleTime);
		// shutdown mp3
		if (!mp3.isPausing())
			mp3.goSleep();
	}
	if (newState == PowerState::LightSleep)
	{
		sleepStateTimer.start(lightSleepTime);
		ChangeWatchDog(sleepCycleTime);
	}
	if (newState == PowerState::DeepSleep)
	{
		sleepStateTimer.start(deepSleepTime);
		ChangeWatchDog(deepSleepCycleTime);
	}
}

void Tonuino::logPowerStateChange(PowerState newState, PowerState oldState)
{
	LOG(powerstate_log, s_debug, getPowerStateName(oldState), F(" -> "), getPowerStateName(newState));
	if (powerstate_log::will_log(s_debug))
		delay(50);
}

const __FlashStringHelper* Tonuino::getPowerStateName(PowerState state)
{
	switch (state)
	{
	case PowerState::Active:
		return F("Active");
	case PowerState::LightSleep:
		return F("LightSleep");
	case PowerState::Mp3Awake:
		return F("Mp3Awake");
	case PowerState::DeepSleep:
		return F("DeepSleep");
	case PowerState::VeryDeepSleep:
		return F("VeryDeepSleep");
	}
	return F("unknown");
}

bool Tonuino::isKeepAwake()
{
	if (myKeepAwake) 
	{
		myKeepAwake--;
		return true;
	}
	return false;
}

unsigned long Tonuino::getWatchDogMillis()
{
	if (powerState == PowerState::DeepSleep || powerState == PowerState::VeryDeepSleep)
		return getWatchDogMillis(deepSleepCycleTime);

	return getWatchDogMillis(sleepCycleTime);
}

unsigned long Tonuino::getWatchDogMillis(uint8_t time)
{
	if (WDTO_1S == time)
		return 1000;
	if (WDTO_2S == time)
		return 2000;
	if (WDTO_4S == time)
		return 4000;
	if (WDTO_8S == time)
		return 8000;
	if (WDTO_500MS == time)
		return 500;
	if (WDTO_250MS == time)
		return 250;
	if (WDTO_120MS == time)
		return 120;
	if (WDTO_60MS == time)
		return 60;
	if (WDTO_30MS == time)
		return 30;
	return 15;
}

void Tonuino::executeSleep()
{
	wdt_reset();
	mp3.SerialStopListening();
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	cli();
	sleep_enable();
	power_adc_disable();
	power_usart0_disable();
	power_spi_disable();
	power_timer0_disable();
	power_timer1_disable();
	power_timer2_disable();
	power_twi_disable();
	sei();
	sleep_cpu();
	sleep_disable();
	power_all_enable();
	mp3.SerialStartListening();
}
