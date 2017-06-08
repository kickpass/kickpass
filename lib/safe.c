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

static kp_error_t kp_safe_mkdir(struct kp_ctx *, char *);

kp_error_t
kp_safe_init(struct kp_ctx *ctx, struct kp_safe *safe, const char *name)
{
	char **password;
	char **metadata;

	assert(ctx);
	assert(safe);
	assert(name);

	safe->open = false;

	password = (char **)&safe->password;
	metadata = (char **)&safe->metadata;

	*password = NULL;
	*metadata = NULL;

	if (strlcpy(safe->name, name, PATH_MAX) >= PATH_MAX) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_safe_open(struct kp_ctx *ctx, struct kp_safe *safe, int flags)
{
	kp_error_t ret;
	char **password;
	char **metadata;
	char path[PATH_MAX] = "";

	assert(ctx);
	assert(safe);
	assert(!safe->open);

	password = (char **)&safe->password;
	metadata = (char **)&safe->metadata;

	if ((ret = kp_safe_get_path(ctx, safe, path, PATH_MAX)) != KP_SUCCESS) {
		return ret;
	}

	safe->open = true;

	*password = sodium_malloc(KP_PASSWORD_MAX_LEN);
	safe->password[0] = '\0';
	*metadata = sodium_malloc(KP_METADATA_MAX_LEN);
	safe->metadata[0] = '\0';

	if (KP_CREATE & flags) {
		struct stat stats;

		if ((ret = kp_safe_mkdir(ctx, path)) != KP_SUCCESS) {
			return ret;
		}

		if (stat(path, &stats) == 0) {
			errno = EEXIST;
			return KP_ERRNO;
		} else if (errno != ENOENT) {
			return KP_ERRNO;
		}

		return KP_SUCCESS;
	}

	if (!(KP_FORCE & flags) && ctx->agent.connected) {
		struct kp_unsafe unsafe = KP_UNSAFE_INIT;

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
		if ((ret = kp_password_prompt(ctx, false,
		                              (char *)ctx->password,
		                              "master")) != KP_SUCCESS) {
			return ret;
		}
	}

	return kp_storage_open(ctx, safe);
}

kp_error_t
kp_safe_save(struct kp_ctx *ctx, struct kp_safe *safe)
{
	assert(ctx);
	assert(safe);
	assert(safe->open);

	if (ctx->password[0] == '\0') {
		kp_error_t ret;
		if ((ret = kp_password_prompt(ctx, false,
		                              (char *)ctx->password,
		                              "master")) != KP_SUCCESS) {
			return ret;
		}
	}

	if (ctx->agent.connected) {
		kp_error_t ret;
		struct kp_unsafe unsafe = KP_UNSAFE_INIT;
		char path[PATH_MAX] = "";

		if ((ret = kp_safe_get_path(ctx, safe, path, PATH_MAX)) != KP_SUCCESS) {
			return ret;
		}

		if ((ret = kp_agent_send(&ctx->agent, KP_MSG_SEARCH, path,
		    PATH_MAX)) != KP_SUCCESS) {
			/* TODO log reason in verbose mode */
			goto finally;
		}

		if ((ret = kp_agent_receive(&ctx->agent, KP_MSG_SEARCH, &unsafe,
		    sizeof(struct kp_unsafe))) != KP_SUCCESS) {
			if (ret != KP_ERRNO || errno != ENOENT) {
				/* TODO log reason in verbose mode */
			}
			goto finally;
		}

		if (strlcpy(unsafe.password, safe->password,
			    KP_PASSWORD_MAX_LEN) >= KP_PASSWORD_MAX_LEN) {
			errno = ENOMEM;
			goto finally;
		}
		if (strlcpy(unsafe.metadata, safe->metadata,
			    KP_METADATA_MAX_LEN) >= KP_METADATA_MAX_LEN) {
			errno = ENOMEM;
			goto finally;
		}

		if ((ret = kp_agent_send(&ctx->agent, KP_MSG_STORE, &unsafe,
		   sizeof(struct kp_unsafe))) != KP_SUCCESS) {
			/* TODO log reason in verbose mode */
			goto finally;
		}

	}

finally:
	return kp_storage_save(ctx, safe);
}

kp_error_t
kp_safe_close(struct kp_ctx *ctx, struct kp_safe *safe)
{
	assert(ctx);
	assert(safe);

	sodium_free((char *)safe->password);
	sodium_free((char *)safe->metadata);

	safe->open = false;

	return KP_SUCCESS;
}

kp_error_t
kp_safe_delete(struct kp_ctx *ctx, struct kp_safe *safe)
{
	kp_error_t ret;
	char path[PATH_MAX] = "";

	assert(ctx);
	assert(safe);
	assert(safe->open);

	if ((ret = kp_safe_get_path(ctx, safe, path, PATH_MAX)) != KP_SUCCESS) {
		return ret;
	}

	if (ctx->agent.connected) {
		bool result;

		if ((ret = kp_agent_send(&ctx->agent, KP_MSG_DISCARD, path,
		    PATH_MAX)) != KP_SUCCESS) {
			/* TODO log reason in verbose mode */
			return ret;
		}

		if ((ret = kp_agent_receive(&ctx->agent, KP_MSG_DISCARD,
		    &result, sizeof(bool))) != KP_SUCCESS) {
			/* TODO log reason in verbose mode */
			return ret;
		}
	}

	if (unlink(path) != 0) {
		ret = KP_ERRNO;
		return ret;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_safe_rename(struct kp_ctx *ctx, struct kp_safe *safe, const char *name)
{
	kp_error_t ret;
	char oldpath[PATH_MAX] = "", newpath[PATH_MAX] = "";

	assert(ctx);
	assert(safe);
	assert(name);

	if ((ret = kp_safe_get_path(ctx, safe, oldpath, PATH_MAX)) != KP_SUCCESS) {
		return ret;
	}

	if (strlcpy(safe->name, name, PATH_MAX) >= PATH_MAX) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	if ((ret = kp_safe_get_path(ctx, safe, newpath, PATH_MAX)) != KP_SUCCESS) {
		return ret;
	}

	if (ctx->agent.connected) {
		struct kp_unsafe unsafe = KP_UNSAFE_INIT;
		bool result;

		if ((ret = kp_agent_send(&ctx->agent, KP_MSG_DISCARD, oldpath,
		    PATH_MAX)) != KP_SUCCESS) {
			/* TODO log reason in verbose mode */
			goto finally;
		}

		if ((ret = kp_agent_receive(&ctx->agent, KP_MSG_DISCARD,
		    &result, sizeof(bool))) != KP_SUCCESS) {
			if (ret != KP_ERRNO || errno != ENOENT) {
				/* TODO log reason in verbose mode */
			}
			goto finally;
		}

		if (strlcpy(unsafe.path, newpath, PATH_MAX) >= PATH_MAX) {
			errno = ENAMETOOLONG;
			goto finally;
		}

		if (strlcpy(unsafe.password, safe->password,
			    KP_PASSWORD_MAX_LEN) >= KP_PASSWORD_MAX_LEN) {
			errno = ENOMEM;
			goto finally;
		}
		if (strlcpy(unsafe.metadata, safe->metadata,
			    KP_METADATA_MAX_LEN) >= KP_METADATA_MAX_LEN) {
			errno = ENOMEM;
			goto finally;
		}

		if ((ret = kp_agent_send(&ctx->agent, KP_MSG_STORE, &unsafe,
		   sizeof(struct kp_unsafe))) != KP_SUCCESS) {
			/* TODO log reason in verbose mode */
			goto finally;
		}
	}


finally:
	if ((ret = kp_safe_mkdir(ctx, newpath)) != KP_SUCCESS) {
		return ret;
	}

	if (rename(oldpath, newpath) != 0) {
		return KP_ERRNO;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_safe_get_path(struct kp_ctx *ctx, struct kp_safe *safe, char *path, size_t size)
{
	assert(ctx);
	assert(safe);
	assert(path);

	if (strlcpy(path, ctx->ws_path, size) >= size) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	if (strlcat(path, "/", size) >= size) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	if (strlcat(path, safe->name, size) >= size) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_safe_store(struct kp_ctx *ctx, struct kp_safe *safe, int timeout)
{
	kp_error_t ret;
	struct kp_unsafe unsafe = KP_UNSAFE_INIT;

	if (!ctx->agent.connected) {
		ret = KP_EINPUT;
		return ret;
	}

	unsafe.timeout = timeout;
	if ((ret = kp_safe_get_path(ctx, safe, unsafe.path,
	                            PATH_MAX)) != KP_SUCCESS) {
		return ret;
	}
	if (strlcpy(unsafe.password, safe->password,
	            KP_PASSWORD_MAX_LEN) >= KP_PASSWORD_MAX_LEN) {
		errno = ENOMEM;
		return KP_ERRNO;
	}
	if (strlcpy(unsafe.metadata, safe->metadata,
	            KP_METADATA_MAX_LEN) >= KP_METADATA_MAX_LEN) {
		errno = ENOMEM;
		return KP_ERRNO;
	}

	kp_agent_send(&ctx->agent, KP_MSG_STORE, &unsafe,
	              sizeof(struct kp_unsafe));

	return KP_SUCCESS;
}

static kp_error_t
kp_safe_mkdir(struct kp_ctx *ctx, char *path)
{
	struct stat  stats;
	char   *rdir;

	assert(ctx);
	assert(path);

	rdir = path + strlen(ctx->ws_path);
	rdir++; /* Skip first / as it is part of the ctx->ws */
	while ((rdir = strchr(rdir, '/'))) {
		*rdir = '\0';
		if (stat(path, &stats) != 0) {
			if (errno == ENOENT) {
				if (mkdir(path, 0700) < 0) {
					return KP_ERRNO;
				}
			} else {
				return KP_ERRNO;
			}
		}
		*rdir = '/';
		rdir++;
	}
	return KP_SUCCESS;
}
