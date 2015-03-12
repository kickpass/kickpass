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

#include "kickpass.h"

#ifndef KP_SAFE_TEMPLATE
#define KP_SAFE_TEMPLATE "\n"                                                  \
                         "url: \n"                                             \
                         "username: \n"                                        \
                         "comment: \n"
#endif

enum kp_safe_plaintext_type {
	KP_SAFE_PLAINTEXT_FILE,
	KP_SAFE_PLAINTEXT_MEMORY,
};

struct kp_safe_plaintext {
	enum kp_safe_plaintext_type type;
	union {
		/* plaintext file */
		struct {
			char path[PATH_MAX];
			int fd;
		};
		struct {
			size_t size;
			char *data;
		};
	};
};

struct kp_safe_ciphertext {
	int fd;
};

struct kp_safe {
	bool open;
	char path[PATH_MAX];
	struct kp_safe_plaintext plain;
	struct kp_safe_ciphertext cipher;
};

kp_error_t kp_safe_open(struct kp_ctx *ctx, const char *path, enum kp_safe_plaintext_type type, struct kp_safe *safe);
kp_error_t kp_safe_create(struct kp_ctx *ctx, const char *path, enum kp_safe_plaintext_type type, struct kp_safe *safe);
kp_error_t kp_safe_close(struct kp_ctx *ctx, struct kp_safe *safe);

#endif /* KP_SAFE_H */
