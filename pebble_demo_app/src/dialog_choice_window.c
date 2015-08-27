/**
 * Example implementation of the dialog choice UI pattern.
 */

#include "dialog_choice_window.h"
#include "xadow.h"

static Window *s_diag_window;
static TextLayer *s_label_layer;
static BitmapLayer *s_icon_layer;
static ActionBarLayer *s_action_bar_layer;
static uint8_t resp_buffer[3];

static GBitmap *s_icon_bitmap, *s_tick_bitmap, *s_cross_bitmap;

static void charge_setting_result_read_done(bool success, uint32_t length) {
    if(success)
    {
        window_stack_remove(s_diag_window, true);
    }
}


static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  Window *win = context;
  uint8_t *data = window_get_user_data(win);
  //*data = 1;
  uint8_t req_type = ENABLE_CHG;
  SmartstrapResult result = smartstrap_send((uint8_t *)&req_type, 1,
                                             resp_buffer, 2,
                                             charge_setting_result_read_done, 1000);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "set enable charge (result=%d)", result);
  //window_stack_remove(win, true);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  Window *win = context;
  uint8_t *data = window_get_user_data(win);
  //*data = 0;
  uint8_t req_type = DISABLE_CHG;
  SmartstrapResult result = smartstrap_send((uint8_t *)&req_type, 1,
                                             resp_buffer, 2,
                                             charge_setting_result_read_done, 1000);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "set disable charge (result=%d)", result);
  //window_stack_remove(win, true);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}


static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CHARGE);
  GRect bitmap_bounds = gbitmap_get_bounds(s_icon_bitmap);

  s_icon_layer = bitmap_layer_create(GRect((bounds.size.w / 2) - (bitmap_bounds.size.w / 2) - (ACTION_BAR_WIDTH / 2), 10, bitmap_bounds.size.w, bitmap_bounds.size.h));
  bitmap_layer_set_bitmap(s_icon_layer, s_icon_bitmap);
  bitmap_layer_set_compositing_mode(s_icon_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_icon_layer));

  s_label_layer = text_layer_create(GRect(10, 10 + bitmap_bounds.size.h + 5, 124 - ACTION_BAR_WIDTH, bounds.size.h - (10 + bitmap_bounds.size.h + 15)));
  text_layer_set_text(s_label_layer, DIALOG_CHOICE_WINDOW_MESSAGE);
  text_layer_set_background_color(s_label_layer, GColorClear);
  text_layer_set_text_alignment(s_label_layer, GTextAlignmentCenter);
  text_layer_set_font(s_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_label_layer));

  s_tick_bitmap = gbitmap_create_with_resource(RESOURCE_ID_TICK);
  s_cross_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CROSS);

  s_action_bar_layer = action_bar_layer_create();
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_UP, s_tick_bitmap);
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_DOWN, s_cross_bitmap);
  action_bar_layer_add_to_window(s_action_bar_layer, window);

  action_bar_layer_set_context(s_action_bar_layer, window);
  action_bar_layer_set_click_config_provider(s_action_bar_layer, click_config_provider);

}

static void window_unload(Window *window) {
  text_layer_destroy(s_label_layer);
  action_bar_layer_destroy(s_action_bar_layer);
  bitmap_layer_destroy(s_icon_layer);

  gbitmap_destroy(s_icon_bitmap);
  gbitmap_destroy(s_tick_bitmap);
  gbitmap_destroy(s_cross_bitmap);

  window_destroy(window);
  s_diag_window = NULL;
}

void dialog_choice_window_push(uint8_t *charge) {
  if(!s_diag_window) {
    s_diag_window = window_create();
    window_set_user_data(s_diag_window, charge);
    window_set_background_color(s_diag_window, GColorWhite);
    window_set_window_handlers(s_diag_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
  }
  window_stack_push(s_diag_window, true);
}
