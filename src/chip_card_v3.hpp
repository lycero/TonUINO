#ifndef SRC_CHIP_CARD_V3_HPP_
#define SRC_CHIP_CARD_V3_HPP_

#include <stdint.h>

#include <PN532.h>
#include <PN532_HSU.h>

#include "constants.hpp"

enum class mode_t: uint8_t {
  none          =   0,

  // folder modes
  hoerspiel     =   1,
  album         =   2,
  party         =   3,
  einzel        =   4,
  hoerbuch      =   5,
  admin         =   6,
  hoerspiel_vb  =   7,
  album_vb      =   8,
  party_vb      =   9,
  hoerbuch_1    =  10,
  repeat_last   =  11,

  // modifier modes
  sleep_timer   =   1,
  freeze_dance  =   2,
  locked        =   3,
  toddler       =   4,
  kindergarden  =   5,
  repeat_single =   6,

  admin_card    = 0xff,
};

struct folderSettings {
  uint8_t folder;
  mode_t  mode;
  uint8_t special;
  uint8_t special2;
  bool operator==(const folderSettings& rhs) const {
    return folder   == rhs.folder  &&
           mode     == rhs.mode    &&
           special  == rhs.special &&
           special2 == rhs.special2;
  }
};

// this object stores nfc tag data
struct nfcTagObject {
  folderSettings nfcFolderSettings;
  bool operator==(const nfcTagObject& rhs) const {
    return nfcFolderSettings == rhs.nfcFolderSettings;
  }
};

enum class cardEvent: uint8_t {
  none,
  removed,
  inserted,
};

class Mp3;     // forward declaration to not have to include it here

class delayedSwitchOn {
public:
  delayedSwitchOn(uint8_t delay)
  : delaySteps(delay)
  , counter(delay)
  {}
  delayedSwitchOn& operator++() { if (counter < delaySteps) ++counter; return *this; }
  void reset() { counter = 0; }
  bool on()    { return counter == delaySteps; }

private:
  const uint8_t delaySteps;
  uint8_t       counter;
};

class Chip_card {
public:
  Chip_card(Mp3 &mp3);
  bool readCard (      nfcTagObject &nfcTag);
  bool writeCard(const nfcTagObject &nfcTag);
  void sleepCard();
  void wakeCard();
  void initCard ();
  cardEvent getCardEvent();
  bool isCardRemoved() { return cardRemoved; }

private:
  Mp3                 &mp3;
  
  PN532_HSU           pn532hsu;
  PN532               pn532;

  delayedSwitchOn     cardRemovedSwitch;
  bool                cardRemoved = true;

  uint8_t uid[7] = {0, 0, 0, 0, 0, 0, 0};
  uint8_t uidLength = 0;
  uint8_t cardType = 0;
};

#endif /* SRC_CHIP_CARD_V3_HPP_ */