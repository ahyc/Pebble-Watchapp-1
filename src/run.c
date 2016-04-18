#include <pebble.h>
#include <time.h>
#include "run.h"

static Window *s_run_window;
static Layer *s_run_layer;
static Layer *save_no_layer;
static TextLayer *s_run_info_layer;
static Layer *bbutton_layer;

static int s_uptime;
static uint8_t saveInNo;

Runner runmax;
timeSaver times[maxNoRecords];

struct tm *timeinfo;
static time_t currentTime;

//Initialising functions:
void initialiseRunner (struct Runner *run);
void initialiseCheckPoints (void);
void initialiseDateTime (void);

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  //Increment timer:
  s_uptime++;

	layer_mark_dirty(s_run_layer);
}

static void changeSaveNo (void) {
	//Increment record number:
	saveInNo = ((saveInNo+1) % maxNoRecords == 0)?	0: saveInNo+1;
	
	//Reset timer and redraw record number layer:
	s_uptime = 0;
	layer_mark_dirty(save_no_layer);
	shortVibrate(vibes.run);
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "Save index is now %d, no of checkPoints: %d", saveInNo+1, times[saveInNo].noOfCheckPoints);
}

static void endStopWatch(void) {
	//Adding current time as last checkpoint if record not full:
	if(times[saveInNo].noOfCheckPoints<maxCheckPoints) {
		times[saveInNo].checkPoints[times[saveInNo].noOfCheckPoints] = s_uptime;
		times[saveInNo].noOfCheckPoints++;
	}
	
	//Unsubscribe from timer:
	tick_timer_service_unsubscribe();
	doubleVibrate(vibes.run);
	runmax.status = stopped;

	layer_mark_dirty(save_no_layer);
	layer_mark_dirty(s_run_layer);
	layer_mark_dirty(bbutton_layer);
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
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "The timer is now: %d, The no of checkPoints is now: %d", s_uptime, times[saveInNo].noOfCheckPoints);

	layer_mark_dirty(save_no_layer);
	layer_mark_dirty(s_run_layer);
	layer_set_hidden(bbutton_layer, true);
}

//Method to check whether the record is full:
static void checkCheckPoints (void) {
	//Ends timer if checkpoint number is one away from the maximum number:
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
	//Ends timer if timer is running and returns to main window:
	if(runmax.status != stopped) {
		endStopWatch();
	}
	window_stack_remove(s_run_window,true);
}


//Initialise and draw the date in which the checkpoints were recorded:
void initialiseDateTime (void) {
	if(times[saveInNo].noOfCheckPoints == 0) {
		time(&currentTime);
		struct tm *ptr = localtime(&currentTime);
		strftime(times[saveInNo].dateTime, sizeof(times[saveInNo].dateTime), "%e/%m/%Y", ptr);
	}
	text_layer_set_text(s_run_info_layer, times[saveInNo].dateTime);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
	//Increments record number if timer is stopped and redraws current window layer:
	if(runmax.status == stopped) {
		changeSaveNo();
		initialiseRunner(&runmax);
		
		initialiseDateTime();
			APP_LOG(APP_LOG_LEVEL_WARNING, "%s", times[saveInNo].dateTime);
		
		layer_mark_dirty(text_layer_get_layer(s_run_info_layer));
		layer_set_hidden(bbutton_layer, (times[saveInNo].noOfCheckPoints!= 0)? false: true);
	}
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
	switch(runmax.status) {
		case paused:
		case running: 
			//Saves current time as checkpoint/ends timer if record is one checkpoint from full:
			checkCheckPoints();
			break;
		case stopped: 
			//Starts timer:
			if (times[saveInNo].checkPoints[0] == 0) {
				shortVibrate(vibes.run);
				// Subscribe to TickTimerService:
				tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
				runmax.status = running;
				layer_set_hidden(bbutton_layer, false);
			}
			break;
	}
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
	switch(runmax.status) {
		case paused:
		case running:	
			//Prematurely ends timer:
			endStopWatch();
			break;
		case stopped:	
			//Deletes current completed record:
			resetCheckPoint();
			break;
	}
}

static void click_config_provider(void *context) {
	//Register the ClickHandlers:
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
	window_long_click_subscribe(BUTTON_ID_UP, 0, up_click_handler, NULL);
	window_long_click_subscribe(BUTTON_ID_DOWN, 0, down_click_handler, NULL);
}

void initialiseRunner (struct Runner *run) {
	//Initialises struct info:
	run->saveID = saveInNo;
	run->status = stopped;
}

void initialiseCheckPoints (void) {
	//Reads record data from key if it exists, else initialises the data:
	if (persist_exists(STIME_KEY)) {
		persist_read_data(STIME_KEY, &times, sizeof(times));
			//APP_LOG(APP_LOG_LEVEL_WARNING, "Read %d bytes from settings", valueRead);
	}
	else {
		times[saveInNo].noOfCheckPoints = 0;
		for(int index = 0; index < maxCheckPoints; index++) {
				times[saveInNo].checkPoints[index] = 0;
		}
	}
	initialiseDateTime();
		//APP_LOG(APP_LOG_LEVEL_WARNING, "%s, size is: %d", times[saveInNo].dateTime,(int)sizeof(times[saveInNo].dateTime));
}

