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
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "kickpass.h"

#include "delete.h"
#include "prompt.h"
#include "safe.h"

static kp_error_t delete(struct kp_ctx *, int, char **);

struct kp_cmd kp_cmd_delete = {
	.main  = delete,
	.usage = NULL,
	.opts  = "delete <safe>",
	.desc  = "Delete a password safe after password confirmation",
};

kp_error_t
delete(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret;
	struct kp_safe safe;
	char path[PATH_MAX];

	if (argc - optind != 1) {
		warnx("missing safe name");
		return KP_EINPUT;
	}

	if ((ret = kp_safe_load(ctx, &safe, argv[optind])) != KP_SUCCESS) {
		warnx("cannot load safe");
		return ret;
	}

	if ((ret = kp_safe_open(ctx, &safe)) != KP_SUCCESS) {
		warnx("don't delete safe");
		return ret;
	}

	if (kp_safe_get_path(ctx, &safe, path, PATH_MAX) != KP_SUCCESS) {
		warnx("cannot compute safe path");
		return ret;
	}

	if (unlink(path) != 0) {
		warn("cannot delete safe %s", argv[optind]);
		warn("you can delete it manually with the following command:");
		warn("\n\trm %s", path);
		return errno;
	}

	return KP_SUCCESS;
}
