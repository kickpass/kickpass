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

#include <sys/types.h>
#ifndef __linux__
#include <sys/endian.h>
#else
#include <endian.h>
#endif

#include <stdlib.h>
#include <stdint.h>

#include <assert.h>
#include <sodium.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>

#include "kickpass.h"

#include "safe.h"
#include "storage.h"

static uint16_t kp_storage_version = 0x0001;

#ifndef betoh16
#define betoh16 be16toh
#endif
#ifndef betoh64
#define betoh64 be64toh
#endif

#define KP_STORAGE_SALT_SIZE   crypto_pwhash_scryptsalsa208sha256_SALTBYTES
#define KP_STORAGE_NONCE_SIZE  crypto_aead_chacha20poly1305_NPUBBYTES
#define KP_STORAGE_HEADER_SIZE (2+2+8+8+KP_STORAGE_SALT_SIZE+KP_STORAGE_NONCE_SIZE)

struct kp_storage_header {
	uint16_t       version;
	uint16_t       sodium_version;
	uint64_t       opslimit;
	uint64_t       memlimit;
	unsigned char  salt[KP_STORAGE_SALT_SIZE];
	unsigned char  nonce[KP_STORAGE_NONCE_SIZE];
};

#define KP_STORAGE_HEADER_INIT { 0, 0, 0, 0, { 0 }, { 0 } }

static void kp_storage_header_pack(const struct kp_storage_header *,
		unsigned char *);
static void kp_storage_header_unpack(struct kp_storage_header *,
		const unsigned char *);
static kp_error_t kp_storage_encrypt(struct kp_ctx *,
		struct kp_storage_header *,
		const unsigned char *, unsigned long long,
		const unsigned char *, unsigned long long,
		unsigned char *, unsigned long long *);
static kp_error_t kp_storage_decrypt(struct kp_ctx *,
		struct kp_storage_header *,
		const unsigned char *, unsigned long long,
		unsigned char *, unsigned long long *,
		const unsigned char *, unsigned long long);

#define READ_HEADER(s, packed, field) do {\
	memcpy(&(field), (packed), (s)/8);\
	(field) = betoh##s(field);\
	(packed) = (packed) + (s)/8;\
} while(0)

#define WRITE_HEADER(s, packed, field)  do {\
	memcpy((packed), &(field), (s)/8);\
	*(uint##s##_t *)(packed) = htobe##s(*(uint##s##_t *)(packed));\
	(packed) = (packed) + (s)/8;\
} while(0)

static void
kp_storage_header_pack(const struct kp_storage_header *header,
		unsigned char *packed)
{
	assert(header);
	assert(packed);

	WRITE_HEADER(16, packed, header->version);
	WRITE_HEADER(16, packed, header->sodium_version);
	WRITE_HEADER(64, packed, header->opslimit);
	WRITE_HEADER(64, packed, header->memlimit);
	memcpy(packed, header->salt, KP_STORAGE_SALT_SIZE);
	packed = packed + KP_STORAGE_SALT_SIZE;
	memcpy(packed, header->nonce, KP_STORAGE_NONCE_SIZE);
}

static void
kp_storage_header_unpack(struct kp_storage_header *header,
		const unsigned char *packed)
{
	assert(header);
	assert(packed);

	READ_HEADER(16, packed, header->version);
	READ_HEADER(16, packed, header->sodium_version);
	READ_HEADER(64, packed, header->opslimit);
	READ_HEADER(64, packed, header->memlimit);
	memcpy(header->salt, packed, KP_STORAGE_SALT_SIZE);
	packed = packed + KP_STORAGE_SALT_SIZE;
	memcpy(header->nonce, packed, KP_STORAGE_NONCE_SIZE);
}


