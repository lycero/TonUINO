#include "src/tonuino.hpp"

#include "src/settings.hpp"
#include "src/mp3.hpp"
#include "src/buttons.hpp"
#include "src/logger.hpp"
#include "src/constants.hpp"
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>


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

volatile bool toggle = true;
// Watchdog timer Interrupt Service Routine
ISR(WDT_vect)
{
    toggle = true;
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
  Tonuino::getTonuino().setup();

    //watchdog initialisieren
  MCUSR &= ~(1<<WDRF);
  WDTCSR = (1<<WDCE) | (1<<WDE);
  WDTCSR = 1<<WDP1 | 1<<WDP0 | 1<<WDP2; // timeout 125ms
  WDTCSR |= 1<<WDIE;
}

void enterSleep(){
  wdt_reset();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  power_adc_disable();
  power_spi_disable();
  power_timer0_disable();
  power_timer2_disable();
  power_twi_disable();
  interrupts ();
  sleep_cpu (); 
  sleep_disable();
  power_all_enable();
  updatemillis(125);
}

void loop() {
  if(toggle){
    toggle = false;
    Tonuino::getTonuino().loop();
    enterSleep();
  }
}
