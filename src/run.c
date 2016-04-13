#include <pebble.h>
#include <time.h>
#include "run.h"

static Window *s_run_window;
static Layer *s_run_layer;
static Layer *save_no_layer;
static TextLayer *s_run_info_layer;

static int s_uptime;
static uint8_t saveInNo;

Runner runmax;
timeSaver times[maxNoRecords];

struct tm *timeinfo;
static time_t currentTime;

// Initialising functions:
void initialiseRunner (struct Runner *run);
void initialiseCheckPoints (void);
void initialiseDateTime (void);

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // Increment s_uptime:
  s_uptime++;

	layer_mark_dirty(s_run_layer);
}

static void changeSaveNo (void) {
	if((saveInNo+1) % maxNoRecords == 0) {		saveInNo = 0;		}
	else {		saveInNo++;		}
	
	s_uptime = 0; 	
	layer_mark_dirty(save_no_layer);
	shortVibrate(vibes.run);

	APP_LOG(APP_LOG_LEVEL_DEBUG, "Save index is now %d, no of checkPoints: %d", saveInNo+1, times[saveInNo].noOfCheckPoints);
}

static void endStopWatch(void) {
		if(times[saveInNo].noOfCheckPoints<maxCheckPoints) {
			times[saveInNo].checkPoints[times[saveInNo].noOfCheckPoints] = s_uptime;
			times[saveInNo].noOfCheckPoints++;
		}
	
		tick_timer_service_unsubscribe();
		doubleVibrate(vibes.run);
		runmax.status = stopped;
		
		layer_mark_dirty(save_no_layer);
		layer_mark_dirty(s_run_layer);

		//APP_LOG(APP_LOG_LEVEL_DEBUG, "%d, %d, %d", times[saveInNo].checkPoints[0],times[saveInNo].checkPoints[1],times[saveInNo].checkPoints[9]);
}

static void resetCheckPoint (void) {
	for(int column = 0; column<maxCheckPoints; column ++ ) {
		times[saveInNo].checkPoints[column] = 0;
	}
	times[saveInNo].noOfCheckPoints = 0;
	s_uptime = 0;
	
	initialiseDateTime();
	longVibrate(vibes.run);

	APP_LOG(APP_LOG_LEVEL_DEBUG, "The timer is now: %d, The no of checkPoints is now: %d", s_uptime, times[saveInNo].noOfCheckPoints);

	layer_mark_dirty(save_no_layer);
	layer_mark_dirty(s_run_layer);
}

static void checkCheckPoints (void) {
	if(times[saveInNo].noOfCheckPoints < maxCheckPoints-1) {
		times[saveInNo].checkPoints[times[saveInNo].noOfCheckPoints] = s_uptime;		
		times[saveInNo].noOfCheckPoints++;
		layer_mark_dirty(save_no_layer);
		shortVibrate(vibes.run);
	}
	else {
		endStopWatch();
	}
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
	if(runmax.status != stopped) {
		endStopWatch();
	}

	//persist_write_data(STIME_KEY, &times, sizeof(times[maxNoRecords]));
	window_stack_remove(s_run_window,true);
}

void initialiseDateTime (void) {
	if(times[saveInNo].noOfCheckPoints == 0) {
		time(&currentTime);
		struct tm *ptr = localtime(&currentTime);
		strftime(times[saveInNo].dateTime, sizeof(times[saveInNo].dateTime), "%e/%m/%Y", ptr);
	}
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	if(runmax.status == stopped) {
		changeSaveNo();
		initialiseRunner(&runmax);
		
		initialiseDateTime();
			APP_LOG(APP_LOG_LEVEL_WARNING, "%s, size is: %d", times[saveInNo].dateTime,(int)sizeof(times[saveInNo].dateTime));
		
		layer_mark_dirty(text_layer_get_layer(s_run_info_layer));
	}
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
	switch(runmax.status) {
		case paused:
		case running: checkCheckPoints();
									break;
		case stopped: if (times[saveInNo].checkPoints[0] == 0) {
										shortVibrate(vibes.run);
										// Subscribe to TickTimerService:
  									tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
										runmax.status = running;
									}
									break;
	}
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
	switch(runmax.status) {
		case paused:
		case running:	endStopWatch();
									break;
		case stopped:	resetCheckPoint();
									break;
	}
}

static void click_config_provider(void *context) {
	// Register the ClickHandl  ers:
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
	window_long_click_subscribe(BUTTON_ID_UP, 0, up_click_handler, NULL);
	window_long_click_subscribe(BUTTON_ID_DOWN, 0, down_click_handler, NULL);
}

void initialiseRunner (struct Runner *run) {
	run->saveID = saveInNo;
	run->status = stopped;

	//APP_LOG(APP_LOG_LEVEL_DEBUG, "Save index is now %d, Timer is now %u, status is no %d", run->saveID, (uint8_t) run->totalTime, run->status);
}

void initialiseCheckPoints (void) {
	if (persist_exists(STIME_KEY)) {
		int valueRead;
		valueRead = persist_read_data(STIME_KEY, &times, sizeof(times));
		APP_LOG(APP_LOG_LEVEL_WARNING, "Read %d bytes from settings", valueRead);
  	//persist_read_data(STIME_KEY, times, sizeof(times[maxNoRecords]));
		//persist_delete(STIME_KEY);
	}
	else {
		initialiseDateTime();
			APP_LOG(APP_LOG_LEVEL_WARNING, "%s, size is: %d", times[saveInNo].dateTime,(int)sizeof(times[saveInNo].dateTime));
		times[saveInNo].noOfCheckPoints = 0;
		for(int index = 0; index < maxCheckPoints; index++) {
				times[saveInNo].checkPoints[index] = 0;
		}
	}

	//APP_LOG(APP_LOG_LEVEL_DEBUG, "%d, %d, %d", times->checkPoints[0],times->checkPoints[1],times->checkPoints[9]);
}

