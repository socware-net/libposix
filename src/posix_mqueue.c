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
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <semaphore.h>
#include "utils.h"

static ll_t allq;

static mut_t allq_mux;

static int inited;
/**
 * @brief Convert an absolute timespec into a tick timeout, taking into account
 * queue flags.
 *
 * @param[in] flags Message queue flags to consider.
 * @param[in] timeout The absolute timespec to convert.
 * @param[out] ptimeout Output parameter of the timeout in ticks.
 *
 * @return 0 if successful; EINVAL if timeout is invalid, or ETIMEDOUT
 * if timeout is in the past.
 */
static int get_tick_timeout(long flags,
			    const struct timespec *const timeout,
			    unsigned *ptimeout)
{
	int ret = 0;

	// Check for nonblocking queue.
	if (flags & O_NONBLOCK) {
		// No additional checks are done for nonblocking queues. Timeout is 0.
		*ptimeout = 0;
	} else {
		// No absolute timeout given. Block forever.
		if (!timeout) {
			*ptimeout = WAIT;
		} else {
			struct timespec cur = { 0 };

			// Check that the given timespec is valid.
			if (validate_timespec(timeout) == false) {
				ret = EINVAL;
			}
			// Get current time
			if ((ret == 0)
			    && (clock_gettime(CLOCK_REALTIME, &cur) != 0)) {
				ret = EINVAL;
			}
			// Convert absolute timespec to ticks.
			if ((ret == 0)
			    && (abs_timespec2ticks(timeout, &cur, ptimeout) !=
				0)) {
				ret = ETIMEDOUT;
			}
		}
	}
	return ret;
}

/**
 * @brief Checks that name is a valid name for a message queue.
 *
 * Also outputs the length of name.
 * @param[in] name The name to check.
 * @param[out] len Output parameter for name length.
 *
 * @return 1 if the name is valid; 0 otherwise.
 */
static int validate_name(const char *name, size_t * _len)
{
	int ret = 1;
	size_t len = 0;

	// All message queue names must start with '/'
	if (name[0] != '/') {
		ret = 0;
	} else {
		// Get the length of name, excluding the first '/' and null-terminator.
		len = strnlen(name, POSIX_NAME_MAX + 2);
		if (len == POSIX_NAME_MAX + 2) {
			// Name too long.
			ret = 0;
		} else {
			// Name length passes, set output parameter.
			*_len = len;
		}
	}

	return ret;
}

static void mq_check_init()
{
	if (inited)
		return;
	ll_init(&allq);
	mut_init(&allq_mux);
	inited = 1;
}

static mqd_internal_t *mq_extract_locked(mqd_t _mq)
{
	lle_t *p;
	ll_for_each(&allq, p) {
		mqd_internal_t *mq = lle_get(p, mqd_internal_t, ll);
		if (mq == _mq) {
			return mq;
		}
	}
	return 0;
}

static mqd_internal_t *mq_find_locked(const char *name)
{
	lle_t *p;
	ll_for_each(&allq, p) {
		mqd_internal_t *mq = lle_get(p, mqd_internal_t, ll);
		if (strcmp(name, mq->name) == 0) {
			return mq;
		}
	}
	return 0;
}

static void mq_free(mqd_internal_t * mq)
{
	hcos_posix_free(POSIX_MQUEUE_BUF, mq->buf);
	hcos_posix_free(POSIX_MQUEUE, mq);
}

int mq_close(mqd_t _mq)
{
	int ret = 0;
	int removed = 0;
	mqd_internal_t *mq = 0;
	mq_check_init();
	mut_lock(&allq_mux, WAIT);
	if ((mq = mq_extract_locked(_mq)) != 0) {
		if (mq->open_count > 0) {
			mq->open_count--;
		}
		if (mq->open_count == 0) {
			if (mq->pending_unlink) {
				lle_del(&mq->ll);
				removed = 1;
			} else {
				mq->pending_unlink = 1;
			}
		}
	} else {
		errno = EBADF;
		ret = -1;
	}
	mut_unlock(&allq_mux);
	if (removed) {
		mq_free(mq);
	}
	return ret;
}

