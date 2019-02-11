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

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kickpass.h"

#include "command.h"
#include "list.h"
#include "log.h"

static kp_error_t list(struct kp_ctx *, int, char **);
static kp_error_t list_dir(struct kp_ctx *, char *, char *, bool);
static int        path_sort(const void *, const void *);

struct kp_cmd kp_cmd_list = {
	.main  = list,
	.usage = NULL,
	.opts  = "list",
	.desc  = "List available safes",
};

static kp_error_t
list(struct kp_ctx *ctx, int argc, char **argv)
{
	int i;

	if (argc == optind) {
		list_dir(ctx, "", "", false);
	}

	for (i = optind; i < argc; i++) {
		list_dir(ctx, argv[i], "  ", true);
	}

	return KP_SUCCESS;
}

static kp_error_t
list_dir(struct kp_ctx *ctx, char *root, char *indent, bool print_path)
{
	kp_error_t ret;
	char **safes = NULL;
	int nsafes = 0, i = 0;
	size_t ignore;

	safes = calloc(nsafes, sizeof(char *));

	if ((ret = kp_list(ctx, &safes, &nsafes, root)) != KP_SUCCESS) {
		goto out;
	}

	qsort(safes, nsafes, sizeof(char *), path_sort);

	if (print_path) {
		printf("%s/\n", root);
	}
	ignore = strlen(root);
	if (ignore > 0) {
		ignore++; /* Ignore dir separator */
	}
	for (i = 0; i < nsafes; i++) {
		printf("%s%s\n", indent, safes[i] + ignore);
	}

out:
	for (i = 0; i < nsafes; i++) {
		free(safes[i]);
	}
	free(safes);

	return ret;
}

static int
path_sort(const void *a, const void *b)
{
	return strncmp(*(const char **)a, *(const char **)b, PATH_MAX);
}
