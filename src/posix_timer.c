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
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <utils.h>
#include <time.h>
#include <hcos/dbg.h>
#include "hcos_posix.h"

// Check for 0.
#define TIMESPEC_IS_ZERO( ts )        ( ts.tv_sec == 0 && ts.tv_nsec == 0 )

// Check for not 0.
#define TIMESPEC_IS_NOT_ZERO( ts )    ( !( TIMESPEC_IS_ZERO( ts ) ) )

typedef struct timer_internal {
	tmr_t ost;		///< Memory that holds the RTOS timer.
	struct sigevent event;	///< What to do when this timer expires.
	unsigned period;	///< Period of this timer.
} timer_internal_t;

static int timer_do(void *p)
{
	timer_internal_t *timer = (timer_internal_t *) p;

	// The value of the timer ID, set in timer_create, should not be NULL.
	_assert(timer != 0);

	// A value of SIGEV_SIGNAL isn't supported
	_assert(timer->event.sigev_notify != SIGEV_SIGNAL);

	// Update the timer period
	if (timer->period > 0) {
		tmr_on(&timer->ost, timer->period);
	}
	// Create the timer notification thread if requested.
	if (timer->event.sigev_notify == SIGEV_THREAD) {
		// if the user has provided thread attributes, create a thread
		// with the provided attributes. Otherwise dispatch callback directly
		if (timer->event.sigev_notify_attributes == 0) {
			timer->event.sigev_notify_function(timer->
							   event.sigev_value);
		} else {
			pthread_t thread;
			(void)pthread_create(&thread,
					     timer->
					     event.sigev_notify_attributes,
					     (void *(*)(void *))
					     timer->event.sigev_notify_function,
					     timer->event.
					     sigev_value.sival_ptr);
		}
	}
	return 0;
}

int timer_create(clockid_t clockid, struct sigevent *evp, timer_t * timerid)
{
	int ret = 0;
	timer_internal_t *timer = 0;
	// POSIX specifies that when evp is NULL, the behavior shall be as is
	// sigev_notify is SIGEV_SIGNAL. SIGEV_SIGNAL is currently not supported.
	if ((!evp) || (evp->sigev_notify == SIGEV_SIGNAL)) {
		errno = ENOTSUP;
		ret = -1;
	}
	if (hcos_posix_alloc == 0) {
		errno = EAGAIN;
		ret = -1;
	}
	// Allocate memory for a new timer object.
	if (ret == 0) {
		timer = hcos_posix_alloc(POSIX_TIMER, sizeof(timer_internal_t));
		memset(timer, 0, sizeof(timer_internal_t));
		if (!timer) {
			errno = EAGAIN;
			ret = -1;
		}
	}
	if (ret == 0) {
		// Copy the event notification structure and set the current timer period.
		timer->event = *evp;
		timer->period = 0;
		tmr_init(&timer->ost, timer, timer_do);
		*timerid = (timer_t) timer;
	}
	return ret;
}

int timer_delete(timer_t timerid)
{
	timer_internal_t *timer = (timer_internal_t *) timerid;
	_assert(timer != 0);
	tmr_of(&timer->ost);
	hcos_posix_free(POSIX_TIMER, timer);
	return 0;
}

int timer_getoverrun(timer_t timerid)
{
	return 0;
}

int timer_settime(timer_t timerid, int flags,
		  const struct itimerspec *value, struct itimerspec *ovalue)
{
	int ret = 0;
	timer_internal_t *timer = (timer_internal_t *) timerid;
	unsigned next_expire = 0;
	unsigned period = 0;

	// Validate the value argument, but only if the timer isn't being disarmed.
	if (TIMESPEC_IS_NOT_ZERO(value->it_value)) {
		if ((validate_timespec(&value->it_interval) == false) ||
		    (validate_timespec(&value->it_value) == false)) {
			errno = EINVAL;
			ret = -1;
		}
	}

	if (ovalue) {
		(void)timer_gettime(timerid, ovalue);
	}

	if (ret == 0) {
		tmr_of(&timer->ost);
	}
	// Only restart the timer if it_value is not zero.
	if ((ret == 0) && TIMESPEC_IS_NOT_ZERO(value->it_value)) {
		// Convert it_interval to ticks, but only if it_interval is not 0. If
		// it_interval is 0, then the timer is not periodic.
		if (TIMESPEC_IS_NOT_ZERO(value->it_interval)) {
			(void)timespec2ticks(&value->it_interval, &period);
		}
		// Set the new timer period. A non-periodic timer will have its period set
		// to portMAX_DELAY.
		timer->period = period;

		// Convert it_value to ticks, but only if it_value is not 0. If it_value
		// is 0, then the timer will remain disarmed.
		if (TIMESPEC_IS_NOT_ZERO(value->it_value)) {
			// Absolute timeout.
			if ((flags & TIMER_ABSTIME) == TIMER_ABSTIME) {
				struct timespec cur = { 0 };

				// Get current time
				if (clock_gettime(CLOCK_REALTIME, &cur)
				    != 0) {
					ret = EINVAL;
				} else {
					ret =
					    abs_timespec2ticks(&value->it_value,
							       &cur,
							       &next_expire);
				}

				// Make sure next_expire is zero in case we got negative time difference
				if (ret != 0) {
					next_expire = 0;

					if (ret == ETIMEDOUT) {
						// Set Status to 0 as absolute time is past is treated as expiry but not an error
						ret = 0;
					}
				}
			}
			// Relative timeout.
			else {
				(void)timespec2ticks(&value->it_value,
						     &next_expire);
			}
		}
		// If next_expire is still 0, that means that it_value specified
		// an absolute timeout in the past. Per POSIX spec, a notification should be
		// triggered immediately.
		if (next_expire == 0) {
			timer_do(timer);
		} else {
			/* Set the timer to expire at the it_value, then start it. */
			tmr_on(&timer->ost, next_expire);
		}
	}
	return ret;
}

int timer_gettime(timer_t timerid, struct itimerspec *value)
{
	timer_internal_t *timer = (timer_internal_t *) timerid;
	unsigned next_expire = timer->ost.expire - tmr_ticks;
	unsigned period = timer->period;

	// Set it_value only if the timer is armed. Otherwise, set it to 0.
	if (tmr_active(&timer->ost)) {
		value->it_value.tv_sec = (time_t) (next_expire / tmr_hz);
		value->it_value.tv_nsec =
		    (long)((next_expire % tmr_hz) * NANOSECONDS_PER_TICK);
	} else {
		value->it_value.tv_sec = 0;
		value->it_value.tv_nsec = 0;
	}

	value->it_interval.tv_sec = (time_t) (period / tmr_hz);
	value->it_interval.tv_nsec =
	    (long)((period % tmr_hz) * NANOSECONDS_PER_TICK);

	return 0;
}

/*-----------------------------------------------------------*/
