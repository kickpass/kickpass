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

#include "../lib/storage.c"
#include "../lib/safe.c"

START_TEST(test_safe_init)
{
	/* Given */
	struct kp_ctx  ctx;
	struct kp_safe safe;
	char path[PATH_MAX];

	strlcpy(ctx.ws_path, "/home/user/.kickpass", PATH_MAX);

	/* When */
	kp_safe_init(&ctx, &safe, "dir/safe");

	/* Then */
	kp_safe_get_path(&ctx, &safe, path, PATH_MAX);
	ck_assert_str_eq(path, "/home/user/.kickpass/dir/safe");
}
END_TEST

int
main(int argc, char **argv)
{
	int number_failed;

	Suite *suite = suite_create("safe_test_suite");
	TCase *tcase = tcase_create("case");
	tcase_add_test(tcase, test_safe_init);
	suite_add_tcase(suite, tcase);

	SRunner *runner = srunner_create(suite);
	srunner_set_fork_status(runner, CK_NOFORK);
	srunner_run_all(runner, CK_VERBOSE);
	number_failed = srunner_ntests_failed(runner);
	srunner_free(runner);

	return number_failed;
}
