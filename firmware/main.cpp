#include "mbed.h"
#include "USBSerial.h"
#include "BufferedSerial.h"
#include "XadowGPS.h"
#include "XadowNFC.h"

extern "C" {
#include "PebbleSerial.h"
}

#define _BV(bit) (1<<(bit))
#define cbi(sfr, bit) ((sfr) &= ~_BV(bit))
#define sbi(sfr, bit) ((sfr) |= _BV(bit))


enum{
    ENABLE_CHG = 0,
    DISABLE_CHG,
    GET_VBAT,
    CHK_GPS,
    GET_GPS_INFO,
    CHK_NFC,
    INIT_NFC_AS_ADAPTER,  
    GET_NFC_TAGID,
    GET_NFC_NDEF,
    SET_NFC_NDEF,
    ERASE_NFC_NDEF,
    INIT_NFC_AS_TAG,  
    COMMANDS_CNT
};

static const uint8_t PEBBLE_RECV_BUFFER_SIZE = 100;
static uint8_t s_pebble_buffer[PEBBLE_RECV_BUFFER_SIZE+5];

DigitalOut pin_chg_led_on(P0_2);
DigitalOut pin_5v_en(P1_19);
AnalogIn   ain(A0);

USBSerial dbg_serial;
BufferedSerial serial(P0_19, P0_18);  //(tx,rx)

I2C i2c(P0_5, P0_4);

/*
 * Below are some platform-dependant callbacks for controling the half-duplex UART
 */
#ifdef __cplusplus
extern "C" {
#endif
static void prv_pebble_control(PebbleControl cmd)
{
    switch (cmd) {
        case PebbleControlEnableTX:
            // enable transmitter
            sbi(LPC_USART->TER, 7);
            // disable receiver
            sbi(LPC_USART->RS485CTRL, 1);
            break;
        case PebbleControlDisableTX:
            // disable transmitter
            cbi(LPC_USART->TER, 7);
            // set TX pin as input with pullup
            //pinMode(1, INPUT_PULLUP);
            // enable receiver
            cbi(LPC_USART->RS485CTRL, 1);
            break;
        case PebbleControlFlushTX:
            // wait for the data to be transmitted
                        while (serial._txbuf.available());
            while (!(LPC_USART->LSR & _BV(5)));
            // small delay for the lines to settle down
            wait_ms(1);
            break;
        default:
            break;
    }
}
static void prv_pebble_write_byte(uint8_t data)
{
    serial.putc(data);
}
#ifdef __cplusplus
} // extern c
#endif


float get_vbat()
{
    return 3.3f*ain*2.25;
}

