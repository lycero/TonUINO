#include "src/tonuino.hpp"

#include "src/settings.hpp"
//#include "src/mp3.hpp"
#include "src/buttons.hpp"
#include "src/logger.hpp"
#include "src/constants.hpp"
#include "src/InputTrigger.h"

/*
   _____         _____ _____ _____ _____
  |_   _|___ ___|  |  |     |   | |     |
	| | | . |   |  |  |-   -| | | |  |  |
	|_| |___|_|_|_____|_____|_|___|_____|
	TonUINO Version 3.0

	created by Thorsten Voß and licensed under GNU/GPL.
	refactored by Boerge1
	Information and contribution at https://tonuino.de.
*/

volatile bool watchDogToggle = true;
// Watchdog timer Interrupt Service Routine
ISR(WDT_vect) {
  watchDogToggle = true;
}

void setup() {
  Serial.begin(115200);  // Es gibt ein paar Debug Ausgaben über die serielle Schnittstelle
  pinMode(buttonPausePin, INPUT_PULLUP);
  pinMode(buttonUpPin, INPUT_PULLUP);
  pinMode(buttonDownPin, INPUT_PULLUP);

  digitalWrite(resetPin, HIGH);
  pinMode(resetPin, OUTPUT);
  pinMode(cardPowerDownPin, OUTPUT);
  digitalWrite(cardPowerDownPin, HIGH);

  pinMode(dfPlayer_powerPin, OUTPUT);
  digitalWrite(dfPlayer_powerPin, 0);

  digitalWrite(dfPlayer_ampPin, 1);
  pinMode(dfPlayer_ampPin, OUTPUT);

  // Wert für randomSeed() erzeugen durch das mehrfache Sammeln von rauschenden LSBs eines offenen Analogeingangs
  uint32_t ADC_LSB = 0;
  uint32_t ADCSeed = 0;
  for (uint8_t i = 0; i < 128; i++) {
    ADC_LSB = analogRead(openAnalogPin) & 0x1;
    ADCSeed ^= ADC_LSB << (i % 32);
  }
  randomSeed(ADCSeed);  // Zufallsgenerator initialisieren

  // // Dieser Hinweis darf nicht entfernt werden
  LOG(init_log, s_debug, F(" _____         _____ _____ _____ _____"));
  LOG(init_log, s_debug, F("|_   _|___ ___|  |  |     |   | |     |"));
  LOG(init_log, s_debug, F("  | | | . |   |  |  |-   -| | | |  |  |"));
  LOG(init_log, s_debug, F("  |_| |___|_|_|_____|_____|_|___|_____|\n"));
  LOG(init_log, s_info, F("TonUINO Version 3.0"));
  LOG(init_log, s_info, F("created by Thorsten Voß and licensed under GNU/GPL."));
  LOG(init_log, s_info, F("refactored by Boerge1."));
  LOG(init_log, s_debug, F("Information and contribution at https://tonuino.de.\n"));

  Tonuino::getTonuino().setup();
}

void loop() {
  if (watchDogToggle) 
  {
    watchDogToggle = false;
    Tonuino::getTonuino().loop(WakeupSource::Watchdog);
  } else 
  {
    Tonuino::getTonuino().loop(WakeupSource::None);
  }
}
