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

#include <sys/stat.h>

#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "editor.h"
#include "error.h"
#include "log.h"
#include "safe.h"
#include "storage.h"

kp_error_t
kp_safe_open(struct kp_storage_ctx *ctx, const char *path, struct kp_safe *safe)
{
	struct stat stats;

	if (strlcpy(safe->path, path, PATH_MAX) >= PATH_MAX) return KP_ENOMEM;
	safe->open = false;

	if (stat(path, &stats) == 0) {
		LOGE("unknown safe");
		return KP_EINPUT;
	}

	return KP_NYI;
}

kp_error_t
kp_safe_create(struct kp_storage_ctx *ctx, const char *path, struct kp_safe *safe)
{
	kp_error_t ret;
	struct stat stats;

	if (strlcpy(safe->path, path, PATH_MAX) >= PATH_MAX) return KP_ENOMEM;
	safe->open = true;

	if (stat(path, &stats) == 0) {
		LOGE("safe %s already exists", path);
		return KP_EINPUT;
	} else if (errno != ENOENT) {
		LOGE("cannot create safe %s: %s (%d)", path, strerror(errno), errno);
		return KP_EINPUT;
	}

	if ((ret = kp_editor_get_tmp(ctx, safe, true)) != KP_SUCCESS) {
		return ret;
	}

	dprintf(safe->plain.fd, "%s", KP_SAFE_TEMPLATE);

	if ((ret = kp_editor_open(safe)) != KP_SUCCESS) {
		LOGE("cannot edit safe");
		return ret;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_safe_edit(struct kp_storage_ctx *ctx, struct kp_safe *safe)
{
	if (safe->open) {
		LOGE("cannot edit closed safe");
		return KP_EINPUT;
	}

	return KP_NYI;
}

kp_error_t
kp_safe_close(struct kp_storage_ctx *ctx, struct kp_safe *safe)
{
	if (!safe->open) {
		LOGW("safe already closed");
		return KP_EINPUT;
	}

	return KP_NYI;
}
