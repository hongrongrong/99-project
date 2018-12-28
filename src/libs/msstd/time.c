#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "msstd.h"


void time_to_string(int ntime, char stime[32])
{
	struct tm temp;

	localtime_r((long*)&ntime, &temp);
	snprintf(stime, 32, "%4d-%02d-%02d %02d:%02d:%02d", 
		temp.tm_year + 1900, temp.tm_mon + 1, temp.tm_mday, temp.tm_hour, temp.tm_min, temp.tm_sec);
}

void time_to_string_ms(char stime[32])
{
	struct tm temp;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &temp);
	snprintf(stime, 32, "%4d-%02d-%02d %02d:%02d:%02d.%03d", 
		temp.tm_year + 1900, temp.tm_mon + 1, temp.tm_mday, temp.tm_hour, temp.tm_min, temp.tm_sec,tv.tv_usec/1000);
}

