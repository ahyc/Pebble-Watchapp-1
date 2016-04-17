#include <pebble.h>
#include "main.h"

//Initialising layers and windows in menu:
static Window *s_main_window;
static StatusBarLayer *status_bar;
static Layer *s_canvas_layer;

//Initialising global font:
static GFont s_font_30;	

//Initialising animated gif layer/sequences/method:
static GBitmap *s_bitmap = NULL;
static BitmapLayer *s_bitmap_layer;
static GBitmapSequence *s_sequence = NULL;
static void load_sequence();

static void timer_handler(void *context) {
  uint32_t next_delay;
	
  //Advance to the next APNG frame:
  if(gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_bitmap, &next_delay)) {
    bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));

    //Timer for that delay:
    app_timer_register(next_delay, timer_handler, NULL);
  } else {
    //Start sequence again:
    load_sequence();
  }
}

//Function to draw the menu layer primitives:
static void drawMenu (Layer *layer, GContext *ctx, GColor color1, GColor color2, GColor color3) {
	GRect bounds = layer_get_bounds(layer);  
	graphics_context_set_stroke_width(ctx, 5);
	
	for(int x = 1; x < 4; x++) {	
		switch(x)
		{
			case 1:
			graphics_context_set_stroke_color(ctx, color1);
			break;
			case 2:
			graphics_context_set_stroke_color(ctx, color2);
			break;
			case 3:
			graphics_context_set_stroke_color(ctx, color3);
			break;
		}

		//Drawing the 3 button blocks onto the menu layer:
		graphics_draw_round_rect(ctx, GRect(bounds.size.w -2, ((x-1)*(bounds.size.h/3)+11), 4, (bounds.size.h/3)-25), 25);

		//Drawing the 3 rectangular blocks onto the menu layer:
		graphics_draw_round_rect(ctx, GRect(5,(5+((x-1)*bounds.size.h/3)), bounds.size.w - 15, (bounds.size.h/3)-10), 10);
	}

	//Initialising rectangular Frames for the menu text and drawing the text onto the menu layer:
	graphics_context_set_text_color(ctx, GColorBlack);
	GRect frames[3];
	frames[0] = GRect(5, 7, bounds.size.w - 42, (bounds.size.h/3)-13);
	const char* mLabel[] = {"Run", "Stats", "Settings"};

	for(int x = 1; x<3; x++) {
		frames[x] = GRect(5, (x*bounds.size.h/3)+7, bounds.size.w - 10, (bounds.size.h/3)-13);
	}

	for(int y = 0; y<3; y++ ){
		graphics_draw_text(ctx, mLabel[y], s_font_30, frames[y], GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
	}		
}

static void load_sequence() {
  //Free old data:
  if(s_sequence) {
    gbitmap_sequence_destroy(s_sequence);
    s_sequence = NULL;
  }
  if(s_bitmap) {
    gbitmap_destroy(s_bitmap);
    s_bitmap = NULL;
  }

  //Create sequence:
  s_sequence = gbitmap_sequence_create_with_resource(RESOURCE_ID_WATCH_GIF);

  //Create GBitmap:
  s_bitmap = gbitmap_create_blank(gbitmap_sequence_get_bitmap_size(s_sequence), GBitmapFormat8Bit);

  //Begin animation:
  app_timer_register(1, timer_handler, NULL);
}

static void main_update_callback(Layer *layer, GContext *ctx) {
	//Draw Menu layer primitives:
	drawMenu(layer, ctx, GColorMayGreen, GColorRajah, GColorLavenderIndigo);
}

//Up button handler for run window:
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	shortVibrate(vibes.main);
	run_create();
}

//Middle button handler for stats window:
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
	shortVibrate(vibes.main);
	stats_create();
}

//Down button handler for settings window:
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
	
	//Initialising custom font:
	s_font_30 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_AGENCY_BOLD_30));

  //Create Layer below the status bar of width 15:
  s_canvas_layer = layer_create(GRect(0, STATUS_BAR_HEIGHT, window_bounds.size.w, window_bounds.size.h-STATUS_BAR_HEIGHT));
  layer_add_child(window_layer, s_canvas_layer);
	
	//Initialising automatic redrawing of the menu layer:
	layer_set_update_proc(s_canvas_layer, main_update_callback);

	//Initialising animated gif layers:
	s_bitmap_layer = bitmap_layer_create(GRect(85,20,40,40));
	bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  layer_insert_above_sibling(bitmap_layer_get_layer(s_bitmap_layer), s_canvas_layer);
  load_sequence();
	
	//Check vibration status:
	checkVSettings();
}	

static void main_window_unload(Window *window) {
	//Unloading all custom fonts used:
	fonts_unload_custom_font(s_font_30);
	
	//Destroying the menu and image-holding layer:
	layer_destroy(s_canvas_layer);
	bitmap_layer_destroy(s_bitmap_layer);
	
	//Destroys sequence and gbitmap if not null:
  if(s_sequence) {	gbitmap_sequence_destroy(s_sequence);	}
  if(s_bitmap) {	gbitmap_destroy(s_bitmap);	}
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