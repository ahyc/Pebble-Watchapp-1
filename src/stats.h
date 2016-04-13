#pragma once
#include <pebble.h>
#include "main.h"

typedef enum {
	TAB1 = 0,
	TAB2 = 1, 
	TAB3 = 2,
} statTab;

extern void stats_create(void);