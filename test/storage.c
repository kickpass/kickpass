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

#include <check.h>

#include "check_compat.h"

#include "../lib/safe.c"
#include "../lib/storage.c"

START_TEST(test_storage_header_pack_should_be_successful)
{
	/* Given */
	struct kp_storage_header header = KP_STORAGE_HEADER_INIT;
	unsigned char packed_header[KP_STORAGE_HEADER_SIZE] = { 0 };
	unsigned char salt[] = {
		0x12, 0x10, 0xcb, 0x68, 0x45, 0xeb, 0xc7, 0x6a,
		0x7b, 0x91, 0x00, 0xcf, 0xed, 0x42, 0xc8, 0xcf,
		0xcb, 0x66, 0x50, 0xd1, 0x04, 0x2e, 0xe8, 0x81,
		0xcb, 0x5f, 0x96, 0x4c, 0xe8, 0x65, 0x1e, 0x2c,
	};
	unsigned char nonce[] = {
		0xe6, 0x59, 0x12, 0x7a, 0xf5, 0x7d, 0xfc, 0xf8,
	};

	header.version = 0xdead;
	header.sodium_version = 0xbaad;
	header.opslimit = 0x71f97b79931b97d8LL;
	header.memlimit = 0x50b77cc354846208LL;
	memcpy(header.salt, salt, KP_STORAGE_SALT_SIZE);
	memcpy(header.nonce, nonce, KP_STORAGE_NONCE_SIZE);

	/* When */
	kp_storage_header_pack(&header, packed_header);

	/* Then */
	unsigned char ref[KP_STORAGE_HEADER_SIZE] = {
		0xde, 0xad, 0xba, 0xad, 0x71, 0xf9, 0x7b, 0x79,
		0x93, 0x1b, 0x97, 0xd8, 0x50, 0xb7, 0x7c, 0xc3,
		0x54, 0x84, 0x62, 0x08, 0x12, 0x10, 0xcb, 0x68,
		0x45, 0xeb, 0xc7, 0x6a, 0x7b, 0x91, 0x00, 0xcf,
		0xed, 0x42, 0xc8, 0xcf, 0xcb, 0x66, 0x50, 0xd1,
		0x04, 0x2e, 0xe8, 0x81, 0xcb, 0x5f, 0x96, 0x4c,
		0xe8, 0x65, 0x1e, 0x2c, 0xe6, 0x59, 0x12, 0x7a,
		0xf5, 0x7d, 0xfc, 0xf8,
	};
	ck_assert_int_eq(memcmp(packed_header, ref, KP_STORAGE_HEADER_SIZE), 0);
}
END_TEST

START_TEST(test_storage_header_unpack_should_be_successful)
{
	/* Given */
	struct kp_storage_header header = KP_STORAGE_HEADER_INIT;
	unsigned char packed_header[KP_STORAGE_HEADER_SIZE] = {
		0xaa, 0xd0, 0xe5, 0x23, 0x3a, 0xcf, 0xd7, 0xa6,
		0xd0, 0x54, 0x21, 0xc0, 0x6a, 0x26, 0xf8, 0x1b,
		0x96, 0x7f, 0x6d, 0x9b, 0x52, 0x21, 0x1e, 0x1c,
		0x1d, 0x89, 0x49, 0x60, 0xc2, 0x42, 0x3a, 0x0d,
		0xc2, 0x5f, 0xe8, 0x2c, 0xd0, 0xb6, 0x07, 0xcd,
		0x33, 0xd1, 0xbc, 0x2d, 0x2b, 0x4a, 0x5a, 0x84,
		0x69, 0x02, 0x12, 0xa3, 0x6e, 0x22, 0xa3, 0x28,
		0x93, 0x0a, 0xb6, 0xb6,
	};

	/* When */
	kp_storage_header_unpack(&header, packed_header);

	/* Then */
	unsigned char salt[] = {
		0x52, 0x21, 0x1e, 0x1c, 0x1d, 0x89, 0x49, 0x60,
		0xc2, 0x42, 0x3a, 0x0d, 0xc2, 0x5f, 0xe8, 0x2c,
		0xd0, 0xb6, 0x07, 0xcd, 0x33, 0xd1, 0xbc, 0x2d,
		0x2b, 0x4a, 0x5a, 0x84, 0x69, 0x02, 0x12, 0xa3,

	};
	unsigned char nonce[] = {
		0x6e, 0x22, 0xa3, 0x28, 0x93, 0x0a, 0xb6, 0xb6,
	};
	ck_assert_int_eq(header.version, 0xaad0);
	ck_assert_int_eq(header.sodium_version, 0xe523);
	ck_assert(header.opslimit == 0x3acfd7a6d05421c0LL);
	ck_assert(header.memlimit == 0x6a26f81b967f6d9bLL);
	ck_assert_int_eq(memcmp(header.salt, salt, KP_STORAGE_SALT_SIZE), 0);
	ck_assert_int_eq(memcmp(header.nonce, nonce, KP_STORAGE_NONCE_SIZE), 0);
}
END_TEST

