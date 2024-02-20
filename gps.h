#ifndef GPS_H
#define GPS_H

#include <cstring>

#define GPS_RECV_BUF_LEN 100
// protocol: https://raw.githubusercontent.com/Longan-Labs/OBD-II_Slaver_GPS_kit/master/NEO-6_DataSheet_(GPS.G6-HW-09005).pdf

extern char gps_message[GPS_RECV_BUF_LEN];

int receive_gps(char *receive_buf, size_t buf_len);

class GPS_GPRMC {
    char utc_time[12];
    char status;
    char latitude[11];
    char latitude_direction;
    char longitude[12];
    char longitude_direction;
    char speed_knots[6];
    char course_deg[7];
    char date[7];

    public:
    GPS_GPRMC();
    void print();
    bool parse(char *gps_message);
};

#endif // GPS_H