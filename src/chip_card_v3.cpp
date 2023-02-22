#include "chip_card_v3.hpp"

#include <Arduino.h>
//#include <SPI.h>

#include "mp3.hpp"
#include "constants.hpp"
#include "logger.hpp"
//#include "chip_card_ISO_14443_4.hpp"

// select whether StatusCode and PiccType are printed as names
// that uses about 690 bytes or 2.2% of flash
constexpr bool verbosePrintStatusCode = true;
constexpr bool verbosePrintPiccType   = false;

namespace {

constexpr size_t buffferSizeRead  = 18; // buffer size for read and write
constexpr size_t buffferSizeWrite = 16; // buffer size for read and write

// const __FlashStringHelper* str_failed      () { return F(" failed: ") ; }
// const __FlashStringHelper* str_MIFARE_Read () { return F("MIFARE_Read ") ; }
// const __FlashStringHelper* str_MIFARE_Write() { return F("MIFARE_Write ") ; }

/**
  Helper routine to dump a byte array as hex values to Serial.
*/
void u8toa_hex(uint8_t number, char *arr)
{
    int pos = 0;
    if (number < 16)
        arr[pos++] = '0';
    do {
        const int r = number % 16;
        arr[pos++] = (r > 9) ? (r - 10) + 'a' : r + '0';
        number /= 16;
    } while (number != 0);
}
const char* dump_byte_array(byte * buffer, size_t bufferSize) {
  static char ret[3*10+1];
  ret[0] = '\0';
  if (bufferSize > 10)
    return ret;
  size_t pos = 0;
  for (uint8_t i = 0; i < bufferSize; ++i) {
    ret[pos++] = ' ';
    u8toa_hex(buffer[i], &ret[pos]);
    pos +=2;
  }
  ret[pos] = '\0';
  return ret;
}

uint8_t key[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
const byte sector       = 1;
const byte trailerBlock = 7;
} // namespace

Chip_card::Chip_card(Mp3 &mp3)
: mp3(mp3)
, pn532hsu(Serial)
, pn532(pn532hsu)
, cardRemovedSwitch(cardRemoveDelay)
{
}

bool Chip_card::readCard(nfcTagObject &nfcTag)
{
  LOG(card_log, s_debug, F("Read Card Type "), cardType);
  bool success = false;
  byte buffer[32];
  byte bufferSize = 32;
  if (cardType == 1)
  {
    if (pn532.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, key))
    {
      if (pn532.mifareclassic_ReadDataBlock(4, buffer))
      {
        bufferSize = 16;
        success = true;
      }
    }
  }
  else if (cardType == 2)
  {   
    byte apdu[] = {         
        0x00, /* CLA */
        0xA4, /* INS */
        0x04, /* P1  */
        0x00, /* P2  */
        0x05, /* Lc  */
        0xF0, 0x13, 0x37, 0xB3, 0x47, /* APID */
        0x00 };     
    if (pn532.inDataExchange(apdu, sizeof(apdu), buffer, &bufferSize))
    {
      success = true;
    }
  }

  if(!success)
    return false;
  
 // LOG(card_log, s_info, F("Data Read "), dump_byte_array(buffer, bufferSize));
  PN532::PrintHex(buffer, bufferSize);

  const uint32_t version = buffer[4];
  if (version == cardVersion)
  {
    nfcTag.nfcFolderSettings.folder = buffer[5];
    nfcTag.nfcFolderSettings.mode = static_cast<mode_t>(buffer[6]);
    nfcTag.nfcFolderSettings.special = buffer[7];
    nfcTag.nfcFolderSettings.special2 = buffer[8];
  }
  else
  {
    LOG(card_log, s_warning, F("Unknown version "), version);
    nfcTag.nfcFolderSettings.folder = 0;
    nfcTag.nfcFolderSettings.mode = mode_t::none;
  }

  return true;
}

bool Chip_card::writeCard(const nfcTagObject &nfcTag) {
  return true;
}

void Chip_card::sleepCard() {
    pn532.setRFField( 0x00, 0x00);  
    pn532.shutPowerDown();  
}

void Chip_card::wakeCard() {
    pn532.wakeup();
    pn532.setRFField( 0x02, 0x01);
}

void Chip_card::initCard()
{
  pn532.begin();
  uint32_t versiondata = pn532.getFirmwareVersion();
  if (!versiondata)
  {
    LOG(card_log, s_debug, F("Didn't Find PN53x Module"));
    digitalWrite(resetPin, LOW);
    return;
  }

  LOG(card_log, s_debug, F("Found chip PN532") );
  LOG(card_log, s_debug, F("Firmware ver. ") , (versiondata >> 16) & 0xFF,  F("."), (versiondata >> 8) & 0xFF);

  // Configure board to read RFID tags
  pn532.SAMConfig();
}

cardEvent Chip_card::getCardEvent()
{
  // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  int success = pn532.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 25);
  if (!success)
  {
    ++cardRemovedSwitch;
  }
  else
  {
    cardRemovedSwitch.reset();
  }

  if (cardRemovedSwitch.on())
  {
    if (not cardRemoved)
    {
      LOG(card_log, s_info, F("Card Removed"));
      cardRemoved = true;
      uid[0] = uid[1] = uid[2] = uid[3] = uid[4]= uid[5]= 0;
      uidLength = 0;
      cardType = 0;
      return cardEvent::removed;
    }
  }
  else
  {
    if (cardRemoved)
    {
      LOG(card_log, s_info, F("Card Inserted"));
      PN532::PrintHex(uid, uidLength);
      //cardRemoved = false;
      cardType = success;
      return cardEvent::inserted;
    }
  }
  return cardEvent::none;
}
