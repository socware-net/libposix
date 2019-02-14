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
#include <pthread.h>
#include <utils.h>
#include "hcos_types.h"
#include <hcos/mut.h>
#include <hcos/task.h>

int pthread_mutex_destroy(pthread_mutex_t * mux)
{
	return 0;
}

int pthread_mutex_init(pthread_mutex_t * mux, const pthread_mutexattr_t * attr)
{
	int ret = 0;
	if (mux == NULL) {
		ret = ENOMEM;
	}
	if (ret == 0) {
		if (attr == NULL) {
			mux->attr.type = PTHREAD_MUTEX_DEFAULT;
		} else {
			mux->attr.type = attr->type;
		}
		mut_init(&mux->mux);
		mux->inited = 1;
	}

	return ret;
}

int pthread_mutex_lock(pthread_mutex_t * mux)
{
	return pthread_mutex_timedlock(mux, NULL);
}

int pthread_mutex_timedlock(pthread_mutex_t * mux,
			    const struct timespec *abstime)
{
	unsigned sleep_ticks;
	int ret = 0;
	if (mux->inited == 0)
		return EINVAL;

	if (abstime) {
		struct timespec cur = { 0 };

		if (clock_gettime(CLOCK_REALTIME, &cur) != 0) {
			ret = EINVAL;
		} else {
			ret = abs_timespec2ticks(abstime, &cur, &sleep_ticks);
		}

		if (ret == ETIMEDOUT) {
			sleep_ticks = 0;
			ret = 0;
		}
	}
	// Only PTHREAD_MUTEX_ERRORCHECK type detects deadlock.
	// Check if locking a currently owned mutex.
	if ((ret == 0) && (mux->attr.type == PTHREAD_MUTEX_ERRORCHECK) &&
	    (mux->mux.own == _task_cur)) {
		ret = EDEADLK;
	}

	if (ret == 0) {
		mut_lock(&mux->mux, sleep_ticks);
	}

	return ret;
}

int pthread_mutex_trylock(pthread_mutex_t * mux)
{
	int ret = 0;
	struct timespec timeout = {
		.tv_sec = 0,
		.tv_nsec = 0
	};
	ret = pthread_mutex_timedlock(mux, &timeout);
	if (ret == ETIMEDOUT) {
		ret = EBUSY;
	}
	return ret;
}

int pthread_mutex_unlock(pthread_mutex_t * mux)
{
	int ret = 0;

	if (mux->inited == 0)
		return EINVAL;
	if (((mux->attr.type == PTHREAD_MUTEX_ERRORCHECK) ||
	     (mux->attr.type == PTHREAD_MUTEX_RECURSIVE)) &&
	    (mux->mux.own != _task_cur)) {
		ret = EPERM;
	}

	if (ret == 0) {
		mut_unlock(&mux->mux);
	}

	return ret;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t * attr)
{
	return 0;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t * attr, int *type)
{
	*type = attr->type;
	return 0;
}

int pthread_mutexattr_init(pthread_mutexattr_t * attr)
{
	int ret = 0;
	if (!attr) {
		ret = EINVAL;
	}
	if (ret == 0) {
		attr->type = PTHREAD_MUTEX_DEFAULT;
	}
	return ret;
}

int pthread_mutexattr_settype(pthread_mutexattr_t * attr, int type)
{
	int ret = 0;
	switch (type) {
	case PTHREAD_MUTEX_NORMAL:
	case PTHREAD_MUTEX_RECURSIVE:
	case PTHREAD_MUTEX_ERRORCHECK:
		attr->type = type;
		break;
	default:
		ret = EINVAL;
		break;
	}
	return ret;
}
