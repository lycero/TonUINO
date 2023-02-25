#ifndef SRC_TONUINO_HPP_
#define SRC_TONUINO_HPP_

#include "settings.hpp"
#include "commands.hpp"
#include "buttons.hpp"
#include "serial_input.hpp"
#include "mp3.hpp"
#include "chip_card_v3.hpp"
#include "modifier.hpp"
#include "loopModifier.h"
#include "timer.hpp"

enum class WakeupSource{
  None,
  Watchdog,
  KeyInput,
  Mp3BusyChange,
  CardReader
};

enum class PowerState{
  Active,
  LightSleep,
  Mp3Awake,
  DeepSleep,
  VeryDeepSleep
};

class Tonuino {
public:
  Tonuino() {}
  static Tonuino& getTonuino() { static Tonuino tonuino; return tonuino; }

  void setup          ();
  void loop           (WakeupSource source);
  void ChangeLoopModifier(LoopModifier::LoopModifierId id);
  void runActiveLoop  ();

  void playFolder     ();
  void playTrackNumber();

  void       nextTrack(bool fromOnPlayFinished = false);
  void   previousTrack();

  void resetActiveModifier();
  Modifier& getActiveModifier() { return *activeModifier; }

  void setCard  (const nfcTagObject   &newCard) { myCard = newCard; setFolder(&myCard.nfcFolderSettings); }
  const nfcTagObject& getCard() const           { return myCard; }
  void setFolder(folderSettings *newFolder    ) { myFolder = newFolder; }

  void keepAwake();
  void executeSleep();
  void OnPlayFinished(uint16_t track);

  Mp3&      getMp3      () { return mp3      ; }
  Commands& getCommands () { return commands ; }
  Settings& getSettings () { return settings ; }
  Chip_card& getChipCard() { return chip_card; }

private:

  void ReactOnWakeup(WakeupSource source);
  bool specialCard(const nfcTagObject &nfcTag);
  bool isKeepAwake();
  unsigned long getWatchDogMillis();
  unsigned long getWatchDogMillis(uint8_t time);

  Settings             settings            {};
  Mp3                  mp3                 {settings};
  Buttons              buttons             {};
//  SerialInput          serialInput         {};
//  Commands             commands            {settings, &buttons, &serialInput};
  Commands             commands            {settings, &buttons};
  Chip_card            chip_card           {mp3};

  friend class Base;

  Modifier             noneModifier        {*this, mp3, settings};
  SleepTimer           sleepTimer          {*this, mp3, settings};
  FreezeDance          freezeDance         {*this, mp3, settings};
  Locked               locked              {*this, mp3, settings};
  ToddlerMode          toddlerMode         {*this, mp3, settings};
  KindergardenMode     kindergardenMode    {*this, mp3, settings};
  RepeatSingleModifier repeatSingleModifier{*this, mp3, settings};

  Modifier*            activeModifier      {&noneModifier};

  LoopModifier::Active loopActive{ *this, mp3, settings };
  LoopModifier::KeyRead loopKeyRead{ *this, commands, settings };
  LoopModifier::CardRead loopCardRead{ *this, chip_card, settings };
  LoopModifier::LightSleep loopLightSleep{ *this, mp3, settings };
  LoopModifier::DeepSleep loopDeepSleep{ *this, mp3, settings };
  LoopModifier::VeryDeepSleep loopVeryDeepSleep{ *this, mp3, settings };

  LoopModifier::LoopModifier* activeLoopModifier{&loopActive};

  nfcTagObject         myCard              {};
  folderSettings*      myFolder            {&myCard.nfcFolderSettings};
  uint16_t             numTracksInFolder   {};
  volatile uint8_t     myKeepAwake         {0};
};

#endif /* SRC_TONUINO_HPP_ */
