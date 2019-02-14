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
#include <hcos/mut.h>
#include <hcos/sem.h>
#include "hcos_posix.h"

#define MASK_DETACH 0x8000
#define MASK_PRIORITY 0x7FFF
#define SHFT_DETACH 15
#define STATUS_PRIORITY( var ) ( (var)  & (MASK_PRIORITY) )
#define STATUS_JOINABLE( var ) ( ( (var) & (MASK_DETACH) ) == MASK_DETACH )

#define HCOS_LOWEST_PRIORITY  (CFG_TPRI_NUM - 2)

static const pthread_attr_t PTHTREAD_ATTR_DEFAULT = {
	.stack_sz = PTHREAD_STACK_MIN,
	.status =
	    ((unsigned short)HCOS_LOWEST_PRIORITY & MASK_PRIORITY) |
	    (PTHREAD_CREATE_JOINABLE << SHFT_DETACH),
};

/**
 * @brief Terminates the calling thread.
 *
 * For joinable threads, this function waits for pthread_join. Otherwise,
 * it deletes the thread and frees up resources used by the thread.
 *
 * @return This function does not return.
 */
static void exit_thread(void)
{
	pthread_internal_t *thread = (pthread_internal_t *) pthread_self();

	// If this thread is joinable, wait for a call to pthread_join
	if (STATUS_JOINABLE(thread->attr.status)) {
		sem_post(&thread->barrier);
		sem_get(&thread->joined, WAIT);
	} else {
		hcos_posix_free(POSIX_STACK, thread->stack);
		hcos_posix_free(POSIX_THREAD, thread);
	}
}

/**
 * @brief Wrapper function for the user's thread routine.
 *
 * This function is executed as a FreeRTOS task function.
 * @param[in] pxArg A pointer to a pthread_internal_t.
 *
 * @return nothing
 */
static void run_thread(void *pxArg)
{
	pthread_internal_t *thread = (pthread_internal_t *) pxArg;

	// Run the thread routine.
	thread->ret = thread->fun((void *)thread->fun_arg);

	// Exit once finished. This function does not return.
	exit_thread();
}

int pthread_attr_destroy(pthread_attr_t * attr)
{
	int ret = 0;
	if (!attr) {
		ret = EINVAL;
	}
	return ret;
}

int pthread_attr_getdetachstate(const pthread_attr_t * attr, int *detach)
{
	if (STATUS_JOINABLE(attr->status)) {
		*detach = PTHREAD_CREATE_JOINABLE;
	} else {
		*detach = PTHREAD_CREATE_DETACHED;
	}

	return 0;
}

int pthread_attr_getschedparam(const pthread_attr_t * attr,
			       struct sched_param *param)
{
	param->sched_priority = (int)(STATUS_PRIORITY(attr->status));
	return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t * attr, size_t * stacksize)
{
	*stacksize = (size_t) attr->stack_sz;
	return 0;
}

int pthread_attr_init(pthread_attr_t * attr)
{
	int ret = 0;
	if (!attr) {
		ret = EINVAL;
	}
	if (ret == 0) {
		*((pthread_attr_t *) (attr)) = PTHTREAD_ATTR_DEFAULT;
	}
	return ret;
}

int pthread_attr_setdetachstate(pthread_attr_t * attr, int detach)
{
	int ret = 0;
	if ((detach != PTHREAD_CREATE_DETACHED)
	    && (detach != PTHREAD_CREATE_JOINABLE)) {
		ret = EINVAL;
	} else {
		// clear and then set msb bit to detach)
		attr->status &= ~MASK_DETACH;
		attr->status |= ((unsigned short)detach << SHFT_DETACH);
	}
	return ret;
}

int pthread_attr_setschedparam(pthread_attr_t * attr,
			       const struct sched_param *param)
{
	int ret = 0;

	if (!param) {
		ret = EINVAL;
	}
	// Ensure that param.sched_priority is valid.
	if ((ret == 0) &&
	    ((param->sched_priority > sched_get_priority_max(SCHED_OTHER)) ||
	     (param->sched_priority < 0))) {
		ret = ENOTSUP;
	}
	// Set the sched_param.
	if (ret == 0) {
		/* clear and then set  15 LSB to schedule priority) */
		attr->status &= ~MASK_PRIORITY;
		attr->status |= ((unsigned short)param->sched_priority);
	}

	return ret;
}

int pthread_attr_setstacksize(pthread_attr_t * attr, size_t stacksize)
{
	int ret = 0;
	if (stacksize < PTHREAD_STACK_MIN) {
		ret = EINVAL;
	} else {
		attr->stack_sz = (unsigned short)stacksize;
	}

	return ret;
}

