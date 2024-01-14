#include <SPI.h>
#include <mcp_canbus.h>

#define SERIAL_USB Serial
#define SERIAL_GPS Serial1

#define GPS_RECV_BUF_LEN 100

/* Please modify SPI_CS_PIN to adapt to different baords.

   CANBed V1        - 17
   CANBed M0        - 3
   CAN Bus Shield   - 9
   CANBed 2040      - 9
   CANBed Dual      - 9
   OBD-2G Dev Kit   - 9
   Hud Dev Kit      - 9
*/

#define SPI_CS_PIN 9

MCP_CAN CAN(SPI_CS_PIN); // Set CS pin

#define PID_ENGIN_PRM 0x0Cß
#define PID_VEHICLE_SPEED 0x0D
#define PID_COOLANT_TEMP 0x05

#define CAN_ID_PID 0x7DF

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

const int pinPwrCtrl = 22; // for RP2040 verison
// const int pinPwrCtrl = A3;        // for Atmaega32U4 version

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
    SERIAL_USB.print("can: {");
    int __speed = 0;
    int ret = getSpeed(&__speed);
    if (ret)
    {
        SERIAL_USB.print("Vehicle Speed: ");
        SERIAL_USB.print(__speed);
        SERIAL_USB.println(" kmh");
    }
    SERIAL_USB.println("}");
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
        SERIAL_USB.println("CAN init fail, retry...");
        delay(100);
    }
    SERIAL_USB.println("CAN init ok!");
    set_mask_filt();

    SERIAL_USB.println("Enabling GPS");
    SERIAL_GPS.begin(9600);
}

void loop()
{
    /** service the GPS serial and print a completed message if there is one */
    char message_buf[GPS_RECV_BUF_LEN];
    message_buf[0] = 0;
    if (receive_gps(message_buf, GPS_RECV_BUF_LEN) > 0)
    {
        SERIAL_USB.print("gps: {");
        SERIAL_USB.write(message_buf);
        SERIAL_USB.println("}");
        int length = strlen(message_buf);
        SERIAL_USB.print("length: ");
        SERIAL_USB.println(length);
    }
    /** print CAN bus data */
    // print_can_bus_data()
}

// END FILE