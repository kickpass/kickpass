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
	gpgme_ctx_t gpgme_ctx;
};

static const char *kp_storage_engine = "gpg";

kp_error_t
kp_storage_init(struct kp_storage_ctx **ctx)
{
	gpgme_error_t ret;
	setlocale(LC_ALL, "");

	*ctx = malloc(sizeof(struct kp_storage_ctx));
	if (!*ctx) return KP_ENOMEM;

	(*ctx)->engine = kp_storage_engine;
	(*ctx)->version = gpgme_check_version(NULL);

	gpgme_set_locale (NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
#ifdef LC_MESSAGES
	gpgme_set_locale(NULL, LC_MESSAGES, setlocale(LC_MESSAGES, NULL));
#endif

	switch (gpgme_new(&(*ctx)->gpgme_ctx)) {
	case GPG_ERR_NO_ERROR:
		break;
	case GPG_ERR_ENOMEM:
		return KP_ENOMEM;
	default:
		return KP_EINTERNAL;
	}

	gpgme_set_armor((*ctx)->gpgme_ctx, 1);

	return KP_SUCCESS;
}

kp_error_t
kp_storage_fini(struct kp_storage_ctx *ctx)
{
	gpgme_release(ctx->gpgme_ctx);
	free(ctx);
}

kp_error_t
kp_storage_get_engine(struct kp_storage_ctx *ctx, char *engine, size_t dstsize)
{
	if (ctx == NULL || engine == NULL) return KP_EINPUT;
	if (strlcpy(engine, ctx->engine, dstsize) >= dstsize) return KP_ENOMEM;
	return KP_SUCCESS;
}

kp_error_t
kp_storage_get_version(struct kp_storage_ctx *ctx, char *version, size_t dstsize)
{
	if (ctx == NULL || version == NULL) return KP_EINPUT;
	if (strlcpy(version, ctx->version, dstsize) >= dstsize) return KP_ENOMEM;
	return KP_SUCCESS;
}

kp_error_t
kp_storage_save(struct kp_storage_ctx *ctx, gpgme_data_t plain, gpgme_data_t cipher)
{
	gpgme_error_t ret;
	gpgme_user_id_t uid;

	ret = gpgme_op_encrypt(ctx->gpgme_ctx, NULL, GPGME_ENCRYPT_NO_ENCRYPT_TO, plain, cipher);

	switch(ret) {
	case GPG_ERR_NO_ERROR:
		break;
	default:
		printf("Internal error: %s\n", gpgme_strerror(ret));
		return KP_EINTERNAL;
	}

	return KP_SUCCESS;
}
