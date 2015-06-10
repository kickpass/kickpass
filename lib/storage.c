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
		const unsigned char *, size_t,
		const unsigned char *, size_t,
		unsigned char *, size_t *);
static kp_error_t kp_storage_decrypt(struct kp_ctx *,
		struct kp_storage_header *,
		const unsigned char *, size_t,
		unsigned char *, size_t *,
		const unsigned char *, size_t);

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
		const unsigned char *packed_header, size_t header_size,
		const unsigned char *plain, size_t plain_size,
		unsigned char *cipher, size_t *cipher_size)
{
	unsigned char key[crypto_aead_chacha20poly1305_KEYBYTES];

	crypto_pwhash_scryptsalsa208sha256(key,
			crypto_aead_chacha20poly1305_KEYBYTES,
			ctx->password, strlen(ctx->password),
			header->salt, header->opslimit, header->memlimit);

	if (crypto_aead_chacha20poly1305_encrypt(cipher,
				(unsigned long long *)cipher_size,
				plain,
				(unsigned long long)plain_size,
				packed_header,
				(unsigned long long)header_size,
				NULL,
				header->nonce, key) != 0) {
		return KP_EINTERNAL;
	}

	return KP_SUCCESS;
}
static kp_error_t
kp_storage_decrypt(struct kp_ctx *ctx, struct kp_storage_header *header,
		const unsigned char *packed_header, size_t header_size,
		unsigned char *plain, size_t *plain_size,
		const unsigned char *cipher, size_t cipher_size)
{
	unsigned char key[crypto_aead_chacha20poly1305_KEYBYTES];

	crypto_pwhash_scryptsalsa208sha256(key,
			crypto_aead_chacha20poly1305_KEYBYTES,
			ctx->password, strlen(ctx->password),
			header->salt, header->opslimit, header->memlimit);

	if (crypto_aead_chacha20poly1305_decrypt(plain,
				(unsigned long long *)plain_size,
				NULL,
				cipher,
				(unsigned long long)cipher_size,
				packed_header,
				(unsigned long long)header_size,
				header->nonce, key) != 0) {
		return KP_EINTERNAL;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_storage_save(struct kp_ctx *ctx, struct kp_safe *safe)
{
	kp_error_t ret = KP_SUCCESS;
	unsigned char *cipher = NULL;
	size_t cipher_size;
	struct kp_storage_header header = KP_STORAGE_HEADER_INIT;
	unsigned char packed_header[KP_STORAGE_HEADER_SIZE];
	char path[PATH_MAX];

	assert(ctx);
	assert(safe);
	assert(safe->open == true);

	if ((ret = kp_safe_get_path(ctx, safe, path, PATH_MAX)) != KP_SUCCESS) {
		return ret;
	}

	/* alloc cipher to max size */
	cipher = malloc(safe->plain_size+crypto_aead_chacha20poly1305_ABYTES);
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
	randombytes_buf(header.salt, KP_STORAGE_SALT_SIZE);
	randombytes_buf(header.nonce, KP_STORAGE_NONCE_SIZE);

	kp_storage_header_pack(&header, packed_header);

	if ((ret = kp_storage_encrypt(ctx, &header,
					packed_header, KP_STORAGE_HEADER_SIZE,
					safe->plain, safe->plain_size,
					cipher, &cipher_size))
		!= KP_SUCCESS) {
		warnx("cannot encrypt safe");
		goto out;
	}

	if (write(safe->cipher, packed_header, KP_STORAGE_HEADER_SIZE)
			< KP_STORAGE_HEADER_SIZE) {
		warn("cannot write safe to file %s", path);
		ret = errno;
		goto out;
	}

	if (write(safe->cipher, cipher, cipher_size)
			< cipher_size) {
		warn("cannot write safe to file %s", path);
		ret = errno;
		goto out;
	}

out:
	free(cipher);

	return ret;
}

kp_error_t
kp_storage_open(struct kp_ctx *ctx, struct kp_safe *safe)
{
	kp_error_t ret = KP_SUCCESS;
	unsigned char *cipher = NULL;
	size_t cipher_size;
	struct kp_storage_header header = KP_STORAGE_HEADER_INIT;
	unsigned char packed_header[KP_STORAGE_HEADER_SIZE];

	assert(ctx);
	assert(safe);
	assert(safe->open == false);

	errno = 0;
	if (read(safe->cipher, packed_header, KP_STORAGE_HEADER_SIZE)
			!= KP_STORAGE_HEADER_SIZE) {
		if (errno != 0) {
			warn("cannot read safe");
			ret = errno;
		} else {
			warnx("cannot read safe");
			ret = KP_EINPUT;
		}
		goto out;
	}

	kp_storage_header_unpack(&header, packed_header);

	cipher = malloc(KP_PLAIN_MAX_SIZE+crypto_aead_chacha20poly1305_ABYTES);
	cipher_size = read(safe->cipher, cipher,
			KP_PLAIN_MAX_SIZE+crypto_aead_chacha20poly1305_ABYTES);

	if (cipher_size - crypto_aead_chacha20poly1305_ABYTES
			> KP_PLAIN_MAX_SIZE) {
		warnx("safe too long");
		ret = KP_EINPUT;
		goto out;
	}

	if ((ret = kp_storage_decrypt(ctx, &header,
					packed_header, KP_STORAGE_HEADER_SIZE,
					safe->plain, &safe->plain_size,
					cipher, cipher_size))
			!= KP_SUCCESS) {
		warnx("cannot decrypt safe. Bad password ?");
		goto out;
	}

	safe->plain[safe->plain_size] = '\0';
	safe->open = true;

out:
	free(cipher);

	return ret;
}
