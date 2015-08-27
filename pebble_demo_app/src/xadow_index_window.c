#include <pebble.h>
#include "dialog_choice_window.h"
#include "xadow.h"

#define TIMEOUT_MS 1000
#define MAX_READ_SIZE 100

static Window *s_main_window;
static TextLayer *s_conn_status_layer;
static TextLayer *s_data_layer;
static StatusBarLayer *s_status_bar;

static int cnt_dot = 0;
static int cnt_fail = 0;

static uint8_t s_buffer[MAX_READ_SIZE + 1];
static char connection_text[20];
uint8_t charge = 0;
static uint8_t connected = 0;
static uint8_t req_type = GET_VBAT;

//gps data
static uint8_t gps_online = 0;
static float vbat,lat,lon,velocity,alt;
static uint8_t fix, sat;
static char str_vbat[10];
static char str_lat[10];
static char str_lon[10];
static char str_vel[10];
static char str_alt[10];

//nfc data
static uint8_t nfc_online = 0;
static uint8_t nfc_initialed = 0;
static char tagid[10];

static void conn_timer_handler(void *context);
static void prv_send_request(void *context);
static void connection_status_text_show();
static void connection_status_text_hide();
static void data_text_show();
static void data_text_hide();
void ftoa(char* str, double val, int precision);

static void update_connection_status_text(void) {
    if (connected) {
        memcpy(connection_text, "Connected", 9);
        connection_text[9]='\0';
        text_layer_set_text(s_conn_status_layer, connection_text);
        return;
    }
    if(++cnt_dot > 3) cnt_dot = 1;
    memcpy(connection_text, "Connecting\n...", 11+cnt_dot);
    connection_text[11+cnt_dot]='\0';
    text_layer_set_text(s_conn_status_layer, connection_text);
}

static void update_data_text(void)
{
    if (!connected) {
        return;
    }
    ftoa(str_vbat, vbat, 1);
    ftoa(str_lat, lat, 3);
    ftoa(str_lon, lon, 3);
    ftoa(str_vel, velocity, 3);
    ftoa(str_alt, alt, 3);
    snprintf((char *)s_buffer, sizeof(s_buffer), "VBAT: %s\nGPS:\nlat: %s lon: %s\nvel: %s alt: %s\nfix: %d num of sate: %d\n\nNFC TAG ID:\n %0X %0X %0X %0X", str_vbat, str_lat, str_lon, str_vel, str_alt, fix, sat, tagid[0], tagid[1], tagid[2], tagid[3]);
    text_layer_set_text(s_data_layer, (const char *)s_buffer);
}

static void extract_data_from_stream(uint8_t *req, uint8_t *buff, int len)
{
    switch(*req)
    {
        case GET_VBAT:
        {
            if (len >= 4)
            {
                memcpy(&vbat, buff, 4);
                if(vbat > 2.0)
                {
                    if(gps_online)
                        *req = GET_GPS_INFO;
                    else
                        *req = CHK_GPS;
                }
            }
            break;
        }

        case CHK_GPS:
        {
            memcpy(&gps_online, buff, 1);
            if(gps_online)
                *req = GET_GPS_INFO;
            else
                *req = CHK_NFC;
            break;
        }
        case GET_GPS_INFO:
        {
            if(len >= 18)
            {
                memcpy(&lat, buff, 4); buff+=4;
                memcpy(&lon, buff, 4); buff+=4;
                memcpy(&velocity, buff, 4); buff+=4;
                memcpy(&alt, buff, 4); buff+=4;
                memcpy(&fix, buff, 1); buff+=1;
                memcpy(&sat, buff, 1);
            }
            if(nfc_online)
                *req = GET_NFC_TAGID;
            else
                *req = CHK_NFC;
            break;
        }
        case CHK_NFC:
        {
            memcpy(&nfc_online, buff, 1);
            if(nfc_online && !nfc_initialed)
                *req = INIT_NFC_AS_ADAPTER;
            else if (nfc_online && nfc_initialed)
                *req = GET_NFC_TAGID;
            else
                *req = GET_VBAT;
            break;
        }
        case INIT_NFC_AS_ADAPTER:
        {
            *req = GET_NFC_TAGID;
            break;
        }
        case GET_NFC_TAGID:
        {
            memcpy(tagid, buff, len);
            *req = GET_VBAT;
            break;
        }
        default:
            break;
    }
}

