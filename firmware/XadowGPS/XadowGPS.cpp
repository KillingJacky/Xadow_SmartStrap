
#include <stdlib.h>
#include "mbed.h"
#include "XadowGPS.h"
#include "USBSerial.h"



unsigned char gps_utc_date_time[GPS_UTC_DATE_TIME_SIZE] = {0};
static char cmd[2];

extern I2C i2c;
extern USBSerial dbg_serial;


unsigned char gps_check_online(void)
{
    unsigned char data[GPS_SCAN_SIZE+2];
    unsigned char i;

    //dlc_i2c_configure(GPS_DEVICE_ADDR, 100);
    //dlc_i2c_send_byte(GPS_SCAN_ID);
    cmd[0] = GPS_SCAN_ID;
    i2c.write(GPS_DEVICE_ADDR, cmd, 1);

    for(i=0;i<(GPS_SCAN_SIZE+2);i++)
    {
        i2c.read(GPS_DEVICE_ADDR, (char *)&data[i], 1);  //the gps module's i2c supports only 1 byte read/write per burst
    }

    if(data[5] == (GPS_DEVICE_ADDR>>1))return 1;
    else return 0;
}

unsigned char* gps_get_utc_date_time(void)
{
    unsigned char data[GPS_UTC_DATE_TIME_SIZE+2];
    unsigned char i;

    //dlc_i2c_configure(GPS_DEVICE_ADDR, 100);
    //dlc_i2c_send_byte(GPS_UTC_DATE_TIME_ID);
	
	cmd[0] = GPS_UTC_DATE_TIME_ID;
    i2c.write(GPS_DEVICE_ADDR, cmd, 1);

    for(i=0;i<(GPS_UTC_DATE_TIME_SIZE+2);i++)
    {
        //data[i] = dlc_i2c_receive_byte();
        i2c.read(GPS_DEVICE_ADDR, (char *)&data[i], 1);  //the gps module's i2c supports only 1 byte read/write per burst
    }

    for(i=0;i<GPS_UTC_DATE_TIME_SIZE;i++)
        gps_utc_date_time[i] = data[i+2];

    return gps_utc_date_time;
}

unsigned char gps_get_status(void)
{
    unsigned char data[GPS_STATUS_SIZE+2];
    unsigned char i;

    //dlc_i2c_configure(GPS_DEVICE_ADDR, 100);
    //dlc_i2c_send_byte(GPS_STATUS_ID);
	
	cmd[0] = GPS_STATUS_ID;
    i2c.write(GPS_DEVICE_ADDR, cmd, 1);

  	for(i=0;i<(GPS_STATUS_SIZE+2);i++)
    {
        //data[i] = dlc_i2c_receive_byte();
        i2c.read(GPS_DEVICE_ADDR, (char *)&data[i], 1);  //the gps module's i2c supports only 1 byte read/write per burst
    }

    return data[2];
}

float gps_get_latitude(void)
{
    unsigned char data[GPS_LATITUDE_SIZE+2];
    unsigned char i;

    //dlc_i2c_configure(GPS_DEVICE_ADDR, 100);
    //dlc_i2c_send_byte(GPS_LATITUDE_ID);
	cmd[0] = GPS_LATITUDE_ID;
    i2c.write(GPS_DEVICE_ADDR, cmd, 1);
	
    for(i=0;i<(GPS_LATITUDE_SIZE+2);i++)
    {
        i2c.read(GPS_DEVICE_ADDR, (char *)&data[i], 1);  //the gps module's i2c supports only 1 byte read/write per burst
    }

    return atof((const char *)&data[2]);
}

unsigned char gps_get_ns(void)
{
    unsigned char data[GPS_NS_SIZE+2];
    unsigned char i;

    //dlc_i2c_configure(GPS_DEVICE_ADDR, 100);
    //dlc_i2c_send_byte(GPS_NS_ID);
	cmd[0] = GPS_NS_ID;
    i2c.write(GPS_DEVICE_ADDR, cmd, 1);

    for(i=0;i<(GPS_NS_SIZE+2);i++)
    {
        i2c.read(GPS_DEVICE_ADDR, (char *)&data[i], 1);  //the gps module's i2c supports only 1 byte read/write per burst
    }

    if(data[2] == 'N' || data[2] == 'S')return data[2];
    else return data[2] = '-';

}

float gps_get_longitude(void)
{
    unsigned char data[GPS_LONGITUDE_SIZE+2];
    unsigned char i;

    //dlc_i2c_configure(GPS_DEVICE_ADDR, 100);
    //dlc_i2c_send_byte(GPS_LONGITUDE_ID);
	cmd[0] = GPS_LONGITUDE_ID;
    i2c.write(GPS_DEVICE_ADDR, cmd, 1);

    for(i=0;i<(GPS_LONGITUDE_SIZE+2);i++)
    {
        i2c.read(GPS_DEVICE_ADDR, (char *)&data[i], 1);  //the gps module's i2c supports only 1 byte read/write per burst
    }

    return atof((const char *)&data[2]);
}

unsigned char gps_get_ew(void)
{
    unsigned char data[GPS_EW_SIZE+2];
    unsigned char i;

    //dlc_i2c_configure(GPS_DEVICE_ADDR, 100);
    //dlc_i2c_send_byte(GPS_EW_ID);
	cmd[0] = GPS_EW_ID;
    i2c.write(GPS_DEVICE_ADDR, cmd, 1);

    for(i=0;i<(GPS_EW_SIZE+2);i++)
    {
        i2c.read(GPS_DEVICE_ADDR, (char *)&data[i], 1);  //the gps module's i2c supports only 1 byte read/write per burst
    }

    if(data[2] == 'E' || data[2] == 'W')return data[2];
    else return data[2] = '-';
}

