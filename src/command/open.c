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
#include <string.h>

#include "kickpass.h"

#include "command.h"
#include "open.h"
#include "editor.h"
#include "safe.h"
#include "storage.h"

static kp_error_t open(struct kp_ctx *ctx, int argc, char **argv);
static kp_error_t usage(void);

struct kp_cmd kp_cmd_open = {
	.main  = open,
	.usage = usage,
};

kp_error_t
open(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret = KP_SUCCESS;
	char path[PATH_MAX];
	struct kp_safe safe;

	if (argc - optind != 1) {
		warnx("missing safe name");
		return KP_EINPUT;
	}

	if (strlcpy(path, ctx->ws_path, PATH_MAX) >= PATH_MAX) {
		warnx("memory error");
		return KP_ENOMEM;
	}

	if (strlcat(path, "/", PATH_MAX) >= PATH_MAX) {
		warnx("memory error");
		return KP_ENOMEM;
	}

	if (strlcat(path, argv[optind], PATH_MAX) >= PATH_MAX) {
		warnx("memory error");
		return KP_ENOMEM;
	}

	if ((ret = kp_safe_load(ctx, &safe, path)) != KP_SUCCESS) {
		warnx("cannot load safe");
		return ret;
	}

	if ((ret = kp_load_passwd(ctx)) != KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_storage_open(ctx, &safe)) != KP_SUCCESS) {
		warnx("cannot save safe");
		return ret;
	}

	printf("%s\n", safe.plain);

	if ((ret = kp_safe_close(ctx, &safe)) != KP_SUCCESS) {
		warnx("cannot cleanly close safe");
		warnx("clear text password might have leaked");
	}

	return KP_SUCCESS;
}

kp_error_t
usage(void)
{
	printf("    %-10s%s\n", "open", "Open a password safe and print its content on stdout");
	return KP_SUCCESS;
}
