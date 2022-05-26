#ifndef SRC_CHIP_CARD_ISO_14443_4_HPP_
#define SRC_CHIP_CARD_ISO_14443_4_HPP_

#include <stdint.h>
#include <Arduino.h>

class MFRC522; // forward declaration to not have to include it here

class Chip_card_ISO_14443_4 {
public:
  static bool readCard(MFRC522 &mfrc522, byte *buffer);

private:
  Chip_card_ISO_14443_4();
  static bool ISO_14443_4_readCard(MFRC522 &mfrc522, byte *buffer);
  static bool ISO_14443_4_APDU(MFRC522 &mfrc522, byte *_buffer);
  static bool ISO_14443_4_InitialRATS(MFRC522 &mfrc522);
  static bool ISO_14443_4_DeSelectCard(MFRC522 &mfrc522);
};

#endif /* SRC_CHIP_CARD_ISO_14443_4_HPP_ */