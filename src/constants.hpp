#ifndef SRC_CONSTANTS_HPP_
#define SRC_CONSTANTS_HPP_

#include <Arduino.h>
#include <avr/wdt.h>

// ####### helper for level ############################

enum class level: uint8_t {
  inactive,
  active  ,
};
enum class levelType: uint8_t {
  activeHigh,
  activeLow,
};

inline constexpr int getLevel(levelType t, level l) { return (l == level::inactive) ? (t==levelType::activeHigh ? LOW  : HIGH)
                                                                                    : (t==levelType::activeHigh ? HIGH : LOW ); }

// ####### buttons #####################################

// uncomment the below line to enable five button support
//#define FIVEBUTTONS

inline constexpr uint32_t buttonLongPress      = 1000; // timeout for long press button in ms
inline constexpr uint8_t  buttonPausePin       = 62;
inline constexpr uint8_t  buttonUpPin          = A1;
inline constexpr uint8_t  buttonDownPin        = A2;

#ifdef FIVEBUTTONS
inline constexpr uint8_t  buttonFourPin        = A3;
inline constexpr uint8_t  buttonFivePin        = A4;
#endif

inline constexpr levelType buttonPinType       = levelType::activeHigh;
inline constexpr uint32_t  buttonDbTime        = 25; // Debounce time in milliseconds (default 25ms)

// ####### chip_card ###################################

inline constexpr uint32_t cardCookie           = 0x1337b347;
inline constexpr uint8_t  cardVersion          = 0x02;
inline constexpr uint8_t  cardRemoveDelay      =  1;


// ####### mp3 #########################################

inline constexpr uint8_t       maxTracksInFolder        = 255;
inline constexpr uint8_t       dfPlayer_receivePin      = 51;
inline constexpr uint8_t       dfPlayer_transmitPin     = 50;
inline constexpr uint8_t       dfPlayer_busyPin         = 63;
inline constexpr levelType     dfPlayer_busyPinType     = levelType::activeLow;
inline constexpr unsigned long dfPlayer_timeUntilStarts = 2000;


// ####### tonuino #####################################

inline constexpr uint8_t   openAnalogPin        = A7;
inline constexpr unsigned long cycleTime        = 50;
inline constexpr uint8_t sleepCycleTime   = WDTO_2S;
inline constexpr uint8_t deepSleepCycleTime   = WDTO_4S;
inline constexpr unsigned long awakeTime        = 4000;
inline constexpr unsigned long lightSleepTime   = 8000;
inline constexpr unsigned long deepSleepTime    = 16000;

#endif /* SRC_CONSTANTS_HPP_ */
