#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;

	/* TODO: Add more members if necessary */
	char expr[32];
	uint32_t val;

} WP;

WP* new_wp(char*, uint32_t);
void free_wp(int);
void printWatchpoints();
bool CheckWatchpoints();
bool haveWatchpoints();

#endif
