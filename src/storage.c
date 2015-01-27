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

#include <gpgme.h>
#include <locale.h>
#include <stdlib.h>

#include "error.h"
#include "storage.h"

struct kp_storage_ctx
{
	const char *engine;
	const char *version;
};

static const char *kp_storage_engine = "gpg";

kp_error_t
kp_storage_init(struct kp_storage_ctx **ctx)
{
	setlocale(LC_ALL, "");

	*ctx = malloc(sizeof(struct kp_storage_ctx));
	if (!*ctx) return KP_MEMORY_ERROR;

	(*ctx)->engine = kp_storage_engine;
	(*ctx)->version = gpgme_check_version(NULL);

	gpgme_set_locale (NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
#ifdef LC_MESSAGES
	gpgme_set_locale(NULL, LC_MESSAGES, setlocale(LC_MESSAGES, NULL));
#endif

	/* gpgme_new */
	return KP_SUCCESS;
}

kp_error_t
kp_storage_fini(struct kp_storage_ctx *ctx)
{
	free(ctx);
}

kp_error_t
kp_storage_get_engine(struct kp_storage_ctx *ctx, char *engine, size_t dstsize)
{
	if (ctx == NULL || engine == NULL) return KP_USER_ERROR;
	if (strlcpy(engine, ctx->engine, dstsize) >= dstsize) return KP_MEMORY_ERROR;
	return KP_SUCCESS;
}

kp_error_t
kp_storage_get_version(struct kp_storage_ctx *ctx, char *version, size_t dstsize)
{
	if (ctx == NULL || version == NULL) return KP_USER_ERROR;
	if (strlcpy(version, ctx->version, dstsize) >= dstsize) return KP_MEMORY_ERROR;
	return KP_SUCCESS;
}
