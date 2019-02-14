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
#include <pthread.h>

int pthread_barrier_destroy(pthread_barrier_t * b)
{
	return 0;
}

int pthread_barrier_init(pthread_barrier_t * b,
			 const pthread_barrierattr_t * attr, unsigned count)
{
	int ret = 0;
	if (count == 0) {
		ret = EINVAL;
	}
	if (ret == 0) {
		sem_init(&b->sem, 0);
		mut_init(&b->mux);
		b->cur_thread = 0;
		b->threshold = count;
	}
	return ret;
}

int pthread_barrier_wait(pthread_barrier_t * b)
{
	int cur;
	mut_lock(&b->mux, WAIT);
	if (b->cur_thread >= b->threshold) {
		mut_unlock(&b->mux);
		return EINVAL;
	}
	cur = ++b->cur_thread;
	mut_unlock(&b->mux);
	if (b->cur_thread >= b->threshold) {
		sem_post_n(&b->sem, b->threshold);
	}
	sem_get(&b->sem, WAIT);
	return (cur == 1) ? PTHREAD_BARRIER_SERIAL_THREAD : 0;
}
