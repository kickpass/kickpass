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

#include <stddef.h>
#include <stdlib.h>

#include <err.h>
#include <readpassphrase.h>
#include <sodium.h>
#include <string.h>

#include "kickpass.h"

#include "config.h"

/*
 * Init kickpass with a working directory and its corresponding master password.
 */
kp_error_t
kp_init(struct kp_ctx *ctx)
{
	const char *home;
	char **password;

	password = (char **)&ctx->password;

	home = getenv("HOME");
	if (!home)
		errx(KP_EINPUT, "cannot find $HOME environment variable");

	if (strlcpy(ctx->ws_path, home, PATH_MAX) >= PATH_MAX)
		errx(KP_ENOMEM, "memory error");

	if (strlcat(ctx->ws_path, "/" KP_PATH, PATH_MAX) >= PATH_MAX)
		errx(KP_ENOMEM, "memory error");

	if (sodium_init() != 0)
		err(KP_EINTERNAL, "cannot initialize sodium");

	*password = sodium_malloc(KP_PASSWORD_MAX_LEN);
	if (!ctx->password) {
		warnx("memory error");
		return KP_ENOMEM;
	}

	ctx->cfg.memlimit = crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_SENSITIVE/5;
	ctx->cfg.opslimit = crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_SENSITIVE/5;

	return KP_SUCCESS;
}

/*
 * Open the main configuration with the master password.
 */
kp_error_t
kp_load(struct kp_ctx *ctx)
{
	kp_error_t ret;

	if ((ret = kp_cfg_load(ctx)) != KP_SUCCESS) {
		return ret;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_fini(struct kp_ctx *ctx)
{
	sodium_free(ctx->password);

	return KP_SUCCESS;
}

kp_error_t
kp_init_workspace(struct kp_ctx *ctx)
{
	kp_error_t ret = KP_SUCCESS;
	struct stat stats;

	if (stat(ctx->ws_path, &stats) == 0) {
		warnx("workspace already exists");
		ret = KP_EINPUT;
		goto out;
	} else if (errno & ENOENT) {
		printf("creating workspace %s\n", ctx->ws_path);
		mkdir(ctx->ws_path, 0700);
	} else {
		warn("invalid workspace %s", ctx->ws_path);
		ret = errno;
		goto out;
	}

out:
	return ret;
}
