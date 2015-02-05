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
#include <gpgme.h>

#include "check_compat.h"

#include "../src/storage.c"

gpgme_error_t gpgme_passphrase_cb(void *hook, const char *uid_hint, const char *passphrase_info, int prev_was_bad, int fd)
{
	const char passwd[] = "test\n";
	gpgme_io_write(fd, passwd, sizeof(passwd)-1);
	return 0;
}

START_TEST(test_storage_should_init)
{
	/* Given */
	int ret = KP_SUCCESS;
	struct kp_storage_ctx *ctx;

	/* When */
	ret |= kp_storage_init(&ctx);

	/* Then */
	ck_assert_int_eq(ret, KP_SUCCESS);
	ck_assert_ptr_ne(ctx, NULL);

	/* Cleanup */
	kp_storage_fini(ctx);
}
END_TEST

START_TEST(test_storage_should_provide_engine_and_version)
{
	/* Given */
	char engine[10];
	char version[10];
	int ret = KP_SUCCESS;
	struct kp_storage_ctx *ctx;

	ret = kp_storage_init(&ctx);

	/* When */
	ret |= kp_storage_get_engine(ctx, engine, sizeof(engine));
	ret |= kp_storage_get_version(ctx, version, sizeof(version));

	/* Then */
	ck_assert_int_eq(ret, KP_SUCCESS);
	ck_assert_str_eq(engine, "gpg");
	ck_assert_int_gt(strlen(version), 0);

	/* Cleanup */
	kp_storage_fini(ctx);
}
END_TEST

START_TEST(test_storage_save_should_be_successful)
{
	/* Given */
	char engine[10];
	char version[10];
	int ret = KP_SUCCESS;
	struct kp_storage_ctx *ctx;
	gpgme_data_t plain, cipher;
	char plaintext[] = "test";
	char ciphertext[256] = { 0 };
#define CIPHER_HEADER "-----BEGIN PGP MESSAGE-----"

	ret = kp_storage_init(&ctx);
	gpgme_data_new_from_mem(&plain, plaintext, sizeof(plaintext), 1);
	gpgme_data_new_from_mem(&cipher, ciphertext, sizeof(ciphertext), 0);
	gpgme_set_passphrase_cb(ctx->gpgme_ctx, gpgme_passphrase_cb, NULL);

	/* When */
	ret |= kp_storage_save(ctx, plain, cipher);

	/* Then */
	ck_assert_int_eq(ret, KP_SUCCESS);
	gpgme_data_seek(cipher, 0, SEEK_SET);
	gpgme_data_read(cipher, ciphertext, strlen(CIPHER_HEADER));
	ck_assert_str_eq(ciphertext, CIPHER_HEADER);

	/* Cleanup */
	kp_storage_fini(ctx);
#undef CIPHER_HEADER
}
END_TEST

START_TEST(test_storage_open_should_be_successful)
{
	/* Given */
	char engine[10];
	char version[10];
	int ret = KP_SUCCESS;
	struct kp_storage_ctx *ctx;
	gpgme_data_t plain, cipher;
	char plaintext[5] = { 0 };
	char ciphertext[] =
		"-----BEGIN PGP MESSAGE-----\n"
		"Version: GnuPG v2\n"
		"\n"
		"jA0EBwMC19HAbfCKnrC+0joB2oeqgmUTMsDnJrRlZ1KmSr2FtpUCa+ikN2sas+87\n"
		"MQRE3V5DijqMMDbVnVXhC8K4u/jHP4MXUAJa\n"
		"=Lmrh\n"
		"-----END PGP MESSAGE-----";

	ret = kp_storage_init(&ctx);
	gpgme_data_new_from_mem(&plain, plaintext, sizeof(plaintext), 1);
	gpgme_data_new_from_mem(&cipher, ciphertext, sizeof(ciphertext), 0);
	gpgme_set_passphrase_cb(ctx->gpgme_ctx, gpgme_passphrase_cb, NULL);

	/* When */
	ret |= kp_storage_open(ctx, cipher, plain);

	/* Then */
	ck_assert_int_eq(ret, KP_SUCCESS);
	ck_assert_int_eq(gpgme_data_seek(plain, 0, SEEK_CUR), sizeof(plaintext));
	gpgme_data_seek(plain, 0, SEEK_SET);
	gpgme_data_read(plain, plaintext, sizeof(plaintext));
	ck_assert_str_eq(plaintext, "test");

	/* Cleanup */
	kp_storage_fini(ctx);
}
END_TEST

int
main(int argc, char **argv)
{
	int number_failed;

	Suite *suite = suite_create("storage_test_suite");
	TCase *tcase = tcase_create("case");
	tcase_add_test(tcase, test_storage_should_init);
	tcase_add_test(tcase, test_storage_should_provide_engine_and_version);
	tcase_add_test(tcase, test_storage_save_should_be_successful);
	tcase_add_test(tcase, test_storage_open_should_be_successful);
	suite_add_tcase(suite, tcase);

	SRunner *runner = srunner_create(suite);
	srunner_run_all(runner, CK_VERBOSE);
	number_failed = srunner_ntests_failed(runner);
	srunner_free(runner);

	return number_failed;
}