float gps_get_speed(void)
{
    unsigned char data[GPS_SPEED_SIZE+2];
    unsigned char i;

    //dlc_i2c_configure(GPS_DEVICE_ADDR, 100);
    //dlc_i2c_send_byte(GPS_SPEED_ID);
	cmd[0] = GPS_SPEED_ID;
    i2c.write(GPS_DEVICE_ADDR, cmd, 1);

    for(i=0;i<(GPS_SPEED_SIZE+2);i++)
    {
        i2c.read(GPS_DEVICE_ADDR, (char *)&data[i], 1);  //the gps module's i2c supports only 1 byte read/write per burst
    }

    return atof((const char *)&data[2]);
}

float gps_get_course(void)
{
    unsigned char data[GPS_COURSE_SIZE+2];
    unsigned char i;

    //dlc_i2c_configure(GPS_DEVICE_ADDR, 100);
    //dlc_i2c_send_byte(GPS_COURSE_ID);
	cmd[0] = GPS_COURSE_ID;
    i2c.write(GPS_DEVICE_ADDR, cmd, 1);

    for(i=0;i<(GPS_COURSE_SIZE+2);i++)
    {
        i2c.read(GPS_DEVICE_ADDR, (char *)&data[i], 1);  //the gps module's i2c supports only 1 byte read/write per burst
    }

    return atof((const char *)&data[2]);
}

unsigned char gps_get_position_fix(void)
{
    unsigned char data[GPS_POSITION_FIX_SIZE+2];
    unsigned char i;

    //dlc_i2c_configure(GPS_DEVICE_ADDR, 100);
    //dlc_i2c_send_byte(GPS_POSITION_FIX_ID);
	cmd[0] = GPS_POSITION_FIX_ID;
    i2c.write(GPS_DEVICE_ADDR, cmd, 1);

    for(i=0;i<(GPS_POSITION_FIX_SIZE+2);i++)
    {
        i2c.read(GPS_DEVICE_ADDR, (char *)&data[i], 1);  //the gps module's i2c supports only 1 byte read/write per burst
    }

    return data[2] - '0';
}

unsigned char gps_get_sate_used(void)
{
    unsigned char data[GPS_SATE_USED_SIZE+2];
    unsigned char i;
    unsigned char value;

    //dlc_i2c_configure(GPS_DEVICE_ADDR, 100);
    //dlc_i2c_send_byte(GPS_SATE_USED_ID);
	cmd[0] = GPS_SATE_USED_ID;
    i2c.write(GPS_DEVICE_ADDR, cmd, 1);

    for(i=0;i<(GPS_SATE_USED_SIZE+2);i++)
    {
        i2c.read(GPS_DEVICE_ADDR, (char *)&data[i], 1);  //the gps module's i2c supports only 1 byte read/write per burst
    }
    if(data[3] >= '0' && data[3] <= '9' )value = (data[3] - '0') * 10;
    else value = 0;
    if(data[2] >= '0' && data[2] <= '9' )value += (data[2] - '0');
    else value += 0;

    return value;
}

float gps_get_altitude(void)
{
    unsigned char data[GPS_ALTITUDE_SIZE+2];
    unsigned char i;

    //dlc_i2c_configure(GPS_DEVICE_ADDR, 100);
    //dlc_i2c_send_byte(GPS_ALTITUDE_ID);
 	cmd[0] = GPS_ALTITUDE_ID;
    i2c.write(GPS_DEVICE_ADDR, cmd, 1);
 
    for(i=0;i<(GPS_ALTITUDE_SIZE+2);i++)
    {
        i2c.read(GPS_DEVICE_ADDR, (char *)&data[i], 1);  //the gps module's i2c supports only 1 byte read/write per burst
    }

    return atof((const char *)&data[2]);
}

char gps_get_mode(void)
{
    char data[GPS_MODE_SIZE+2];
    unsigned char i;

    //dlc_i2c_configure(GPS_DEVICE_ADDR, 100);
    //dlc_i2c_send_byte(GPS_MODE_ID);
 	cmd[0] = GPS_MODE_ID;
    i2c.write(GPS_DEVICE_ADDR, cmd, 1);

    for(i=0;i<(GPS_MODE_SIZE+2);i++)
    {
        i2c.read(GPS_DEVICE_ADDR, (char *)&data[i], 1);  //the gps module's i2c supports only 1 byte read/write per burst
    }

    return data[2];
}

unsigned char gps_get_mode2(void)
{
    unsigned char data[GPS_MODE2_SIZE+2];
    unsigned char i;

    //dlc_i2c_configure(GPS_DEVICE_ADDR, 100);
    //dlc_i2c_send_byte(GPS_MODE2_ID);
 	cmd[0] = GPS_MODE2_ID;
    i2c.write(GPS_DEVICE_ADDR, cmd, 1);

    for(i=0;i<(GPS_MODE2_SIZE+2);i++)
    {
        i2c.read(GPS_DEVICE_ADDR, (char *)&data[i], 1);  //the gps module's i2c supports only 1 byte read/write per burst
    }

    return data[2] - '0';
}
