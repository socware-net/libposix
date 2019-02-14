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
 * @file utils.h
 * @brief Utility functions used by FreeRTOS+POSIX.
 */

#ifndef _HCOS_POSIX_UTILS_
#define _HCOS_POSIX_UTILS_

/* C standard library includes. */
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/**
 * @brief Calculates the number of ticks between now and a given timespec.
 *
 * @param[in] pxAbsoluteTime A time in the future, specified as seconds and
 * nanoseconds since CLOCK_REALTIME's 0.
 * @param[in] pxCurrentTime current time, specified as seconds and
 * nanoseconds.
 * @param[out] pxResult Where the result of the conversion is stored. The result
 * is rounded up for fractional ticks.
 *
 * @return 0 on success. Otherwise, ETIMEDOUT if pxAbsoluteTime is in the past,
 * or EINVAL for invalid parameters.
 */
int abs_timespec2ticks(const struct timespec *const
		       pxAbsoluteTime,
		       const struct timespec *const
		       pxCurrentTime, unsigned *const pxResult);

/**
 * @brief Converts a struct timespec to FreeRTOS ticks.
 *
 * @param[in] pxTimespec The timespec to convert.
 * @param[out] Where the result of the conversion is stored. The result is rounded
 * up for fractional ticks.
 *
 * @return 0 on success. Otherwise, EINVAL for invalid parameters.
 */
int timespec2ticks(const struct timespec *const pxTimespec,
		   unsigned *const pxResult);

/**
 * @brief Converts an integer value to a timespec.
 *
 * @param[in] llSource The value to convert.
 * @param[out] pxDestination Where to store the converted value.
 *
 * @return No return value.
 */
void nano2timespec(int64_t llSource, struct timespec *const pxDestination);

/**
* @brief Checks that a timespec conforms to POSIX.
*
* A valid timespec must have 0 <= tv_nsec < 1000000000.
*
* @param[in] pxTimespec The timespec to validate.
*
* @return true if the pxTimespec is valid, false otherwise.
*/
int validate_timespec(const struct timespec *const pxTimespec);

#endif /* ifndef _HCOS_POSIX_UTILS_ */
