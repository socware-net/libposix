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
#include <limits.h>

#include <errno.h>
#include <pthread.h>
#include <utils.h>

int pthread_cond_broadcast(pthread_cond_t * cond)
{
	if (cond->inited == 0)
		return EINVAL;

	// Lock mux to protect access to waiting.
	// This call will never fail because it blocks forever.
	mut_lock(&cond->mux, WAIT);

	// Unblock all threads waiting on this condition variable.
	sem_post_n(&cond->cond, cond->waiting);
	// All threads were unblocked, set waiting threads to 0.
	cond->waiting = 0;

	// Release mux.
	mut_unlock(&cond->mux);

	return 0;
}

int pthread_cond_destroy(pthread_cond_t * cond)
{
	return 0;
}

int pthread_cond_init(pthread_cond_t * cond, const pthread_condattr_t * attr)
{
	int ret = 0;
	if (cond == 0) {
		ret = ENOMEM;
	}
	if (ret == 0) {
		cond->inited = 1;
		mut_init(&cond->mux);
		sem_init(&cond->cond, 0);
		cond->waiting = 0;
	}
	return ret;
}

int pthread_cond_signal(pthread_cond_t * cond)
{
	if (cond->inited == 0)
		return EINVAL;
	if (cond->waiting > 0) {
		// Lock mux to protect access to waiting.
		mut_lock(&cond->mux, WAIT);
		if (cond->waiting > 0) {
			sem_post(&cond->cond);
			cond->waiting--;
		}
		mut_unlock(&cond->mux);
	}
	return 0;
}

int pthread_cond_timedwait(pthread_cond_t * cond,
			   pthread_mutex_t * mux,
			   const struct timespec *abstime)
{
	int ret = 0;
	unsigned sleep_ticks = WAIT;

	if (cond->inited == 0)
		return EINVAL;

	if (abstime != 0) {
		struct timespec cur = { 0 };
		if (clock_gettime(CLOCK_REALTIME, &cur) != 0) {
			ret = EINVAL;
		} else {
			ret = abs_timespec2ticks(abstime, &cur, &sleep_ticks);
		}
	}

	if (ret == 0) {
		mut_lock(&cond->mux, WAIT);
		cond->waiting++;
		mut_unlock(&cond->mux);
		ret = pthread_mutex_unlock(mux);
	}
	// Wait on the condition variable.
	if (ret == 0) {
		if (sem_get(&cond->cond, sleep_ticks) == 0) {
			// When successful, relock mux.
			ret = pthread_mutex_lock(mux);
		} else {
			// Timeout. Relock mux and decrement number of waiting threads.
			ret = ETIMEDOUT;
			pthread_mutex_lock(mux);
			mut_lock(&cond->mux, WAIT);
			cond->waiting--;
			mut_unlock(&cond->mux);
		}
	}

	return ret;
}

int pthread_cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex)
{
	return pthread_cond_timedwait(cond, mutex, 0);
}
