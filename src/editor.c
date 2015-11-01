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
			warn("cannot run editor");
			ret = KP_EINPUT;
			goto clean;
		}
	}

	wait(NULL);

	fd = fopen(path, "r");
	if (fd == NULL) {
		warn("cannot open temporary clear text file %s", path);
		ret = errno;
		goto clean;
	}

	clearerr(fd);
	metadata_len = fread(safe->metadata, 1, KP_METADATA_MAX_LEN, fd);
	safe->metadata[metadata_len] = '\0';

	if (ferror(fd) != 0) {
		warn("error while reading temporary clear text file %s", path);
		ret = errno;
		goto clean;
	}
	if (!feof(fd)) {
		warnx("safe too long, storing only %lu bytes", metadata_len);
		ret = KP_ENOMEM;
		goto clean;
	}

	ret = KP_SUCCESS;

clean:
	fclose(fd);
	if (unlink(path) < 0) {
		warn("cannot delete temporary clear text file %s", path);
		warnx("ensure to delete it manually to avoid password leak");
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

	if (strlcpy(path, ctx->ws_path, size) >= size) {
		warnx("memory error");
		return KP_ENOMEM;
	}

	if (strlcat(path, "/.kpXXXXXX", size) >= size) {
		warnx("memory error");
		return KP_ENOMEM;
	}

	fd = mkstemp(path);
	if (fd < 0) {
		warn("cannot create temporary file %s",
				path);
		return errno;
	}

	metadata_len = strlen(safe->metadata);
	if (write(fd, safe->metadata, metadata_len) != metadata_len) {
		warn("cannot dump safe on temp file %s for edition", path);
		return errno;
	}

	if (close(fd) < 0) {
		warn("cannot close temp file %s with plain safe", path);
		return errno;
	}

	return KP_SUCCESS;
}
