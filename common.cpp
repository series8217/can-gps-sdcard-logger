#include "common.h"
#include "sdios.h"

ArduinoOutStream cout(SERIAL_USB);

// input buffer for line
char cinBuf[40];
ArduinoInStream cin(Serial, cinBuf, sizeof(cinBuf));
