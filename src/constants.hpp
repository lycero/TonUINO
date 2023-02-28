#ifndef SRC_CONSTANTS_HPP_
#define SRC_CONSTANTS_HPP_

#include <Arduino.h>
#include <avr/wdt.h>

// ####### helper for level ############################

enum class level : uint8_t {
	inactive,
	active,
};
enum class levelType : uint8_t {
	activeHigh,
	activeLow,
};

inline constexpr int getLevel(levelType t, level l) {
	return (l == level::inactive) ? (t == levelType::activeHigh ? LOW : HIGH)
		: (t == levelType::activeHigh ? HIGH : LOW);
}

//#define TESTBOARD
//#define DISABLE_NFC


// ####### buttons #####################################

// uncomment the below line to enable five button support
//#define FIVEBUTTONS

inline constexpr uint32_t buttonLongPress = 1000; // timeout for long press button in ms
#ifndef TESTBOARD
inline constexpr uint8_t  buttonPausePin = 6;
inline constexpr uint8_t  buttonUpPin = 5;
inline constexpr uint8_t  buttonDownPin = 4;
inline constexpr uint8_t  resetPin = A0;
#else
inline constexpr uint8_t  buttonPausePin = 62;
inline constexpr uint8_t  buttonUpPin = 64;
inline constexpr uint8_t  buttonDownPin = 65;
inline constexpr uint8_t  resetPin = 43;
#endif // !TESTBOARD

#ifdef FIVEBUTTONS
inline constexpr uint8_t  buttonFourPin = A3;
inline constexpr uint8_t  buttonFivePin = A4;
#endif

inline constexpr levelType buttonPinType = levelType::activeLow;
inline constexpr uint32_t  buttonDbTime = 25; // Debounce time in milliseconds (default 25ms)

// ####### chip_card ###################################

inline constexpr uint32_t cardCookie = 0x1337b347;
inline constexpr uint8_t  cardVersion = 0x02;
inline constexpr uint8_t  cardRemoveDelay = 1;
inline constexpr unsigned long cardSleep = 1000;

// ####### mp3 #########################################
inline constexpr uint8_t       maxTracksInFolder = 100;
#ifndef TESTBOARD
inline constexpr uint8_t       dfPlayer_receivePin = 8;
inline constexpr uint8_t       dfPlayer_transmitPin = 9;
inline constexpr uint8_t       dfPlayer_busyPin = 7;
inline constexpr uint8_t       dfPlayer_ampPin = 11;
inline constexpr uint8_t       dfPlayer_powerPin = 12;
#else
inline constexpr uint8_t       dfPlayer_receivePin = 51;
inline constexpr uint8_t       dfPlayer_transmitPin = 50;
inline constexpr uint8_t       dfPlayer_busyPin = 63;
inline constexpr uint8_t       dfPlayer_ampPin = 41;
inline constexpr uint8_t       dfPlayer_powerPin = 42;
#endif // !TESTBOARD
inline constexpr levelType     dfPlayer_busyPinType = levelType::activeLow;
inline constexpr unsigned long dfPlayer_timeUntilStarts = 3000;


// ####### tonuino #####################################
inline constexpr unsigned long baseTimeMulti = 1000;
inline constexpr uint8_t openAnalogPin = A7;
inline constexpr unsigned long cycleTime = 50;
inline constexpr uint8_t sleepCycleTime = WDTO_2S;
inline constexpr unsigned long keyReadTimerDuration = buttonLongPress * 2;
inline constexpr unsigned long cardReadTimerDuration = 30 * baseTimeMulti;
inline constexpr unsigned long lightSleepTimerDuration = 30 * baseTimeMulti;
inline constexpr unsigned long deepSleepTimerDuration = 30 * 60 * baseTimeMulti;

#endif /* SRC_CONSTANTS_HPP_ */
