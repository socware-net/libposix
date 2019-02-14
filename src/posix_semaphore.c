/**
 * Copyright (C) 2018 socware.net.  All Rights Reserved.
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
#include <utils.h>
#include <hcos/sem.h>
#include "utils.h"

int sem_destroy(sem_t * sem)
{
	return 0;
}

int sem_init_posix(sem_t * sem, int pshared, unsigned value)
{
	int ret = 0;
	if (ret == 0) {
		sem_init(sem, value);
	}
	return ret;
}

int sem_timedwait(sem_t * sem, const struct timespec *abstime)
{
	int ret = 0;
	unsigned sleep = WAIT;
	if (abstime) {
		// If the provided timespec is invalid, still attempt to take the
		// semaphore without blocking, per POSIX spec.
		if (validate_timespec(abstime) == false) {
			sleep = 0;
			ret = EINVAL;
		} else {
			struct timespec cur = { 0 };
			// Get current time
			if (clock_gettime(CLOCK_REALTIME, &cur) != 0) {
				ret = EINVAL;
			} else {
				ret = abs_timespec2ticks(abstime, &cur, &sleep);
			}
			// If abstime was in the past, still attempt to take the semaphore without
			// blocking, per POSIX spec.
			if (ret == ETIMEDOUT) {
				sleep = 0;
			}
		}
	}
	if (sem_get(sem, sleep) != 0) {
		if (ret == 0) {
			errno = ETIMEDOUT;
		} else {
			errno = ret;
		}
		ret = -1;
	} else {
		ret = 0;
	}

	return ret;
}

int sem_trywait(sem_t * sem)
{
	int ret = 0;
	struct timespec xTimeout = { 0 };
	ret = sem_timedwait(sem, &xTimeout);
	if ((ret == -1) && (errno == ETIMEDOUT)) {
		errno = EAGAIN;
	}
	return ret;
}

int sem_wait(sem_t * sem)
{
	return sem_timedwait(sem, 0);
}
