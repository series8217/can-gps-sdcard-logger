#include "common.h"
#include "sdcard.h"
#include "gps.h"

#include "sdios.h"
#include <SPI.h>
#include <mcp_canbus.h>


MCP_CAN CAN(SPI_CS_PIN_MCP_CAN);

/************************************/
/**    General Utility Functions    */
/************************************/
void clear_serial_input()
{
    /** exhaust the serial input buffer */
    uint32_t m = micros();
    do {
        if (Serial.read() >= 0) {
            m = micros();
        }
    } while (micros() - m < 10000);
}

/************************************/
/**    CAN Bus Helper Functions     */
/************************************/
void can_bus_set_mask_filter()
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

void can_bus_send_pid(unsigned char __pid)
{
    /** Request an OBD II PID */
    unsigned char tmp[8] = {0x02, 0x01, __pid, 0, 0, 0, 0, 0};
    CAN.sendMsgBuf(CAN_ID_PID, 0, 8, tmp);
}

bool can_bus_get_vehicle_speed(int *s)
{
    /** Get VEHICLE speed via OBD II PID message */
    can_bus_send_pid(PID_VEHICLE_SPEED);
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

void print_can_bus_data()
{
    cout << "can: {";
    int __speed = 0;
    int ret = can_bus_get_vehicle_speed(&__speed);
    if (ret)
    {
        cout << "Vehicle Speed: " << __speed << " kmh" << endl;
    }
    cout << "}" << endl;
}

bool can_init(){
    // start the CAN bus at 500 kbps
    // if the CAN transceiver isn't working this will loop forever
    if (CAN_OK != CAN.begin(CAN_500KBPS)) {
        cout << "CAN init fail... will retry" << endl;
        return false;
    }
    // set the CAN filter/masks to only listen to the messages we want.
    // we can't service the CAN bus fast enough to record everything.
    can_bus_set_mask_filter();
    cout << "CAN init ok!" << endl;
    return true;
}

/************************************/
/**            GPS                  */
/************************************/

bool gps_init(){
    // enable the GPS serial port at 9600 baud
    // IMPROVE: increase the GPS baud rate so it takes less time to
    //          receive the GPS message
    cout << "Enabling GPS" << endl;
    SERIAL_GPS.begin(9600);
    while (!SERIAL_GPS){
        ; // wait for serial port to connect
    }
    return true;
}

void setup()
{
    // console port for debugging
    SERIAL_USB.begin(230400);
    while (!SERIAL_USB){
        ; // wait for serial port to connect. Needed for native USB
    }

    // the power control pin enables the GPS and CAN transceivers
    pinMode(pinPwrCtrl, OUTPUT);
    digitalWrite(pinPwrCtrl, HIGH);

    // initialize the SD card including checking if it's formatted
    cout << "Initializing SD card..." << endl;
    setup_sd_card();

    cout << "Starting logging loop..." << endl;
    // when setup returns we enter loop() forever
}

void loop()
{
    /************************************/
    /**            CAN Bus              */
    /************************************/
    static bool can_ok = false;
    static int64_t next_can_init = 0;

    if (!can_ok && millis() > next_can_init){
        // keep trying to init CAN. we don't wantto hold up the GPS
        // or other processes though.
        can_ok = can_init();
        next_can_init = millis() + 100;
    }

    if (can_ok){
        //print_can_bus_data();
    }

    /***********************************/
    /**         GPS Serial Port        */
    /***********************************/
    static bool gps_ok = false;
    char message_buf[GPS_RECV_BUF_LEN];
    message_buf[0] = 0;
    if (!gps_ok){
        gps_ok = gps_init();
        GPS_GNRMC GNRMC = GPS_GNRMC();
        GNRMC.print_columns();
    }

    if (gps_ok && receive_gps(message_buf, GPS_RECV_BUF_LEN) > 0)
    {
        GPS_GNRMC GNRMC = GPS_GNRMC();
        if (GNRMC.parse(message_buf)){
            GNRMC.print_csv();
        }
    }

    /***********************************/
    /**           SD Card              */
    /***********************************/
    //cout << "gps: {" << message_buf << "}" << endl;
    // if (log_file){
    //     log_file.println(message_buf);
    // }

    /***********************************/
    /**            Heartbeat           */
    /***********************************/
    static int64_t next_heartbeat = 0;
    if (millis() > next_heartbeat){
        next_heartbeat = millis() + 1000;
        //cout << "." << endl;
    }
}

// END FILE