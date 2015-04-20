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

#include <assert.h>
#include <fcntl.h>
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kickpass.h"

#include "editor.h"
#include "safe.h"
#include "storage.h"

static kp_error_t kp_safe_init(struct kp_safe *, const char *, bool);

/*
 * Load an existing safe.
 * Init the kp_safe structure and open the cipher file.
 * The returned safe is closed.
 */
kp_error_t
kp_safe_load(struct kp_ctx *ctx, struct kp_safe *safe, const char *path)
{
	kp_error_t ret;
	struct stat stats;

	if ((ret = kp_safe_init(safe, path, false)) != KP_SUCCESS) {
		return ret;
	}

	if (stat(path, &stats) != 0) {
		warn("unknown safe %s", path);
		return KP_EINPUT;
	}

	safe->cipher = open(safe->path, O_RDWR | O_NONBLOCK);
	if (safe->cipher < 0) {
		warn("cannot open safe %s", safe->path);
		return KP_EINPUT;
	}

	return KP_SUCCESS;
}

/*
 * Create a new safe.
 * The returned safe is opened.
 */
kp_error_t
kp_safe_create(struct kp_ctx *ctx, struct kp_safe *safe, const char *path)
{
	kp_error_t ret;
	struct stat stats;

	if ((ret = kp_safe_init(safe, path, true)) != KP_SUCCESS) {
		return ret;
	}

	if (stat(path, &stats) == 0) {
		warnx("safe %s already exists", path);
		return KP_EEXIST;
	} else if (errno != ENOENT) {
		warn("cannot create safe %s", path);
		return KP_EINPUT;
	}

	safe->cipher = open(safe->path, O_RDWR | O_NONBLOCK | O_CREAT, S_IRUSR | S_IWUSR);
	if (safe->cipher < 0) {
		warn("cannot open safe %s", safe->path);
		return KP_EINPUT;
	}

	memcpy(safe->plain, KP_SAFE_TEMPLATE, sizeof(KP_SAFE_TEMPLATE));
	safe->plain_size = sizeof(KP_SAFE_TEMPLATE)-1;

	return KP_SUCCESS;
}

/*
 * Close a safe.
 * Take care of cleaning the safe plain text and closing the opened file
 * descriptor.
 */
kp_error_t
kp_safe_close(struct kp_ctx *ctx, struct kp_safe *safe)
{
	if (close(safe->cipher) < 0) {
		warn("cannot close safe");
		return errno;
	}

	safe->cipher = 0;

	sodium_free(safe->plain);

	return KP_SUCCESS;
}

static kp_error_t
kp_safe_init(struct kp_safe *safe, const char *path, bool open)
{
	if (strlcpy(safe->path, path, PATH_MAX) >= PATH_MAX) {
		warnx("memory error");
		return KP_ENOMEM;
	}

	safe->open = open;
	safe->plain = sodium_malloc(KP_PLAIN_MAX_SIZE);

	return KP_SUCCESS;
}

size_t
kp_safe_password_len(const struct kp_safe *safe)
{
	char *end;

	assert(safe);
	assert(safe->open);

	if ((end = strchr((char *)safe->plain, '\r')) != NULL) {
		goto out;
	}

	if ((end = strchr((char *)safe->plain, '\n')) != NULL) {
		goto out;
	}

	return strlen((char *)safe->plain);

out:
	return end - (char *)safe->plain;
}
