#include "src/tonuino.hpp"

#include "src/settings.hpp"
//#include "src/mp3.hpp"
#include "src/buttons.hpp"
#include "src/logger.hpp"
#include "src/constants.hpp"
#include "src/ChangeCounter.h"

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
ISR(WDT_vect)
{
    watchDogToggle = true;
}

ChangeCounter pauseCounter(buttonPausePin);
ChangeCounter nextCounter(buttonUpPin);
ChangeCounter prevCounter(buttonDownPin);
ChangeCounter mp3BusyCounter(dfPlayer_busyPin);

void setup() {

  Serial.begin(115200); // Es gibt ein paar Debug Ausgaben über die serielle Schnittstelle
  // Wert für randomSeed() erzeugen durch das mehrfache Sammeln von rauschenden LSBs eines offenen Analogeingangs
  uint32_t ADC_LSB = 0;
  uint32_t ADCSeed = 0;
  for (uint8_t i = 0; i < 128; i++) {
    ADC_LSB = analogRead(openAnalogPin) & 0x1;
    ADCSeed ^= ADC_LSB << (i % 32);
  }
  randomSeed(ADCSeed); // Zufallsgenerator initialisieren

  // // Dieser Hinweis darf nicht entfernt werden
  LOG(init_log, s_debug, F(" _____         _____ _____ _____ _____"));
  LOG(init_log, s_debug, F("|_   _|___ ___|  |  |     |   | |     |"));
  LOG(init_log, s_debug, F("  | | | . |   |  |  |-   -| | | |  |  |"));
  LOG(init_log, s_debug, F("  |_| |___|_|_|_____|_____|_|___|_____|\n"));
  LOG(init_log, s_info , F("TonUINO Version 3.0"));
  LOG(init_log, s_info , F("created by Thorsten Voß and licensed under GNU/GPL."));
  LOG(init_log, s_info , F("refactored by Boerge1."));
  LOG(init_log, s_debug, F("Information and contribution at https://tonuino.de.\n"));

  delay(50);
  pauseCounter.begin();
  prevCounter.begin();
  nextCounter.begin();
  mp3BusyCounter.begin();
  Tonuino::getTonuino().setup();
 }

void loop() 
{
  if(watchDogToggle)
  {
    watchDogToggle = false;
    Tonuino::getTonuino().loop(WakeupSource::Watchdog);
  }
  else if(mp3BusyCounter.getRise())
  {
    Tonuino::getTonuino().loop(WakeupSource::Mp3BusyChange);
  }  
  else if(pauseCounter.getRise() || prevCounter.getRise() || nextCounter.getRise())
  {
    Tonuino::getTonuino().loop(WakeupSource::KeyInput);
  }
  else
  {
    Tonuino::getTonuino().loop(WakeupSource::None);
  }
}
