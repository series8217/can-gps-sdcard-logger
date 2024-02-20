#ifndef COMMON_H
#define COMMON_H

// for ArduinoOutStream and ArduinoInStream
#include "sdios.h"

#include <SPI.h>
#include <mcp_canbus.h>

/************************************/
/** CAN Bus interface configuration */
/************************************/

/* Please modify SPI_CS_PIN_MCP_CAN to adapt to different baords.

   CANBed V1        - 17
   CANBed M0        - 3
   CAN Bus Shield   - 9
   CANBed 2040      - 9
   CANBed Dual      - 9
   OBD-2G Dev Kit   - 9
   Hud Dev Kit      - 9
*/

// SPI CS pin for the CAN bus chip
#define SPI_CS_PIN_MCP_CAN 9

// this tells the CAN library which pin to use
extern MCP_CAN CAN;

#define PID_ENGIN_PRM 0x0C
#define PID_VEHICLE_SPEED 0x0D
#define PID_COOLANT_TEMP 0x05

#define CAN_ID_PID 0x7DF

/************************************/
/**           SD Card               */
/************************************/
// SD_FAT_TYPE = 0 for SdFat/File as defined in SdFatConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 3

// Test with reduced SPI speed for breadboards.  SD_SCK_MHZ(4) will select
// the highest speed supported by the board that is not over 4 MHz.
// Change SPI_SPEED to SD_SCK_MHZ(50) for best performance.
#define SPI_SPEED SD_SCK_MHZ(4)
//------------------------------------------------------------------------------

// SD chip select is the key hardware option
// Common values are:
// Arduino Ethernet shield, pin 4
// Sparkfun SD shield, pin 8
// Adafruit SD shields and modules, pin 10
// Longan Labs RP2040 boards with SD, pin 5
#define SPI_CS_PIN_SDCARD 5
#define SDCARD_SS_PIN SPI_CS_PIN_SDCARD
// SD card chip select
#define SDCARD_CS SPI_CS_PIN_SDCARD;

/************************************/
/** Power Control Pin configuration */
/************************************/
// this was erroneously set to 9 in the Longan Labs example code
#define pinPwrCtrl 22
//#define pinPwrCtrl A3        // for Atmaega32U4 version

/************************************/
/**     Serial Streams              */
/************************************/
#define SERIAL_USB Serial
#define SERIAL_GPS Serial1

extern ArduinoOutStream cout;
// input buffer for line
extern char cinBuf[40];
extern ArduinoInStream cin;
#endif // COMMON_H