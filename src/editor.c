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
#include "storage.h"

/*
 * Open editor on the given safe.
 * Fork, exec $EDITOR and wait for it to return.
 * The safe must be open.
 */
kp_error_t
kp_editor_open(struct kp_safe *safe)
{
	const char *editor;
	pid_t pid;

	assert(safe->plain.fd >= 0);
	if (close(safe->plain.fd) < 0) {
		warn("cannot close temporary clear text file %s",
				safe->plain.path);
	}

	editor = getenv("EDITOR");
	if (editor == NULL) editor = "vi";

	pid = fork();

	if (pid == 0) {
		execlp(editor, editor, safe->plain.path, NULL);
	} else {
		wait(NULL);
		// TODO check for return value of editor

		safe->plain.fd = open(safe->plain.path, O_RDONLY);
		if (safe->plain.fd < 0) {
			warn("cannot open temporary clear text file %s",
					safe->plain.path);
			if (unlink(safe->plain.path) < 0) {
				warn("cannot delete temporary clear text file %s",
						safe->plain.path);
				warnx("ensure to delete it manually to avoid password leak");
			}
			return errno;
		}
	}

	return KP_SUCCESS;
}

/*
 * Create and open a temporary file in current workspace for later edition.
 */
kp_error_t
kp_editor_get_tmp(struct kp_ctx *ctx, struct kp_safe *safe, bool keep_open)
{
	if (strlcpy(safe->plain.path, ctx->ws_path, PATH_MAX) >= PATH_MAX) {
		warnx("memory error");
		return KP_ENOMEM;
	}

	if (strlcat(safe->plain.path, "/.kpXXXXXX", PATH_MAX) >= PATH_MAX) {
		warnx("memory error");
		return KP_ENOMEM;
	}

	safe->plain.fd = mkstemp(safe->plain.path);
	if (safe->plain.fd < 0) {
		warn("cannot create temporary file %s",
				safe->plain.path);
		return errno;
	}

	if (!keep_open) {
		close(safe->plain.fd);
		safe->plain.fd = 0;
	}

	return KP_SUCCESS;
}