static kp_error_t
kp_storage_encrypt(struct kp_ctx *ctx, struct kp_storage_header *header,
		const unsigned char *packed_header, unsigned long long header_size,
		const unsigned char *plain, unsigned long long plain_size,
		unsigned char *cipher, unsigned long long *cipher_size)
{
	unsigned char key[crypto_aead_chacha20poly1305_KEYBYTES];

	if (crypto_pwhash_scryptsalsa208sha256(key,
			crypto_aead_chacha20poly1305_KEYBYTES,
			ctx->password, strlen(ctx->password),
			header->salt, header->opslimit, header->memlimit) != 0) {
		errno = ENOMEM;
		return KP_ERRNO;
	}

	if (crypto_aead_chacha20poly1305_encrypt(cipher, cipher_size,
				plain, plain_size,
				packed_header, header_size,
				NULL, header->nonce, key) != 0) {
		return KP_EENCRYPT;
	}

	return KP_SUCCESS;
}
static kp_error_t
kp_storage_decrypt(struct kp_ctx *ctx, struct kp_storage_header *header,
		const unsigned char *packed_header, unsigned long long header_size,
		unsigned char *plain, unsigned long long *plain_size,
		const unsigned char *cipher, unsigned long long cipher_size)
{
	unsigned char key[crypto_aead_chacha20poly1305_KEYBYTES];

	if (crypto_pwhash_scryptsalsa208sha256(key,
			crypto_aead_chacha20poly1305_KEYBYTES,
			ctx->password, strlen(ctx->password),
			header->salt, header->opslimit, header->memlimit) != 0) {
		errno = ENOMEM;
		return KP_ERRNO;
	}

	if (crypto_aead_chacha20poly1305_decrypt(plain, plain_size,
				NULL, cipher, cipher_size,
				packed_header, header_size,
				header->nonce, key) != 0) {
		return KP_EDECRYPT;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_storage_save(struct kp_ctx *ctx, struct kp_safe *safe)
{
	kp_error_t ret = KP_SUCCESS;
	unsigned char *cipher = NULL, *plain = NULL;
	unsigned long long cipher_size, plain_size;
	struct kp_storage_header header = KP_STORAGE_HEADER_INIT;
	unsigned char packed_header[KP_STORAGE_HEADER_SIZE];
	char path[PATH_MAX];
	size_t password_len, metadata_len;

	assert(ctx);
	assert(safe);
	assert(safe->open == true);

	password_len = strlen(safe->password);
	assert(password_len < KP_PASSWORD_MAX_LEN);
	metadata_len = strlen(safe->metadata);
	assert(metadata_len < KP_METADATA_MAX_LEN);

	/* Ensure we are at the beginning of the safe */
	if (lseek(safe->cipher, 0, SEEK_SET) != 0) {
		return KP_ERRNO;
	}

	if (ftruncate(safe->cipher, 0) != 0) {
		return KP_ERRNO;
	}

	if ((ret = kp_safe_get_path(ctx, safe, path, PATH_MAX)) != KP_SUCCESS) {
		return ret;
	}

	/* construct full plain */
	/* plain is password + '\0' + metadata + '\0' */
	plain_size = password_len + metadata_len + 2;
	plain = sodium_malloc(plain_size);
	strncpy((char *)plain, (char *)safe->password, password_len);
	plain[password_len] = '\0';
	strncpy((char *)&plain[password_len+1], (char *)safe->metadata, metadata_len);
	plain[plain_size-1] = '\0';

	/* alloc cipher to max size */
	cipher = malloc(plain_size+crypto_aead_chacha20poly1305_ABYTES);
	if (!cipher) {
		ret = ENOMEM;
		goto out;
	}

	header.version = kp_storage_version;
	header.sodium_version = SODIUM_LIBRARY_VERSION_MAJOR << 8 |
		SODIUM_LIBRARY_VERSION_MINOR;
	header.opslimit = ctx->cfg.opslimit;
	header.memlimit = ctx->cfg.memlimit;
	randombytes_buf(header.salt, KP_STORAGE_SALT_SIZE);
	randombytes_buf(header.nonce, KP_STORAGE_NONCE_SIZE);

	kp_storage_header_pack(&header, packed_header);

	if ((ret = kp_storage_encrypt(ctx, &header,
					packed_header, KP_STORAGE_HEADER_SIZE,
					plain, plain_size,
					cipher, &cipher_size))
		!= KP_SUCCESS) {
		goto out;
	}

	if (write(safe->cipher, packed_header, KP_STORAGE_HEADER_SIZE)
			< KP_STORAGE_HEADER_SIZE) {
		ret = KP_ERRNO;
		goto out;
	}

	if (write(safe->cipher, cipher, cipher_size)
			< cipher_size) {
		ret = KP_ERRNO;
		goto out;
	}

out:
	sodium_free(plain);
	free(cipher);

	return ret;
}

kp_error_t
kp_storage_open(struct kp_ctx *ctx, struct kp_safe *safe)
{
	kp_error_t ret = KP_SUCCESS;
	unsigned char *cipher = NULL, *plain = NULL;
	unsigned long long cipher_size, plain_size;
	struct kp_storage_header header = KP_STORAGE_HEADER_INIT;
	unsigned char packed_header[KP_STORAGE_HEADER_SIZE];
	size_t password_len;

	assert(ctx);
	assert(safe);
	assert(safe->open == false);

	errno = 0;
	if (read(safe->cipher, packed_header, KP_STORAGE_HEADER_SIZE)
			!= KP_STORAGE_HEADER_SIZE) {
		if (errno != 0) {
			ret = KP_ERRNO;
		} else {
			ret = KP_INVALID_STORAGE;
		}
		goto out;
	}

	kp_storage_header_unpack(&header, packed_header);

	/* alloc plain to max size */
	plain = sodium_malloc(KP_PLAIN_MAX_SIZE);

	/* alloc cipher to max size */
	cipher = malloc(KP_PLAIN_MAX_SIZE+crypto_aead_chacha20poly1305_ABYTES);
	cipher_size = read(safe->cipher, cipher,
			KP_PLAIN_MAX_SIZE+crypto_aead_chacha20poly1305_ABYTES);

	if (cipher_size <= crypto_aead_chacha20poly1305_ABYTES) {
		ret = KP_INVALID_STORAGE;
		goto out;
	}

	if (cipher_size - crypto_aead_chacha20poly1305_ABYTES
			> KP_PLAIN_MAX_SIZE) {
		ret = ENOMEM;
		goto out;
	}

	if ((ret = kp_storage_decrypt(ctx, &header,
					packed_header, KP_STORAGE_HEADER_SIZE,
					plain, &plain_size,
					cipher, cipher_size))
			!= KP_SUCCESS) {
		goto out;
	}

	strncpy((char *)safe->password, (char *)plain, KP_PASSWORD_MAX_LEN);
	/* ensure null termination */
	safe->password[KP_PASSWORD_MAX_LEN-1] = '\0';
	password_len = strlen((char *)safe->password);

	strncpy((char *)safe->metadata, (char *)&plain[password_len+1], KP_METADATA_MAX_LEN);
	/* ensure null termination */
	safe->metadata[KP_METADATA_MAX_LEN-1] = '\0';

	safe->open = true;

out:
	sodium_free(plain);
	free(cipher);

	return ret;
}
