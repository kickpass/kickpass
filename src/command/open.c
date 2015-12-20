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
#include <sys/queue.h>

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <imsg.h>

#include "kickpass.h"

#include "command.h"
#include "open.h"
#include "prompt.h"
#include "safe.h"
#include "log.h"
#include "kpagent.h"

static kp_error_t open_safe(struct kp_ctx *ctx, int argc, char **argv);

struct kp_cmd kp_cmd_open = {
	.main  = open_safe,
	.usage = NULL,
	.opts  = "open <safe>",
	.desc  = "Open a password safe and load it in kickpass agent",
	.lock  = false,
};

kp_error_t
open_safe(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret;
	char *socket_path;
	int sock;
	struct imsgbuf ibuf;

	if (argc - optind != 1) {
		ret = KP_EINPUT;
		kp_warn(ret, "missing safe name");
		return ret;
	}

	if ((socket_path = getenv(KP_AGENT_SOCKET_ENV)) == NULL) {
		ret = KP_EINPUT;
		kp_warn(ret, "missing environment var %s", KP_AGENT_SOCKET_ENV);
		return ret;
	}

	if ((ret = kp_agent_socket(socket_path, false, &sock)) != KP_SUCCESS) {
		kp_warn(ret, "cannot connect to agent socket %s", socket_path);
		return ret;
	}

	imsg_init(&ibuf, sock);

	imsg_compose(&ibuf, KP_MSG_OPEN, 1, 2, sock, "toto", 5);
	msgbuf_write(&ibuf.w);

	imsg_clear(&ibuf);
	return KP_SUCCESS;
}
