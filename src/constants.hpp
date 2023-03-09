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


// ####### buttons #####################################

// uncomment the below line to enable five button support
//#define FIVEBUTTONS

inline constexpr uint32_t buttonLongPress = 1000; // timeout for long press button in ms
inline constexpr uint8_t  buttonPausePin = 6;
inline constexpr uint8_t  buttonUpPin = 5;
inline constexpr uint8_t  buttonDownPin = 4;

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
inline constexpr uint8_t  cardPowerDownPin = 3;
inline constexpr unsigned long cardSleep = 250;

// ####### mp3 #########################################
inline constexpr uint8_t       maxTracksInFolder = 100;
inline constexpr uint8_t       dfPlayer_receivePin = 8;
inline constexpr uint8_t       dfPlayer_transmitPin = 9;
inline constexpr uint8_t       dfPlayer_busyPin = 7;
inline constexpr uint8_t       dfPlayer_ampPin = 11;
inline constexpr uint8_t       dfPlayer_powerPin = 12;
inline constexpr levelType     dfPlayer_busyPinType = levelType::activeLow;
inline constexpr unsigned long dfPlayer_timeUntilStarts = 3000;


// ####### tonuino #####################################
inline constexpr unsigned long baseTimeMulti = 1000;
inline constexpr uint8_t openAnalogPin = A7;
inline constexpr unsigned long cycleTime = 50;
inline constexpr uint8_t sleepCycleTime = WDTO_2S;
inline constexpr uint8_t deepSleepCycleTime = WDTO_2S;
inline constexpr uint8_t cardReadSleepTime = WDTO_1S;
inline constexpr unsigned long keyReadTimerDuration = buttonLongPress * 2;
inline constexpr unsigned long cardReadTimerDuration = 30 * baseTimeMulti;
inline constexpr unsigned long lightSleepTimerDuration = 10 * baseTimeMulti;
inline constexpr unsigned long deepSleepTimerDuration = 30 * 60 * baseTimeMulti;

#endif /* SRC_CONSTANTS_HPP_ */