START_TEST(test_storage_encrypt_should_be_successful)
{
	/* Given */
	int ret = KP_SUCCESS;
	struct kp_ctx ctx;
	char **password;
	struct kp_storage_header header = KP_STORAGE_HEADER_INIT;
	unsigned char packed_header[KP_STORAGE_HEADER_SIZE] = { 0 };
	unsigned char plain[] = "the quick brown fox jumps over the lobster dog";
	unsigned char cipher[sizeof(plain)+crypto_aead_chacha20poly1305_ABYTES] = { 0 };
	unsigned long long cipher_size;

	header.version = 0xde;
	header.sodium_version = 0xad;
	header.opslimit = crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_INTERACTIVE;
	header.memlimit = crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_INTERACTIVE;
	/* keep header.salt  = { 0 } */
	/* keep header.nonce = { 0 } */

	password = (char **)&ctx.password;
	*password = "test";

	/* When */
	ret |= kp_storage_encrypt(&ctx,
			&header, packed_header, KP_STORAGE_HEADER_SIZE,
			plain, sizeof(plain),
			cipher, &cipher_size);

	/* Then */
	unsigned char ref[] = {
		0x01, 0x82, 0xbf, 0x1f, 0x22, 0x57, 0x3a, 0x63,
		0xa3, 0x22, 0x03, 0x3e, 0x85, 0x73, 0xcd, 0x18,
		0x2d, 0x2a, 0x36, 0xf7, 0x94, 0x95, 0x53, 0x47,
		0x8a, 0x2f, 0x3f, 0xfc, 0xda, 0x88, 0x0e, 0xf1,
		0x04, 0x64, 0xe3, 0xb0, 0xf2, 0xbf, 0xee, 0x20,
		0x3a, 0x79, 0x88, 0x6e, 0x1a, 0x8c, 0xa7, 0xd8,
		0x04, 0x8c, 0x66, 0x60, 0x32, 0xec, 0xc8, 0xa0,
		0xe9, 0x7c, 0x21, 0xd5, 0xf3, 0xfe, 0x44,
	};
	ck_assert_int_eq(ret, KP_SUCCESS);
	ck_assert_int_eq(cipher_size, sizeof(ref));
	ck_assert_int_eq(memcmp(cipher, ref, sizeof(ref)), 0);
}
END_TEST

START_TEST(test_storage_decrypt_should_be_successful)
{
	/* Given */
	int ret = KP_SUCCESS;
	struct kp_ctx ctx;
	char **password;
	struct kp_storage_header header = KP_STORAGE_HEADER_INIT;
	unsigned char packed_header[KP_STORAGE_HEADER_SIZE] = { 0 };
	unsigned char cipher[] = {
		0x01, 0x82, 0xbf, 0x1f, 0x22, 0x57, 0x3a, 0x63,
		0xa3, 0x22, 0x03, 0x3e, 0x85, 0x73, 0xcd, 0x18,
		0x2d, 0x2a, 0x36, 0xf7, 0x94, 0x95, 0x53, 0x47,
		0x8a, 0x2f, 0x3f, 0xfc, 0xda, 0x88, 0x0e, 0xf1,
		0x04, 0x64, 0xe3, 0xb0, 0xf2, 0xbf, 0xee, 0x20,
		0x3a, 0x79, 0x88, 0x6e, 0x1a, 0x8c, 0xa7, 0xd8,
		0x04, 0x8c, 0x66, 0x60, 0x32, 0xec, 0xc8, 0xa0,
		0xe9, 0x7c, 0x21, 0xd5, 0xf3, 0xfe, 0x44,
	};
	unsigned char plain[sizeof(cipher)-crypto_aead_chacha20poly1305_ABYTES] = { 0 };
	unsigned long long plain_size;

	header.version = 0xde;
	header.sodium_version = 0xad;
	header.opslimit = crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_INTERACTIVE;
	header.memlimit = crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_INTERACTIVE;
	/* keep header.salt  = { 0 } */
	/* keep header.nonce = { 0 } */

	password = (char **)&ctx.password;
	*password = "test";

	/* When */
	ret |= kp_storage_decrypt(&ctx,
			&header, packed_header, KP_STORAGE_HEADER_SIZE,
			plain, &plain_size,
			cipher, sizeof(cipher));

	/* Then */
	unsigned char ref[] = "the quick brown fox jumps over the lobster dog";
	ck_assert_int_eq(ret, KP_SUCCESS);
	ck_assert_int_eq(plain_size, sizeof(ref));
	ck_assert_str_eq((char *)plain, (char *)ref);
}
END_TEST

int
main(int argc, char **argv)
{
	int number_failed;

	Suite *suite = suite_create("storage_test_suite");
	TCase *tcase = tcase_create("case");
	tcase_add_test(tcase, test_storage_header_pack_should_be_successful);
	tcase_add_test(tcase, test_storage_header_unpack_should_be_successful);
	tcase_add_test(tcase, test_storage_encrypt_should_be_successful);
	tcase_add_test(tcase, test_storage_decrypt_should_be_successful);
	suite_add_tcase(suite, tcase);

	SRunner *runner = srunner_create(suite);
	srunner_set_fork_status(runner, CK_NOFORK);
	srunner_run_all(runner, CK_VERBOSE);
	number_failed = srunner_ntests_failed(runner);
	srunner_free(runner);

	return number_failed;
}
