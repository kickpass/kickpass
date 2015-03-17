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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "kickpass.h"

#include "command.h"
#include "init.h"
#include "storage.h"

static kp_error_t init(struct kp_ctx *ctx, int argc, char **argv);
static kp_error_t usage(void);

struct kp_cmd kp_cmd_init = {
	.main  = init,
	.usage = usage,
};

kp_error_t
init(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret = KP_SUCCESS;
	struct stat stats;

	if (stat(ctx->ws_path, &stats) == 0) {
		warnx("workspace already exists");
		ret = KP_EINPUT;
		goto out;
	} else if (errno & ENOENT) {
		printf("creating workspace %s", ctx->ws_path);
		mkdir(ctx->ws_path, 0700);
	} else {
		warn("invalid workspace %s", ctx->ws_path);
		ret = errno;
		goto out;
	}

	/* TODO create gpg config (force-mdc) */
	/* TODO git init */

out:
	return ret;
}

kp_error_t
usage(void)
{
	printf("    %-10s%s\n", "init", "Initialize a new password safe directory");
	return KP_SUCCESS;
}
