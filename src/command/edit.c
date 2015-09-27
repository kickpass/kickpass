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
#include "edit.h"
#include "editor.h"
#include "prompt.h"
#include "safe.h"
#include "storage.h"

static kp_error_t edit(struct kp_ctx *ctx, int argc, char **argv);
static kp_error_t parse_opt(struct kp_ctx *, int, char **);
static void usage(void);

struct kp_cmd kp_cmd_edit = {
	.main  = edit,
	.usage = usage,
	.opts  = "edit [-pm] <safe>",
	.desc  = "Edit a password safe with $EDIT",
};

static bool password = false;
static bool metadata = false;

kp_error_t
edit(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret = KP_SUCCESS;
	struct kp_safe safe;

	if ((ret = parse_opt(ctx, argc, argv)) != KP_SUCCESS) {
		return ret;
	}

	if (argc - optind != 1) {
		warnx("missing safe name");
		return KP_EINPUT;
	}

	if ((ret = kp_safe_load(ctx, &safe, argv[optind])) != KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_storage_open(ctx, &safe)) != KP_SUCCESS) {
		return ret;
	}

	if (password) {
		if ((ret = kp_prompt_password("safe", true, (char *)safe.password)) != KP_SUCCESS) {
			return ret;
		}
		safe.password_len = strlen((char *)safe.password);
	}

	if (metadata) {
		if ((ret = kp_edit(ctx, &safe)) != KP_SUCCESS) {
			return ret;
		}
	}

	if ((ret = kp_storage_save(ctx, &safe)) != KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_safe_close(ctx, &safe)) != KP_SUCCESS) {
		return ret;
	}

	return KP_SUCCESS;
}

static kp_error_t
parse_opt(struct kp_ctx *ctx, int argc, char **argv)
{
	int opt;
	static struct option longopts[] = {
		{ "password", no_argument, NULL, 'p' },
		{ "metadata", no_argument, NULL, 'm' },
		{ NULL,       0,           NULL, 0   },
	};

	while ((opt = getopt_long(argc, argv, "pm", longopts, NULL)) != -1) {
		switch (opt) {
		case 'p':
			password = true;
			break;
		case 'm':
			metadata = true;
			break;
		default:
			warnx("unknown option %c", opt);
			return KP_EINPUT;
		}
	}

	if (password && metadata) {
		warnx("Editing both password and metadata is default behavior. So you can ommit options.");
	}

	/* Default edit all */
	if (!password && !metadata) {
		password = true;
		metadata = true;
	}

	return KP_SUCCESS;
}

void
usage(void)
{
	printf("options:\n");
	printf("    -p, --password     Edit only password\n");
	printf("    -m, --metadata     Edit only metadata\n");
}
