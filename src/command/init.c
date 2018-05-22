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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "kickpass.h"

#include "command.h"
#include "init.h"
#include "prompt.h"
#include "config.h"
#include "log.h"

static kp_error_t init(struct kp_ctx *ctx, int argc, char **argv);
static kp_error_t parse_opt(struct kp_ctx *, int, char **);

struct kp_cmd kp_cmd_init = {
	.main  = init,
	.usage = NULL,
	.opts  = "init",
	.desc  = "Initialize a new password safe directory. "
	         "Default to ~/" KP_PATH,
};

kp_error_t
init(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret = KP_SUCCESS;
	char sub[PATH_MAX] = "";

	if ((ret = parse_opt(ctx, argc, argv)) != KP_SUCCESS) {
		return ret;
	}

	if (optind < argc) {
		if (strlcpy(sub, argv[optind++], PATH_MAX) >= PATH_MAX) {
			errno = ENOMEM;
			return KP_ERRNO;
		}
	}

	if ((ret = kp_password_prompt(ctx, true, (char *)ctx->password,
	                              "master")) != KP_SUCCESS) {
		kp_warn(ret, "cannot prompt password");
		return ret;
	}

	if ((ret = kp_init_workspace(ctx, sub)) != KP_SUCCESS) {
		kp_warn(ret, "cannot init workspace");
		return ret;
	}

	if ((ret = kp_cfg_create(ctx, sub)) != KP_SUCCESS) {
		kp_warn(ret, "cannot create configuration");
		return ret;
	}

	return ret;
}

static kp_error_t
parse_opt(struct kp_ctx *ctx, int argc, char **argv)
{
	int opt;
	kp_error_t ret = KP_SUCCESS;
	static struct option longopts[] = {
		{ "memlimit", required_argument, NULL, 'm' }, /* hidden option */
		{ "opslimit", required_argument, NULL, 'o' }, /* hidden option */
		{ NULL,       0,                 NULL, 0   },
	};

	while ((opt = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
		switch (opt) {
		case 'm':
			ctx->cfg.memlimit = atol(optarg);
			break;
		case 'o':
			ctx->cfg.opslimit = atoll(optarg);
			break;
		default:
			ret = KP_EINPUT;
			kp_warn(ret, "unknown option %c", opt);
		}
	}

	return ret;
}