int pthread_create(pthread_t * _thread,
		   const pthread_attr_t * attr,
		   void *(*fun) (void *), void *arg)
{
	int ret = 0;
	pthread_internal_t *thread = 0;
	struct sched_param param = {.sched_priority = HCOS_LOWEST_PRIORITY };

	if (hcos_posix_alloc == 0) {
		errno = EAGAIN;
		ret = -1;
	} else {
		// Allocate memory for new thread object.
		thread =
		    (pthread_internal_t *) hcos_posix_alloc(POSIX_THREAD,
							    sizeof
							    (pthread_internal_t));
		memset(thread, 0, sizeof(pthread_internal_t));
	}
	if (!thread) {
		// No memory.
		ret = EAGAIN;
	}

	if (ret == 0) {
		// No attributes given, use default attributes.
		if (!attr) {
			thread->attr = PTHTREAD_ATTR_DEFAULT;
		}
		// Otherwise, use provided attributes.
		else {
			thread->attr = *((pthread_attr_t *) (attr));
		}

		// Get priority from attributes
		param.sched_priority =
		    (int)STATUS_PRIORITY(thread->attr.status);

		// Set argument and start routine.
		thread->fun_arg = arg;
		thread->fun = fun;

		// If this thread is joinable, create the synchronization mechanisms for
		// pthread_join.
		if (STATUS_JOINABLE(thread->attr.status)) {
			mut_init(&thread->mux);
			sem_init(&thread->barrier, 1);
			sem_init(&thread->joined, 1);
		}
	}

	if (ret == 0) {
		thread->stack =
		    hcos_posix_alloc(POSIX_STACK, thread->attr.stack_sz);
		if (thread->stack == 0) {
			hcos_posix_free(POSIX_THREAD, thread);
			ret = EAGAIN;
		}
	}
	if (ret == 0) {
		if (task_init(&thread->task,
			      "pthread",
			      run_thread,
			      param.sched_priority,
			      thread->stack,
			      thread->attr.stack_sz, 10, (void *)thread)) {
			// Task creation failed, no memory.
			hcos_posix_free(POSIX_STACK, thread->stack);
			hcos_posix_free(POSIX_THREAD, thread);
			ret = EAGAIN;
		} else {
			*_thread = (pthread_t) thread;
		}
	}

	return ret;
}

int pthread_getschedparam(pthread_t _thread,
			  int *policy, struct sched_param *param)
{
	int ret = 0;
	pthread_internal_t *thread = (pthread_internal_t *) _thread;
	*policy = SCHED_OTHER;
	param->sched_priority = (int)STATUS_PRIORITY(thread->attr.status);

	return ret;
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
	int ret = 0;
	// Compare the thread IDs.
	if ((t1 != 0) && (t2 != 0)) {
		ret = (t1 == t2);
	}

	return ret;
}

void pthread_exit(void *value_ptr)
{
	pthread_internal_t *thread = (pthread_internal_t *) pthread_self();
	// Set the return value
	thread->ret = value_ptr;
	exit_thread();
}

int pthread_join(pthread_t pthread, void **retval)
{
	int ret = 0;
	pthread_internal_t *thread = (pthread_internal_t *) pthread;

	// Make sure pthread is joinable. Otherwise, this function would block
	// forever waiting for an unjoinable thread.
	if (!STATUS_JOINABLE(thread->attr.status)) {
		ret = EDEADLK;
	}
	// Only one thread may attempt to join another. Lock the join mutex
	// to prevent other threads from calling pthread_join on the same thread.
	if (ret == 0) {
		if (mut_lock(&thread->mux, WAIT_NO) != 0) {
			// Another thread has already joined the requested thread, which would
			// cause this thread to wait forever.
			ret = EDEADLK;
		}
	}
	// Attempting to join the calling thread would cause a deadlock.
	if (ret == 0) {
		if (pthread_equal(pthread_self(), pthread) != 0) {
			ret = EDEADLK;
		}
	}

	if (ret == 0) {
		// Wait for the joining thread to finish. Because this call waits forever,
		// it should never fail.
		sem_get(&thread->barrier, WAIT);
		sem_post(&thread->joined);
		mut_unlock(&thread->mux);
		if (retval) {
			*retval = thread->ret;
		}
		hcos_posix_free(POSIX_STACK, thread->stack);
		hcos_posix_free(POSIX_THREAD, thread);
	}

	return ret;
}

pthread_t pthread_self(void)
{
	return (pthread_t) _task_cur->priv;
}

int pthread_setschedparam(pthread_t _thread,
			  int policy, const struct sched_param *param)
{
	int ret = 0;
	pthread_internal_t *thread = (pthread_internal_t *) _thread;
	// Copy the given sched_param.
	ret =
	    pthread_attr_setschedparam((pthread_attr_t *) & thread->attr,
				       param);
	if (ret == 0) {
		// Change the priority of the FreeRTOS task.
		task_pri(&thread->task, param->sched_priority);
	}

	return ret;
}
