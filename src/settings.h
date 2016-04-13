#pragma once
#include <pebble.h>
#define VSET_KEY 222

typedef enum {
    noVibes = 0,
    Vibes = 1,
} vibeStatus;

typedef struct vibes {
	vibeStatus main;
	vibeStatus run;
	vibeStatus stats;
} vibeSettings;
extern vibeSettings vibes;

extern void settings_create(void);

//Method to check the vibration settings for the current window:
extern void checkVSettings(void);

//Methods to check whether the vibration for the window is on/off:
extern void shortVibrate(vibeStatus page);
extern void longVibrate(vibeStatus page);
extern void doubleVibrate(vibeStatus page);