int mq_getattr(mqd_t _mq, struct mq_attr *mqstat)
{
	int ret = 0;
	mqd_internal_t *mq = 0;
	mq_check_init();
	mut_lock(&allq_mux, WAIT);
	if ((mq = mq_extract_locked(_mq)) != 0) {
		mqstat->mq_flags = mq->flags;
		mqstat->mq_maxmsg = mq->q.sz;
		mqstat->mq_msgsize = mq->q.isz;
		mqstat->mq_curmsgs = mq->q.n;
	} else {
		errno = EBADF;
		ret = -1;
	}
	mut_unlock(&allq_mux);
	return ret;
}

mqd_t mq_open(const char *name, int oflag, mode_t mode, struct mq_attr * attr)
{
	mqd_internal_t *mq = 0;
	size_t len = 0;

	struct mq_attr def_attr = {
		.mq_flags = 0,
		.mq_maxmsg = POSIX_MQUEUE_DEF_MSG_MAX,
		.mq_msgsize = POSIX_MQUEUE_DEF_MSG_SZ,
		.mq_curmsgs = 0
	};
	mq_check_init();

	// Check queue name.
	if (!validate_name(name, &len)) {
		errno = EINVAL;
		mq = (mqd_t) - 1;
	}
	// Check attributes, if given.
	if (mq == 0) {
		if ((attr != 0)
		    && ((attr->mq_maxmsg < 0) || (attr->mq_msgsize < 0))) {
			// Invalid mq_attr.mq_maxmsg or mq_attr.mq_msgsize.
			errno = EINVAL;
			mq = (mqd_t) - 1;
		}
	}

	if (mq == 0) {

		mut_lock(&allq_mux, WAIT);
		mq = mq_find_locked(name);
		// Search the queue list to check if the queue exists.
		if (mq != 0) {
			// If the mq exists, check that this function wasn't called with
			// O_CREAT and O_EXCL.
			if ((oflag & O_EXCL) && (oflag & O_CREAT)) {
				errno = EEXIST;
				mq = (mqd_t) - 1;
			} else {
				// Increase count of open file descriptors for queue.
				mq->open_count++;
			}
		} else {
			// Only create the new queue if O_CREAT was specified.
			if (oflag & O_CREAT) {
				unsigned buf_sz = 0;
				unsigned nwords = 0;
				// Copy attributes if provided.
				if (attr != 0) {
					def_attr = *attr;
				}
				// Copy oflags.
				def_attr.mq_flags = (long)oflag;

				mq = hcos_posix_alloc(POSIX_MQUEUE,
						      sizeof(mqd_internal_t));
				if (!mq) {
					errno = ENOENT;
					mq = (mqd_t) - 1;
					goto out;
				}
				memset(mq, 0, sizeof(mqd_internal_t));

				nwords = (def_attr.mq_msgsize + 3) >> 2;
				buf_sz = def_attr.mq_maxmsg * nwords;
				mq->buf =
				    hcos_posix_alloc(POSIX_MQUEUE_BUF, buf_sz);
				if (!mq->buf) {
					errno = ENOENT;
					mq = (mqd_t) - 1;
					goto out;
				}
				memset(mq->buf, 0, buf_sz);
				strcpy(mq->name, name);
				mq->flags = def_attr.mq_flags;
				lle_init(&mq->ll);
				mq->open_count = 0;
				mq_init(&mq->q, nwords, mq->buf, buf_sz);
			} else {
				errno = ENOENT;
				mq = (mqd_t) - 1;
			}
		}
		mut_unlock(&allq_mux);

	}
out:
	return mq;
}

ssize_t mq_receive(mqd_t mqdes,
		   char *msg_ptr, size_t msg_len, unsigned int *msg_prio)
{
	return mq_timedreceive(mqdes, msg_ptr, msg_len, msg_prio, 0);
}

int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned msg_prio)
{
	return mq_timedsend(mqdes, msg_ptr, msg_len, msg_prio, 0);
}

