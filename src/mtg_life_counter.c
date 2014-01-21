#include <pebble.h>

#define PLAYER_LIFE_1_PKEY  10
#define PLAYER_LIFE_2_PKEY  11
#define DIRECTION_PKEY      12
#define HISTORY_1_PLAYER_1_PKEY  13
#define HISTORY_2_PLAYER_1_PKEY  14
#define HISTORY_1_PLAYER_2_PKEY  17
#define HISTORY_2_PLAYER_2_PKEY  18

#define PLAYER_LIFE_1_DEFAULT 20
#define PLAYER_LIFE_2_DEFAULT 20
#define DIRECTION_DEFAULT     -1

static Window *window;
static TextLayer *life_total_1;
static TextLayer *life_total_2;
static TextLayer *direction_layer;
static TextLayer *confirm_accept_layer;
static TextLayer *confirm_reject_layer;
static TextLayer *me_layer;
static TextLayer *me_change_layer;
static TextLayer *you_layer;
static TextLayer *you_change_layer;
static InverterLayer *history_layer;
static TextLayer *history_player_1;
static TextLayer *history_player_2;
static AppTimer *timer;

static int player_life_1 = 20;
static int player_life_1_change = 0;
static int player_life_2 = 20;
static int player_life_2_change = 0;
static int repeating_click_interval = 300;
static int reset_life_total_click_interval = 600;
static int direction = DIRECTION_DEFAULT;

static char history_1_player_1[18];
static char history_2_player_1[18];
static char history_1_player_2[18];
static char history_2_player_2[18];

static bool reset_prompt = false;

static void draw_history()
{
  static char history_1_char[25];
  snprintf(history_1_char, sizeof(history_1_player_1) + sizeof(history_2_player_1) + 2, "%s\n%s", history_1_player_1, history_2_player_1);
  text_layer_set_text(history_player_1, history_1_char);

  static char history_2_char[25];
  snprintf(history_2_char, sizeof(history_1_player_2) + sizeof(history_2_player_2) + 2, "%s\n%s", history_1_player_2, history_2_player_2);
  text_layer_set_text(history_player_2, history_2_char);
}

static void update_history()
{
  if (player_life_1_change != 0){
    snprintf(history_2_player_1, sizeof(history_1_player_1) + 2, "%s", history_1_player_1);
    snprintf(history_1_player_1, sizeof(player_life_1_change) + sizeof(player_life_1) + 2, "%+d/%d", player_life_1_change, player_life_1);
    draw_history();
  }
  
  if (player_life_2_change != 0){
    snprintf(history_2_player_2, sizeof(history_1_player_2) + 2, "%s", history_1_player_2);
    snprintf(history_1_player_2, sizeof(player_life_2_change) + sizeof(player_life_2) + 2, "%+d/%d", player_life_2_change, player_life_2);
    draw_history();
  }
}

static int change_life_total_history(int life_total_history, int change)
{
  return life_total_history + (change * direction);
}

static void history_timer_callback(void *data)
{
  update_history();
  player_life_1_change = 0;
  player_life_2_change = 0;
}

static void start_history_timer()
{
  timer = app_timer_register(1000, history_timer_callback, NULL);
}

static void cancel_history_timer()
{
  app_timer_cancel(timer);
}

static void draw_direction()
{
  if (direction > 0){
    text_layer_set_text(direction_layer, "+"); 
  } else {
    text_layer_set_text(direction_layer, "-");
  }
}

static void change_direction()
{
  direction = direction * -1;
  draw_direction();
}

static void set_direction(int new_direction)
{
  direction = new_direction;
  draw_direction();
}

static void draw_life_totals()
{
  static char life_total_1_char[25];
  snprintf(life_total_1_char, sizeof(player_life_1), "%d", player_life_1);
  static char life_total_2_char[25];
  snprintf(life_total_2_char, sizeof(player_life_2), "%d", player_life_2);
  
  text_layer_set_text(life_total_1, life_total_1_char);
  text_layer_set_text(life_total_2, life_total_2_char);
}

static int change_life_total(int life_total, int change)
{
  cancel_history_timer();
  start_history_timer();
  life_total = life_total + (change * direction);
  
  if (life_total < 0){
    life_total = 0;
  }

  return life_total;
}

static int set_life_total(int life_total, int new_total)
{
  life_total = new_total;
  return life_total;
}

static void reset_life_totals()
{
  player_life_1 = set_life_total(player_life_1, 20);
  player_life_2 = set_life_total(player_life_2, 20);
  draw_life_totals();

  *history_1_player_1 = 0;
  *history_2_player_1 = 0;
  *history_1_player_2 = 0;
  *history_2_player_2 = 0;
  player_life_1_change = 0;
  player_life_2_change = 0;
  draw_history();
}

