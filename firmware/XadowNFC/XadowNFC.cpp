
#include <stdlib.h>
#include "mbed.h"
#include "XadowNFC.h"
#include "USBSerial.h"


unsigned char uid[10] = {0};
static char cmd[2];

extern I2C i2c;
extern USBSerial dbg_serial;

unsigned char nfc_check_online(void)
{
	unsigned char data[6];
	unsigned char i;

	//dlc_i2c_configure(NFC_DEVICE_ADDR, 100);
	//dlc_i2c_send_byte(NFC_SCAN_ID);
	//dlc_i2c_send_byte(0);
    
    cmd[0] = NFC_SCAN_ID;
    cmd[1] = 0;
    i2c.write(NFC_DEVICE_ADDR, cmd, 1);  //the nfc module's i2c supports only 1 byte read/write per burst
    i2c.write(NFC_DEVICE_ADDR, cmd+1, 1);


	for(i=0;i<6;i++)
	{
		//data[i] = dlc_i2c_receive_byte();
        i2c.read(NFC_DEVICE_ADDR, (char *)&data[i], 1);  //the nfc module's i2c supports only 1 byte read/write per burst
	}

	if(data[5] == (NFC_DEVICE_ADDR>>1))return 1;
	else return 0;
}

void nfc_adapter_init(void)
{
    cmd[0] = NFC_ADAPTER_INIT_ID;
    cmd[1] = 0;
    i2c.write(NFC_DEVICE_ADDR, cmd, 1);
    i2c.write(NFC_DEVICE_ADDR, cmd+1, 1);
}

unsigned char *nfc_adapter_get_uid(void)
{
	unsigned char i;

    cmd[0] = NFC_ADAPTER_GET_UID_ID;
    cmd[1] = 0;
    i2c.write(NFC_DEVICE_ADDR, cmd, 1);
    i2c.write(NFC_DEVICE_ADDR, cmd+1, 1);


	for(i=0;i<2;i++)
	{
		//uid[i] = dlc_i2c_receive_byte();
        i2c.read(NFC_DEVICE_ADDR, (char *)&uid[i], 1);
	}

	for(i=0;i<uid[1];i++)
	{
		//uid[i + 2]= dlc_i2c_receive_byte();
        i2c.read(NFC_DEVICE_ADDR, (char *)&uid[i+2], 1);
	}
	uid[uid[1] + 2] = '\0';

	return (unsigned char *)(uid+2);
}

unsigned char *nfc_adapter_read(void)
{
	unsigned char data[2];
	unsigned char i;

    cmd[0] = NFC_ADAPTER_READ_ID;
    cmd[1] = 0;
    i2c.write(NFC_DEVICE_ADDR, cmd, 1);
    i2c.write(NFC_DEVICE_ADDR, cmd+1, 1);

	for(i=0;i<2;i++)
	{
		//data[i] = dlc_i2c_receive_byte();
        i2c.read(NFC_DEVICE_ADDR, (char *)&data[i], 1);
	}

	unsigned char length = data[1];
	unsigned char *ptr = (unsigned char *)malloc(length + 2 + 1);
	*ptr = data[0];
	*(ptr + 1) = data[1];
	*(ptr + length + 2) = '\0';

	for(i=0;i<*(ptr + 1);i++)
	{
		//*(ptr + i + 2) = dlc_i2c_receive_byte();
        i2c.read(NFC_DEVICE_ADDR, (char *)(ptr + i + 2), 1);
	}
	
	return ptr;
}

void nfc_adapter_write(char *string)
{
	unsigned char length;
	unsigned char i;

	length = strlen(string);

    cmd[0] = NFC_ADAPTER_WRITE_ID;
    cmd[1] = length;
    i2c.write(NFC_DEVICE_ADDR, cmd, 1);
    i2c.write(NFC_DEVICE_ADDR, cmd+1, 1);

	for(i=0;i<length;i++)
	{
		//dlc_i2c_send_byte(*(string + i));
        i2c.write(NFC_DEVICE_ADDR, (string + i), 1);
	}
}

void nfc_adapter_erase(void)
{    
    cmd[0] = NFC_ADAPTER_ERASE_ID;
    cmd[1] = 0;
    i2c.write(NFC_DEVICE_ADDR, cmd, 1);
    i2c.write(NFC_DEVICE_ADDR, cmd+1, 1);
}

void nfc_emulate_init(unsigned char *id)
{
	unsigned char i;
    
    cmd[0] = NFC_EMULATE_INIT_ID;
    cmd[1] = 3;
    i2c.write(NFC_DEVICE_ADDR, cmd, 1);
    i2c.write(NFC_DEVICE_ADDR, cmd+1, 1);

	for(i=0;i<3;i++)
	{
		//dlc_i2c_send_byte(*(id + i));
        i2c.write(NFC_DEVICE_ADDR, (char *)(id + i), 1);
	}
}