static void prv_read_done(bool success, uint32_t length) {
  if (success) {
    cnt_fail = 0;
    s_buffer[length] = '\0';
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Read %d bytes: %s", (int)length, s_buffer);
    if (length == 0) {
      //text_layer_set_text(s_rfid_layer, "-");
    } else {
      //extract data from s_buffer
      extract_data_from_stream(&req_type, s_buffer, length);
      update_data_text();
    }

    //if(++req_type >= GET_ITEM_CNT) req_type = GET_VBAT;

    app_timer_register(500, prv_send_request, NULL);
  }else
  {
    cnt_fail++;
    if (cnt_fail > 10)
    {
        cnt_fail = 0;
        connected = 0;
        data_text_hide();
        connection_status_text_show();
        update_connection_status_text();
        smartstrap_disconnect();
        app_timer_register(100, conn_timer_handler, NULL);
        return;
    }
    app_timer_register(1000, prv_send_request, NULL);
  }

}

static void prv_send_request(void *context) {
  SmartstrapResult result = smartstrap_send((uint8_t *)&req_type, 1,
                                             s_buffer, MAX_READ_SIZE, prv_read_done, TIMEOUT_MS);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent data (result=%d)", result);
}

static void prv_connection_callback(bool is_connected) {
  connected = is_connected;

  if (is_connected) {
    update_connection_status_text();
    connection_status_text_hide();
    data_text_show();
    app_timer_register(100, prv_send_request, NULL);
  } else {
    update_connection_status_text();
    app_timer_register(1000, conn_timer_handler, NULL);
  }
}

static void conn_timer_handler(void *context) {
  smartstrap_connect(prv_connection_callback);
}

static void connection_status_text_show()
{
  layer_set_hidden(text_layer_get_layer(s_conn_status_layer), false);
}

static void connection_status_text_hide()
{
    layer_set_hidden(text_layer_get_layer(s_conn_status_layer), true);
}

static void data_text_show()
{
  layer_set_hidden(text_layer_get_layer(s_data_layer), false);
  update_data_text();
}

static void data_text_hide()
{
    layer_set_hidden(text_layer_get_layer(s_data_layer), true);
}

static void prv_main_window_load(Window *window) {

  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_separator_mode(s_status_bar, StatusBarLayerSeparatorModeDotted);
  status_bar_layer_set_colors(s_status_bar, GColorDarkGreen, GColorWhite);
  layer_add_child(window_get_root_layer(window), status_bar_layer_get_layer(s_status_bar));

  s_conn_status_layer = text_layer_create(GRect(0, 56, 144, 80));
  text_layer_set_font(s_conn_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  update_connection_status_text();
  text_layer_set_text_color(s_conn_status_layer, GColorDarkGreen);
  text_layer_set_background_color(s_conn_status_layer, GColorClear);
  text_layer_set_text_alignment(s_conn_status_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_conn_status_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_conn_status_layer));

  s_data_layer = text_layer_create(GRect(0, 10, 144, 160));
  text_layer_set_font(s_data_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  //update_data_text();
  text_layer_set_text_color(s_data_layer, GColorBlack);
  text_layer_set_background_color(s_data_layer, GColorClear);
  text_layer_set_text_alignment(s_data_layer, GTextAlignmentLeft);
  text_layer_set_overflow_mode(s_data_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_data_layer));

  data_text_hide();
}

static void prv_main_window_unload(Window *window) {
  text_layer_destroy(s_conn_status_layer);
  text_layer_destroy(s_data_layer);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  //uint8_t *data = context;
  //ask_for_scroll(data, ScrollDirectionUp);
  if(connected)
  {
      connection_status_text_hide();
      data_text_show();
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  //WeatherAppData *data = context;
  //ask_for_scroll(data, ScrollDirectionDown);
  //connected = 0;
  data_text_hide();
  connection_status_text_show();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(connected){
      dialog_choice_window_push(context);
  }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void prv_init(void) {
  s_main_window = window_create();
  window_set_click_config_provider_with_context(s_main_window, click_config_provider, NULL);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = prv_main_window_load,
    .unload = prv_main_window_unload
  });
  window_stack_push(s_main_window, true);
  app_timer_register(100, conn_timer_handler, NULL);
  smartstrap_set_power(true);
}

static void prv_deinit(void) {
  window_destroy(s_main_window);
  smartstrap_disconnect();
  smartstrap_set_power(false);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}

void ftoa(char* str, double val, int precision) {
  //  start with positive/negative
  if (val < 0) {
    *(str++) = '-';
    val = -val;
  }
  //  integer value
  snprintf(str, 12, "%d", (int) val);
  str += strlen(str);
  val -= (int) val;
  //  decimals
  if ((precision > 0) && (val >= .00001)) {
    //  add period
    *(str++) = '.';
    //  loop through precision
    for (int i = 0;  i < precision;  i++)
      if (val > 0) {
        val *= 10;
        int j = (int) (val + ((i == precision - 1) ? .5 : 0));
        j = (j>9)?9:j;
        *(str++) = '0' + j;
        val -= (int) val;
      } else
        break;
  }
  //  terminate
  *str = '\0';
}
