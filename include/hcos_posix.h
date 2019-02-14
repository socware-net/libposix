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
#ifndef _HCOS_POSIX
#define _HCOS_POSIX

typedef enum {
	POSIX_TIMER,
	POSIX_THREAD,
	POSIX_STACK,
	POSIX_MQUEUE,
	POSIX_MQUEUE_BUF,
} posix_obj_t;

typedef void *(*hcos_posix_alloc_t) (posix_obj_t type, unsigned sz);

typedef void (*hcos_posix_free_t) (posix_obj_t type, void *p);

extern hcos_posix_alloc_t hcos_posix_alloc;

extern hcos_posix_free_t hcos_posix_free;

static inline void hcos_posix_init(hcos_posix_alloc_t alloc,
				   hcos_posix_free_t free)
{
	hcos_posix_alloc = alloc;
	hcos_posix_free = free;
}

#ifndef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN  (1024)
#endif

#ifndef POSIX_MQUEUE_NAME_SZ
#define POSIX_MQUEUE_NAME_SZ   16
#endif

#ifndef POSIX_MQUEUE_DEF_MSG_MAX
#define POSIX_MQUEUE_DEF_MSG_MAX  128
#endif

#ifndef POSIX_MQUEUE_DEF_MSG_SZ
#define POSIX_MQUEUE_DEF_MSG_SZ  4
#endif

#ifndef POSIX_NAME_MAX
#define POSIX_NAME_MAX 64
#endif

#endif