int main()
{
    /* init io */
    pin_chg_led_on = 0; //pull low to enable charge led
    pin_5v_en      = 1; //pull high to enable battery to charge pebble

    /* init uart */
    serial.baud(57600);

    wait(1);
    dbg_serial.printf("I am a virtual serial port\n");

    PebbleCallbacks callbacks = {
        .write_byte = prv_pebble_write_byte,
        .control = prv_pebble_control,
    };
    pebble_init(PebbleProtocolRawData, callbacks);
    pebble_prepare_for_read(s_pebble_buffer, PEBBLE_RECV_BUFFER_SIZE);

    dbg_serial.printf("init done!\n");

    /* init i2c */
    i2c.frequency(100000); //100khz
    
    while(0) {
        float vbat = get_vbat();
                        
        dbg_serial.printf("vbat: %f\n", vbat);
        
        wait(1);    
    }
    
    
    while(1) {
        while (serial.readable()) {
            PebbleProtocol protocol;
            uint8_t length;
            bool is_read;
            uint8_t data = (uint8_t)serial.getc();
            //dbg_serial.printf("0x%x\n", data);
            if (pebble_handle_byte(data, &protocol, &length, &is_read)) {
                // we have a frame to process
                if (protocol == PebbleProtocolRawData) {
                    s_pebble_buffer[length] = '\0';
                    dbg_serial.printf("Read %d bytes (is_read=%s): ", length, is_read ? "Y" : "N");
                    //dbg_serial.printf((char *)s_pebble_buffer);
                    dbg_serial.printf("\n");
                    
                    //---- transfer data to pebble ----
                    if (s_pebble_buffer[0] == GET_VBAT && is_read)
                    {
                        dbg_serial.printf("GET_VBAT\n");
                        
                        memset(s_pebble_buffer, 0, PEBBLE_RECV_BUFFER_SIZE);
                        float vbat = get_vbat();
                        
                        dbg_serial.printf("vbat: %f\n", vbat);
                        
                        uint8_t *tmp = s_pebble_buffer;
                        
                        memcpy(tmp, &vbat, 4);
                        
                        pebble_write(s_pebble_buffer, 4);
                    }
                    else if (s_pebble_buffer[0] == ENABLE_CHG)
                    {
                        dbg_serial.printf("ENABLE_CHG\n");
                        
                        pin_5v_en = 1;
                        memset(s_pebble_buffer, 0, PEBBLE_RECV_BUFFER_SIZE);
                        s_pebble_buffer[0] = '#';
                        pebble_write(s_pebble_buffer, 2);
                    }
                    else if (s_pebble_buffer[0] == DISABLE_CHG)
                    {
                        dbg_serial.printf("DISABLE_CHG\n");
                        
                        pin_5v_en = 0;
                        memset(s_pebble_buffer, 0, PEBBLE_RECV_BUFFER_SIZE);
                        s_pebble_buffer[0] = '#';
                        pebble_write(s_pebble_buffer, 2);
                    }
                    else if (s_pebble_buffer[0] == CHK_GPS && is_read)
                    {
                        dbg_serial.printf("CHK_GPS\n");
                        
                        memset(s_pebble_buffer, 0, PEBBLE_RECV_BUFFER_SIZE);
                        uint8_t online = gps_check_online();
                        dbg_serial.printf("gps online: %d\n", online);
                        memcpy(s_pebble_buffer, &online, 1);
                        
                        pebble_write(s_pebble_buffer, 1);
                    }
                    else if (s_pebble_buffer[0] == GET_GPS_INFO && is_read)
                    {
                        dbg_serial.printf("GET_GPS_INFO\n");
                        
                        memset(s_pebble_buffer, 0, PEBBLE_RECV_BUFFER_SIZE);
                        float lat, lon, v, alt;
                        uint8_t fix, sat;
                        lat = gps_get_latitude();
                        lon = gps_get_longitude();
                        v = gps_get_speed();
                        alt = gps_get_altitude();
                        fix = gps_get_position_fix();
                        fix = (fix == 1 || fix == 2)? 1:0;
                        sat = gps_get_sate_used();
                        dbg_serial.printf("fix: %d, sate: %d\n", fix, sat);
                        
                        uint8_t *tmp = s_pebble_buffer;
                        
                        memcpy(tmp, &lat, 4); tmp+=4;
                        memcpy(tmp, &lon, 4); tmp+=4; 
                        memcpy(tmp, &v, 4); tmp+=4;
                        memcpy(tmp, &alt, 4); tmp+=4;
                        memcpy(tmp, &fix, 1); tmp+=1;
                        memcpy(tmp, &sat, 1);
                        
                        pebble_write(s_pebble_buffer, 18);
                    }
                    else if (s_pebble_buffer[0] == CHK_NFC && is_read)
                    {
                        dbg_serial.printf("CHK_NFC\n");
                        
                        memset(s_pebble_buffer, 0, PEBBLE_RECV_BUFFER_SIZE);
                        uint8_t online = nfc_check_online();
                        dbg_serial.printf("nfc online: %d\n", online);
                        uint8_t *tmp = s_pebble_buffer;
                        memcpy(tmp, &online, 1);
                        
                        pebble_write(s_pebble_buffer, 1);
                    }                 
                    else if (s_pebble_buffer[0] == INIT_NFC_AS_ADAPTER)
                    {
                        dbg_serial.printf("INIT_NFC_AS_ADAPTER\n");
                        
                        nfc_adapter_init();
                        memset(s_pebble_buffer, 0, PEBBLE_RECV_BUFFER_SIZE);
                        s_pebble_buffer[0] = '#';
                        pebble_write(s_pebble_buffer, 2);
                    }
                    else if (s_pebble_buffer[0] == GET_NFC_TAGID && is_read)
                    {   
                        memset(s_pebble_buffer, 0, PEBBLE_RECV_BUFFER_SIZE);
                        uint8_t *uid = nfc_adapter_get_uid();
                        dbg_serial.printf("GET_NFC_TAGID: %s\n", uid);
                        strncpy((char *)s_pebble_buffer, (char *)uid, 8);
                        
                        pebble_write(s_pebble_buffer, 10);
                    }
                    else if (s_pebble_buffer[0] == GET_NFC_NDEF && is_read)
                    {
                        dbg_serial.printf("GET_NFC_NDEF\n");
                        
                        uint8_t *ndef = nfc_adapter_read();
                        int len = strlen((char *)ndef)+1;
                        int burst_cnt = (len/PEBBLE_RECV_BUFFER_SIZE) + ((len % PEBBLE_RECV_BUFFER_SIZE)==0?0:1);
                        
                        for (int i=0; i<burst_cnt; i++)
                        {
                            memset(s_pebble_buffer, 0, PEBBLE_RECV_BUFFER_SIZE);
                            strncpy((char *)s_pebble_buffer, (char *)(ndef + i*PEBBLE_RECV_BUFFER_SIZE), PEBBLE_RECV_BUFFER_SIZE);
                            pebble_write(s_pebble_buffer, 10);
                        }
                        free(ndef);
                    }
                    else if (s_pebble_buffer[0] == SET_NFC_NDEF)
                    {
                        dbg_serial.printf("SET_NFC_NDEF\n");
                        
                        if(s_pebble_buffer[PEBBLE_RECV_BUFFER_SIZE-1] != '\0')
                            s_pebble_buffer[PEBBLE_RECV_BUFFER_SIZE-1] = '\0';
                        nfc_adapter_write((char *)(s_pebble_buffer+1));
                    }
                    else if (s_pebble_buffer[0] == ERASE_NFC_NDEF)
                    {
                        dbg_serial.printf("ERASE_NFC_NDEF\n");
                        
                        nfc_adapter_erase();
                        
                        memset(s_pebble_buffer, 0, PEBBLE_RECV_BUFFER_SIZE);
                        s_pebble_buffer[0] = '#';
                        pebble_write(s_pebble_buffer, 2);
                    }
                    else if (s_pebble_buffer[0] == INIT_NFC_AS_TAG)
                    {
                        dbg_serial.printf("INIT_NFC_AS_TAG\n");
                        
                        nfc_emulate_init(s_pebble_buffer+1);
                        
                        memset(s_pebble_buffer, 0, PEBBLE_RECV_BUFFER_SIZE);
                        s_pebble_buffer[0] = '#';
                        pebble_write(s_pebble_buffer, 2);
                    }
                    
                    
                } else if (protocol == PebbleProtocolLinkControl) {
                    pebble_handle_link_control(s_pebble_buffer);
                }
                // re-post the receive buffer
                pebble_prepare_for_read(s_pebble_buffer, PEBBLE_RECV_BUFFER_SIZE);
            }
        }
    }
}
