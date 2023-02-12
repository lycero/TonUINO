
#include "timer.hpp"
#include "log.hpp"
#include "logger.hpp"

#include <Arduino.h>

void Timer::start(unsigned long timeout) {
  expireTime = millis() + timeout;
  epiredTime = 0;
  //LOG(powerstate_log, s_debug, F("Timer::start: "), millis(), F(" -> "), expireTime);
  active = true;
}
void Timer::updateMillis(unsigned long time)
{
    epiredTime += time;
}

bool Timer::isExpired() {
  if (not active)
    return true;
  if (expireTime <= (epiredTime + millis())) {
    active = false;
    return true;
  }
  return false;
}

void Timer::stop() {
  active = false;
}





