/*
 * Amazon FreeRTOS POSIX V1.0.0
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

/**
 * @file time.h
 * @brief Time types.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/time.h.html
 */

#ifndef _HCOS_POSIX_TIME_H_
#define _HCOS_POSIX_TIME_H_

#include <sys/types.h>
#include <signal.h>
#include <hcos/tmr.h>

/**
 * @defgroup Unit conversion constants.
 */
#define MICROSECONDS_PER_SECOND    ( 1000000LL )		       ///< Microseconds per second.
#define NANOSECONDS_PER_SECOND     ( 1000000000LL )		       ///< Nanoseconds per second.
#define NANOSECONDS_PER_TICK       ( NANOSECONDS_PER_SECOND / tmr_hz ) ///< Nanoseconds per RTOS tick.

#define CLOCK_REALTIME     0	 ///< The identifier of the system-wide clock measuring real time.
#define CLOCK_MONOTONIC    1	 ///< The identifier for the system-wide monotonic clock.

/**
 * @brief A number used to convert the value returned by the clock() function into seconds.
 */
#define CLOCKS_PER_SEC    ( ( clock_t ) tmr_hz )

/**
 * @brief Flag indicating time is absolute.
 *
 * For functions taking timer objects, this refers to the clock associated with the timer.
 */
#define TIMER_ABSTIME     0x01

struct tm {
	time_t tm_tick;	///< tick count.
	int tm_sec;		///< Seconds [0,60]. Not used.
	int tm_min;		///< Minutes [0,59]. Not used.
	int tm_hour;	///< Hour [0,23]. Not used.
	int tm_mday;	///< Day of month [1,31]. Not used.
	int tm_mon;		///< Month of year [0,11]. Not used.
	int tm_year;	///< Years since 1900. Not used.
	int tm_wday;	///< Day of week [0,6] (Sunday=0). Not used.
	int tm_yday;	///< Day of year [0,365]. Not used.
	int tm_isdst;	///< Daylight Savings flag. Not used.
};

/**
 * @brief Report CPU time used.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock.html
 *
 * @note This function reports the number of RTOS ticks since the scheduler
 * was started minus the ticks spent in the idle task. It does NOT report the
 * number of ticks spent by the calling thread.
 */
clock_t clock(void);

/**
 * @brief Access a process CPU-time clock.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_getcpuclockid.html
 *
 * @note This function is currently unsupported. It will always return -1 and
 * set errno to EPERM.
 */
int clock_getcpuclockid(pid_t pid, clockid_t * clock_id);

/**
 * @brief Returns the resolution of a clock.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_getres.html
 *
 * @note clock_id is ignored; this function returns the resolution of the FreeRTOS
 * tick count.
 */
int clock_getres(clockid_t clock_id, struct timespec *res);

/**
 * @brief Returns the current value for the specified clock, clock_id.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_gettime.html
 *
 * @note clock_id is ignored; this function returns the RTOS tick count.
 * Also, this function does not check for overflows of time_t.
 */
int clock_gettime(clockid_t clock_id, struct timespec *tp);

/**
 * @brief High resolution sleep with specifiable clock.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_nanosleep.html
 *
 * @note clock_id is ignored; this function uses the RTOS tick count as its
 * clock. rmtp is also ignored, as signals are not implemented. flags is ignored
 * if INCLUDE_vTaskDelayUntil is 0, i.e. the RTOS function vTaskDelayUntil
 * isn't available.
 */
int clock_nanosleep(clockid_t clock_id,
		    int flags,
		    const struct timespec *rqtp, struct timespec *rmtp);

/**
 * @brief Sets the time for the specified clock.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_settime.html
 *
 * @note This function is currently unsupported, as RTOS does not provide
 * a function to modify the tick count. It will always return -1 and set errno
 * to EPERM.
 */
int clock_settime(clockid_t clock_id, const struct timespec *tp);

/**
 * @brief High resolution sleep.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/nanosleep.html
 *
 * @note rmtp is ignored, as signals are not implemented.
 */
int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);

/**
 * @brief Convert date and time to a string.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/strftime.html
 *
 * @note format is ignored.
 */
size_t strftime(char *s,
		size_t maxsize, const char *format, const struct tm *timeptr);

/**
 * @brief Get time.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/time.html
 *
 * @note This function returns the RTOS tick count, not the seconds since
 * UNIX epoch.
 */
time_t time(time_t * tloc);

/**
 * @brief Create a per-process timer.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/timer_create.html
 *
 * @note clock_id is ignored; this function used the RTOS tick count as its
 * clock. Because signals are currently unimplemented, evp.sigev_notify must be
 * set to SIGEV_THREAD.
 */
int timer_create(clockid_t clockid, struct sigevent *evp, timer_t * timerid);

/**
 * @brief Delete a per-process timer.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/timer_delete.html
 */
int timer_delete(timer_t timerid);

/**
 * @brief Get the timer overrun count.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/timer_getoverrun.html
 *
 * Signals are not implemented, so this function will always return 0.
 */
int timer_getoverrun(timer_t timerid);

/**
 * @brief Get the amount of time until the timer expires.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/timer_gettime.html
 */
int timer_gettime(timer_t timerid, struct itimerspec *value);

/**
 * @brief Set the time until the next expiration of the timer.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/timer_settime.html
 */
int timer_settime(timer_t timerid,
		  int flags,
		  const struct itimerspec *value, struct itimerspec *ovalue);

#endif /* ifndef _HCOS_POSIX_TIME_H_ */
