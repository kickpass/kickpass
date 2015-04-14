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
#include <endian.h>
#include <sodium.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <readpassphrase.h>

#include "kickpass.h"

#include "safe.h"
#include "storage.h"

#ifndef betoh16
#define betoh16 be16toh
#endif
#ifndef betoh64
#define betoh64 be64toh
#endif

static uint16_t kp_storage_version = 0x0001;

struct kp_storage_header {
	uint16_t       version;
	uint16_t       sodium_version;
	uint64_t       opslimit;
	uint64_t       memlimit;
	uint64_t       data_size;
	unsigned char *data;
};

#define KP_STORAGE_HEADER_INIT { 0, 0, 0, 0, 0, NULL }

static kp_error_t kp_storage_save_header(const struct kp_safe *,
		struct kp_storage_header *);
static kp_error_t kp_storage_load_header(const struct kp_safe *,
		struct kp_storage_header *);
static kp_error_t kp_storage_encrypt(struct kp_ctx *,
		struct kp_storage_header *, const unsigned char *,
		unsigned char *, size_t);
static kp_error_t kp_storage_decrypt(struct kp_ctx *,
		struct kp_storage_header *, unsigned char *,
		const unsigned char *, size_t);

#define RW_HEADER(f, s, cipher, headfield) do {\
	(headfield) = htobe##s(headfield);\
	if (f(cipher, &(headfield), sizeof(headfield)) < sizeof(headfield)) {\
		warn("cannot " #f " safe header");\
		return errno;\
	}\
	(headfield) = betoh##s(headfield);\
} while(0)

#define READ_HEADER(s, cipher, headfield) RW_HEADER(read, s, cipher, headfield)
#define WRITE_HEADER(s, cipher, headfield) RW_HEADER(write, s, cipher, headfield)

static kp_error_t
kp_storage_save_header(const struct kp_safe *safe,
		struct kp_storage_header *header)
{
	assert(safe);
	assert(header->data);
	assert(header->data_size);

	WRITE_HEADER(16, safe->cipher, header->version);
	WRITE_HEADER(16, safe->cipher, header->sodium_version);
	WRITE_HEADER(64, safe->cipher, header->opslimit);
	WRITE_HEADER(64, safe->cipher, header->memlimit);
	WRITE_HEADER(64, safe->cipher, header->data_size);

	if (write(safe->cipher, header->data, header->data_size)
			< header->data_size) {
		warn("cannot write safe header");
		return errno;
	}

	return KP_SUCCESS;
}

static kp_error_t
kp_storage_load_header(const struct kp_safe *safe,
		struct kp_storage_header *header)
{
	assert(safe);
	assert(header->data == NULL);
	assert(header->data_size == 0);

	READ_HEADER(16, safe->cipher, header->version);
	READ_HEADER(16, safe->cipher, header->sodium_version);
	READ_HEADER(64, safe->cipher, header->opslimit);
	READ_HEADER(64, safe->cipher, header->memlimit);
	READ_HEADER(64, safe->cipher, header->data_size);

	header->data = malloc(header->data_size);
	if (!header->data) {
		warnx("memory error");
		return KP_ENOMEM;
	}

	errno = 0;
	if (read(safe->cipher, header->data, header->data_size) < header->data_size) {
		if (errno == 0) {
			warnx("invalid safe file");
			return KP_EINPUT;
		} else {
			warn("cannot read safe header");
			return errno;
		}
	}

	return KP_SUCCESS;
}


