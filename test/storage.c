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

#include "check_improved.h"

#include "../src/storage.c"

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

	ret = kp_storage_init(&ctx);

	/* When */
	ret |= kp_storage_save(ctx, "intest", "outtest");

	/* Then */
	ck_assert_int_eq(ret, KP_SUCCESS);

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
	suite_add_tcase(suite, tcase);

	SRunner *runner = srunner_create(suite);
	srunner_run_all(runner, CK_VERBOSE);
	number_failed = srunner_ntests_failed(runner);
	srunner_free(runner);

	return number_failed;
}
