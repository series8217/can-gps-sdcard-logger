#include "gps.h"
#include "common.h"

#include <cstring>

// protocol: https://raw.githubusercontent.com/Longan-Labs/OBD-II_Slaver_GPS_kit/master/NEO-6_DataSheet_(GPS.G6-HW-09005).pdf

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
        if (next_value == 0 || next_value > 255)
        {
            // skip invalid characters
            continue;
        }
        if (next_value < 0)
        {
            // no data available
            break;
        }
        gps_message[gps_message_length] = (char)next_value;
        gps_message_length++;
        if ((char)next_value == '\n')
        {
            // SERIAL_USB.write(gps_message);
            message_complete = 1;
            break;
        }
    }
    // if we have a complete message, copy it to the receive buffer
    if (message_complete > 0)
    {
        gps_message[gps_message_length] = 0;
        gps_message_length += 1;
        if (buf_len > 0)
        {
            strlcpy(receive_buf, gps_message, buf_len);
        }
        size_t final_length = gps_message_length;
        // start over on next message
        gps_message_length = 0;
        return final_length;
    }
    if (gps_message_length >= GPS_RECV_BUF_LEN - 1)
    {
        // something went wrong.. we didn't see an EOL char.
        // reset the buffer
        gps_message_length = 0;
        gps_message[0] = 0;
    }
    if (buf_len > 0)
    {
        // null terminate the receive buffer since we didn't copy a message
        receive_buf[0] = 0;
    }
    return 0;
}

GPS_GPRMC::GPS_GPRMC() {
    utc_time[0] = 0;
    status = 0;
    latitude[0] = 0;
    latitude_direction = 0;
    longitude[0] = 0;
    longitude_direction = 0;
    speed_knots[0] = 0;
    course_deg[0] = 0;
    date[0] = 0;
}

void GPS_GPRMC::print() {
    cout << "gps: {";
    cout << "utc_time: " << utc_time << ", ";
    cout << "status: " << status << ", ";
    cout << "latitude: " << latitude << ", ";
    cout << "latitude_direction: " << latitude_direction << ", ";
    cout << "longitude: " << longitude << ", ";
    cout << "longitude_direction: " << longitude_direction << ", ";
    cout << "speed_knots: " << speed_knots << ", ";
    cout << "course_deg: " << course_deg << ", ";
    cout << "date: " << date;
    cout << "}" << endl;
}

bool GPS_GPRMC::parse(char *gps_message) {
    char *token = strtok(gps_message, ",");
    if (strcmp(token, "$GPRMC") != 0){
        return false;
    }
    // UTC time
    token = strtok(NULL, ",");
    if (token){
        strcpy(utc_time, token);
    }
    // status
    token = strtok(NULL, ",");
    if (token){
        status = token[0];
    }
    // latitude
    token = strtok(NULL, ",");
    if (token){
        strcpy(latitude, token);
    }
    // latitude direction
    token = strtok(NULL, ",");
    if (token){
        latitude_direction = token[0];
    }
    // longitude
    token = strtok(NULL, ",");
    if (token){
        strcpy(longitude, token);
    }
    // longitude direction
    token = strtok(NULL, ",");
    if (token){
        longitude_direction = token[0];
    }
    // speed in knots
    token = strtok(NULL, ",");
    if (token){
        strcpy(speed_knots, token);
    }
    // course in degrees
    token = strtok(NULL, ",");
    if (token){
        strcpy(course_deg, token);
    }
    // date
    token = strtok(NULL, ",");
    if (token){
        strcpy(date, token);
    }
    return true;
}