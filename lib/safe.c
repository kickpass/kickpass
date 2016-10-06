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
#include "kpagent.h"

static kp_error_t kp_safe_init(struct kp_safe *, const char *, bool, bool);

/*
 * Load an existing safe.
 * Init the kp_safe structure and open the cipher file.
 * The returned safe is closed.
 */
kp_error_t
kp_safe_load(struct kp_ctx *ctx, struct kp_safe *safe, const char *name)
{
	kp_error_t ret;
	struct stat stats;
	char path[PATH_MAX];

	if ((ret = kp_safe_init(safe, name, false, false)) != KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_safe_get_path(ctx, safe, path, PATH_MAX)) != KP_SUCCESS) {
		return ret;
	}

	if (stat(path, &stats) != 0) {
		return KP_ERRNO;
	}

	safe->cipher = open(path, O_RDWR | O_NONBLOCK);
	if (safe->cipher < 0) {
		return KP_ERRNO;
	}

	return KP_SUCCESS;
}

static kp_error_t
kp_safe_mkdir(struct kp_ctx *ctx, char *path)
{
	struct stat  stats;
	char   *rdir;

	rdir = path + strlen(ctx->ws_path);
	rdir++; /* Skip first / as it is part of the ctx->ws */
	while ((rdir = strchr(rdir, '/'))) {
		*rdir = '\0';
		if (stat(path, &stats) != 0) {
			if (errno == ENOENT) {
				if (mkdir(path, 0700) < 0) {
					return errno;
				}
			} else {
				return errno;
			}
		}
		*rdir = '/';
		rdir++;
	}
	return KP_SUCCESS;
}

/*
 * Create a new safe.
 * The returned safe is opened.
 */
kp_error_t
kp_safe_create(struct kp_ctx *ctx, struct kp_safe *safe, const char *name)
{
	kp_error_t   ret;
	struct stat  stats;
	char         path[PATH_MAX];

	if ((ret = kp_safe_init(safe, name, true, false)) != KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_safe_get_path(ctx, safe, path, PATH_MAX)) != KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_safe_mkdir(ctx, path)) != KP_SUCCESS) {
		return ret;
	}

	if (stat(path, &stats) == 0) {
		errno = EEXIST;
		return KP_ERRNO;
	} else if (errno != ENOENT) {
		return errno;
	}

	safe->cipher = open(path, O_RDWR | O_NONBLOCK | O_CREAT, S_IRUSR | S_IWUSR);
	if (safe->cipher < 0) {
		return errno;
	}

	return KP_SUCCESS;
}

/*
 * Open a safe.
 */
kp_error_t
kp_safe_open(struct kp_ctx *ctx, struct kp_safe *safe)
{
	/* handle not connect or not found and ask password */
	if (ctx->agent.connected) {
		kp_error_t ret;
		struct kp_unsafe unsafe;
		char path[PATH_MAX];

		if ((ret = kp_safe_get_path(ctx, safe, path, PATH_MAX)) != KP_SUCCESS) {
			return ret;
		}

		if ((ret = kp_agent_send(&ctx->agent, KP_MSG_SEARCH, path,
		    PATH_MAX)) != KP_SUCCESS) {
			/* TODO log reason in verbose mode */
			goto fallback;
		}

		if ((ret = kp_agent_receive(&ctx->agent, KP_MSG_SEARCH, &unsafe,
		    sizeof(struct kp_unsafe))) != KP_SUCCESS) {
			/* TODO log reason in verbose mode */
			goto fallback;
		}

		if (strlcpy(safe->password, unsafe.password, KP_PASSWORD_MAX_LEN)
		    >= KP_PASSWORD_MAX_LEN) {
			errno = ENOMEM;
			return KP_ERRNO;
		}
		if (strlcpy(safe->metadata, unsafe.metadata, KP_METADATA_MAX_LEN)
		    >= KP_METADATA_MAX_LEN) {
			errno = ENOMEM;
			return KP_ERRNO;
		}

		return KP_SUCCESS;
	}

fallback:
	if (ctx->password[0] == '\0') {
		kp_error_t ret;
		if ((ret = ctx->password_cb(ctx)) != KP_SUCCESS) {
			return ret;
		}
	}
	return kp_storage_open(ctx, safe);
}

/*
 * Save a safe.
 */
kp_error_t
kp_safe_save(struct kp_ctx *ctx, struct kp_safe *safe)
{
	return kp_storage_save(ctx, safe);
}

/*
 * Close a safe.
 * Take care of cleaning the safe plain text and closing the opened file
 * descriptor.
 */
kp_error_t
kp_safe_close(struct kp_ctx *ctx, struct kp_safe *safe)
{
	sodium_free((char *)safe->password);
	sodium_free((char *)safe->metadata);

	if (close(safe->cipher) < 0) {
		return errno;
	}

	safe->cipher = 0;

	return KP_SUCCESS;
}

static kp_error_t
kp_safe_init(struct kp_safe *safe, const char *name, bool open, bool ro)
{
	char **password;
	char **metadata;

	/* Init is the only able to set password and metadata memory */
	password = (char **)&safe->password;
	metadata = (char **)&safe->metadata;

	if (strlcpy(safe->name, name, PATH_MAX) >= PATH_MAX) {
		errno = ENOMEM;
		return KP_ERRNO;
	}

	safe->open = open;
	*password = sodium_malloc(KP_PASSWORD_MAX_LEN);
	safe->password[0] = '\0';
	*metadata = sodium_malloc(KP_METADATA_MAX_LEN);
	safe->metadata[0] = '\0';

	return KP_SUCCESS;
}

kp_error_t
kp_safe_rename(struct kp_ctx *ctx, struct kp_safe *safe, const char *name)
{
	kp_error_t ret;
	char oldpath[PATH_MAX], newpath[PATH_MAX];

	if ((ret = kp_safe_get_path(ctx, safe, oldpath, PATH_MAX)) != KP_SUCCESS) {
		return ret;
	}

	if (strlcpy(safe->name, name, PATH_MAX) >= PATH_MAX) {
		errno = ENOMEM;
		return KP_ERRNO;
	}

	if ((ret = kp_safe_get_path(ctx, safe, newpath, PATH_MAX)) != KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_safe_mkdir(ctx, newpath)) != KP_SUCCESS) {
		return ret;
	}

	if (rename(oldpath, newpath) != 0) {
		return errno;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_safe_get_path(struct kp_ctx *ctx, struct kp_safe *safe, char *path, size_t size)
{

	if (strlcpy(path, ctx->ws_path, size) >= size) {
		errno = ENOMEM;
		return KP_ERRNO;
	}

	if (strlcat(path, "/", size) >= size) {
		errno = ENOMEM;
		return KP_ERRNO;
	}

	if (strlcat(path, safe->name, size) >= size) {
		errno = ENOMEM;
		return KP_ERRNO;
	}

	return KP_SUCCESS;
}
