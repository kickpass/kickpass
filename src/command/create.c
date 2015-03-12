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

#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>

#include "kickpass.h"

#include "command.h"
#include "create.h"
#include "editor.h"
#include "safe.h"
#include "storage.h"

static kp_error_t create(struct kp_ctx *ctx, int argc, char **argv);
static kp_error_t usage(void);

struct kp_cmd kp_cmd_create = {
	.main  = create,
	.usage = usage,
};

kp_error_t
create(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret = KP_SUCCESS;
	char path[PATH_MAX];
	struct kp_storage *storage;
	struct kp_safe safe;

	if (argc - optind != 1) {
		LOGE("missing safe name");
		return KP_EINPUT;
	}

	if ((ret = kp_storage_init(ctx, &storage)) != KP_SUCCESS) return ret;

	if (strlcpy(path, ctx->ws_path, PATH_MAX) >= PATH_MAX) {
		LOGE("memory error");
		ret = KP_ENOMEM;
		goto out;
	}

	if (strlcat(path, "/", PATH_MAX) >= PATH_MAX) {
		LOGE("memory error");
		ret = KP_ENOMEM;
		goto out;
	}

	if (strlcat(path, argv[optind], PATH_MAX) >= PATH_MAX) {
		LOGE("memory error");
		ret = KP_ENOMEM;
		goto out;
	}

	if ((ret = kp_safe_create(ctx, path, &safe)) != KP_SUCCESS) {
		if (ret == KP_EEXIST) {
			LOGI("please use edit command to edit an existing safe");
		} else {
			LOGE("cannot create safe");
		}
		goto out;
	}

	if ((ret = kp_editor_open(&safe)) != KP_SUCCESS) {
		LOGE("cannot edit safe");
		return ret;
	}

	if ((ret = kp_storage_save(storage, &safe)) != KP_SUCCESS) {
		LOGE("cannot save safe");
		return ret;
	}

out:
	ret = kp_storage_fini(storage);
	return ret;
}

kp_error_t
usage(void)
{
	printf("    %-10s%s\n", "create", "Create a new password safe");
	return KP_SUCCESS;
}
