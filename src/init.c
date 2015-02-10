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
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "error.h"
#include "log.h"
#include "storage.h"

kp_error_t
kp_cmd_init(int argc, char **argv)
{
	kp_error_t ret = KP_SUCCESS;
	char path[PATH_MAX];
	struct kp_storage_ctx *ctx;
	struct stat stats;

	ret = kp_storage_init(&ctx);
	if (ret != KP_SUCCESS) return ret;

	ret = kp_storage_get_path(ctx, path, PATH_MAX);
	if (ret != KP_SUCCESS) return ret;

	if (stat(path, &stats) == 0) {
		LOGW("workspace already exists");
		ret = KP_EINPUT;
		goto out;
	} else if (errno & ENOENT) {
		LOGI("creating workspace %s", path);
		mkdir(path, 0700);
	} else {
		LOGE("invalid workspace %s: %s (%d)", path, errno, strerror(errno));
		ret = errno;
		goto out;
	}

out:
	ret = kp_storage_fini(ctx);
	return ret;
}
