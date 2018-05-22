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
#include "cat.h"
#include "editor.h"
#include "prompt.h"
#include "safe.h"
#include "log.h"

static kp_error_t cat(struct kp_ctx *ctx, int argc, char **argv);
static kp_error_t parse_opt(struct kp_ctx *, int, char **);
static void usage(void);

struct kp_cmd kp_cmd_cat = {
	.main  = cat,
	.usage = usage,
	.opts  = "cat [-pm] <safe>",
	.desc  = "Open a password safe and print its content on stdout",
};

static bool password = false;
static bool metadata = false;

kp_error_t
cat(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret;
	struct kp_safe safe;

	if ((ret = parse_opt(ctx, argc, argv)) != KP_SUCCESS) {
		return ret;
	}

	if (argc - optind != 1) {
		ret = KP_EINPUT;
		kp_warn(ret, "missing safe name");
		return ret;
	}

	if ((ret = kp_safe_init(ctx, &safe, argv[optind])) != KP_SUCCESS) {
		kp_warn(ret, "cannot init %s", argv[optind]);
		return ret;
	}

	if ((ret = kp_safe_open(ctx, &safe, 0)) != KP_SUCCESS) {
		kp_warn(ret, "cannot open %s", argv[optind]);
		return ret;
	}

	if (password) {
		printf("%s\n", safe.password);
	}
	if (metadata) {
		printf("%s\n", safe.metadata);
	}

	if ((ret = kp_safe_close(ctx, &safe)) != KP_SUCCESS) {
		kp_warn(ret, "cannot cleanly close safe"
			"clear text password might have leaked");
		return ret;
	}

	return KP_SUCCESS;
}

static kp_error_t
parse_opt(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret = KP_SUCCESS;
	int opt;
	static struct option longopts[] = {
		{ "password", no_argument, NULL, 'p' },
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
			ret = KP_EINPUT;
			kp_warn(ret, "unknown option %c", opt);
			return ret;
		}
	}

	if (!password && metadata) {
		kp_warnx(KP_EINPUT, "Opening only metadata is default behavior."
				"You can ommit option.");
	}

	/* Default open only metadata */
	if (!password && !metadata) {
		password = false;
		metadata = true;
	}

	return ret;
}

void
usage(void)
{
	printf("options:\n");
	printf("    -p, --password     Open password (This should be used very carefully)\n");
	printf("    -m, --metadata     Open metadata\n");
}
