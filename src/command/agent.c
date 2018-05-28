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
#include <sys/wait.h>

#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <event2/event.h>

#include "kickpass.h"

#include "command.h"
#include "imsg.h"
#include "kpagent.h"
#include "log.h"

#ifndef EPROTO
#define EPROTO ENOPROTOOPT
#endif

#define TMP_TEMPLATE "/tmp/kickpass-XXXXXX"

struct agent {
	struct event_base *evb;
	struct kp_agent kp_agent;
};

struct conn {
	struct event *ev;
	struct agent agent;
	struct imsgbuf ibuf;
};

struct timeout {
	struct agent *agent;
	char name[PATH_MAX];
};

static kp_error_t agent(struct kp_ctx *, int, char **);
static void agent_accept(evutil_socket_t, short, void *);
static void agent_kill(evutil_socket_t, short, void *);
static void timeout_discard(evutil_socket_t, short, void *);
static void dispatch(evutil_socket_t, short, void *);
static kp_error_t parse_opt(struct kp_ctx *, int, char **);
static kp_error_t store(struct agent *, struct kp_unsafe *);
static void       usage(void);

struct kp_cmd kp_cmd_agent = {
	.main  = agent,
	.usage = usage,
	.opts  = "agent [-d] [command [arg ...]]",
	.desc  = "Run a kickpass agent in background",
};

static bool daemonize = true;

static void
agent_accept(evutil_socket_t fd, short events, void *_agent)
{
	struct agent *agent = _agent;
	struct conn *conn;

	if ((conn = malloc(sizeof(struct conn))) == NULL) {
		errno = ENOMEM;
		kp_warn(KP_ERRNO, "cannot accept client");
		return;
	}

	if ((kp_agent_accept(&agent->kp_agent,
	                     &conn->agent.kp_agent)) != KP_SUCCESS) {
		kp_warn(KP_ERRNO, "cannot accept client");
		return;
	}

	conn->agent.evb = agent->evb;
	imsg_init(&conn->ibuf, conn->agent.kp_agent.sock);
	conn->ev = event_new(agent->evb, conn->agent.kp_agent.sock,
	               EV_READ | EV_PERSIST, dispatch, conn);
	event_add(conn->ev, NULL);
}

static void
agent_kill(evutil_socket_t fd, short events, void *_agent)
{
	struct agent *agent = _agent;

	event_base_loopexit(agent->evb, NULL);
}

static void
dispatch(evutil_socket_t fd, short events, void *_conn)
{
	struct imsg imsg;
	struct conn *conn = _conn;

	if (imsg_read(&conn->ibuf) <= 0) {
		imsg_clear(&conn->ibuf);
		event_del(conn->ev);
		free(conn);
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
			store(&conn->agent, (struct kp_unsafe *)imsg.data);
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
			kp_agent_search(&conn->agent.kp_agent,
			                (char *)imsg.data);
			break;
		case KP_MSG_DISCARD:
			if (data_size != PATH_MAX) {
				errno = EPROTO;
				kp_warn(KP_ERRNO, "invalid message");
				break;
			}
			/* ensure null termination */
			((char *)imsg.data)[PATH_MAX-1] = '\0';
			kp_agent_discard(&conn->agent.kp_agent,
			                 (char *)imsg.data, false);
			break;
		}

		imsg_free(&imsg);
	}
}

