#include "src/tonuino.hpp"

#include "src/settings.hpp"
#include "src/mp3.hpp"
#include "src/buttons.hpp"
#include "src/logger.hpp"
#include "src/constants.hpp"
#include "src/PinChangeInterruptHandler/PinChangeInterruptHandler.h"

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

// Inherit from PinChangeInterruptHandler
class ChangeCounter 
: PinChangeInterruptHandler 
{
  byte inputPin;
  byte inputMode;
  unsigned long riseCounter;
  unsigned long fallCounter;

public:
  ChangeCounter(byte pin, byte mode) : riseCounter(0), fallCounter(0), inputPin(pin), inputMode(mode) {}
  
  void begin() {
    pinMode(inputPin, inputMode);
    riseCounter = 0;
    fallCounter = 0;    
    attachPCInterrupt(digitalPinToPCINT(inputPin));
  }

  void stop() {
    detachPCInterrupt(digitalPinToPCINT(inputPin));
  }

  bool getRise() {
    bool res = riseCounter > 0;
    riseCounter = 0;
    return res;
  }

  bool getFall() {
    bool res = fallCounter > 0;
    fallCounter = 0;
    return res;
  }

  // Overwrite handlePCInterrupt() method
  virtual void handlePCInterrupt(int8_t interruptNum, bool value) {
    if (digitalPinToPCINT(inputPin) != interruptNum)
      return;

    if (value)
      ++riseCounter;
    else
      ++fallCounter;
    Tonuino::getTonuino().keepAwake();
  }
};

ChangeCounter counter(buttonPausePin, INPUT);
ChangeCounter counter2(dfPlayer_busyPin, INPUT);

volatile bool watchDogToggle = true;
// Watchdog timer Interrupt Service Routine
ISR(WDT_vect)
{
    watchDogToggle = true;
}

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
  counter.begin();
  counter2.begin();
  Tonuino::getTonuino().setup();
 }

void loop() 
{
  if(watchDogToggle)
  {
    watchDogToggle = false;
    Tonuino::getTonuino().loop(WakeupSource::Watchdog);
  }
  else if(counter2.getRise())
  {
    Tonuino::getTonuino().loop(WakeupSource::Mp3BusyChange);
  }  
  else if(counter.getRise())
  {
    Tonuino::getTonuino().loop(WakeupSource::KeyInput);
  }
  else
  {
    Tonuino::getTonuino().loop(WakeupSource::None);
  }
}