//Method to draw the current timer time:
void drawRunTimer(Layer *layer, GContext *ctx) {
	//Use a long-lived buffer:
  static char s_uptime_buffer[1024];

  //Get time of the run:
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
	//Method to draw the record number top right of the window:
	if(times[saveInNo].checkPoints[0] != 0) {
		//Depending on the status of the timer, the fill colour of the record's background is changed:
		(runmax.status == stopped)? graphics_context_set_fill_color(ctx, GColorBlack): graphics_context_set_fill_color(ctx, GColorRed);
		
		graphics_fill_rect(ctx, GRect(125, 5, 15, 20), 0, GCornerNone);
		graphics_context_set_text_color(ctx, GColorWhite);
	}
	else {
		graphics_context_set_text_color(ctx, GColorBlack);
	}

	//Long lived buffer to hold current record number:
	static char buf[] = "00000000000";
	snprintf(buf, sizeof(buf), "%d", saveInNo+1);
	graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(97, 3, 40, 40), GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
	
	//Use a long-lived buffer to output current record information:
  static char saved_uptime_buffer[1024];
	graphics_context_set_text_color(ctx, GColorBlack);
	
	//'If' loop to get the number of checkpoints and time of the last checkpoint for current record:
	int checkPointNo, lastCheckPoint;
	if(times[saveInNo].checkPoints[0] != 0 && times[saveInNo].noOfCheckPoints != 0) {
		checkPointNo = times[saveInNo].noOfCheckPoints;
		lastCheckPoint = times[saveInNo].checkPoints[times[saveInNo].noOfCheckPoints-1];
	}
	else {
		checkPointNo = 0, lastCheckPoint = 0;
	}
	
  //Get time of the run:
  int seconds = lastCheckPoint % 60;
  int minutes = (lastCheckPoint % 3600) / 60;
  int hours = lastCheckPoint / 3600;

	//Information displayed in different formats if the last checkpoint time is close to an hour:
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
	//Draw timer background rectangle:
	GRect bounds = layer_get_bounds(layer);
	GRect timerRect = GRect(0, 0, bounds.size.w, 138);
	graphics_context_set_fill_color(ctx, GColorMayGreen);
	graphics_fill_rect(ctx, timerRect, 0, GCornerNone);
	
	//Draws current time of the timer:
	drawRunTimer(layer, ctx);
}

//Method draws labels besides the bottom button for deleting the current record or ending the timer prematurely:
static void bbutton_update_callback(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
	graphics_context_set_fill_color(ctx, GColorMayGreen);
	GRect tab = GRect(bounds.size.w-25, bounds.size.h-37, 25, 25);
	graphics_fill_rect(ctx, GRect(bounds.size.w-25, bounds.size.h-40, 25, 27), 5, GCornersLeft);
	
	char* bLabel;
	bLabel = (runmax.status == running)? "END": "DEL";
	graphics_draw_text(ctx, bLabel, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), tab, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void run_window_load(Window *window) {
	//Layer pointer for the bounds of the window layer:
	Layer *window_layer = window_get_root_layer(window);
 	GRect window_bounds = layer_get_bounds(window_layer);

	//Initialising the layer holding the timer information:
	s_run_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h-58));
  layer_add_child(window_layer, s_run_layer);
	layer_set_update_proc(s_run_layer, run_update_callback);

	//Initialising the layer holding the current record information and record number:
	save_no_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
	layer_insert_above_sibling(save_no_layer, s_run_layer);
	layer_set_update_proc(save_no_layer, saveNo_update_callback);

	//Initialising the layer holding the date of the current record:
	s_run_info_layer = text_layer_create(GRect(0, window_bounds.size.h-53, window_bounds.size.w, 20));
	layer_insert_above_sibling(text_layer_get_layer(s_run_info_layer), s_run_layer);
	text_layer_set_text_alignment(s_run_info_layer, GTextAlignmentCenter);
	text_layer_set_font(s_run_info_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	
	//Initialising the layer for the lables besides the bottom button:
	bbutton_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
	layer_insert_above_sibling(bbutton_layer, text_layer_get_layer(s_run_info_layer));
	layer_set_update_proc(bbutton_layer, bbutton_update_callback);

	// Initialise the timer save no and runner struct:
	saveInNo = s_uptime = 0;
	initialiseRunner(&runmax);
	initialiseCheckPoints();
	checkVSettings();
	
	//Set label layer hidden if there are no checkpoints in record:
	layer_set_hidden(bbutton_layer, (times[saveInNo].noOfCheckPoints != 0)? false: true);
}

static void run_window_unload(Window *window) {
	persist_write_data(STIME_KEY, &times, sizeof(times));
		//APP_LOG(APP_LOG_LEVEL_WARNING, "Wrote %d bytes to settings.", valueWritten);

	//Destroying the run layers:
	text_layer_destroy(s_run_info_layer);
	layer_destroy(s_run_layer);
	layer_destroy(save_no_layer);
	layer_destroy(bbutton_layer);
	
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