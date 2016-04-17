#include <pebble.h>
#include "settings.h"

static Window *s_settings_window;
static Layer *s_settings_layer;
static Layer *s_settings_buttons;
static Layer *s_settings_select;

vibeSettings vibes;
static uint8_t hlNo;

//Methods to check whether to vibrate when a process begins/ends:
void longVibrate(vibeStatus page) {
	if(page == Vibes) {	vibes_long_pulse();	}
}

void shortVibrate(vibeStatus page) {
	if(page == Vibes) {	vibes_short_pulse();	}	
}

void doubleVibrate(vibeStatus page) {
	if(page == Vibes) {	vibes_double_pulse();	}	
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	//Toggles the selected option backwards:
	hlNo = ((hlNo - 1) < 0)? 3: hlNo-1;
	layer_mark_dirty(s_settings_select);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
	//Turns on/off specific, or all, vibration:
	switch(hlNo) {
		default:
		case 0: 
		vibes.run = vibes.main = vibes.stats = noVibes;
		break;
		case 1:	
		vibes.main = !vibes.main;
		break;
		case 2:	
		vibes.run = !vibes.run;
		break;
		case 3:	
		vibes.stats = !vibes.stats;
		break;
	};
	layer_mark_dirty(s_settings_buttons);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
	//Toggles the selected option forwards:
	hlNo = ((hlNo + 1) % 4 != 0)? hlNo+1: 0;
	layer_mark_dirty(s_settings_select);
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
	window_stack_remove(s_settings_window,true);
}