ssize_t mq_timedreceive(mqd_t _mq,
			char *msg_ptr,
			size_t msg_len,
			unsigned *msg_prio, const struct timespec * abstime)
{
	ssize_t ret = 0;
	mqd_internal_t *mq;
	int timeout_ret = 0;
	unsigned timeout = 0;

	mut_lock(&allq_mux, WAIT);
	if (!(mq = mq_extract_locked(_mq))) {
		// Queue not found; bad descriptor.
		errno = EBADF;
		ret = -1;
	}
	// msg_ptr must be word aligned
	if ((unsigned)msg_ptr & 0x3) {
		errno = EINVAL;
		ret = -1;
	}
	// Verify that msg_len is large enough.
	if (ret == 0) {
		if (msg_len < (size_t) mq->q.isz) {
			// msg_len too small.
			errno = EMSGSIZE;
			ret = -1;
		}
	}

	if (ret == 0) {
		// Convert abstime to a tick timeout.
		timeout_ret = get_tick_timeout(mq->flags, abstime, &timeout);
		if (timeout_ret != 0) {
			errno = timeout_ret;
			ret = -1;
		}
	}
	mut_unlock(&allq_mux);

	if (ret == 0) {
		if (mq_get(&mq->q, (unsigned *)msg_ptr, timeout) != 0) {
			// If queue receive fails, set the appropriate errno.
			if (mq->flags & O_NONBLOCK) {
				// Set errno to EAGAIN for nonblocking mq.
				errno = EAGAIN;
			} else {
				// Otherwise, set errno to ETIMEDOUT.
				errno = ETIMEDOUT;
			}
			ret = -1;
		}
	}

	if (ret == 0) {
		// Get the length of data for return value.
		ret = (ssize_t) mq->q.isz;
	}

	return ret;
}

int mq_timedsend(mqd_t _mq,
		 const char *msg_ptr,
		 size_t msg_len,
		 unsigned int msg_prio, const struct timespec *abstime)
{
	mqd_internal_t *mq = 0;
	int ret = 0, timeout_ret = 0;
	unsigned timeout = 0;

	mut_lock(&allq_mux, WAIT);
	// Find the mq referenced by mqdes.
	if (!(mq = mq_extract_locked(_mq))) {
		// Queue not found; bad descriptor.
		errno = EBADF;
		ret = -1;
	}
	// msg_ptr must be word aligned
	if ((unsigned)msg_ptr & 0x3) {
		errno = EINVAL;
		ret = -1;
	}
	// Verify that mq_msgsize is large enough.
	if (ret == 0) {
		if (msg_len > (size_t) mq->q.isz) {
			// msg_len too large.
			errno = EMSGSIZE;
			ret = -1;
		}
	}

	if (ret == 0) {
		// Convert abstime to a tick timeout.
		timeout_ret = get_tick_timeout(mq->flags, abstime, &timeout);
		if (timeout_ret != 0) {
			errno = timeout_ret;
			ret = -1;
		}
	}
	mut_unlock(&allq_mux);

	if (ret == 0) {
		if (mq_put(&mq->q, (unsigned *)msg_ptr, timeout) != 0) {
			// If queue send fails, set the appropriate errno.
			if (mq->flags & O_NONBLOCK) {
				// Set errno to EAGAIN for nonblocking mq.
				errno = EAGAIN;
			} else {
				// Otherwise, set errno to ETIMEDOUT.
				errno = ETIMEDOUT;
			}
			ret = -1;
		}
	}

	return ret;
}

int mq_unlink(const char *name)
{
	mqd_internal_t *mq = 0;
	int ret = 0;
	int removed = 0;
	size_t size = 0;

	mq_check_init();
	if (!validate_name(name, &size)) {
		errno = ENAMETOOLONG;
		ret = -1;
	}

	if (ret == 0) {
		mut_lock(&allq_mux, WAIT);
		mq = mq_find_locked(name);
		if (mq != 0) {
			// If the queue exists and there are no open descriptors to it,
			// remove it from the list.
			if (mq->open_count == 0) {
				lle_del(&mq->ll);

				// Set the flag to delete the queue. Deleting the queue is deferred
				// until xQueueListMutex is released.
				removed = 1;
			} else {
				// If the queue has open descriptors, set the pending unlink flag
				// so that mq_close will free its resources.
				mq->pending_unlink = 1;
			}
		} else {
			errno = ENOENT;
			ret = -1;
		}
		mut_unlock(&allq_mux);
	}
	// Delete all resources used by the queue if needed. */
	if (removed) {
		mq_free(mq);
	}

	return ret;
}
