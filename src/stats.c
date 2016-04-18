#include <pebble.h>
#include "stats.h"

static Window *s_stats_window;
static Layer *s_stats_layer;
static Layer *s_stats_info;
static TextLayer *s_stats_text;

static uint8_t saveInNo;
struct tm *timeinfostat;

statTab stattab;

//Initialising functions: 
static char* runStats();

static void info_update_callback(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(s_stats_layer);
  
  //Creating the checkpoint information rectangle (holds the no. of checkpoints data):
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_fill_rect(ctx, GRect(10, 39, bounds.size.w-20, 125), 0, GCornerNone);
  
  //Label for the checkpoint information rectangle:
  graphics_context_set_fill_color(ctx, GColorRajah);
  graphics_context_set_text_color(ctx, GColorWhite);
  GRect heading = GRect(10, 39, bounds.size.w-20, 25);
  graphics_fill_rect(ctx, heading, 0, GCornerNone);
  graphics_draw_text(ctx, "#Checkpoints", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), heading, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  graphics_draw_rect(ctx, GRect(10, 39, bounds.size.w-20, 125));
  
  //'For' loop to show the number of checkpoints in each saving point:
  for(int i = 0; i <5; i++) {
    static char buf[] = "00000000000";
    graphics_context_set_text_color(ctx, GColorBlack);
    
    if(times[i].noOfCheckPoints != 0) {
      graphics_context_set_fill_color(ctx, GColorBlack);
      snprintf(buf, sizeof(buf), "%d", times[i].noOfCheckPoints);
    }
    else{
      graphics_context_set_fill_color(ctx, GColorRed);
      graphics_context_set_text_color(ctx, GColorRed);
      snprintf(buf, sizeof(buf), "Empty");
    }
    graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(52, 61+(i*20), 68, 16), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    graphics_fill_circle(ctx, GPoint(35, 73+(i*20)), 8);
    
    graphics_context_set_text_color(ctx, GColorWhite);
    snprintf(buf, sizeof(buf), "%d", i+1);
    graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(27, 61+(i*20), 16, 16), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }	
  
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  //Increments the 'saveInNo', ie the record number:
  saveInNo = (saveInNo % maxNoRecords != 0)? saveInNo+1: 1;
  
  //Initialises the tab number:
  stattab = TAB1;
  
  //May vibrate depending on settings and freshes the stats layer:
  shortVibrate(vibes.stats);
  layer_mark_dirty(s_stats_layer);
  text_layer_set_text(s_stats_text, runStats());
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  //Toggles between the 3 tabs:
  stattab = (stattab<2)? stattab+1 : TAB1;
  
  layer_mark_dirty(s_stats_layer);
  text_layer_set_text(s_stats_text, runStats());
}

static void down_press_handler(ClickRecognizerRef recognizer, void *context) {
  //Unhides the checkpoint information layer when down button is held:
  layer_set_hidden(s_stats_info, false);
}

static void down_release_handler(ClickRecognizerRef recognizer, void *context) {
  //Hides the checkpoint information layer when down button released:
  layer_set_hidden(s_stats_info, true);
} 

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  //Unloads window if back button is pressed:	
  window_stack_remove(s_stats_window,true);
}

static void click_config_provider(void *context) {
  //Register the ClickHandlers:
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_raw_click_subscribe(BUTTON_ID_DOWN, down_press_handler, down_release_handler, NULL);
}

