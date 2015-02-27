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

#include <assert.h>
#include <fcntl.h>
#include <gpgme.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kickpass.h"

#include "safe.h"
#include "storage.h"

struct kp_storage
{
	const char *engine;
	const char *version;
	gpgme_ctx_t gpgme_ctx;
};

static const char *kp_storage_engine = "gpg";

static kp_error_t kp_storage_encrypt(struct kp_storage *storage, gpgme_data_t plain, gpgme_data_t cipher);
gpgme_error_t gpgme_passphrase_cb(void *hook, const char *uid_hint, const char *passphrase_info, int prev_was_bad, int fd);

gpgme_error_t
gpgme_passphrase_cb(void *hook, const char *uid_hint, const char *passphrase_info, int prev_was_bad, int fd)
{
	const char passwd[] = "test\n";
	gpgme_io_write(fd, passwd, sizeof(passwd)-1);
	return 0;
}

kp_error_t
kp_storage_init(struct kp_ctx *ctx, struct kp_storage **storage)
{
	kp_error_t ret;
	gpgme_error_t err;
	setlocale(LC_ALL, "");

	*storage = malloc(sizeof(struct kp_storage));
	if (!*storage) {
		LOGE("memory error");
		return KP_ENOMEM;
	}

	(*storage)->engine = kp_storage_engine;
	(*storage)->version = gpgme_check_version(NULL);

	gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
#ifdef LC_MESSAGES
	gpgme_set_locale(NULL, LC_MESSAGES, setlocale(LC_MESSAGES, NULL));
#endif

	err = gpgme_new(&(*storage)->gpgme_ctx);
	switch (err) {
	case GPG_ERR_NO_ERROR:
		break;
	case GPG_ERR_ENOMEM:
		return KP_ENOMEM;
	default:
		return KP_EINTERNAL;
	}

	err = gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP);
	switch (err) {
	case GPG_ERR_NO_ERROR:
		break;
	case GPG_ERR_INV_ENGINE:
		LOGF("cannot use OpenPGP as a storage engine: %s", gpgme_strerror(err));
		ret = KP_EINTERNAL;
		goto out;
	}

	err = gpgme_set_protocol((*storage)->gpgme_ctx, GPGME_PROTOCOL_OpenPGP);
	assert(err == GPG_ERR_NO_ERROR);

	gpgme_engine_info_t eng = gpgme_ctx_get_engine_info((*storage)->gpgme_ctx);
	err = gpgme_ctx_set_engine_info((*storage)->gpgme_ctx, GPGME_PROTOCOL_OpenPGP, eng->file_name, ctx->ws_path);
	switch (err) {
	case GPG_ERR_NO_ERROR:
		break;
	default:
		LOGF("cannot configure storage engine OpenPGP: %s", gpgme_strerror(err));
		ret = KP_EINTERNAL;
		goto out;
	}

	gpgme_set_armor((*storage)->gpgme_ctx, 1);
	gpgme_set_passphrase_cb((*storage)->gpgme_ctx, gpgme_passphrase_cb, NULL);

	return KP_SUCCESS;

out:
	kp_storage_fini(*storage);
	return ret;
}

kp_error_t
kp_storage_fini(struct kp_storage *storage)
{
	gpgme_release(storage->gpgme_ctx);
	free(storage);

	return KP_SUCCESS;
}

kp_error_t
kp_storage_get_engine(struct kp_storage *storage, char *engine, size_t dstsize)
{
	if (storage == NULL || engine == NULL) return KP_EINPUT;
	if (strlcpy(engine, storage->engine, dstsize) >= dstsize) return KP_ENOMEM;
	return KP_SUCCESS;
}

kp_error_t
kp_storage_get_version(struct kp_storage *storage, char *version, size_t dstsize)
{
	if (storage == NULL || version == NULL) return KP_EINPUT;
	if (strlcpy(version, storage->version, dstsize) >= dstsize) return KP_ENOMEM;
	return KP_SUCCESS;
}

static kp_error_t
kp_storage_encrypt(struct kp_storage *storage, gpgme_data_t plain, gpgme_data_t cipher)
{
	gpgme_error_t ret;
	ret = gpgme_op_encrypt(storage->gpgme_ctx, NULL, GPGME_ENCRYPT_NO_ENCRYPT_TO, plain, cipher);

	switch(ret) {
	case GPG_ERR_NO_ERROR:
		break;
	default:
		LOGE("internal error: %s", gpgme_strerror(ret));
		return KP_EINTERNAL;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_storage_save(struct kp_storage *storage, struct kp_safe *safe)
{
	gpgme_error_t ret;
	int cipher_fd;
	gpgme_data_t plain, cipher;

	ret = gpgme_data_new_from_fd(&plain, safe->plain.fd);
	switch(ret) {
	case GPG_ERR_NO_ERROR:
		break;
	default:
		LOGE("internal error: %s", gpgme_strerror(ret));
		return KP_EINTERNAL;
	}

	cipher_fd = open(safe->path, O_RDWR | O_NONBLOCK | O_CREAT, S_IRUSR | S_IWUSR);
	if (safe->plain.fd < 0) {
		LOGE("cannot open safe %s: %s (%d)", safe->path, strerror(errno), errno);
		return KP_EINPUT;
	}

	gpgme_data_new_from_fd(&cipher, cipher_fd);
	switch(ret) {
	case GPG_ERR_NO_ERROR:
		break;
	default:
		LOGE("internal error: %s", gpgme_strerror(ret));
		return KP_EINTERNAL;
	}

	if (kp_storage_encrypt(storage, plain, cipher) != KP_SUCCESS) {
		LOGE("cannot encrypt safe");
		return KP_EINTERNAL;
	}

	if (close(cipher_fd) < 0) {
		LOGW("cannot close safe: %s (%d)", strerror(errno), errno);
		return errno;
	}

	if (close(safe->plain.fd) < 0) {
		LOGW("cannot close clear text safe: %s (%d)", strerror(errno), errno);
		return errno;
	}

	if (unlink(safe->plain.path) < 0) {
		LOGE("cannot delete clear text safe: %s (%d)", strerror(errno), errno);
		return errno;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_storage_open(struct kp_storage *storage, gpgme_data_t cipher, gpgme_data_t plain)
{
	gpgme_error_t ret;

	ret = gpgme_op_decrypt(storage->gpgme_ctx, cipher, plain);

	switch(ret) {
	case GPG_ERR_NO_ERROR:
		break;
	default:
		LOGE("internal error: %s", gpgme_strerror(ret));
		return KP_EINTERNAL;
	}

	return KP_SUCCESS;
}
