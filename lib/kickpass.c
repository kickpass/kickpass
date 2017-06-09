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

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

#include <readpassphrase.h>
#include <sodium.h>
#include <string.h>

#include "kickpass.h"

#include "config.h"

kp_error_t
kp_init(struct kp_ctx *ctx)
{
	const char *home;
	char **password;

	assert(ctx);

	password = (char **)&ctx->password;

	home = getenv("HOME");
	if (!home) {
		return KP_NO_HOME;
	}

	if (strlcpy(ctx->ws_path, home, PATH_MAX) >= PATH_MAX) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	if (strlcat(ctx->ws_path, "/" KP_PATH, PATH_MAX) >= PATH_MAX) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	if (sodium_init() < 0) {
		return KP_EINTERNAL;
	}

	*password = sodium_malloc(KP_PASSWORD_MAX_LEN);
	if (!ctx->password) {
		errno = ENOMEM;
		return KP_ERRNO;
	}

	ctx->password[0] = '\0';

	ctx->cfg.memlimit = crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_SENSITIVE/5;
	ctx->cfg.opslimit = crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_SENSITIVE/5;

	ctx->agent.connected = false;

	return KP_SUCCESS;
}

kp_error_t
kp_fini(struct kp_ctx *ctx)
{
	assert(ctx);

	sodium_free(ctx->password);

	return KP_SUCCESS;
}

kp_error_t
kp_init_workspace(struct kp_ctx *ctx, const char *sub)
{
	kp_error_t ret = KP_SUCCESS;
	struct stat stats;
	char path[PATH_MAX] = "";

	assert(ctx);
	assert(sub);

	if (strlcpy(path , ctx->ws_path, PATH_MAX) >= PATH_MAX) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	if (strlcat(path , "/", PATH_MAX) >= PATH_MAX) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	if (strlcat(path , sub, PATH_MAX) >= PATH_MAX) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	if (stat(path, &stats) == 0) {
		errno = EEXIST;
		ret = KP_ERRNO;
		goto out;
	} else if (errno & ENOENT) {
		if (mkdir(path, 0700) < 0) {
			ret = KP_ERRNO;
			goto out;
		}
	} else {
		ret = KP_ERRNO;
		goto out;
	}

out:
	return ret;
}

const char *
kp_version_string(void)
{
	return KICKPASS_VERSION_STRING;
}

int
kp_version_major(void)
{
	return KICKPASS_VERSION_MAJOR;
}

kp_error_t
kp_password_prompt(struct kp_ctx *ctx, bool confirm, char *password, const char *fmt, ...)
{
	kp_error_t ret;
	va_list ap;

	assert(ctx);

	if (ctx->password_prompt == NULL) {
		return KP_NOPROMPT;
	}

	va_start(ap, fmt);
	ret = ctx->password_prompt(ctx, confirm, password, fmt, ap);
	va_end(ap);

	return ret;
}
