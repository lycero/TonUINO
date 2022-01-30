#ifndef SRC_CHIP_CARD_HPP_
#define SRC_CHIP_CARD_HPP_

#include <stdint.h>

#include <MFRC522.h>

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
  uint32_t       cookie  = cardCookie;
  uint8_t        version = cardVersion;
  folderSettings nfcFolderSettings;
  bool operator==(const nfcTagObject& rhs) const {
    return cookie            == rhs.cookie           &&
           version           == rhs.version          &&
           nfcFolderSettings == rhs.nfcFolderSettings;
  }
};

enum class cardEvent {
  none,
  removed,
  inserted,
};

class Mp3;     // forward declaration to not have to include it here
class Buttons;

class delayedSwitchOn {
public:
  delayedSwitchOn(unsigned int delay)
  : delaySteps(delay)
  {}
  delayedSwitchOn& operator++() { if (counter < delaySteps) ++counter; return *this; }
  void reset() { counter = 0; }
  bool on()    { return counter == delaySteps; }

private:
  const unsigned int delaySteps;
  unsigned int counter = 0;
};

class Chip_card {
public:
  Chip_card(Mp3 &mp3, Buttons &buttons);

  bool readCard (      nfcTagObject &nfcTag);
  bool writeCard(const nfcTagObject &nfcTag);
  void sleepCard();
  void initCard ();
  cardEvent getCardEvent();
  bool isCardRemoved() { return cardRemoved; }

private:
  void stopCrypto1();
  void stopCard ();
  bool auth(MFRC522::PICC_Type piccType);

  MFRC522             mfrc522;
  Mp3                 &mp3;
  Buttons             &buttons;

  delayedSwitchOn     cardRemovedSwitch;
  bool                cardRemoved = false;
};




#endif /* SRC_CHIP_CARD_HPP_ */