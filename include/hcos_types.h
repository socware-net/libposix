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
#ifndef _HYEPRC_TYPES_DEF
#define _HYEPRC_TYPES_DEF

#include <hcos/mut.h>
#include <hcos/mq.h>
#include <hcos/sem.h>
#include <hcos/task.h>
#include "hcos_posix.h"

typedef struct pthread_attr {
	unsigned short stack_sz;	///< Stack size.
	unsigned short status;	///< Schedule priority 15 bits (LSB) Detach state: 1 bits (MSB)
} pthread_attr_t;

typedef struct pthread_barrier {
	unsigned cur_thread;	///< Current number of threads that have entered barrier.
	unsigned threshold;	///< The count argument of pthread_barrier_init.
	sem_t sem;
	mut_t mux;
} pthread_barrier_t;

typedef void *pthread_barrierattr_t;

typedef void *pthread_t;

typedef void *pthread_condattr_t;

typedef struct pthread_mutexattr {
	unsigned type;
} pthread_mutexattr_t;

typedef struct pthread_mutex {
	int inited;		///< 1 if this is initialized, 0 otherwise.
	mut_t mux;
	pthread_mutexattr_t attr;
} pthread_mutex_t;

typedef struct pthread_cond {
	int inited;		///< 1 if this is initialized, 0 otherwise.
	mut_t mux;		///< Prevents concurrent accesses to waiting
	sem_t cond;		///< Threads block on this semaphore in pthread_cond_wait.
	int waiting;		///< The number of threads currently waiting on this condition variable.
} pthread_cond_t;

typedef struct mqd_internal {
	mq_t q;
	void *buf;
	char name[POSIX_MQUEUE_NAME_SZ];
	long flags;
	lle_t ll;
	unsigned short open_count;
	unsigned short pending_unlink;
} mqd_internal_t;

typedef struct pthread_internal {
	pthread_attr_t attr;
	void *(*fun) (void *);	///< Application thread function.
	void *fun_arg;		///< Arguments for application thread function.
	task_t task;
	unsigned *stack;
	sem_t barrier;		///< Synchronizes the two callers of pthread_join.
	sem_t joined;		///< Synchronizes the two callers of pthread_join.
	mut_t mux;		///< Ensures that only one other thread may join this thread.
	void *ret;		///< Return value of fun.
} pthread_internal_t;
#endif
