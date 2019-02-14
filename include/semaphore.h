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

/**
 * @file semaphore.h
 * @brief Semaphores.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/semaphore.h.html
 */

#ifndef _HCOS_POSIX_SEMAPHORE_H_
#define _HCOS_POSIX_SEMAPHORE_H_

#include <time.h>
#include <hcos/sem.h>

/**
 * @brief Destroy an unnamed semaphore.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_destroy.html
 */
int sem_destroy(sem_t * sem);

/**
 * @brief Get the value of a semaphore.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_getvalue.html
 */
static inline int sem_getvalue(sem_t * sem, int *sval)
{
	*sval = sem->val;
	return 0;
}

/**
 * @brief Initialize an unnamed semaphore.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_init.html
 *
 * @note pshared is ignored. Semaphores will always be considered "shared".
 */
int sem_init_posix(sem_t * sem, int pshared, unsigned value);

/**
 * @brief Unlock a semaphore.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_post.html
 */
int sem_post(sem_t * sem);

/**
 * @brief Lock a semaphore with timeout.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_timedwait.html
 */
int sem_timedwait(sem_t * sem, const struct timespec *abstime);

/**
 * @brief Lock a semaphore if available.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_trywait.html
 */
int sem_trywait(sem_t * sem);

/**
 * @brief Lock a semaphore.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_wait.html
 *
 * @note Deadlock detection is not implemented.
 */
int sem_wait(sem_t * sem);

#endif /* ifndef _HCOS_POSIX_SEMAPHORE_H_ */
