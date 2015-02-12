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

#include "error.h"
#include "storage.h"
#include "log.h"

#define SAFE_TEMPLATE "password: \n"\
                      "url: \n"

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
kp_editor_new(struct kp_storage_ctx *ctx)
{
	kp_error_t ret;
	int fd;
	char tpl[PATH_MAX];

	if ((ret = kp_storage_get_path(ctx, tpl, PATH_MAX)) != KP_SUCCESS)
		return ret;

	if (strlcat(tpl, "/.kpXXXXXX", PATH_MAX) >= PATH_MAX)
		return KP_ENOMEM;

	fd = mkstemp(tpl);
	if (fd < 0) {
		LOGE("cannot create create temporary file %s: %s (%d)",
				tpl, strerror(errno), errno);
		return errno;
	}

	dprintf(fd, "%s", SAFE_TEMPLATE);
	close(fd);

	kp_editor_open(tpl);

	return KP_SUCCESS;
}