static kp_error_t
kp_storage_encrypt(struct kp_ctx *ctx, struct kp_storage_header *header,
		const unsigned char *plain, unsigned char *cipher, size_t size)
{
	unsigned char *salt, *nonce;
	unsigned char key[crypto_secretbox_KEYBYTES];

	header->data_size = crypto_pwhash_scryptsalsa208sha256_SALTBYTES +
		crypto_secretbox_NONCEBYTES;
	header->data      = malloc(header->data_size);

	if (!header->data) {
		warnx("memory error");
		return KP_ENOMEM;
	}

	randombytes_buf(header->data, header->data_size);
	salt = header->data;
	nonce = header->data + crypto_pwhash_scryptsalsa208sha256_SALTBYTES;

	crypto_pwhash_scryptsalsa208sha256(key, crypto_secretbox_KEYBYTES,
			ctx->password, strlen(ctx->password),
			salt, header->opslimit, header->memlimit);

	if (crypto_secretbox_easy(cipher, plain, size, nonce, key) != 0) {
		return KP_EINTERNAL;
	}

	return KP_SUCCESS;
}
static kp_error_t
kp_storage_decrypt(struct kp_ctx *ctx, struct kp_storage_header *header,
		unsigned char *plain, const unsigned char *cipher, size_t size)
{
	unsigned char *salt, *nonce;
	unsigned char key[crypto_secretbox_KEYBYTES];

	salt = header->data;
	nonce = header->data + crypto_pwhash_scryptsalsa208sha256_SALTBYTES;

	crypto_pwhash_scryptsalsa208sha256(key, crypto_secretbox_KEYBYTES,
			ctx->password, strlen(ctx->password),
			salt, header->opslimit, header->memlimit);

	if (crypto_secretbox_open_easy(plain, cipher, size, nonce, key) != 0) {
		return KP_EINTERNAL;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_storage_save(struct kp_ctx *ctx, struct kp_safe *safe)
{
	kp_error_t ret = KP_SUCCESS;
	unsigned char *cipher = NULL;
	struct kp_storage_header header = KP_STORAGE_HEADER_INIT;

	assert(ctx);
	assert(safe);
	assert(safe->open == true);

	cipher = malloc(safe->plain_size+crypto_secretbox_MACBYTES);
	if (!cipher) {
		warnx("memory error");
		ret = KP_ENOMEM;
		goto out;
	}

	header.version = kp_storage_version;
	header.sodium_version = SODIUM_LIBRARY_VERSION_MAJOR << 8 |
		SODIUM_LIBRARY_VERSION_MINOR;
	header.opslimit = ctx->opslimit;
	header.memlimit = ctx->memlimit;

	if ((ret = kp_storage_encrypt(ctx, &header, safe->plain, cipher,
		safe->plain_size))
		!= KP_SUCCESS) {
		warnx("cannot encrypt safe");
		goto out;
	}

	if ((ret = kp_storage_save_header(safe, &header)) != KP_SUCCESS) {
		goto out;
	}

	if (write(safe->cipher, cipher, safe->plain_size+crypto_secretbox_MACBYTES)
			< safe->plain_size+crypto_secretbox_MACBYTES) {
		warn("cannot write safe to file %s", safe->path);
		ret = errno;
		goto out;
	}

out:
	free(cipher);
	free(header.data);

	return ret;
}

kp_error_t
kp_storage_open(struct kp_ctx *ctx, struct kp_safe *safe)
{
	kp_error_t ret = KP_SUCCESS;
	size_t size;
	struct kp_storage_header header = KP_STORAGE_HEADER_INIT;
	unsigned char *cipher = NULL;

	assert(ctx);
	assert(safe);
	assert(safe->open == false);

	if ((ret = kp_storage_load_header(safe, &header)) != KP_SUCCESS) {
		goto out;
	}

	cipher = malloc(KP_PLAIN_MAX_SIZE+crypto_secretbox_MACBYTES);
	size = read(safe->cipher, cipher, KP_PLAIN_MAX_SIZE+crypto_secretbox_MACBYTES);
	safe->plain_size = size - crypto_secretbox_MACBYTES;

	if ((ret = kp_storage_decrypt(ctx, &header, safe->plain, cipher, size))
			!= KP_SUCCESS) {
		warnx("cannot decrypt safe");
		goto out;
	}

	safe->plain[safe->plain_size] = '\0';

out:
	free(cipher);

	return ret;
}
