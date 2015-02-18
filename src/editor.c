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
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "editor.h"
#include "error.h"
#include "log.h"
#include "storage.h"

kp_error_t
kp_editor_open(const char *path)
{
	const char *editor;
	pid_t pid;

	editor = getenv("EDITOR");
	if (editor == NULL) editor = "vi";

	pid = fork();

	if (pid == 0) {
		execlp(editor, editor, path, NULL);
	} else {
		wait(NULL);
	}

	return KP_SUCCESS;
}

kp_error_t
kp_editor_get_tmp(struct kp_storage_ctx *ctx, struct kp_safe *safe, bool keep_open)
{
	kp_error_t ret;

	if ((ret = kp_storage_get_path(ctx, safe->plain.path, PATH_MAX)) != KP_SUCCESS)
		return ret;

	if (strlcat(safe->plain.path, "/.kpXXXXXX", PATH_MAX) >= PATH_MAX)
		return KP_ENOMEM;

	safe->plain.fd = mkstemp(safe->plain.path);
	if (safe->plain.fd < 0) {
		LOGE("cannot create create temporary file %s: %s (%d)",
				safe->plain.path, strerror(errno), errno);
		return errno;
	}

	if (!keep_open) {
		close(safe->plain.fd);
		safe->plain.fd = 0;
	}

	return KP_SUCCESS;
}
