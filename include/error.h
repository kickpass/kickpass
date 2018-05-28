/*
 * Copyright (c) 2015 Paul Fariello <paul@fariello.eu>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef KP_ERROR_H
#define KP_ERROR_H

#include <errno.h>

#define KP_SUCCESS          0
#define KP_NYI              1
#define KP_EINPUT           2
#define KP_EINTERNAL        3
#define KP_INVALID_STORAGE  4
#define KP_ERRNO            5
#define KP_NO_HOME          6
#define KP_EDECRYPT         7
#define KP_EENCRYPT         8
#define KP_INVALID_MSG      9
#define KP_EXIT             10
#define KP_NOPROMPT         11

typedef int kp_error_t;

const char *kp_strerror(kp_error_t errnum);

#endif /* KP_ERROR_H */
