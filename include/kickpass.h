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

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>

#include <imsg.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

#include "error.h"
#include "kickpass_config.h"

#define KP_PASSWORD_MAX_LEN 4096
#define KP_METADATA_MAX_LEN 4096

struct kp_agent {
	int sock;
	struct imsgbuf ibuf;
	struct sockaddr_un sunaddr;
	bool connected;
};

struct kp_ctx {
	char ws_path[PATH_MAX];
	struct kp_agent agent;
	kp_error_t (*password_prompt)(struct kp_ctx *, bool, char *, const char *, va_list ap);
	char * const password;
	struct {
		long long unsigned opslimit;
		size_t memlimit;
	} cfg;
};

kp_error_t kp_init(struct kp_ctx *);
kp_error_t kp_fini(struct kp_ctx *);
kp_error_t kp_init_workspace(struct kp_ctx *, const char *);
const char *kp_version_string(void);
int kp_version_major(void);
kp_error_t kp_password_prompt(struct kp_ctx *, bool, char *, const char *, ...) __attribute__((format(printf, 4, 5)));

#endif /* KP_KICKPASS_H */
