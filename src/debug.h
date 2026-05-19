#pragma once
#include "config.h"

#ifdef DEBUG_MODE
  #define DBG_INIT() do { Serial.begin(115200); } while (0)
  #define DBG_EVENT(fmt, ...) do {                                  \
    char _dbgbuf[64];                                               \
    snprintf(_dbgbuf, sizeof(_dbgbuf), fmt, ##__VA_ARGS__);         \
    Serial.print(millis()); Serial.print(F(": "));                  \
    Serial.println(_dbgbuf);                                        \
  } while (0)
#else
  #define DBG_INIT()
  #define DBG_EVENT(fmt, ...)
#endif
