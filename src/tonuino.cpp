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

	// load Settings from EEPROM
	settings.loadSettingsFromFlash();

	// NFC Leser initialisieren
	delay(25);
	chip_card.initCard();

	digitalWrite(dfPlayer_powerPin, 1);
	delay(25);
	// DFPlayer Mini initialisieren
	mp3.begin();
	// // Zwei Sekunden warten bis der DFPlayer Mini initialisiert ist
	delay(2000);
	digitalWrite(dfPlayer_ampPin, 0);
	mp3.setVolume();
	mp3.setEq(static_cast<DfMp3_Eq>(settings.eq - 1));

	// RESET --- ALLE DREI KNÖPFE BEIM STARTEN GEDRÜCKT HALTEN -> alle EINSTELLUNGEN werden gelöscht
	if (buttons.isReset())
	{
		settings.clearEEPROM();
		settings.loadSettingsFromFlash();
	}

	//activeLoopModifier->Init();
	ChangeLoopModifier(LoopModifier::LoopModifierId::Active);
	SM_tonuino::start();
	// Start Shortcut "at Startup" - e.g. Welcome Sound
	SM_tonuino::dispatch(command_e(commandRaw::start));

	//ChangeLoopModifier(LoopModifier::LoopModifierId::LightSleep);
}

void Tonuino::loop(WakeupSource source)
{
	ReactOnWakeup(source);

	wdt_reset();
	unsigned long start_cycle = millis();  

	activeLoopModifier->Loop();

	auto transition = activeLoopModifier->GetTransition();

	if (transition != LoopModifier::LoopModifierId::None)
		ChangeLoopModifier(transition);

	if (isKeepAwake()) 
	{
		unsigned long stop_cycle = millis();
		if (stop_cycle - start_cycle < cycleTime)
			delay(cycleTime - (stop_cycle - start_cycle));
	}

	activeLoopModifier->EndCycle(start_cycle);
}

void Tonuino::ChangeLoopModifier(LoopModifier::LoopModifierId id)
{
	if (id == LoopModifier::LoopModifierId::None)
		return;

	if (activeModifier != &noneModifier && id != LoopModifier::LoopModifierId::Active)
		return;

	if (!SM_tonuino::is_in_state<Play>() && 
		!SM_tonuino::is_in_state<Idle>() && 
		!SM_tonuino::is_in_state<Pause>() && 
		!SM_tonuino::is_in_state<StartPlay>() &&
		id != LoopModifier::LoopModifierId::Active)
		return;	

	LoopModifier::LoopModifier* newModifier = activeLoopModifier;
	
	switch (id)
	{
	case LoopModifier::LoopModifierId::None:
		break;
	case LoopModifier::LoopModifierId::Active:
		newModifier = &loopActive;
		break;
	case LoopModifier::LoopModifierId::KeyRead:
		newModifier = &loopKeyRead;
		break;
	case LoopModifier::LoopModifierId::CardRead:
		newModifier = &loopCardRead;
		break;
	case LoopModifier::LoopModifierId::LightSleep:
		newModifier = &loopLightSleep;
		break;
	case LoopModifier::LoopModifierId::DeepSleep:
		newModifier = &loopDeepSleep;
		break;
	case LoopModifier::LoopModifierId::VeryDeepSleep:
		newModifier = &loopVeryDeepSleep;
		break;
	default:
		break;
	}

	if (newModifier == activeLoopModifier)
		return;

	activeLoopModifier->HandleModifierChange(id);
	activeLoopModifier = newModifier;

	if (loop_log::will_log(s_debug)) 
	{
		switch (id)
		{
		case LoopModifier::LoopModifierId::None:
			break;
		case LoopModifier::LoopModifierId::Active:
			LOG(loop_log, s_debug, F("# -> Active Loop"));
			break;
		case LoopModifier::LoopModifierId::KeyRead:
			LOG(loop_log, s_debug, F("# -> KeyRead Loop"));
			break;
		case LoopModifier::LoopModifierId::CardRead:
			LOG(loop_log, s_debug, F("# -> CardRead Loop"));
			break;
		case LoopModifier::LoopModifierId::LightSleep:
			LOG(loop_log, s_debug, F("# -> LightSleep Loop"));
			break;
		case LoopModifier::LoopModifierId::DeepSleep:
			LOG(loop_log, s_debug, F("# -> DeepSleep Loop"));
			break;
		case LoopModifier::LoopModifierId::VeryDeepSleep:
			LOG(loop_log, s_debug, F("# -> VeryDeepSleep Loop"));
			break;
		default:
			break;
		}
		delay(50);
	}

	activeLoopModifier->Init();
}

void Tonuino::runActiveLoop()
{
	unsigned long start_cycle = millis();

	SM_tonuino::dispatch(command_e(commands.getCommandRaw()));

	chip_card.wakeCard();
	SM_tonuino::dispatch(card_e(chip_card.getCardEvent()));
	chip_card.sleepCard();

	mp3.loop();
	activeModifier->loop();

	unsigned long stop_cycle = millis();
	if (stop_cycle - start_cycle < cycleTime)
		delay(cycleTime - (stop_cycle - start_cycle));
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

void Tonuino::resetActiveModifier() 
{ 
	activeModifier = &noneModifier;
	ChangeLoopModifier(LoopModifier::LoopModifierId::Active);
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
	ChangeLoopModifier(LoopModifier::LoopModifierId::Active);

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


void Tonuino::ReactOnWakeup(WakeupSource source)
{
	wdt_reset();
	switch (source)
	{
	case WakeupSource::None:
	{
		/* code */		
		break;
	}
	case WakeupSource::Watchdog:
	{
		auto wdmillis = getWatchDogMillis();
		activeLoopModifier->UpdateTimer(wdmillis);
		mp3.updateTimer(wdmillis);
		break;
	}
	case WakeupSource::KeyInput:
	{
		//commands.getCommandRaw();
		ChangeLoopModifier(LoopModifier::LoopModifierId::KeyRead);
		break;
	}

	case WakeupSource::Mp3BusyChange:
	{
		myKeepAwake++;
		if (!SM_tonuino::is_in_state<Play>())
			break;

		if (!(mp3.isPlayingFolder() || mp3.isPlayingMp3()) || mp3.isPlaying())
			break;

		if (activeLoopModifier->GetModifierId() == LoopModifier::LoopModifierId::KeyRead)
			break;

		mp3.OnPlayFinished(mp3.getLastPlayedTrack());
				
		break;
	}
	default:
		break;
	}
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