static void show_reset_prompt()
{
  //HIDE CHANGE LAYERS
  text_layer_set_size (me_change_layer, (GSize) { 0 , 0  } );
  text_layer_set_size (you_change_layer, (GSize) { 0 , 0  } );

  //HIDE HISTORY LAYERS
  text_layer_set_size (history_player_1, (GSize) { 0 , 0  } );
  text_layer_set_size (history_player_2, (GSize) { 0 , 0  } );
  
  //SHOW CONFIRM/REJECT
  text_layer_set_size (confirm_accept_layer, (GSize) { 72 , 28 } );
  text_layer_set_size (confirm_reject_layer, (GSize) { 72 , 28 } );

  //CHANGE RESET TEXT
  text_layer_set_font(direction_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text(direction_layer, "Reset?");

  reset_prompt = true;
}

static void hide_reset_prompt()
{
  //HIDE CONFIRM/REJECT
  text_layer_set_size (confirm_accept_layer, (GSize) { 0 , 0  } );
  text_layer_set_size (confirm_reject_layer, (GSize) { 0 , 0  } );

  //SHOW HISTORY LAYERS
  text_layer_set_size (history_player_1, (GSize) { 72 , 36  } );
  text_layer_set_size (history_player_2, (GSize) { 72 , 36  } );

  //SHOW CHANGE LAYERS
  text_layer_set_size (me_change_layer, (GSize) { 72 , 28  } );
  text_layer_set_size (you_change_layer, (GSize) { 72 , 28  } );

  //RESET RESET TEXT
  text_layer_set_font(direction_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));

  draw_direction();
  reset_prompt = false;
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  change_direction();
}

static void select_click_long_handler(ClickRecognizerRef recognizer, void *context){
  show_reset_prompt();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (reset_prompt == true){
    reset_life_totals();
    hide_reset_prompt();
  }else{
    player_life_1 = change_life_total(player_life_1, 1);
    player_life_1_change = change_life_total_history(player_life_1_change, 1);
    draw_life_totals();
  }
}