static void click_config_provider(void *context) {
	// Register the ClickHandlers:
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
	window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void saveSettings(void) {
	//Saves the vibration settings:
	persist_write_data(VSET_KEY, &vibes, sizeof(vibes));
		//APP_LOG(APP_LOG_LEVEL_WARNING, "Wrote %d bytes to settings.", valueWritten);
}

//Checks the vibration settings of all windows:
void checkVSettings (void) {
	if (persist_exists(VSET_KEY)) {
		persist_read_data(VSET_KEY, &vibes, sizeof(vibes));
			//APP_LOG(APP_LOG_LEVEL_WARNING, "Read %d bytes from settings", valueRead);
	}
	else {
		//If there are no vibration settings, all windows do vibrate with button pushes:
		vibes.run = vibes.main = vibes.stats = Vibes;
		saveSettings();
	}
}

//Method which draws the selection labels and rectangles onto the settings page:
static void select_update_callback(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
	graphics_context_set_stroke_width(ctx, 2);
	graphics_context_set_text_color(ctx, GColorBlack);
		
	const char* optionLabel[] = {"[All OFF]", "Main", "Run", "Stats"};
	
	for(int i = 0; i<4; i++) {
		(hlNo != i)? graphics_context_set_stroke_color(ctx, GColorBlack): graphics_context_set_stroke_color(ctx, GColorLavenderIndigo);	
		GRect setLabels = (i==0)? GRect(5, 34+(i*30)+((i+1)*3), bounds.size.w-9, 30): GRect(5, 34+(i*30)+((i+1)*3), bounds.size.w-49, 30);
		graphics_draw_round_rect(ctx, setLabels, 5);
		GRect setOptions = (i==0)? GRect(5, (i*33)+34, bounds.size.w-10, 33): GRect(5, (i*33)+34, bounds.size.w-50, 33);
		graphics_draw_text(ctx, optionLabel[i], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), setOptions, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
	}
}

//Method to draw the buttons:
static void buttons_update_callback(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
	graphics_context_set_stroke_color(ctx, GColorBlack);
	
	for(int i = 1; i<4; i++) {
		graphics_context_set_fill_color(ctx, GColorWhite);
		
		graphics_draw_circle(ctx, GPoint(bounds.size.w-12, 48+(i*30)+((i+1)*3)), 8);
		graphics_draw_circle(ctx, GPoint(bounds.size.w-32, 48+(i*30)+((i+1)*3)), 8);
		graphics_fill_rect(ctx, GRect(bounds.size.w-31, 43+(i*33), 19, 17), 0, GCornerNone);
		
		graphics_draw_line(ctx, GPoint(bounds.size.w-31, 43+(i*33)), GPoint(bounds.size.w-12, 43+(i*33)));
		graphics_draw_line(ctx, GPoint(bounds.size.w-31, 59+(i*33)), GPoint(bounds.size.w-12, 59+(i*33)));
		
		GPoint OOpos;
		switch(i) {
				default:
				case 1: 
					if(vibes.main == Vibes) {	graphics_context_set_fill_color(ctx, GColorLavenderIndigo);	}
					OOpos = (vibes.main == Vibes)? GPoint(bounds.size.w-12, 48+(i*30)+((i+1)*3)): GPoint(bounds.size.w-32, 48+(i*30)+((i+1)*3));
					break;
				case 2:	
					if(vibes.run == Vibes) {	graphics_context_set_fill_color(ctx, GColorLavenderIndigo);	}
					OOpos = (vibes.run == Vibes)? GPoint(bounds.size.w-12, 48+(i*30)+((i+1)*3)): GPoint(bounds.size.w-32, 48+(i*30)+((i+1)*3));
					break;
				case 3:	
					if(vibes.stats == Vibes) {	graphics_context_set_fill_color(ctx, GColorLavenderIndigo);	}
					OOpos = (vibes.stats == Vibes)? GPoint(bounds.size.w-12, 48+(i*30)+((i+1)*3)): GPoint(bounds.size.w-32, 48+(i*30)+((i+1)*3));
					break;
		}
		graphics_fill_circle(ctx, OOpos, 6);
		graphics_draw_circle(ctx, OOpos, 6);
	}
}

//Method draws the heading for the settings window:
static void settings_update_callback(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
	GFont s_font_24B = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
	
	GRect settingLabel = GRect(0, 0, bounds.size.w, 34);
	graphics_context_set_fill_color(ctx, GColorLavenderIndigo);
	graphics_fill_rect(ctx, settingLabel, 0, GCornerNone);
	graphics_draw_text(ctx, "Vibrations", s_font_24B, settingLabel, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}


static void settings_window_load(Window *window) {
	//Layer pointer for the bounds of the window layer:
	Layer *window_layer = window_get_root_layer(window);
 	GRect window_bounds = layer_get_bounds(window_layer);

	//layer which holds the heading for the window:
	s_settings_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
  layer_add_child(window_layer, s_settings_layer);
	layer_set_update_proc(s_settings_layer, settings_update_callback);
	
	//Layer which holds the buttons, which are positioned depending if the vibration settings is on/off:
	s_settings_buttons = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
	layer_insert_above_sibling(s_settings_buttons, s_settings_layer);
	layer_set_update_proc(s_settings_buttons, buttons_update_callback);
	
	//Layer which holds the selectable options and the current selected option:
	s_settings_select = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
	layer_insert_above_sibling(s_settings_select, s_settings_layer);
	layer_set_update_proc(s_settings_select, select_update_callback);
	
	//Initialises the current option highlighted:
	hlNo = 0;	
	
	//Checks the current vibration settings for the windows:
	checkVSettings();
}

static void settings_window_unload(Window *window) {
	saveSettings();
	
	//Destroying the setting layers:
	layer_destroy(s_settings_layer);
	layer_destroy(s_settings_buttons);
	layer_destroy(s_settings_select);
	
	window_destroy(s_settings_window);
}

void settings_create() {
	s_settings_window = window_create();
	window_set_window_handlers(s_settings_window, (WindowHandlers) {
		.load = settings_window_load,
		.unload = settings_window_unload,
	});

	window_set_click_config_provider(s_settings_window, click_config_provider);
	window_stack_push(s_settings_window, true);
}
