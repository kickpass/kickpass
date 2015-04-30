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

#ifndef KP_KICKPASS_H
#define KP_KICKPASS_H

#include <stdbool.h>
#include <err.h>
#include <limits.h>

#include "error.h"
#include "kickpass_config.h"

#define KP_USAGE_OPT_LEN "20"
#define KP_USAGE_CMD_LEN "20"

struct kp_ctx {
	char                ws_path[PATH_MAX];
	char               *password;
	unsigned long long  opslimit;
	size_t              memlimit;
};

kp_error_t kp_init(struct kp_ctx *);
kp_error_t kp_fini(struct kp_ctx *);
kp_error_t kp_load_passwd(struct kp_ctx *, bool);

#endif /* KP_KICKPASS_H */
