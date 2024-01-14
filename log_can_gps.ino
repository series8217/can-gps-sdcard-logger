#include <SPI.h>
#include <mcp_canbus.h>

#include "SdFat.h"
#include "sdios.h"

#define GPS_RECV_BUF_LEN 100

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
#define SPI_CS_PIN_MCP_CAN 9
MCP_CAN CAN(SPI_CS_PIN_MCP_CAN); // Set CS pin

#define PID_ENGIN_PRM 0x0Cß
#define PID_VEHICLE_SPEED 0x0D
#define PID_COOLANT_TEMP 0x05

#define CAN_ID_PID 0x7DF


/************************************/
/**           SD Card               */
/************************************/
// SD_FAT_TYPE = 0 for SdFat/File as defined in SdFatConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 3
//
// Set DISABLE_CHIP_SELECT to disable a second SPI device.
// For example, with the Ethernet shield, set DISABLE_CHIP_SELECT
// to 10 to disable the Ethernet controller.
const int8_t DISABLE_CHIP_SELECT = SPI_CS_PIN_MCP_CAN;
//
// Test with reduced SPI speed for breadboards.  SD_SCK_MHZ(4) will select
// the highest speed supported by the board that is not over 4 MHz.
// Change SPI_SPEED to SD_SCK_MHZ(50) for best performance.
#define SPI_SPEED SD_SCK_MHZ(4)
//------------------------------------------------------------------------------
#if SD_FAT_TYPE == 0
SdFat sd;
File file;
#elif SD_FAT_TYPE == 1
SdFat32 sd;
File32 file;
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile file;
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile file;
#else  // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE

// SD chip select is the key hardware option
// Common values are:
// Arduino Ethernet shield, pin 4
// Sparkfun SD shield, pin 8
// Adafruit SD shields and modules, pin 10
// Longan Labs RP2040 boards with SD, pin 5
#define SPI_CS_PIN_SDCARD 5
#define SDCARD_SS_PIN SPI_CS_PIN_SDCARD
// SD card chip select
int chipSelect = SPI_CS_PIN_SDCARD;

/************************************/
/** Power Control Pin configuration */
/************************************/
// this was erroneously set to 9 in the Longan Labs example code
const int pinPwrCtrl = 22; // for RP2040 verison
// const int pinPwrCtrl = A3;        // for Atmaega32U4 version

/************************************/
/**     Serial Streams              */
/************************************/
#define SERIAL_USB Serial
#define SERIAL_GPS Serial1

ArduinoOutStream cout(SERIAL_USB);
// input buffer for line
char cinBuf[40];
ArduinoInStream cin(Serial, cinBuf, sizeof(cinBuf));


/************************************/
/**    SD Card Helper Functions     */
/************************************/
void cardOrSpeed() {
  cout << F("Try another SD card or reduce the SPI bus speed.\n");
  cout << F("Edit SPI_SPEED in this program to change it.\n");
}

void clearSerialInput() {
  uint32_t m = micros();
  do {
    if (Serial.read() >= 0) {
      m = micros();
    }
  } while (micros() - m < 10000);
}

void reformatMsg() {
  cout << F("Try reformatting the card.  For best results use\n");
  cout << F("the SdFormatter program in SdFat/examples or download\n");
  cout << F("and use SDFormatter from www.sdcard.org/downloads.\n");
}

int setup_sd_card(){
    cout << F("\nSPI pins:\n");
    cout << F("MISO: ") << int(MISO) << endl;
    cout << F("MOSI: ") << int(MOSI) << endl;
    cout << F("SCK:  ") << int(SCK) << endl;
    cout << F("SS:   ") << int(SS) << endl;
    #ifdef SDCARD_SS_PIN
    cout << F("SDCARD_SS_PIN:   ") << int(SDCARD_SS_PIN) << endl;
    #endif  // SDCARD_SS_PIN

    static bool firstTry = true;

    if (!firstTry){
        cout << F("\nRestarting\n");
    }
    firstTry = false;

    if (DISABLE_CHIP_SELECT >= 0){
        pinMode(DISABLE_CHIP_SELECT, OUTPUT);
        digitalWrite(DISABLE_CHIP_SELECT, HIGH);
    }

    if (!sd.begin(SDCARD_SS_PIN, SPI_SPEED)){
        if (sd.card()->errorCode()){
            cout << F(
                "\nSD initialization failed.\n"
                "Do not reformat the card!\n"
                "Is the card correctly inserted?\n"
                "Is chipSelect set to the correct value?\n"
                "Does another SPI device need to be disabled?\n"
                "Is there a wiring/soldering problem?\n");
            cout << F("\nerrorCode: ") << hex << showbase;
            cout << int(sd.card()->errorCode());
            cout << F(", errorData: ") << int(sd.card()->errorData());
            cout << dec << noshowbase << endl;
            return 1;
        }
        cout << F("\nCard successfully initialized.\n");
        if (sd.vol()->fatType() == 0){
            cout << F("Can't find a valid FAT16/FAT32 partition.\n");
            cout << F("Try reformatting the card.\n");
            reformatMsg();
            return 1;
        }
    }

    uint32_t size = sd.card()->sectorCount();
    if (size == 0) {
        cout << F("Can't determine the card size.\n");
        cardOrSpeed();
        return 1;
    }
    uint32_t sizeMB = 0.000512 * size + 0.5;
    cout << F("Card size: ") << sizeMB;
    cout << F(" MB (MB = 1,000,000 bytes)\n");
    cout << endl;
    cout << F("Volume is FAT") << int(sd.vol()->fatType());
    cout << F(", Cluster size (bytes): ") << sd.vol()->bytesPerCluster();
    cout << endl << endl;

    cout << F("Files found (date time size name):\n");
    sd.ls(LS_R | LS_DATE | LS_SIZE);

    if ((sizeMB > 1100 && sd.vol()->sectorsPerCluster() < 64) ||
        (sizeMB < 2200 && sd.vol()->fatType() == 32)) {
        cout << F("\nThis card should be reformatted for best performance.\n");
        cout << F("Use a cluster size of 32 KB for cards larger than 1 GB.\n");
        cout << F("Only cards larger than 2 GB should be formatted FAT32.\n");
        reformatMsg();
        return 1;
    }

    return 0;
}

    
void set_mask_filt()
{
    // set mask, set both the mask to 0x3ff

    CAN.init_Mask(0, 0, 0x7FC);
    CAN.init_Mask(1, 0, 0x7FC);

    // set filter, we can receive id from 0x04 ~ 0x09

    CAN.init_Filt(0, 0, 0x7E8);
    CAN.init_Filt(1, 0, 0x7E8);

    CAN.init_Filt(2, 0, 0x7E8);
    CAN.init_Filt(3, 0, 0x7E8);
    CAN.init_Filt(4, 0, 0x7E8);
    CAN.init_Filt(5, 0, 0x7E8);
}

