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

#include "../src/storage.c"

START_TEST(test_storage_encrypt_should_be_successful)
{
	/* Given */
	int ret = KP_SUCCESS;
	struct kp_ctx ctx;
	unsigned char plain[] = "test", cipher[256] = { 0 };
	struct kp_storage_header header;

	header.version = 0xde;
	header.sodium_version = 0xad;
	header.opslimit = crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_INTERACTIVE;
	header.memlimit = crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_INTERACTIVE;

	ctx.password = "test";

	/* When */
	ret |= kp_storage_encrypt(&ctx, &header, plain, cipher, sizeof(plain));

	/* Then */
	ck_assert_int_eq(ret, KP_SUCCESS);
	ck_assert(header.data);
	ck_assert_int_gt(header.data_size, 0);
	free(header.data);
}
END_TEST

START_TEST(test_storage_decrypt_should_be_successful)
{
	/* Given */
	int ret = KP_SUCCESS;
	struct kp_ctx ctx;
	struct kp_storage_header header;
	unsigned char plain[256], cipher[] = {
		0x75, 0xbd, 0x17, 0x20, 0x9f, 0xa6, 0xbf, 0x19,
		0x89, 0xdd, 0x33, 0x38, 0x84, 0x74, 0xc9, 0x07,
		0x93, 0xbe, 0xc6, 0x67, 0xaa,
	};
	unsigned char hdata[] = {
		0xb6, 0xa9, 0xb3, 0x8c, 0x10, 0x47, 0xde, 0x9e,
		0xdd, 0x5d, 0xf4, 0x38, 0x36, 0x83, 0x67, 0x9b,
		0x8b, 0x9c, 0x10, 0xab, 0x84, 0x75, 0x7a, 0xc7,
		0x04, 0xee, 0x64, 0xe5, 0xda, 0x5c, 0x90, 0x7d,
		0x34, 0x82, 0x2c, 0x10, 0x58, 0xc9, 0x63, 0x69,
		0xc4, 0x78, 0xce, 0xc1, 0x73, 0x55, 0xcd, 0x0d,
		0x9b, 0x33, 0x17, 0x44, 0x41, 0xfc, 0x45, 0xcc,
	};

	header.version = 0xde;
	header.sodium_version = 0xad;
	header.opslimit = crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_INTERACTIVE;
	header.memlimit = crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_INTERACTIVE;
	header.data = hdata;
	header.data_size = sizeof(hdata);

	ctx.password = "test";

	/* When */
	ret |= kp_storage_decrypt(&ctx, &header, plain, cipher, sizeof(cipher));

	/* Then */
	ck_assert_int_eq(ret, KP_SUCCESS);
	ck_assert_str_eq(plain, "test");
}
END_TEST

int
main(int argc, char **argv)
{
	int number_failed;

	Suite *suite = suite_create("storage_test_suite");
	TCase *tcase = tcase_create("case");
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
