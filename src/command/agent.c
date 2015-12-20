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

#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <imsg.h>

#include <event2/event.h>

#include "kickpass.h"

#include "log.h"
#include "command.h"
#include "kpagent.h"

#define TMP_TEMPLATE "/tmp/kickpass-XXXXXX"

static kp_error_t agent(struct kp_ctx *, int, char **);
static void agent_accept(evutil_socket_t, short, void *);
static void dispatch(evutil_socket_t, short, void *);

struct kp_cmd kp_cmd_agent = {
	.main  = agent,
	.usage = NULL,
	.opts  = "agent",
	.desc  = "Run a kickpass agent in background",
	.lock  = false,
};

static void
agent_accept(evutil_socket_t fd, short events, void *_evb)
{
	struct event_base *evb = _evb;
	int sock;
	struct imsgbuf *ibuf;
	struct event *ev;

	if ((sock = accept(fd, NULL, NULL)) < 0) {
		kp_warn(KP_ERRNO, "cannot accept client");
		return;
	}

	if ((ibuf = malloc(sizeof(struct imsgbuf))) == NULL) {
		errno = ENOMEM;
		kp_warn(KP_ERRNO, "cannot accept client");
		return;
	}

	imsg_init(ibuf, sock);

	ev = event_new(evb, sock, EV_READ | EV_PERSIST, dispatch, ibuf);
	event_add(ev, NULL);
}

static void
dispatch(evutil_socket_t fd, short events, void *_ibuf)
{
	/* struct kp_ctx *ctx = _ctx; */
	struct imsg imsg;
	struct imsgbuf *ibuf = _ibuf;

	imsg_read(ibuf);
	printf("received %zd messages\n", imsg_get(ibuf, &imsg));
}

kp_error_t
agent(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret = KP_SUCCESS;
	pid_t parent_pid;
	char socket_dir[PATH_MAX];
	char socket_path[PATH_MAX];
	int sock;
	struct event_base *evb;
	struct event *ev;
	struct stat sb;

	parent_pid = getpid();

	if (strlcpy(socket_dir, TMP_TEMPLATE, PATH_MAX) >= PATH_MAX) {
		errno = ENAMETOOLONG;
		ret = KP_ERRNO;
		kp_warn(ret, "cannot create socket temp dir");
		goto out;
	}

	if (mkdtemp(socket_dir) == NULL) {
		ret = KP_ERRNO;
		kp_warn(ret, "cannot create socket temp dir");
		goto out;
	}

	if (snprintf(socket_path, PATH_MAX, "%s/agent.%ld", socket_dir,
				(long)parent_pid) >= PATH_MAX) {
		errno = ENAMETOOLONG;
		ret = KP_ERRNO;
		kp_warn(ret, "cannot create socket");
		goto out;
	}

	if ((ret = kp_agent_socket(socket_path, true, &sock)) != KP_SUCCESS) {
		kp_warn(ret, "cannot create socket");
		goto out;
	}
	printf("%s=\"%s\"\n", KP_AGENT_SOCKET_ENV, socket_path);

	evb = event_base_new();

	ev = event_new(evb, sock, EV_READ | EV_PERSIST, agent_accept, evb);
	event_add(ev, NULL);

	event_base_dispatch(evb);

out:
	event_base_free(evb);

	if (sock >= 0) {
		close(sock);
	}

	if (stat(socket_path, &sb) == 0) {
		if (unlink(socket_path) != 0) {
			kp_warn(KP_ERRNO, "cannot delete agent socket %s",
					socket_path);
		}
	}

	if (stat(socket_dir, &sb) == 0) {
		if (unlink(socket_path) != 0) {
			kp_warn(KP_ERRNO, "cannot delete agent socket dir %s",
					socket_dir);
		}
	}

	return ret;
}