kp_error_t
agent(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret = KP_SUCCESS;
	pid_t agent_pid = 0, child_pid = 0, parent_pid = 0;
	char socket_dir[PATH_MAX];
	char socket_path[PATH_MAX];
	struct agent agent;
	struct event *ev;
	struct stat sb;

	if ((ret = parse_opt(ctx, argc, argv)) != KP_SUCCESS) {
		return ret;
	}

	if (daemonize) {
		parent_pid = getpid();

		if ((agent_pid = fork()) < 0) {
			ret = KP_ERRNO;
			kp_warn(ret, "cannot daemonize");
			return ret;
		}

		if (agent_pid != 0) {
			sigset_t set;
			int sig, wstatus;

			if (sigemptyset(&set) < 0) {
				ret = KP_ERRNO;
				kp_warn(ret, "cannot wait for child");
				return ret;
			}

			if (sigaddset(&set, SIGCONT) < 0) {
				ret = KP_ERRNO;
				kp_warn(ret, "cannot wait for child");
				return ret;
			}

			if (sigaddset(&set, SIGCHLD) < 0) {
				ret = KP_ERRNO;
				kp_warn(ret, "cannot wait for child");
				return ret;
			}

			sigprocmask(SIG_BLOCK, &set, NULL);
			sigwait(&set, &sig);

			if (sig == SIGCHLD) {
				waitpid(agent_pid, &wstatus, 0);
				exit(wstatus);
			}

			exit(KP_SUCCESS);
		}
	}

	agent_pid = getpid();
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
				(long)agent_pid) >= PATH_MAX) {
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

	if (setenv(KP_AGENT_SOCKET_ENV, socket_path, 1) < 0) {
		ret = KP_ERRNO;
		kp_warn(ret, "cannot set environment");
		goto out;
	}

	fprintf(stdout, "%s=%s; export %s;\n", KP_AGENT_SOCKET_ENV,
	        socket_path, KP_AGENT_SOCKET_ENV);
	fflush(stdout);

	if (optind < argc) {
		child_pid = fork();
		if (child_pid == 0) {
			if (execvp(argv[optind], argv + optind) < 0) {
				kp_warn(ret, "cannot exec child");
			}
		}
	}

	if (daemonize) {
		int devnull;

		if (chdir("/") < 0) {
			ret = KP_ERRNO;
			kp_warn(ret, "cannot chdir to /");
			return ret;
		}

		if ((devnull = open("/dev/null", O_RDWR)) < 0) {
			ret = KP_ERRNO;
			kp_warn(ret, "cannot close standard inputs");
			return ret;
		}

		dup2(devnull, STDIN_FILENO);
		dup2(devnull, STDOUT_FILENO);
		dup2(devnull, STDERR_FILENO);

		close(devnull);

		kill(parent_pid, SIGCONT);
	}

	agent.evb = event_base_new();

	ev = event_new(agent.evb, agent.kp_agent.sock, EV_READ | EV_PERSIST,
	               agent_accept, &agent);
	event_add(ev, NULL);

signal(SIGPIPE, SIG_IGN);

	if (child_pid != 0) {
		ev = event_new(agent.evb, SIGCHLD, EV_SIGNAL | EV_PERSIST,
		               agent_kill, &agent);
		event_add(ev, NULL);
	}

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
		if (rmdir(socket_dir) != 0) {
			kp_warn(KP_ERRNO, "cannot delete agent socket dir %s",
			        socket_dir);
		}
	}

	return ret;
}

static kp_error_t
store(struct agent *agent, struct kp_unsafe *unsafe)
{
	struct kp_agent *kp_agent = &agent->kp_agent;
	struct timeout *timeout;
	struct event *timer;
	struct timeval timeval;

	if (unsafe->timeout > 0) {
		if ((timeout = malloc(sizeof(struct timeout))) == NULL) {
			errno = ENOMEM;
			return KP_ERRNO;
		}
		timeout->agent = agent;
		if (strlcpy(timeout->name, unsafe->name, PATH_MAX)
		    >= PATH_MAX) {
			errno = ENAMETOOLONG;
			return KP_ERRNO;
		}
		timeval.tv_sec = unsafe->timeout;
		timeval.tv_usec = 0;
		timer = evtimer_new(agent->evb, timeout_discard, timeout);
		evtimer_add(timer, &timeval);
	}

	return kp_agent_store(kp_agent, unsafe);
}

static void
timeout_discard(evutil_socket_t fd, short events, void *_timeout)
{
	struct timeout *timeout = _timeout;
	struct kp_agent *kp_agent = &timeout->agent->kp_agent;

	kp_agent_discard(kp_agent, timeout->name, true);

	free(timeout);
}

static kp_error_t
parse_opt(struct kp_ctx *ctx, int argc, char **argv)
{
	int opt;
	kp_error_t ret = KP_SUCCESS;
	static struct option longopts[] = {
		{ "no-daemon", no_argument,       NULL, 'd' },
		{ NULL,        0,                 NULL, 0   },
	};

	while ((opt = getopt_long(argc, argv, "d", longopts, NULL)) != -1) {
		switch (opt) {
		case 'd':
			daemonize = false;
			break;
		default:
			ret = KP_EINPUT;
			kp_warn(ret, "unknown option %c", opt);
		}
	}

	return ret;
}

void
usage(void)
{
	printf("options:\n");
	printf("    -d, --no-daemon    Do not daemonize\n");
}
