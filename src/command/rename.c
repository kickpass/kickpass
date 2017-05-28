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

#include <sys/stat.h>

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kickpass.h"

#include "command.h"
#include "rename.h"
#include "prompt.h"
#include "safe.h"
#include "log.h"

static kp_error_t do_rename(struct kp_ctx *ctx, int argc, char **argv);

struct kp_cmd kp_cmd_rename = {
	.main  = do_rename,
	.usage = NULL,
	.opts  = "rename <old_safe> <new_safe>",
	.desc  = "Rename a password safe",
};

kp_error_t
do_rename(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret;
	struct kp_safe safe;

	if (argc - optind != 2) {
		ret = KP_EINPUT;
		kp_warn(ret, "missing safe name");
		return ret;
	}

	if ((ret = kp_safe_init(ctx, &safe, argv[optind])) != KP_SUCCESS) {
		kp_warn(ret, "cannot init %s", argv[optind]);
		return ret;
	}

	if ((ret = kp_safe_open(ctx, &safe, KP_FORCE)) != KP_SUCCESS) {
		kp_warn(ret, "cannot open %s", argv[optind]);
		goto fail;
	}

	if ((ret = kp_safe_rename(ctx, &safe, argv[optind+1])) != KP_SUCCESS) {
		kp_warn(ret, "cannot rename %s into %s", argv[optind],
		        argv[optind+1]);
		return ret;
	}

	if ((ret = kp_safe_close(ctx, &safe)) != KP_SUCCESS) {
		kp_warn(ret, "cannot cleanly close safe"
		             "clear text password might have leaked");
		return ret;
	}

	return KP_SUCCESS;

fail:
	if ((ret = kp_safe_close(ctx, &safe)) != KP_SUCCESS) {
		kp_warn(ret, "cannot cleanly close safe"
		             "clear text password might have leaked");
		return ret;
	}

	return ret;
}
