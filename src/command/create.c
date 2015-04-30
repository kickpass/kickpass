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
#include "create.h"
#include "editor.h"
#include "safe.h"
#include "storage.h"

static kp_error_t create(struct kp_ctx *, int, char **);
static kp_error_t parse_opt(struct kp_ctx *, int, char **);
static kp_error_t usage(void);

struct kp_cmd kp_cmd_create = {
	.main  = create,
	.opts  = "create [-hg] <safe>",
	.desc  = "Create a new password safe",
};

static bool help = false;
static bool generate = false;

kp_error_t
create(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret;
	char path[PATH_MAX];
	struct kp_safe safe;

	if ((ret = parse_opt(ctx, argc, argv)) != KP_SUCCESS) {
		return ret;
	}

	if (help) return usage();

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
		ret = KP_ENOMEM;
		return ret;
	}

	if (strlcat(path, argv[optind], PATH_MAX) >= PATH_MAX) {
		warnx("memory error");
		ret = KP_ENOMEM;
		return ret;
	}

	if ((ret = kp_load_passwd(ctx, true)) != KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_safe_create(ctx, &safe, path)) != KP_SUCCESS) {
		if (ret == KP_EEXIST) {
			warnx("please use edit command to edit an existing safe");
		} else {
			warnx("cannot create safe");
		}
		return ret;
	}

	if ((ret = kp_edit(ctx, &safe)) != KP_SUCCESS) {
		warnx("cannot edit safe");
		return ret;
	}

	if ((ret = kp_storage_save(ctx, &safe)) != KP_SUCCESS) {
		warnx("cannot save safe");
		return ret;
	}

	if ((ret = kp_safe_close(ctx, &safe)) != KP_SUCCESS) {
		warnx("cannot cleanly close safe");
		warnx("clear text password might have leaked");
		return ret;
	}

	return KP_SUCCESS;
}

static kp_error_t
parse_opt(struct kp_ctx *ctx, int argc, char **argv)
{
	int opt;
	static struct option longopts[] = {
		{ "help",     no_argument, NULL, 'h' },
		{ "generate", no_argument, NULL, 'g' },
		{ NULL,       0,           NULL, 0   },
	};

	while ((opt = getopt_long(argc, argv, "hg", longopts, NULL)) != -1) {
		switch (opt) {
		case 'h':
			help = true;
			break;
		case 'g':
			generate = true;
			break;
		default:
			warnx("unknown option %c", opt);
			return KP_EINPUT;
		}
	}

	return KP_SUCCESS;
}

kp_error_t
usage(void)
{
	extern char *__progname;

	printf("usage: %s %s\n", __progname, kp_cmd_create.opts);
	printf("options:\n");
	printf("    -h, --help     Display this help\n");
	printf("    -g, --generate Randomly generate a password\n");

	return KP_EINPUT;
}