static void up_click_long_handler(ClickRecognizerRef recognizer, void *context) {
  if (reset_prompt == true){
    reset_life_totals();
    hide_reset_prompt();
  }else{
    player_life_1 = change_life_total(player_life_1, 5);
    player_life_1_change = change_life_total_history(player_life_1_change, 5);
    draw_life_totals();
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (reset_prompt == true){
    hide_reset_prompt();
  }else{
    player_life_2 = change_life_total(player_life_2, 1);
    player_life_2_change = change_life_total_history(player_life_2_change, 1);
    draw_life_totals();
  }
}

static void down_click_long_handler(ClickRecognizerRef recognizer, void *context) {
  if (reset_prompt == true){
    hide_reset_prompt();
  }else{
    player_life_2 = change_life_total(player_life_2, 5);
    player_life_2_change = change_life_total_history(player_life_2_change, 5);
    draw_life_totals();
  }
}

static void click_config_provider(void *context) {
  //UP
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, repeating_click_interval, up_click_long_handler, NULL);
  //SELECT
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, reset_life_total_click_interval, select_click_long_handler, NULL);
  //DOWN
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, repeating_click_interval, down_click_long_handler, NULL);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  life_total_1 = text_layer_create((GRect) { .origin = { 0, 11 }, .size = { bounds.size.w / 2, (bounds.size.h / 2) - 11 } });
  text_layer_set_text_alignment(life_total_1, GTextAlignmentCenter);
  text_layer_set_font(life_total_1, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  layer_add_child(window_layer, text_layer_get_layer(life_total_1));

  life_total_2 = text_layer_create((GRect) { .origin = { 0, (bounds.size.h / 2) + 18 }, .size = { bounds.size.w / 2, (bounds.size.h / 2) - 18 } });
  text_layer_set_text_alignment(life_total_2, GTextAlignmentCenter);
  text_layer_set_font(life_total_2, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  layer_add_child(window_layer, text_layer_get_layer(life_total_2));

  direction_layer = text_layer_create((GRect) { .origin = { bounds.size.w / 2, (bounds.size.h / 2) - 20 }, .size = { bounds.size.w / 2 , 60 } });
  text_layer_set_font(direction_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(direction_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(direction_layer));

  confirm_accept_layer = text_layer_create((GRect) { .origin = { bounds.size.w / 2, 0}, .size = { bounds.size.w / 2 , 28 } });
  text_layer_set_text(confirm_accept_layer, "Yes");
  text_layer_set_font(confirm_accept_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(confirm_accept_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(confirm_accept_layer));

  confirm_reject_layer = text_layer_create((GRect) { .origin = { bounds.size.w / 2, (bounds.size.h / 2) + ((bounds.size.h / 2) / 2) }, .size = { bounds.size.w / 2 , 28 } });
  text_layer_set_text(confirm_reject_layer, "No");
  text_layer_set_text_alignment(confirm_reject_layer, GTextAlignmentRight);
  text_layer_set_font(confirm_reject_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(confirm_reject_layer));

  me_layer = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { (bounds.size.w / 2), 18 } });
  text_layer_set_text(me_layer, "ME");
  text_layer_set_text_alignment(me_layer, GTextAlignmentCenter);
  text_layer_set_font(me_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(me_layer));

  me_change_layer = text_layer_create((GRect) { .origin = { (bounds.size.h / 2), 0}, .size = { (bounds.size.w / 2), 18 } });
  text_layer_set_text(me_change_layer, "CHANGE");
  text_layer_set_text_alignment(me_change_layer, GTextAlignmentCenter);
  text_layer_set_font(me_change_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(me_change_layer));

  you_layer = text_layer_create((GRect) { .origin = { 0, (bounds.size.h / 2) + 7}, .size = { (bounds.size.w / 2), 18 } });
  text_layer_set_text(you_layer, "YOU");
  text_layer_set_text_alignment(you_layer, GTextAlignmentCenter);
  text_layer_set_font(you_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(you_layer));

  you_change_layer = text_layer_create((GRect) { .origin = { (bounds.size.w / 2), (bounds.size.h / 2) + 7}, .size = { (bounds.size.w / 2), 18 } });
  text_layer_set_text(you_change_layer, "CHANGE");
  text_layer_set_text_alignment(you_change_layer, GTextAlignmentCenter);
  text_layer_set_font(you_change_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(you_change_layer));

  history_player_1 = text_layer_create((GRect) { .origin = { (bounds.size.h / 2), 18}, .size = { (bounds.size.w / 2), 36 } });
  text_layer_set_text(history_player_1, "");
  text_layer_set_text_alignment(history_player_1, GTextAlignmentCenter);
  text_layer_set_overflow_mode(history_player_1, GTextOverflowModeWordWrap);
  text_layer_set_font(history_player_1, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(history_player_1));

  history_player_2 = text_layer_create((GRect) { .origin = { (bounds.size.w / 2), (bounds.size.h / 2) + 25}, .size = { (bounds.size.w / 2), 36 } });
  text_layer_set_text(history_player_2, "");
  text_layer_set_text_alignment(history_player_2, GTextAlignmentCenter);
  text_layer_set_overflow_mode(history_player_2, GTextOverflowModeWordWrap);
  text_layer_set_font(history_player_2, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(history_player_2));

  history_layer = inverter_layer_create((GRect) { .origin = { bounds.size.w / 2, 0 }, .size = { bounds.size.w / 2, bounds.size.h } });
  layer_add_child(window_layer, inverter_layer_get_layer(history_layer));

  hide_reset_prompt();
  draw_life_totals();
  draw_direction();
  draw_history();
}

static void window_unload(Window *window) {
  text_layer_destroy(life_total_1);
  text_layer_destroy(life_total_2);
  text_layer_destroy(direction_layer);
  text_layer_destroy(confirm_accept_layer);
  text_layer_destroy(confirm_reject_layer);
  text_layer_destroy(me_layer);
  text_layer_destroy(me_change_layer);
  text_layer_destroy(you_layer);
  text_layer_destroy(you_change_layer);
  inverter_layer_destroy(history_layer);
  text_layer_destroy(history_player_1);
  text_layer_destroy(history_player_2);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  player_life_1 = persist_exists(PLAYER_LIFE_1_PKEY)  ? persist_read_int(PLAYER_LIFE_1_PKEY)  : PLAYER_LIFE_1_DEFAULT;
  player_life_2 = persist_exists(PLAYER_LIFE_2_PKEY)  ? persist_read_int(PLAYER_LIFE_2_PKEY)  : PLAYER_LIFE_2_DEFAULT;
  direction     = persist_exists(DIRECTION_PKEY)      ? persist_read_int(DIRECTION_PKEY)      : DIRECTION_DEFAULT;

  persist_read_string(HISTORY_1_PLAYER_1_PKEY, history_1_player_1, 18);
  persist_read_string(HISTORY_2_PLAYER_1_PKEY, history_2_player_1, 18);
  persist_read_string(HISTORY_1_PLAYER_2_PKEY, history_1_player_2, 18);
  persist_read_string(HISTORY_2_PLAYER_2_PKEY, history_2_player_2, 18);

  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  persist_write_int(PLAYER_LIFE_1_PKEY, player_life_1);
  persist_write_int(PLAYER_LIFE_2_PKEY, player_life_2);
  persist_write_int(DIRECTION_PKEY, direction);
  persist_write_string(HISTORY_1_PLAYER_1_PKEY, history_1_player_1);
  persist_write_string(HISTORY_2_PLAYER_1_PKEY, history_2_player_1);
  persist_write_string(HISTORY_1_PLAYER_2_PKEY, history_1_player_2);
  persist_write_string(HISTORY_2_PLAYER_2_PKEY, history_2_player_2);

  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
