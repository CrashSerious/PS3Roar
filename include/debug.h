/* for debugging the PS3 code */

#include <stdio.h>

#define DEBUG


void debugLog(char * logLine)
{
	int x;
	#ifdef DEBUG
		printf("*DEBUG LOG* %s\n",logLine);
	#endif
	x =0;
}

