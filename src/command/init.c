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
#include "prompt.h"
#include "config.h"

static kp_error_t init(struct kp_ctx *ctx, int argc, char **argv);

struct kp_cmd kp_cmd_init = {
	.main  = init,
	.usage = NULL,
	.opts  = "init",
	.desc  = "Initialize a new password safe directory. "
	         "Default to ~/" KP_PATH,
	.lock  = false,
};

kp_error_t
init(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret;

	kp_prompt_password("master", true, (char *)ctx->password);

	if ((ret = kp_init_workspace(ctx)) != KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_cfg_create(ctx)) != KP_SUCCESS) {
		return ret;
	}

	return ret;
}