static void showNoOfCheckPoints(GContext *ctx) {
  graphics_context_set_text_color(ctx, GColorBlack);
  
  // Use a long-lived buffers to hold the record number and the time stored:
  char time_buffer[512];
  char checkNo[128];
  
  for(uint8_t i =0; i<times[saveInNo-1].noOfCheckPoints; i++) {
    //Puts the current record number into the buffer:
    snprintf(checkNo, sizeof(checkNo), "%d", i+1);
    
    //Curent record is drawn in various positions within the layer:
    GRect position = (i<5)? GRect(2, 38+(i*25), 16, 20): GRect(74, 38+((i-5)*25), 16, 20);
    graphics_draw_text(ctx, checkNo, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), position, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    
    // Get time of the run and place time into buffer:	
    int currentTime = times[saveInNo-1].checkPoints[i];
    int seconds = currentTime % 60;	
    int minutes = currentTime / 60;
    snprintf(time_buffer, sizeof(time_buffer), "%02d:%02d", minutes, seconds);
    
    //Saved time of checkpoint is drawn onto layer:
    GRect timerBox = (i<5)? GRect(18, 38+(i*25), 54, 20): GRect(90, 38+((i-5)*25), 54, 20);
    graphics_draw_text(ctx, time_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_18), timerBox, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

static int findMaxDiff(uint8_t saveInNo) {
  //Method to find the maximum difference between checkpoints in current saving record:
  int maxDiff = 0, diff = 0;
  for(int i =0; i<times[saveInNo].noOfCheckPoints; i++) {
    diff = (i != 0)? times[saveInNo].checkPoints[i] - times[saveInNo].checkPoints[i-1]: times[saveInNo].checkPoints[i];
    maxDiff = diff>maxDiff? diff: maxDiff; 
  }
  return maxDiff;
}

static void showOverview (GContext *ctx, Layer *layer) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_text_color(ctx, GColorBlack);	
  
  char buffer[512]; 
  const char* optionLabel[] = {"Duration:", "Checkpoints:", "Max dT:"};
  
  //Number of checkpoints drawn onto the layer:
  snprintf(buffer, sizeof(buffer), "%d", times[saveInNo-1].noOfCheckPoints);
  graphics_draw_text(ctx, buffer, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(bounds.size.w-42, 92, 40, 28), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  
  //Total duration of the saved times drawn onto layer in (mins:sec) layout:
  int totalDuration = times[saveInNo-1].checkPoints[times[saveInNo-1].noOfCheckPoints-1];
  int sec = totalDuration % 60;		int min = totalDuration / 60;		
  snprintf(buffer, sizeof(buffer), "%02d:%02d", min, sec);
  graphics_draw_text(ctx, buffer, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(bounds.size.w-42, 50, 40, 28), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  
  //Maximum difference between two checkpoints drawn onto the layer:
  int maxD = findMaxDiff(saveInNo-1);
  int seconds = maxD % 60;		int minutes = maxD / 60;		
  snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
  graphics_draw_text(ctx, buffer, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(bounds.size.w-42, 134, 40, 28), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  
  //'For' Loop to draw the containing round rectangles on layer and inserts labels within them:
  for(int i = 0; i<3; i++) {
    graphics_context_set_stroke_color(ctx, GColorRajah);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_round_rect(ctx, GRect(5, 40+(i*42), bounds.size.w-50, 25), 5);
    GRect setOptions = GRect(5, (i*42)+40, bounds.size.w-50, 30); 
    graphics_draw_text(ctx, optionLabel[i], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), setOptions, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_stroke_width(ctx, 1);
    GRect infoRect = GRect(bounds.size.w-42, 50+(i*42), 40, 28);
    graphics_draw_round_rect(ctx, infoRect, 5);
  }		
}

static void showTimeDiff (GContext *ctx) {
  // Use long-lived buffers to store the saved record no and the calculated time differences:
  char time_buffer[512];
  char checkNo[128];
  
  //Finds the maximum difference between checkpoints and stores in 'int':
  int maxD = findMaxDiff(saveInNo-1);
  
  for(uint8_t i =0; i<times[saveInNo-1].noOfCheckPoints; i++) {
    graphics_context_set_text_color(ctx, GColorBlack);
    snprintf(checkNo, sizeof(checkNo), "%d", i+1);
    
    
    //Draws the checkpoint number onto the layer:
    GRect position = (i<5)? GRect(2, 38+(i*25), 16, 20): GRect(74, 38+((i-5)*25), 16, 20);
    graphics_draw_text(ctx, checkNo, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), position, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);			
    
    //Method to calculate the difference between checkpoints:
    int diff = (i != 0)? times[saveInNo-1].checkPoints[i] - times[saveInNo-1].checkPoints[i-1]: times[saveInNo-1].checkPoints[i];
    
    //Get time of the run in (mins:sec) format and places into the time buffer:
    int seconds = diff % 60;	int minutes = diff / 60;
    snprintf(time_buffer, sizeof(time_buffer), "%02d:%02d", minutes, seconds);	
    
    //Draws the time difference between checkpoints into calculated positions within the layer:			
    (diff == maxD)?	graphics_context_set_text_color(ctx, GColorBlack):	graphics_context_set_text_color(ctx, GColorRajah);
    GRect timerBox = (i<5)? GRect(18, 38+(i*25), 54, 20): GRect(90, 38+((i-5)*25), 54, 20);
    graphics_draw_text(ctx, time_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_18), timerBox, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

static void stats_update_callback(Layer *layer, GContext *ctx) {	
  //Draws the header of the window:
  GRect infoRect = GRect(0, 0, layer_get_bounds(layer).size.w, 35);
  graphics_context_set_fill_color(ctx, GColorRajah);
  graphics_fill_rect(ctx, infoRect, 0, GCornerNone);
  (times[saveInNo-1].noOfCheckPoints != 0)?	graphics_context_set_fill_color(ctx, GColorBlack): graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_circle(ctx, GPoint(17, 17), 12);
  
  //Draws the current record number of the layer:
  graphics_context_set_text_color(ctx, GColorWhite);
  static char buf[] = "00000000000";
  snprintf(buf, sizeof(buf), "%d", saveInNo);
  graphics_draw_text(ctx, buf, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(0, 0, 35, 35), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  
  //'If' and 'switch' statements to present the relevant data:
  graphics_context_set_text_color(ctx, GColorBlack);
  if(times[saveInNo-1].noOfCheckPoints == 0 ){
    //A string is outputted if there are no records in the saving point:
    graphics_draw_text(ctx, "There are no records", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(0, 39, 144, 130), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  } else {
    //Different methods are called for different tab numbers:
    switch(stattab) {
      case TAB1: 
      showOverview(ctx, layer);
      break;
      case TAB2: 
      showNoOfCheckPoints(ctx);
      break;
      case TAB3: 
      showTimeDiff(ctx);
      break;
    }
  }
}

static char* runStats(void) {
  //Different labels for when the select button is pressed, tab number is incremented:
  switch(stattab) {
    default:
    case TAB1:	
    if(times[saveInNo-1].noOfCheckPoints != 0) {
      return ((times[saveInNo-1].dateTime != NULL) && (times[saveInNo-1].dateTime[0] == '\0'))? "Buffer not big enough": times[saveInNo-1].dateTime;
    }
    else {
      return "Empty";
    }
    case TAB2: return "Checkpoints";
    case TAB3: return "Durations";
  }
}

static void stats_window_load(Window *window) {
  //Layer pointer for the bounds of the window layer:
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  
  //Layer that holds the header and the contents of each tab:
  s_stats_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
  layer_add_child(window_layer, s_stats_layer);	
  layer_set_update_proc(s_stats_layer, stats_update_callback);
  
  //Text layer to show when the current record was recorded, ie its date:
  s_stats_text = text_layer_create(GRect(30, 1, 110, 29));
  text_layer_set_background_color(s_stats_text, GColorRajah);
  layer_insert_above_sibling(text_layer_get_layer(s_stats_text), s_stats_layer);
  text_layer_set_text_alignment(s_stats_text, GTextAlignmentCenter);
  text_layer_set_font(s_stats_text, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  
  //Layer that holds the no of checkpoints in each saving point:
  s_stats_info = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
  layer_insert_above_sibling(s_stats_info, s_stats_layer);
  layer_set_update_proc(s_stats_info, info_update_callback);
  layer_set_hidden(s_stats_info, true);
  
  //Method to read the data stored in the 'run' class/saved records:
  if (persist_exists(STIME_KEY)) {
    persist_read_data(STIME_KEY, &times, sizeof(times));
    //APP_LOG(APP_LOG_LEVEL_WARNING, "Read %d bytes from settings", valueRead);
  }
  
  //Initialises the current tab, the header of the layer and the record number:
  stattab = TAB1;
  saveInNo = 1;
  text_layer_set_text(s_stats_text, runStats());
  
  //Method to check the vibration settings:
  checkVSettings();
}

static void stats_window_unload(Window *window) {
  //Destroying the stat layers/text layers:
  layer_destroy(s_stats_layer);
  layer_destroy(s_stats_info);
  text_layer_destroy(s_stats_text);
  
  //Destroys the window:
  window_destroy(s_stats_window);
}

void stats_create() {
  s_stats_window = window_create();
  window_set_window_handlers(s_stats_window, (WindowHandlers) {
    .load = stats_window_load,
    .unload = stats_window_unload,
  });
  
  window_set_click_config_provider(s_stats_window, click_config_provider);
  window_stack_push(s_stats_window, true);
}
