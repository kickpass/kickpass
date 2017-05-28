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
#include <sodium.h>

#include "kickpass.h"

#include "command.h"
#include "config.h"
#include "edit.h"
#include "editor.h"
#include "password.h"
#include "prompt.h"
#include "safe.h"
#include "log.h"

static kp_error_t edit(struct kp_ctx *ctx, int argc, char **argv);
static kp_error_t parse_opt(struct kp_ctx *, int, char **);
static kp_error_t edit_password(struct kp_ctx *, struct kp_safe *);
static kp_error_t confirm_empty_password(bool *);
static void usage(void);

struct kp_cmd kp_cmd_edit = {
	.main  = edit,
	.usage = usage,
	.opts  = "edit [-pmgl] <safe>",
	.desc  = "Edit a password safe with $EDIT",
};

static bool password = false;
static bool metadata = false;
static bool generate = false;
static int  password_len = 20;

kp_error_t
edit(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret = KP_SUCCESS;
	char cfg_path[PATH_MAX] = "";
	struct kp_safe safe;

	if ((ret = parse_opt(ctx, argc, argv)) != KP_SUCCESS) {
		return ret;
	}

	if (argc - optind != 1) {
		ret = KP_EINPUT;
		kp_warn(ret, "missing safe name");
		return ret;
	}

	if ((ret = kp_cfg_find(ctx, argv[optind], cfg_path, PATH_MAX))
	    != KP_SUCCESS) {
		kp_warn(ret, "cannot find workspace config");
		return ret;
	}

	if ((ret = kp_cfg_load(ctx, cfg_path)) != KP_SUCCESS) {
		kp_warn(ret, "cannot load kickpass config");
		return ret;
	}

	if ((ret = kp_safe_init(ctx, &safe, argv[optind])) != KP_SUCCESS) {
		kp_warn(ret, "cannot init %s", argv[optind]);
		return ret;
	}

	if ((ret = kp_safe_open(ctx, &safe, KP_FORCE)) != KP_SUCCESS) {
		kp_warn(ret, "cannot open %s", argv[optind]);
		return ret;
	}

	if (password) {
		if (generate) {
			kp_password_generate(safe.password, password_len);
		} else {
			if ((ret = edit_password(ctx, &safe)) != KP_SUCCESS) {
				return ret;
			}
		}
	}

	if (metadata) {
		if ((ret = kp_edit(ctx, &safe)) != KP_SUCCESS) {
			return ret;
		}
	}

	if ((ret = kp_safe_save(ctx, &safe)) != KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_safe_close(ctx, &safe)) != KP_SUCCESS) {
		return ret;
	}

	return KP_SUCCESS;
}

static kp_error_t
edit_password(struct kp_ctx *ctx, struct kp_safe *safe)
{
	kp_error_t ret = KP_SUCCESS;
	char *password;
	size_t password_len;
	bool confirm = true;

	password = sodium_malloc(KP_PASSWORD_MAX_LEN);
	if ((ret = ctx->password_prompt(ctx, true, password, "safe")) != KP_SUCCESS) {
		goto out;
	}

	password_len = strlen(password);
	if (password_len == 0) {
		if ((ret = confirm_empty_password(&confirm)) != KP_SUCCESS) {
			sodium_free(password);
			return ret;
		}
	}

	if (confirm) {
		memcpy(safe->password, password, password_len);
		safe->password[password_len] = '\0';
	}

out:
	sodium_free(password);
	return ret;
}

static kp_error_t
confirm_empty_password(bool *confirm)
{
	kp_error_t ret = KP_SUCCESS;
	char *prompt = "Empty password. Do you really want to update password ? (y/n) [n] ";
	char *response = NULL;
	size_t response_len = 0;
	FILE *tty;

	*confirm = false;

	tty = fopen("/dev/tty", "r+");
	if (!tty) {
		ret = KP_ERRNO;
		kp_warn(ret, "cannot access /dev/tty");
		return ret;
	}

	fprintf(tty, "%s", prompt);
	fflush(tty);
	if (getline(&response, &response_len, stdin) < 0) {
		ret = KP_ERRNO;
		kp_warn(ret, "cannot read answer");
		return ret;
	}

	if (response[0] == 'y') *confirm = true;

	free(response);

	fclose(tty);
	return ret;
}

static kp_error_t
parse_opt(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret = KP_SUCCESS;
	int opt;
	static struct option longopts[] = {
		{ "password", no_argument, NULL, 'p' },
		{ "metadata", no_argument, NULL, 'm' },
		{ "generate", no_argument,       NULL, 'g' },
		{ "length",   required_argument, NULL, 'l' },
		{ NULL,       0,           NULL, 0   },
	};

	while ((opt = getopt_long(argc, argv, "pmgl:", longopts, NULL)) != -1) {
		switch (opt) {
		case 'p':
			password = true;
			break;
		case 'm':
			metadata = true;
			break;
		case 'g':
			generate = true;
			break;
		case 'l':
			password_len = atoi(optarg);
			break;
		default:
			ret = KP_EINPUT;
			kp_warn(ret, "unknown option %c", opt);
			return ret;
		}
	}

	if (password && metadata) {
		kp_warn(KP_EINPUT, "Editing both password and metadata"
			       " is default behavior. You can ommit options.");
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
	printf("    -g, --generate     Randomly generate a password\n");
	printf("    -l, --length=len   Length of the generated passwerd. Default to 20\n");
	printf("    -m, --metadata     Edit only metadata\n");
}
