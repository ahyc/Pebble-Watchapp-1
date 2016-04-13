#pragma once
#include <pebble.h>
#include "draw.h"
#include "run.h"
#include "stats.h"
#include "settings.h"
	
#define STATUS_BAR_HEIGHT 15

extern Window *s_main_window;	
extern StatusBarLayer *status_bar;

extern GFont s_font_30;