void drawRunTimer(Layer *layer, GContext *ctx) {
	 // Use a long-lived buffer
  static char s_uptime_buffer[1024];

  // Get time of the run:
  int seconds = s_uptime % 60;
  int minutes = (s_uptime % 3600) / 60;
  int hours = s_uptime / 3600;

	if(s_uptime>3580)	{
		snprintf(s_uptime_buffer, sizeof(s_uptime_buffer), "%02d:%02d:%02d", hours, minutes, seconds);
	}
	else{
		snprintf(s_uptime_buffer, sizeof(s_uptime_buffer), "%02d:%02d", minutes, seconds);
	}

	GRect timerBox = GRect(0, 39, 144, 69);
	graphics_context_set_text_color(ctx, GColorBlack);
	graphics_draw_text(ctx, s_uptime_buffer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS), timerBox, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

}

static void saveNo_update_callback(Layer *layer, GContext *ctx) {
	if(times[saveInNo].checkPoints[0] != 0) {
		if(runmax.status == stopped) {
			graphics_context_set_fill_color(ctx, GColorBlack);
		}
		else {
			graphics_context_set_fill_color(ctx, GColorRed);
		}
		graphics_fill_rect(ctx, GRect(128, 2, 15, 20), 0, GCornerNone);
		graphics_context_set_text_color(ctx, GColorWhite);
	}
	else {
		graphics_context_set_text_color(ctx, GColorBlack);
	}

	// Long lived buffer:
	static char buf[] = "00000000000";
	snprintf(buf, sizeof(buf), "%d", saveInNo+1);
	graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(100, 0, 40, 40), GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
	
	graphics_context_set_text_color(ctx, GColorBlack);
	
	// Use a long-lived buffer
  static char saved_uptime_buffer[1024];

	int checkPointNo;
	int lastCheckPoint;
	if(times[saveInNo].checkPoints[0] != 0 && times[saveInNo].noOfCheckPoints != 0) {
		checkPointNo = times[saveInNo].noOfCheckPoints;
		lastCheckPoint = times[saveInNo].checkPoints[times[saveInNo].noOfCheckPoints-1];
	}
	else {
		checkPointNo = 0;
		lastCheckPoint = 0;
	}
	
  // Get time of the run:
  int seconds = lastCheckPoint % 60;
  int minutes = (lastCheckPoint % 3600) / 60;
  int hours = lastCheckPoint / 3600;

	if(lastCheckPoint>3580)	{
		snprintf(saved_uptime_buffer, sizeof(saved_uptime_buffer), "%02d:%02d:%02d     %d", hours, minutes, seconds, checkPointNo);
	}
	else{
		snprintf(saved_uptime_buffer, sizeof(saved_uptime_buffer), "%02d:%02d     %d", minutes, seconds, checkPointNo);
	}

	GRect timerBox = GRect(0, 134, 144, 35);
	graphics_draw_text(ctx, saved_uptime_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), timerBox, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

}

static void run_update_callback(Layer *layer, GContext *ctx) {
	drawRun(layer, ctx);
	drawRunTimer(layer, ctx);
}

static void run_window_load(Window *window) {
	//Layer pointer for the bounds of the window layer:
	Layer *window_layer = window_get_root_layer(window);
 	GRect window_bounds = layer_get_bounds(window_layer);

	s_run_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h-58));
  layer_add_child(window_layer, s_run_layer);
	layer_set_update_proc(s_run_layer, run_update_callback);

	save_no_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
	layer_insert_above_sibling(save_no_layer, s_run_layer);
	layer_set_update_proc(save_no_layer, saveNo_update_callback);

	s_run_info_layer = text_layer_create(GRect(0, window_bounds.size.h-53, window_bounds.size.w, 20));
	layer_insert_above_sibling(text_layer_get_layer(s_run_info_layer), s_run_layer);
	text_layer_set_text_alignment(s_run_info_layer, GTextAlignmentCenter);
	text_layer_set_font(s_run_info_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	
	// Initialise the timer save no and runner struct:
	saveInNo = 0;
	initialiseRunner(&runmax);
	initialiseCheckPoints();
	s_uptime = 0;
	text_layer_set_text(s_run_info_layer, times[saveInNo].dateTime);
	checkVSettings();
}

static void run_window_unload(Window *window) {
	int valueWritten;
	valueWritten = persist_write_data(STIME_KEY, &times, sizeof(times));
	APP_LOG(APP_LOG_LEVEL_WARNING, "Wrote %d bytes to settings.", valueWritten);

	//Destroying the run layers:
	text_layer_destroy(s_run_info_layer);
	layer_destroy(s_run_layer);
	layer_destroy(save_no_layer);
	
	window_destroy(s_run_window);
}

void run_create() {
	s_run_window = window_create();
	window_set_window_handlers(s_run_window, (WindowHandlers) {
		.load = run_window_load,
		.unload = run_window_unload,
	});

	window_set_click_config_provider(s_run_window, click_config_provider);
	window_stack_push(s_run_window, true);
}

