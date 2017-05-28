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

#ifndef KP_SAFE_H
#define KP_SAFE_H

#include <stdbool.h>

#include "kickpass.h"

#ifndef KP_METADATA_TEMPLATE
#define KP_METADATA_TEMPLATE "url: \n"                                         \
                             "username: \n"                                    \
                             "comment: \n"
#endif

#define KP_PLAIN_MAX_SIZE (KP_PASSWORD_MAX_LEN + KP_METADATA_MAX_LEN)

#define KP_CREATE 1
#define KP_FORCE  2

/*
 * A safe is either open or close.
 * Plain data are stored in memory.
 * Cipher data are stored in file.
 */
struct kp_safe {
	bool open;           /* whether the safe is open or not */
	char name[PATH_MAX]; /* name of the safe */
	char * const password;      /* plain text password (null terminated) */
	char * const metadata;      /* plain text metadata (null terminated) */
};

kp_error_t kp_safe_init(struct kp_ctx *, struct kp_safe *, const char *);
kp_error_t kp_safe_open(struct kp_ctx *, struct kp_safe *, int);
kp_error_t kp_safe_save(struct kp_ctx *, struct kp_safe *);
kp_error_t kp_safe_close(struct kp_ctx *, struct kp_safe *);
kp_error_t kp_safe_delete(struct kp_ctx *, struct kp_safe *);
kp_error_t kp_safe_get_path(struct kp_ctx *, struct kp_safe *, char *, size_t);
kp_error_t kp_safe_rename(struct kp_ctx *, struct kp_safe *, const char *);
kp_error_t kp_safe_store(struct kp_ctx *, struct kp_safe *, int);


#endif /* KP_SAFE_H */
