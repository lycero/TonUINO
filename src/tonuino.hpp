#ifndef SRC_TONUINO_HPP_
#define SRC_TONUINO_HPP_

#include "settings.hpp"
#include "commands.hpp"
#include "buttons.hpp"
#include "serial_input.hpp"
#include "mp3.hpp"
#include "chip_card_v3.hpp"
#include "modifier.hpp"
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
  DeepSleep
};

class Tonuino {
public:
  Tonuino() {}
  static Tonuino& getTonuino() { static Tonuino tonuino; return tonuino; }

  void setup          ();
  void loop           (WakeupSource source);

  void playFolder     ();
  void playTrackNumber();

  void       nextTrack(bool fromOnPlayFinished = false);
  void   previousTrack();

  void resetActiveModifier   () { activeModifier = &noneModifier; }
  Modifier& getActiveModifier() { return *activeModifier; }

  void setCard  (const nfcTagObject   &newCard) { myCard = newCard; setFolder(&myCard.nfcFolderSettings); }
  const nfcTagObject& getCard() const           { return myCard; }
  void setFolder(folderSettings *newFolder    ) { myFolder = newFolder; }

  Mp3&      getMp3      () { return mp3      ; }
  Commands& getCommands () { return commands ; }
  Settings& getSettings () { return settings ; }
  Chip_card& getChipCard() { return chip_card; }

private:

  void ChangePowerState(WakeupSource source);
  bool specialCard(const nfcTagObject &nfcTag);
  void checkNfc();
  void checkInputs();
  void loopMp3();
  void UpdatePowerState(unsigned long startCycle);
  void executeSleep();
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
  //FeedbackModifier     feedbackModifier    {*this, mp3, settings};

  Modifier*            activeModifier      {&noneModifier};

  Timer                sleepStateTimer     {};

  nfcTagObject         myCard              {};
  folderSettings*      myFolder            {&myCard.nfcFolderSettings};
  uint16_t             numTracksInFolder   {};
  PowerState           powerState          {PowerState::Active};
};

#endif /* SRC_TONUINO_HPP_ */
