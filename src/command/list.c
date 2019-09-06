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

#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kickpass.h"

#include "command.h"
#include "list.h"
#include "log.h"

#ifndef HAS_REALLOCARRAY
#include "reallocarray.h"
#endif

static kp_error_t list(struct kp_ctx *, int, char **);
static kp_error_t list_dir(struct kp_ctx *, char *, char *, bool);
static kp_error_t list_dir_r(char ***, int *, char *);
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
		list_dir(ctx, ctx->ws_path, "", false);
	}

	for (i = optind; i < argc; i++) {
		char path[PATH_MAX];

		if (strlcpy(path, ctx->ws_path, PATH_MAX) >= PATH_MAX) {
			errno = ENOMEM;
			return KP_ERRNO;
		}

		if (strlcat(path, "/", PATH_MAX) >= PATH_MAX) {
			errno = ENOMEM;
			return KP_ERRNO;
		}
		if (strlcat(path, argv[i],  PATH_MAX) >= PATH_MAX) {
			errno = ENOMEM;
			return KP_ERRNO;
		}

		list_dir(ctx, path, "  ", true);
	}

	return KP_SUCCESS;
}

static kp_error_t
list_dir_r(char ***safes, int *nsafes, char *root)
{
	kp_error_t ret = KP_SUCCESS;
	DIR *dirp;
	struct dirent *dirent;

	if ((dirp = opendir(root)) == NULL) {
		ret = KP_ERRNO;
		kp_warn(ret, "cannot open dir %s", root);
		return ret;
	}

	while ((dirent = readdir(dirp)) != NULL) {
		char path[PATH_MAX];
		if (dirent->d_name[0] == '.'
				|| (dirent->d_type != DT_REG
				&& dirent->d_type != DT_DIR)) {
			continue;
		}

		if (strlcpy(path, root, PATH_MAX) >= PATH_MAX) {
			errno = ENOMEM;
			ret = KP_ERRNO;
			kp_warn(ret, "memory error");
			goto out;
		}

		if (strlcat(path, "/", PATH_MAX) >= PATH_MAX) {
			errno = ENOMEM;
			ret = KP_ERRNO;
			kp_warn(ret, "memory error");
			goto out;
		}

		if (strlcat(path, dirent->d_name, PATH_MAX) >= PATH_MAX) {
			errno = ENOMEM;
			ret = KP_ERRNO;
			kp_warn(ret, "memory error");
			goto out;
		}

		switch (dirent->d_type) {
		case DT_REG:
			if ((*safes = reallocarray(*safes, *nsafes + 1, sizeof(char *)))
					== NULL) {
				errno = ENOMEM;
				ret = KP_ERRNO;
				kp_warn(ret, "memory error");
				goto out;
			}

			(*safes)[*nsafes] = strndup(path, PATH_MAX);
			if ((*safes)[*nsafes] == NULL) {
				errno = ENOMEM;
				ret = KP_ERRNO;
				kp_warn(ret, "memory error");
				goto out;
			}

			(*nsafes)++;
			break;
		case DT_DIR:
			ret = list_dir_r(safes, nsafes, path);
		}
	}

out:
	closedir(dirp);

	return ret;
}

static kp_error_t
list_dir(struct kp_ctx *ctx, char *root, char *indent, bool print_path)
{
	kp_error_t ret;
	char **safes = NULL;
	int nsafes = 0, i = 0;
	size_t ignore;

	safes = calloc(nsafes, sizeof(char *));

	if ((ret = list_dir_r(&safes, &nsafes, root)) != KP_SUCCESS) {
		goto out;
	}

	qsort(safes, nsafes, sizeof(char *), path_sort);

	if (print_path) {
		printf("%s/\n", root + strlen(ctx->ws_path) + 1);
	}
	ignore = strlen(root) + 1;
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
