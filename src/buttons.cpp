#include "buttons.hpp"

#include "constants.hpp"
#include "logger.hpp"

namespace {
const bool buttonPinIsActiveLow = (buttonPinType == levelType::activeLow);
}

Buttons::Buttons(const Settings& settings)
//            pin             dbTime        puEnable              invert
: buttonPause(buttonPausePin, buttonDbTime, buttonPinIsActiveLow, buttonPinIsActiveLow)
, buttonUp   (buttonUpPin   , buttonDbTime, buttonPinIsActiveLow, buttonPinIsActiveLow)
, buttonDown (buttonDownPin , buttonDbTime, buttonPinIsActiveLow, buttonPinIsActiveLow)
#ifdef FIVEBUTTONS
, buttonFour (buttonFourPin , buttonDbTime, buttonPinIsActiveLow, buttonPinIsActiveLow)
, buttonFive (buttonFivePin , buttonDbTime, buttonPinIsActiveLow, buttonPinIsActiveLow)
#endif
, settings(settings)
{
  buttonPause.begin();
  buttonUp   .begin();
  buttonDown .begin();
#ifdef FIVEBUTTONS
  buttonFour .begin();
  buttonFive .begin();
#endif
}

buttonRaw Buttons::getButtonRaw() {
  buttonRaw ret = buttonRaw::none;
  readButtons();
  if ((  buttonPause.pressedFor(buttonLongPress)
      || buttonUp   .pressedFor(buttonLongPress)
      || buttonDown .pressedFor(buttonLongPress)
      )
     && buttonPause.isPressed()
     && buttonUp   .isPressed()
     && buttonDown .isPressed())
    ret = buttonRaw::allLong;

  else if (buttonPause.wasReleased()) {
    if (not ignorePauseButton)
      ret = buttonRaw::pause;
    else
      ignorePauseButton = false;
  }

  else if (buttonPause.pressedFor(buttonLongPress) && not ignorePauseButton) {
    ret = buttonRaw::pauseLong;
    ignorePauseButton = true;
  }

  else if (buttonUp.wasReleased()) {
    if (!ignoreUpButton) {
      ret = buttonRaw::up;
    }
    else
      ignoreUpButton = false;
  }

  else if (buttonUp.pressedFor(buttonLongPress) && not ignoreUpButton) {
    ret = buttonRaw::upLong;
    ignoreUpButton = true;
  }

  else if (buttonDown.wasReleased()) {
    if (!ignoreDownButton) {
      ret = buttonRaw::down;
    }
    else
      ignoreDownButton = false;
  }

  else if (buttonDown.pressedFor(buttonLongPress) && not ignoreDownButton) {
    ret = buttonRaw::downLong;
    ignoreDownButton = true;
  }

#ifdef FIVEBUTTONS
  else if (buttonFour.wasReleased()) {
    ret = buttonRaw::four;
  }

  else if (buttonFive.wasReleased()) {
    ret = buttonRaw::five;
  }
#endif

  if (ret != buttonRaw::none) {
    LOG(button_log, s_debug, F("Button raw: "), static_cast<uint8_t>(ret));
  }
  return ret;
}

buttonCmd Buttons::getButtonCmd() {
  buttonCmd ret = buttonCmd::none;
  buttonRaw b = getButtonRaw();

  switch (b) {
  case buttonRaw::none     : ret = buttonCmd::none                                                                  ; break;
  case buttonRaw::pause    : ret = buttonCmd::pause                                                                 ; break;
  case buttonRaw::pauseLong: ret = buttonCmd::track                                                                 ; break;
  case buttonRaw::up       : ret = (!settings.invertVolumeButtons) ? buttonCmd::next        : buttonCmd::volume_up  ; break;
  case buttonRaw::upLong   : ret = (!settings.invertVolumeButtons) ? buttonCmd::volume_up   : buttonCmd::next       ; break;
  case buttonRaw::down     : ret = (!settings.invertVolumeButtons) ? buttonCmd::previous    : buttonCmd::volume_down; break;
  case buttonRaw::downLong : ret = (!settings.invertVolumeButtons) ? buttonCmd::volume_down : buttonCmd::previous   ; break;
  case buttonRaw::allLong  : ret = buttonCmd::admin                                                                 ; break;
#ifdef FIVEBUTTONS
  case buttonRaw::four     : ret = (!settings.invertVolumeButtons) ? buttonCmd::volume_up   : buttonCmd::next       ; break;
  case buttonRaw::five     : ret = (!settings.invertVolumeButtons) ? buttonCmd::volume_down : buttonCmd::previous   ; break;
#endif
  }

  if (ret != buttonCmd::none) {
    LOG(button_log, s_debug, F("Button cmd: "), static_cast<uint8_t>(ret));
  }
  return ret;
}

void Buttons::waitForNoButton() {
  do {
    readButtons();
  } while (  buttonPause.isPressed()
          || buttonUp   .isPressed()
          || buttonDown .isPressed()
#ifdef FIVEBUTTONS
          || buttonFour .isPressed()
          || buttonFive .isPressed()
#endif
          );
  ignorePauseButton = false;
  ignoreUpButton    = false;
  ignoreDownButton  = false;
}

bool Buttons::isReset() {
  const int buttonActiveLevel = getLevel(buttonPinType, level::active);
  return (digitalRead(buttonPausePin) == buttonActiveLevel &&
          digitalRead(buttonUpPin   ) == buttonActiveLevel &&
          digitalRead(buttonDownPin ) == buttonActiveLevel );
}

bool Buttons::isBreak() {
  readButtons();
  if (buttonUp.wasReleased() || buttonDown.wasReleased()) {
    LOG(button_log, s_info, F("Abgebrochen!"));
    return true;
  }
  return false;
}

bool Buttons::askCode(Settings::pin_t &code) {
  uint8_t x = 0;
  while (x < 4) {
    readButtons();
    if (buttonPause.pressedFor(buttonLongPress))
      return false;
    if (buttonPause.wasReleased())
      code[x++] = 1;
    if (buttonUp.wasReleased())
      code[x++] = 2;
    if (buttonDown.wasReleased())
      code[x++] = 3;
  }
  return true;
}

void Buttons::readButtons() {
  buttonPause.read();
  buttonUp   .read();
  buttonDown .read();
#ifdef FIVEBUTTONS
  buttonFour .read();
  buttonFive .read();
#endif
}



