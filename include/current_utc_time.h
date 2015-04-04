#ifndef CURRENT_UTC_TIME_H
#define CURRENT_UTC_TIME_H

#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

void current_utc_time(struct timespec *ts);

#endif
// EOF
