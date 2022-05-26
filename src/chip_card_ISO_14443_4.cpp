#include "chip_card_ISO_14443_4.hpp"

#include <Arduino.h>
#include <SPI.h>

#include <MFRC522.h>

#include "logger.hpp"

extern const char* dump_byte_array(byte *buffer, size_t bufferSize);

bool Chip_card_ISO_14443_4::readCard(MFRC522 &mfrc522, byte *buffer){
    return ISO_14443_4_readCard(mfrc522, buffer);
}

bool Chip_card_ISO_14443_4::ISO_14443_4_DeSelectCard(MFRC522 &mfrc522) {
    MFRC522::StatusCode status = MFRC522::STATUS_ERROR;
    byte close[] = {
        0xC2,
        0x00, 0x00};
    mfrc522.PCD_CalculateCRC(close, 1, &close[1]);
    byte size = sizeof(close);
    status = mfrc522.PCD_TransceiveData(close, 3, &close[0], &size);
    if (status != MFRC522::STATUS_OK)
    {
        LOG(card_log, s_debug, F("ISO 14443_4 DeSelectCard Failed:"), mfrc522.GetStatusCodeName(status), dump_byte_array(&close[0], size));
        return false;
    }
    return true;
}

bool Chip_card_ISO_14443_4::ISO_14443_4_InitialRATS(MFRC522 &mfrc522) {
    MFRC522::StatusCode status = MFRC522::STATUS_ERROR;
    byte buffer[12];
    buffer[0] = MFRC522::PICC_CMD_RATS;
    buffer[1] = MFRC522::PICC_CMD_HLTA;
    mfrc522.PCD_CalculateCRC(buffer, 2, &buffer[2]);
    byte size = sizeof(buffer);
    status = mfrc522.PCD_TransceiveData(buffer, 4, &buffer[4], &size);
    if (status != MFRC522::STATUS_OK)
    {
        LOG(card_log, s_debug, F("ISO 14443_4 InitialRATS Failed:"), mfrc522.GetStatusCodeName(status), dump_byte_array(&buffer[4], size));
        return false;
    }
    LOG(card_log, s_debug, F("ISO 14443_4 InitialRATS:"), dump_byte_array(&buffer[4], size));
    return true;
}

bool Chip_card_ISO_14443_4::ISO_14443_4_APDU(MFRC522 &mfrc522, byte *bufferOut) {
    MFRC522::StatusCode status = MFRC522::STATUS_ERROR;
    byte buffer[32];
    byte bufferSize = 32;
    byte apdu[] = {
        0x02, /* frame header */
        0x00, /* CLA */
        0xA4, /* INS */
        0x04, /* P1  */
        0x00, /* P2  */
        0x05, /* Lc  */
        0xF0, 0x13, 0x37, 0xB3, 0x47,
        0x00, 0x00};
    mfrc522.PCD_CalculateCRC(apdu, 11, &apdu[11]);
    byte size = bufferSize;
    LOG(card_log, s_debug, F("ISO 14443_4 APDU Size: "), size);
    status = mfrc522.PCD_TransceiveData(apdu, 13, &buffer[0], &size);
    if (status == MFRC522::STATUS_TIMEOUT)
    {
        LOG(card_log, s_debug, F("ISO 14443_4 APDU Resend 1: "), size);
        size = bufferSize;
        status = mfrc522.PCD_TransceiveData(apdu, 13, &buffer[0], &size);
    }

    if (status != MFRC522::STATUS_OK)
    {
        LOG(card_log, s_debug, F("ISO 14443_4 APDU Failed: "), mfrc522.GetStatusCodeName(status), dump_byte_array(&buffer[0], size));
        return false;
    }

    while (buffer[0] == 0xF2 && buffer[1] == 0x01)
    {
        delay(10);
        size = bufferSize;
        status = mfrc522.PCD_TransceiveData(buffer, 4, &buffer[0], &size);
        if (status != MFRC522::STATUS_OK)
        {
            LOG(card_log, s_debug, F("ISO 14443_4 APDU Loop Failed: "), mfrc522.GetStatusCodeName(status), dump_byte_array(&buffer[0], size));
            return false;
        }
    }

    LOG(card_log, s_debug, F("ISO 14443_4 APDU Response: "), dump_byte_array(&buffer[0], size));
    memcpy(bufferOut, buffer + 1, (int)size - 3);
    return true;
}

bool Chip_card_ISO_14443_4::ISO_14443_4_readCard(MFRC522 &mfrc522, byte *buffer) {
    if (!ISO_14443_4_InitialRATS(mfrc522))
    {
        ISO_14443_4_DeSelectCard(mfrc522);
        return false;
    }

    if (!ISO_14443_4_APDU(mfrc522, buffer))
    {
        ISO_14443_4_DeSelectCard(mfrc522);
        return false;
    }

    ISO_14443_4_DeSelectCard(mfrc522);
    return true;
}