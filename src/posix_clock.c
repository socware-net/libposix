/*
 * Amazon FreeRTOS+POSIX V1.0.2
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */
/*
 * modified by socware.net to support hyperC OS
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://socware.net
 */
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "utils.h"
#include <hcos/core.h>
#include <hcos/task.h>
#include <stdio.h>

clock_t clock()
{
	unsigned now = tmr_ticks;
	return (clock_t) (now - core_idle()->load.sum);
}

int clock_getcpuclockid(pid_t pid, clockid_t * clock_id)
{
	errno = EPERM;
	return -1;
}

int clock_getres(clockid_t clock_id, struct timespec *res)
{
	if (res != 0) {
		res->tv_sec = 0;
		res->tv_nsec = NANOSECONDS_PER_TICK;
	}
	return 0;
}

int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
	if (tp == 0) {
		return -1;
	} else {
		long long ticks = tmr_ticks;
		nano2timespec(ticks * NANOSECONDS_PER_TICK, tp);
		return 0;
	}
}

int clock_nanosleep(clockid_t clock_id,
		    int flags,
		    const struct timespec *rqtp, struct timespec *rmtp)
{
	int ret = 0;
	struct timespec cur = { 0 };
	unsigned sleep = 0;

	if (!validate_timespec(rqtp)) {
		ret = EINVAL;
	}

	if ((ret == 0)
	    && (clock_gettime(CLOCK_REALTIME, &cur) != 0)) {
		ret = EINVAL;
	}

	if (ret == 0) {
		if ((flags & TIMER_ABSTIME) == TIMER_ABSTIME) {
			if (clock_gettime(CLOCK_REALTIME, &cur) == 0) {
				ret = EINVAL;
			}
			if ((ret == 0) &&
			    (abs_timespec2ticks(rqtp, &cur, &sleep) == 0)) {
				task_sleep(sleep);
			}
		} else {
			if (timespec2ticks(rqtp, &sleep) == 0) {
				task_sleep(sleep);
			}
		}
	}
	return ret;
}

int clock_settime(clockid_t clock_id, const struct timespec *tp)
{
	(void)clock_id;
	(void)tp;
	errno = EPERM;
	return -1;
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	int ret = 0;
	unsigned sleep = 0;

	if (!validate_timespec(rqtp)) {
		errno = EINVAL;
		ret = -1;
	}

	if (ret == 0) {
		if (timespec2ticks(rqtp, &sleep) == 0) {
			task_sleep(sleep);
		}
	}

	return ret;
}

size_t strftime(char *s,
		size_t maxsize, const char *format, const struct tm * timeptr)
{
	int ret = 0;
	size_t n = 0;
	ret = snprintf(s, maxsize, "%ld", (long int)timeptr->tm_tick);
	if ((ret > 0) && ((size_t) ret < maxsize)) {
		n = (size_t) ret;
	}
	return n;
}

time_t time(time_t * tloc)
{
	time_t xCurrentTime = (time_t) (tmr_ticks / tmr_hz);
	if (tloc != 0) {
		*tloc = xCurrentTime;
	}
	return xCurrentTime;
}
