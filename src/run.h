#pragma once
#include <pebble.h>
#include "main.h"
#define maxCheckPoints 10
#define maxNoRecords 5
#define STIME_KEY 111

typedef enum {
    paused = 0,
    running = 1,
		stopped = 2
} runnerStatus;

typedef struct Runner {
	uint8_t saveID;
	runnerStatus status;
} Runner;

typedef struct timeSaver {
	uint16_t checkPoints[maxCheckPoints];	
	char dateTime[30];
	uint8_t noOfCheckPoints;	
} __attribute__((__packed__)) timeSaver;
	
extern void run_create(void);
extern uint8_t getSavedInNo(void);

extern timeSaver times[maxNoRecords];