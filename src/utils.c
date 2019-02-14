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
#include <limits.h>

#include <errno.h>
#include <utils.h>

static int timespec_sub(const struct timespec *const x,
			const struct timespec *const y,
			struct timespec *const result);

int abs_timespec2ticks(const struct timespec *const abs_time,
		       const struct timespec *const cur_time,
		       unsigned *const result)
{
	int ret = 0;
	struct timespec diff = { 0 };

	// Check parameters.
	if ((abs_time == NULL) || (cur_time == NULL)
	    || (result == NULL)) {
		ret = EINVAL;
	}
	// Calculate the difference between the current time and absolute time.
	if (ret == 0) {
		ret = timespec_sub(abs_time, cur_time, &diff);
		if (ret == 1) {
			// abs_time was in the past.
			ret = ETIMEDOUT;
		} else if (ret == -1) {
			ret = EINVAL;
		}
	}
	// Convert the time difference to ticks.
	if (ret == 0) {
		ret = timespec2ticks(&diff, result);
	}

	return ret;
}

int timespec2ticks(const struct timespec *const time_spec,
		   unsigned *const result)
{
	int ret = 0;
	int64_t total_ticks = 0;
	long nanos = 0;

	if ((time_spec == NULL) || (result == NULL)) {
		ret = EINVAL;
	} else if ((ret == 0)
		   && (validate_timespec(time_spec) == false)) {
		ret = EINVAL;
	}

	if (ret == 0) {
		// Convert timespec.tv_sec to ticks.
		total_ticks = (int64_t) tmr_hz *(time_spec->tv_sec);

		// Convert timespec.tv_nsec to ticks. This value does not have to be checked
		// for overflow because a valid timespec has 0 <= tv_nsec < 1000000000 and
		// NANOSECONDS_PER_TICK > 1.
		nanos = time_spec->tv_nsec / (long)NANOSECONDS_PER_TICK +	// Whole nanoseconds.
		    (long)(time_spec->tv_nsec % (long)NANOSECONDS_PER_TICK != 0);	// Add 1 to round up if needed.

		// Add the nanoseconds to the total ticks.
		total_ticks += (int64_t) nanos;

		// Check for overflow
		if (total_ticks < 0) {
			ret = EINVAL;
		} else {
			// check if unsigned is 32 bit or 64 bit
			uint32_t ulTickTypeSize = sizeof(unsigned);
			// check for downcast overflow
			if (ulTickTypeSize == sizeof(uint32_t)) {
				if (total_ticks > UINT_MAX) {
					ret = EINVAL;
				}
			}
		}
		*result = (unsigned)total_ticks;
	}

	return ret;
}

void nano2timespec(int64_t llSource, struct timespec *const dest)
{
	long carry = 0;
	// Convert to timespec.
	dest->tv_sec = (time_t) (llSource / NANOSECONDS_PER_SECOND);
	dest->tv_nsec = (long)(llSource % NANOSECONDS_PER_SECOND);

	// Subtract from tv_sec if tv_nsec < 0.
	if (dest->tv_nsec < 0L) {
		// Compute the number of seconds to carry.
		carry = (dest->tv_nsec / (long)NANOSECONDS_PER_SECOND) + 1L;

		dest->tv_sec -= (time_t) (carry);
		dest->tv_nsec += carry * (long)NANOSECONDS_PER_SECOND;
	}
}

static int timespec_cmp(const struct timespec *const x,
			const struct timespec *const y)
{
	int ret = 0;
	if ((x == NULL) && (y == NULL)) {
		ret = 0;
	} else if (y == NULL) {
		ret = 1;
	} else if (x == NULL) {
		ret = -1;
	} else if (x->tv_sec > y->tv_sec) {
		ret = 1;
	} else if (x->tv_sec < y->tv_sec) {
		ret = -1;
	} else {
		// seconds are equal compare nano seconds
		if (x->tv_nsec > y->tv_nsec) {
			ret = 1;
		} else if (x->tv_nsec < y->tv_nsec) {
			ret = -1;
		} else {
			ret = 0;
		}
	}

	return ret;
}

static int timespec_sub(const struct timespec *const x,
			const struct timespec *const y,
			struct timespec *const result)
{

	int cmp = 0;
	int ret = 0;

	if ((result == NULL) || (x == NULL) || (y == NULL)) {
		ret = -1;
	}

	if (ret == 0) {
		cmp = timespec_cmp(x, y);
		// if x < y then result would be negative, return 1
		if (cmp == -1) {
			ret = 1;
		} else if (cmp == 0) {
			// if times are the same return zero
			result->tv_sec = 0;
			result->tv_nsec = 0;
		} else {
			// If x > y Perform subtraction.
			result->tv_sec = x->tv_sec - y->tv_sec;
			result->tv_nsec = x->tv_nsec - y->tv_nsec;

			// check if nano seconds value needs to borrow
			if (result->tv_nsec < 0) {
				// Based on comparison, tv_sec > 0
				result->tv_sec--;
				result->tv_nsec += (long)NANOSECONDS_PER_SECOND;
			}
			// if nano second is negative after borrow, it is an overflow error
			if (result->tv_nsec < 0) {
				ret = -1;
			}
		}
	}

	return ret;
}

int validate_timespec(const struct timespec *const time_spec)
{
	int ret = 0;

	if (time_spec != NULL) {
		// Verify 0 <= tv_nsec < 1000000000.
		if ((time_spec->tv_nsec >= 0) &&
		    (time_spec->tv_nsec < NANOSECONDS_PER_SECOND)) {
			ret = 1;
		}
	}

	return ret;
}