void sendPid(unsigned char __pid)
{
    unsigned char tmp[8] = {0x02, 0x01, __pid, 0, 0, 0, 0, 0};
    CAN.sendMsgBuf(CAN_ID_PID, 0, 8, tmp);
}

bool getSpeed(int *s)
{
    sendPid(PID_VEHICLE_SPEED);
    unsigned long __timeout = millis();

    while (millis() - __timeout < 1000) // 1s time out
    {
        unsigned char len = 0;
        unsigned char buf[8];

        if (CAN_MSGAVAIL == CAN.checkReceive())
        {                              // check if get data
            CAN.readMsgBuf(&len, buf); // read data,  len: data length, buf: data buf

            if (buf[1] == 0x41)
            {
                *s = buf[3];
                return 1;
            }
        }
    }

    return 0;
}

char gps_message[GPS_RECV_BUF_LEN];
volatile unsigned int gps_message_length = 0;

int receive_gps(char *receive_buf, size_t buf_len)
{
    /** receive a message. if a message is received, copy it to receive_buf
     * and return the length of the message. if no message is received,
     * return 0. if the message is longer than buf_len, it will be truncated.
     * if buf_len is 0, the message will not be copied to receive_buf.
     */
    int message_complete = 0;
    // the receive buffer for the serial port is only 64 bytes. if we let it
    // fill up, we will lose data. so we need to read as much as we can
    // before returning.
    // when we receive <CR><LF>, we know we have a complete message and we
    // return it right away. 
    while (SERIAL_GPS.available() && gps_message_length < GPS_RECV_BUF_LEN)
    {
        int next_value = SERIAL_GPS.read();
        if (next_value == 0 || next_value > 255){
            // skip invalid characters
            continue;
        }
        if (next_value < 0){
            // no data available
            break;
        }
        gps_message[gps_message_length] = (char)next_value;
        gps_message_length++;
        if ((char)next_value == '\n'){
            //SERIAL_USB.write(gps_message);
            message_complete = 1;
            break;
        }
    }
    // if we have a complete message, copy it to the receive buffer
    if (message_complete > 0){
        gps_message[gps_message_length] = 0;
        gps_message_length += 1;
        if (buf_len > 0){
            strlcpy(receive_buf, gps_message, buf_len);
        }
        size_t final_length = gps_message_length;
        // start over on next message
        gps_message_length = 0;
        return final_length;
    }
    if (gps_message_length >= GPS_RECV_BUF_LEN - 1){
        // something went wrong.. we didn't see an EOL char.
        // reset the buffer
        gps_message_length = 0;
        gps_message[0] = 0;
    }
    if (buf_len > 0){
        // null terminate the receive buffer since we didn't copy a message
        receive_buf[0] = 0;
    }
    return 0;
}

void print_can_bus_data()
{
    cout << "can: {";
    int __speed = 0;
    int ret = getSpeed(&__speed);
    if (ret)
    {
        cout << "Vehicle Speed: " << __speed << " kmh" << endl;
    }
    cout << "}" << endl;
}

void setup()
{
    SERIAL_USB.begin(115200);
    while (!SERIAL_USB)
        ;

    pinMode(pinPwrCtrl, OUTPUT); // Enable power for GPS and CAN Bus
    digitalWrite(pinPwrCtrl, HIGH);

    while (CAN_OK != CAN.begin(CAN_500KBPS))
    { // init can bus : baudrate = 500k
        cout << "CAN init fail, retry..." << endl;
        delay(100);
    }
    set_mask_filt();
    cout << "CAN init ok!" << endl;

    cout << "Initializing SD card..." << endl;
    setup_sd_card();

    cout << "Enabling GPS" << endl;
    SERIAL_GPS.begin(9600);

    cout << "Starting logging loop..." << endl;
}

void loop()
{
    /** service the GPS serial and print a completed message if there is one */
    char message_buf[GPS_RECV_BUF_LEN];
    message_buf[0] = 0;
    if (receive_gps(message_buf, GPS_RECV_BUF_LEN) > 0)
    {
        cout << "gps: {" << message_buf << "}" << endl;
        //if (log_file){
        //    log_file.println(message_buf);
        //}
    }
    /** print CAN bus data */
    // print_can_bus_data()
}

// END FILE