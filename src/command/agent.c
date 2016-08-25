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
#include <imsg.h>
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <event2/event.h>

#include "kickpass.h"

#include "log.h"
#include "command.h"
#include "kpagent.h"

#define TMP_TEMPLATE "/tmp/kickpass-XXXXXX"

struct agent {
	struct event_base *evb;
	struct kp_agent kp_agent;
};

struct conn {
	struct kp_agent agent;
	struct imsgbuf ibuf;
};

static kp_error_t agent(struct kp_ctx *, int, char **);
static void agent_accept(evutil_socket_t, short, void *);
static void dispatch(evutil_socket_t, short, void *);
static kp_error_t store(struct kp_agent *, struct kp_unsafe *);
static kp_error_t search(struct kp_agent *, char *);

struct kp_cmd kp_cmd_agent = {
	.main  = agent,
	.usage = NULL,
	.opts  = "agent",
	.desc  = "Run a kickpass agent in background",
	.lock  = false,
};


static void
agent_accept(evutil_socket_t fd, short events, void *_agent)
{
	struct agent *agent = _agent;
	struct conn *conn;
	struct event *ev;

	if ((conn = malloc(sizeof(struct conn))) == NULL) {
		errno = ENOMEM;
		kp_warn(KP_ERRNO, "cannot accept client");
		return;
	}

	if ((kp_agent_accept(&agent->kp_agent, &conn->agent)) != KP_SUCCESS) {
		kp_warn(KP_ERRNO, "cannot accept client");
		return;
	}

	imsg_init(&conn->ibuf, conn->agent.sock);

	ev = event_new(agent->evb, conn->agent.sock, EV_READ | EV_PERSIST, dispatch,
	               conn);
	event_add(ev, NULL);
}

static void
dispatch(evutil_socket_t fd, short events, void *_conn)
{
	struct imsg imsg;
	struct conn *conn = _conn;

	if (imsg_read(&conn->ibuf) < 0) {
		kp_warnx(KP_EINTERNAL, "connection lost");
		imsg_clear(&conn->ibuf);
		/* XXX clean conn */
		return;
	}

	while (imsg_get(&conn->ibuf, &imsg) > 0) {
		size_t data_size;

		data_size = imsg.hdr.len - IMSG_HEADER_SIZE;

		switch (imsg.hdr.type) {
		case KP_MSG_STORE:
			if (data_size != sizeof(struct kp_unsafe)) {
				errno = EPROTO;
				kp_warn(KP_ERRNO, "invalid message");
				break;
			}
			store(&conn->agent,
			      (struct kp_unsafe *)imsg.data);
			/* XXX handle error */
			break;
		case KP_MSG_SEARCH:
			if (data_size != PATH_MAX) {
				errno = EPROTO;
				kp_warn(KP_ERRNO, "invalid message");
				break;
			}
			/* ensure null termination */
			((char *)imsg.data)[PATH_MAX-1] = '\0';
			search(&conn->agent, (char *)imsg.data);
			break;
		}

		imsg_free(&imsg);
	}
}

kp_error_t
agent(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret = KP_SUCCESS;
	pid_t parent_pid;
	char socket_dir[PATH_MAX];
	char socket_path[PATH_MAX];
	struct agent agent;
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

	if ((ret = kp_agent_init(&agent.kp_agent, socket_path)) != KP_SUCCESS) {
		kp_warn(ret, "cannot create socket");
		goto out;
	}

	if ((ret = kp_agent_listen(&agent.kp_agent)) != KP_SUCCESS) {
		kp_warn(ret, "cannot create socket");
		goto out;
	}

	printf("%s=\"%s\"\n", KP_AGENT_SOCKET_ENV, socket_path);

	agent.evb = event_base_new();

	ev = event_new(agent.evb, agent.kp_agent.sock, EV_READ | EV_PERSIST,
	               agent_accept, &agent);
	event_add(ev, NULL);

	event_base_dispatch(agent.evb);

out:
	event_base_free(agent.evb);

	kp_agent_close(&agent.kp_agent);

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

static kp_error_t
store(struct kp_agent *agent, struct kp_unsafe *unsafe)
{
	kp_error_t ret;
	struct kp_agent_safe *safe;

	if ((ret = kp_agent_safe_create(agent, &safe)) != KP_SUCCESS) {
		return ret;
	}

	safe->timeout = unsafe->timeout;
	if (strlcpy(safe->path, unsafe->path, PATH_MAX) >= PATH_MAX) {
		errno = ENOMEM;
		return KP_ERRNO;
	}
	if (strlcpy(safe->password, unsafe->password, KP_PASSWORD_MAX_LEN)
	    >= KP_PASSWORD_MAX_LEN) {
		errno = ENOMEM;
		return KP_ERRNO;
	}
	if (strlcpy(safe->metadata, unsafe->metadata, KP_METADATA_MAX_LEN)
	    >= KP_METADATA_MAX_LEN) {
		errno = ENOMEM;
		return KP_ERRNO;
	}

	return kp_agent_store(agent, safe);
}

static kp_error_t
search(struct kp_agent *agent, char *path)
{
	kp_error_t ret;
	struct kp_agent_safe *safe;
	struct kp_unsafe unsafe;

	if ((ret = kp_agent_search(agent, path, &safe)) != KP_SUCCESS) {
		errno = ENOENT;
		kp_agent_error(agent, KP_ERRNO);
		return ret;
	}

	safe->timeout = 0; /* TODO compute remaining time ? */
	if (strlcpy(unsafe.path, safe->path, PATH_MAX) >= PATH_MAX) {
		errno = ENOMEM;
		kp_agent_error(agent, KP_ERRNO);
		return KP_ERRNO;
	}
	if (strlcpy(unsafe.password, safe->password, KP_PASSWORD_MAX_LEN)
	    >= KP_PASSWORD_MAX_LEN) {
		errno = ENOMEM;
		kp_agent_error(agent, KP_ERRNO);
		return KP_ERRNO;
	}
	if (strlcpy(unsafe.metadata, safe->metadata, KP_METADATA_MAX_LEN)
	    >= KP_METADATA_MAX_LEN) {
		errno = ENOMEM;
		kp_agent_error(agent, KP_ERRNO);
		return KP_ERRNO;
	}

	if ((ret = kp_agent_send(agent, KP_MSG_SEARCH, &unsafe,
	                     sizeof(struct kp_unsafe))) != KP_SUCCESS) {
		kp_warn(ret, "cannot send response");
	}

	return ret;
}
