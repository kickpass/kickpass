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

#include <sys/wait.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "editor.h"
#include "error.h"
#include "log.h"

static kp_error_t kp_editor_get_tmp(struct kp_ctx *, struct kp_safe *, char *, size_t);

/*
 * Open editor on the given safe.
 * Fork, exec $EDITOR and wait for it to return.
 * The safe must be open.
 */
kp_error_t
kp_edit(struct kp_ctx *ctx, struct kp_safe *safe)
{
	kp_error_t ret;
	FILE *fd;
	const char *editor;
	char path[PATH_MAX];
	pid_t pid;
	size_t metadata_len;

	assert(safe->open);

	kp_editor_get_tmp(ctx, safe, path, PATH_MAX);

	editor = getenv("EDITOR");
	if (editor == NULL) editor = "vi";

	pid = fork();

	if (pid == 0) {
		if (execlp(editor, editor, path, NULL) < 0) {
			ret = KP_ERRNO;
			kp_warn(ret, "cannot run editor");
			goto clean;
		}
	}

	wait(NULL);

	fd = fopen(path, "r");
	if (fd == NULL) {
		ret = KP_ERRNO;
		kp_warn(ret, "cannot open temporary clear text file %s", path);
		goto clean;
	}

	clearerr(fd);
	metadata_len = fread(safe->metadata, 1, KP_METADATA_MAX_LEN, fd);
	safe->metadata[metadata_len] = '\0';

	if ((errno = ferror(fd)) != 0) {
		ret = KP_ERRNO;
		kp_warn(ret, "error while reading temporary clear text file %s", path);
		goto clean;
	}
	if (!feof(fd)) {
		errno = ENOMEM;
		ret = KP_ERRNO;
		kp_warn(ret, "safe too long, storing only %lu bytes", metadata_len);
		goto clean;
	}

	ret = KP_SUCCESS;

clean:
	fclose(fd);
	if (unlink(path) < 0) {
		kp_warn(KP_ERRNO, "cannot delete temporary clear text file %s"
			"ensure to delete it manually to avoid metadata leak",
			path);
	}

	return ret;
}

/*
 * Create and open a temporary file in current workspace for later edition.
 */
static kp_error_t
kp_editor_get_tmp(struct kp_ctx *ctx, struct kp_safe *safe, char *path, size_t size)
{
	int fd;
	size_t metadata_len;
	kp_error_t ret;

	if (strlcpy(path, ctx->ws_path, size) >= size) {
		errno = ENOMEM;
		ret = KP_ERRNO;
		kp_warn(ret, "memory error");
		return ret;
	}

	if (strlcat(path, "/.kpXXXXXX", size) >= size) {
		errno = ENOMEM;
		ret = KP_ERRNO;
		kp_warn(ret, "memory error");
		return ret;
	}

	if ((fd = mkstemp(path)) < 0) {
		ret = KP_ERRNO;
		kp_warn(ret, "cannot create temporary file %s", path);
		return ret;
	}

	metadata_len = strlen(safe->metadata);
	if (write(fd, safe->metadata, metadata_len) != metadata_len) {
		ret = KP_ERRNO;
		kp_warn(ret, "cannot dump safe on temp file %s for edition", path);
		return ret;
	}

	if (close(fd) < 0) {
		ret = KP_ERRNO;
		kp_warn(ret, "cannot close temp file %s with plain safe", path);
		return ret;
	}

	return KP_SUCCESS;
}
