#include <pebble.h>
#include "main.h"

//Initialising layers and windows in menu:
Window *s_main_window;
StatusBarLayer *status_bar;
static Layer *s_canvas_layer;

//Initialising global fonts:
GFont s_font_30;	

static GBitmap *s_bitmap = NULL;
static BitmapLayer *s_bitmap_layer;
static GBitmapSequence *s_sequence = NULL;

static void load_sequence();

static void timer_handler(void *context) {
  uint32_t next_delay;
	
  // Advance to the next APNG frame
  if(gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_bitmap, &next_delay)) {
    bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));

    // Timer for that delay
    app_timer_register(next_delay, timer_handler, NULL);
  } else {
    // Start again
    load_sequence();
  }
}

static void load_sequence() {
  // Free old data
  if(s_sequence) {
    gbitmap_sequence_destroy(s_sequence);
    s_sequence = NULL;
  }
  if(s_bitmap) {
    gbitmap_destroy(s_bitmap);
    s_bitmap = NULL;
  }

  // Create sequence
  s_sequence = gbitmap_sequence_create_with_resource(RESOURCE_ID_WATCH_GIF);

  // Create GBitmap
  s_bitmap = gbitmap_create_blank(gbitmap_sequence_get_bitmap_size(s_sequence), GBitmapFormat8Bit);

  // Begin animation
  app_timer_register(1, timer_handler, NULL);
}

static void main_update_callback(Layer *layer, GContext *ctx) {
	
	//Draw Menu layer primitives:
	drawMenu(layer, ctx, GColorMayGreen, GColorRajah, GColorLavenderIndigo);

}

//Up button handler:
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	shortVibrate(vibes.main);
	run_create();
}

//Middle button handler:
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
	shortVibrate(vibes.main);
	stats_create();
}

//Down button handler:
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
	shortVibrate(vibes.main);
	settings_create();
}

static void click_config_provider(void *context) {
	// Register the ClickHandlers:
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static StatusBarLayer* status_bar_create() {
	//Initialisation of the status bar and its position:	
	status_bar = status_bar_layer_create();
	GRect frame = GRect(0, 0, 144, STATUS_BAR_HEIGHT);
	layer_set_frame(status_bar_layer_get_layer(status_bar), frame);
	
	return status_bar;
}

static void main_window_load(Window *window) {
  //Layer pointer for the bounds of the window layer:
	Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
	
	//Initialising various custom fonts:
	s_font_30 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_AGENCY_BOLD_30));


  // Create Layer below the status bar of width 15:
  s_canvas_layer = layer_create(GRect(0, STATUS_BAR_HEIGHT, window_bounds.size.w, window_bounds.size.h-STATUS_BAR_HEIGHT));
  layer_add_child(window_layer, s_canvas_layer);
	
	//Initialising automatic redrawing of the menu layer:
	layer_set_update_proc(s_canvas_layer, main_update_callback);

	s_bitmap_layer = bitmap_layer_create(GRect(85,20,40,40));
	bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  layer_insert_above_sibling(bitmap_layer_get_layer(s_bitmap_layer), s_canvas_layer);

  load_sequence();
	
	//Check vibration status:
	checkVSettings();
}	

static void main_window_unload(Window *window) {
	//Unloading all fonts used:
	fonts_unload_custom_font(s_font_30);
	
	//Destroying the menu and image-holding layer:
	layer_destroy(s_canvas_layer);
	bitmap_layer_destroy(s_bitmap_layer);
	
	//Destroys sequence and gbitmap if not null:
  if(s_sequence) {
    gbitmap_sequence_destroy(s_sequence);
  }
  if(s_bitmap) {
    gbitmap_destroy(s_bitmap);
  }
}

static void init() {
  // Create main Window
  s_main_window = window_create();

	//Adding status bar onto the main window and initialising its colours:
	layer_add_child(window_get_root_layer(s_main_window), status_bar_layer_get_layer(status_bar_create()));	
	status_bar_layer_set_colors(status_bar, GColorBlack, GColorWhite);
	
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
	window_set_click_config_provider(s_main_window, click_config_provider);
	window_stack_push(s_main_window, true);
	
}

static void deinit() {	 
	//Destroys status bar:
	status_bar_layer_destroy(status_bar);
	
  // Destroy main Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

