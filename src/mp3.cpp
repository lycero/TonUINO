#include "mp3.hpp"

#include "tonuino.hpp"
#include "constants.hpp"

namespace {

const __FlashStringHelper* str_Volume() { return F("Volume: ") ; }

}

uint16_t Mp3Notify::lastTrackFinished = 0;

void Mp3Notify::OnError(uint16_t errorCode) {
  // see DfMp3_Error for code meaning
  LOG(mp3_log, s_error, F("Com Error: "), errorCode);
}
void Mp3Notify::OnPlaySourceOnline  (DfMp3_PlaySources source) { PrintlnSourceAction(source, F("online"  )); }
void Mp3Notify::OnPlaySourceInserted(DfMp3_PlaySources source) { PrintlnSourceAction(source, F("bereit"  )); }
void Mp3Notify::OnPlaySourceRemoved (DfMp3_PlaySources source) { PrintlnSourceAction(source, F("entfernt")); }
void Mp3Notify::PrintlnSourceAction(DfMp3_PlaySources source, const __FlashStringHelper* action) {
  if (source & DfMp3_PlaySources_Sd   ) LOG(mp3_log, s_debug, F("SD Karte "), action);
  if (source & DfMp3_PlaySources_Usb  ) LOG(mp3_log, s_debug, F("USB "     ), action);
  if (source & DfMp3_PlaySources_Flash) LOG(mp3_log, s_debug, F("Flash "   ), action);
}

void Mp3Notify::OnPlayFinished(DfMp3_PlaySources /*source*/, uint16_t track) {
  LOG(mp3_log, s_debug, F("Track beendet: "), track);
  if (track == lastTrackFinished)
    return;
  else
    lastTrackFinished = track;
  Tonuino::getTonuino().nextTrack();
}

Mp3::Mp3(const Settings& settings)
: DFMiniMp3<SoftwareSerial, Mp3Notify>{softwareSerial}
, softwareSerial{dfPlayer_receivePin, dfPlayer_transmitPin}
, settings{settings}
{
  // Busy Pin
  pinMode(dfPlayer_busyPin, dfPlayer_busyPinType == levelType::activeHigh ? INPUT : INPUT_PULLUP);
}

bool Mp3::isPlaying() const {
  return !digitalRead(dfPlayer_busyPin);
}

void Mp3::waitForTrackToFinish() {

  // wait until track is started
  waitForTrackToStart();
  delay(1000);

  // wait until track is finished
  do {
    loop();
  } while (isPlaying());
}

void Mp3::waitForTrackToStart() {
  unsigned long currentTime = millis();
  const unsigned long maxStartTime = 1000;

  // wait until track is started
  do {
    loop();
  } while (!isPlaying() && millis() < currentTime + maxStartTime);
}

void Mp3::playMp3FolderTrack(uint16_t track) {
  DFMiniMp3<SoftwareSerial, Mp3Notify>::playMp3FolderTrack(track);
}

void Mp3::playMp3FolderTrack(mp3Tracks track) {
  DFMiniMp3<SoftwareSerial, Mp3Notify>::playMp3FolderTrack(static_cast<uint16_t>(track));
}

void Mp3::playAdvertisement(uint16_t track, bool olnyIfIsPlaying) {
  if (isPlaying()) {
    DFMiniMp3<SoftwareSerial, Mp3Notify>::playAdvertisement(track);
  } else if (not olnyIfIsPlaying) {
    start();
    DFMiniMp3<SoftwareSerial, Mp3Notify>::playAdvertisement(track);
    waitForTrackToFinish(); // TODO remove waitForTrackToFinish
    pause();
  }
}

void Mp3::playAdvertisement(advertTracks track, bool olnyIfIsPlaying) {
  playAdvertisement(static_cast<uint16_t>(track), olnyIfIsPlaying);
}


void Mp3::increaseVolume() {
  if (volume < settings.maxVolume) {
    DFMiniMp3<SoftwareSerial, Mp3Notify>::setVolume(++volume);
  }
  LOG(mp3_log, s_info, str_Volume(), volume);
}

void Mp3::decreaseVolume() {
  if (volume > settings.minVolume) {
    DFMiniMp3<SoftwareSerial, Mp3Notify>::setVolume(--volume);
  }
  LOG(mp3_log, s_info, str_Volume(), volume);
}

void Mp3::setVolume() {
  volume = settings.initVolume;
  DFMiniMp3<SoftwareSerial, Mp3Notify>::setVolume(volume);
  LOG(mp3_log, s_info, str_Volume(), volume);
}