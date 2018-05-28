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
#include <sys/tree.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>

#include <assert.h>
#include <sodium.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kickpass.h"

#include "error.h"
#include "imsg.h"
#include "kpagent.h"

#define SOCKET_BACKLOG 128

/* on some system stat.h uses a variable named __unused */
#ifndef __unused
#define __unused __attribute__((unused))
#endif

struct kp_agent_safe {
	char name[PATH_MAX]; /* name of the safe */
	char * const password;      /* plain text password (null terminated) */
	char * const metadata;      /* plain text metadata (null terminated) */
};

struct kp_store {
	RB_ENTRY(kp_store) tree;
	struct kp_agent_safe *safe;
};

static int store_cmp(struct kp_store *, struct kp_store *);

RB_HEAD(storage, kp_store) storage = RB_INITIALIZER(&storage);
RB_PROTOTYPE_STATIC(storage, kp_store, tree, store_cmp);

static kp_error_t kp_agent_safe_create(struct kp_agent *, struct kp_agent_safe **);
static kp_error_t kp_agent_safe_free(struct kp_agent *, struct kp_agent_safe *);

kp_error_t
kp_agent_init(struct kp_agent *agent, const char *socket_path)
{
	assert(agent);

	agent->sock = -1;
	agent->connected = false;

	memset(&agent->sunaddr, 0, sizeof(struct sockaddr_un));
	agent->sunaddr.sun_family = AF_UNIX;
	if (strlcpy(agent->sunaddr.sun_path, socket_path, sizeof(agent->sunaddr.sun_path))
			>= sizeof(agent->sunaddr.sun_path)) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	if ((agent->sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		return KP_ERRNO;
	}

	imsg_init(&agent->ibuf, agent->sock);

	return KP_SUCCESS;
}

kp_error_t
kp_agent_connect(struct kp_agent *agent)
{
	assert(agent);

	if (connect(agent->sock, (struct sockaddr *)&agent->sunaddr,
				sizeof(struct sockaddr_un)) < 0) {
		return KP_ERRNO;
	}
	agent->connected = true;

	return KP_SUCCESS;
}

kp_error_t
kp_agent_listen(struct kp_agent *agent)
{
	assert(agent);

	if (bind(agent->sock, (struct sockaddr *)&agent->sunaddr,
				sizeof(struct sockaddr_un)) < 0) {
		return KP_ERRNO;
	}

	if (listen(agent->sock, SOCKET_BACKLOG) < 0) {
		return KP_ERRNO;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_agent_accept(struct kp_agent *agent, struct kp_agent *out)
{
	socklen_t addrlen = sizeof(struct sockaddr_un);

	assert(agent);
	assert(out);

	out->sock = -1;
	out->connected = false;


	if ((out->sock = accept(agent->sock, (struct sockaddr *)&out->sunaddr, &addrlen)) < 0) {
		return KP_ERRNO;
	}

	imsg_init(&out->ibuf, out->sock);
	out->connected = true;

	return KP_SUCCESS;
}

kp_error_t
kp_agent_send(struct kp_agent *agent, enum kp_agent_msg_type type, void *data,
              size_t size)
{
	assert(agent);

	if (imsg_compose(&agent->ibuf, type, 1, 0, -1, data, size) < 0) {
		return KP_ERRNO;
	}
	if (imsg_flush(&agent->ibuf) < 0) {
		return KP_ERRNO;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_agent_error(struct kp_agent *agent, kp_error_t err)
{
	struct kp_msg_error error;

	error.err = err;
	error.err_no = 0;
	if (err == KP_ERRNO) {
		error.err_no = errno;
	}

	return kp_agent_send(agent, KP_MSG_ERROR, &error,
	                     sizeof(struct kp_msg_error));
}

kp_error_t
kp_agent_receive(struct kp_agent *agent, enum kp_agent_msg_type type, void *data,
                 size_t size)
{
	kp_error_t ret;
	struct imsg imsg;
	ssize_t ssize = 0;

	assert(agent);

	do {
		/* Try to get one from ibuf */
		if ((ssize = imsg_get(&agent->ibuf, &imsg)) > 0) {
			continue;
		}

		/* Nothing in buf try to read from conn */
		if (imsg_read(&agent->ibuf) <= 0) {
			imsg_clear(&agent->ibuf);
			/* XXX clean conn */
			return KP_ERRNO;
		}

		/* Try to get one from ibuf */
		if ((ssize = imsg_get(&agent->ibuf, &imsg)) < 0) {
			return KP_ERRNO;
		}
	} while (ssize <= 0);

	if (imsg.hdr.type > KP_MSG_ERROR) {
		/* XXX report real error */
		ret = KP_INVALID_MSG;
		goto out;
	}

	if (imsg.hdr.type != type) {
		if (imsg.hdr.type == KP_MSG_ERROR) {
			struct kp_msg_error *error = (struct kp_msg_error *)imsg.data;
			if (error->err == KP_ERRNO) {
				errno = error->err_no;
			}
			ret = error->err;
			goto out;
		} else {
			ret = KP_INVALID_MSG;
			goto out;
		}
	}

	if (imsg.hdr.len - IMSG_HEADER_SIZE != size) {
		errno = EMSGSIZE;
		ret = KP_ERRNO;
		goto out;
	}

	ret = KP_SUCCESS;

	if (data) {
		memcpy(data, imsg.data, size);
	}

out:
	imsg_free(&imsg);
	return ret;
}

kp_error_t
kp_agent_close(struct kp_agent *agent)
{
	imsg_clear(&agent->ibuf);

	if (agent->sock >= 0) {
		close(agent->sock);
	}

	return KP_SUCCESS;
}

static kp_error_t
kp_agent_safe_create(struct kp_agent *agent, struct kp_agent_safe **_safe)
{
	struct kp_agent_safe *safe;
	char **password;
	char **metadata;

	if ((safe = malloc(sizeof(struct kp_agent_safe))) == NULL) {
		errno = ENOMEM;
		return KP_ERRNO;
	}

	/* Store is the only able to set password and metadata memory */
	password = (char **)&safe->password;
	metadata = (char **)&safe->metadata;

	*password = sodium_malloc(KP_PASSWORD_MAX_LEN);
	*metadata = sodium_malloc(KP_METADATA_MAX_LEN);

	*_safe = safe;

	return KP_SUCCESS;
}

static kp_error_t
kp_agent_safe_free(struct kp_agent *agent, struct kp_agent_safe *safe)
{
	assert(safe);

	sodium_free(safe->password);
	sodium_free(safe->metadata);

	return KP_SUCCESS;
}

kp_error_t
kp_agent_store(struct kp_agent *agent, struct kp_unsafe *unsafe)
{
	kp_error_t ret;
	struct kp_store *store, *existing;
	struct kp_agent_safe *safe;

	if ((ret = kp_agent_safe_create(agent, &safe)) != KP_SUCCESS) {
		return ret;
	}

	if (strlcpy(safe->name, unsafe->name, PATH_MAX) >= PATH_MAX) {
		errno = ENOMEM;
		ret = KP_ERRNO;
		goto out;
	}
	if (strlcpy(safe->password, unsafe->password, KP_PASSWORD_MAX_LEN)
	    >= KP_PASSWORD_MAX_LEN) {
		errno = ENOMEM;
		ret = KP_ERRNO;
		goto out;
	}
	if (strlcpy(safe->metadata, unsafe->metadata, KP_METADATA_MAX_LEN)
	    >= KP_METADATA_MAX_LEN) {
		errno = ENOMEM;
		ret = KP_ERRNO;
		goto out;
	}

	if ((store = malloc(sizeof(struct kp_store))) == NULL) {
		errno = ENOMEM;
		ret = KP_ERRNO;
		goto out;
	}

	store->safe = safe;

	existing = RB_INSERT(storage, &storage, store);
	if (existing != NULL) {
		kp_agent_safe_free(agent, existing->safe);
		existing->safe = safe;
	}

	ret = KP_SUCCESS;

out:
	return ret;
}

kp_error_t
kp_agent_discard(struct kp_agent *agent, const char *name, bool silent)
{
	kp_error_t ret;
	struct kp_store needle, *store;
	struct kp_agent_safe safe;
	bool result = true;

	if (strlcpy(safe.name, name, PATH_MAX) > PATH_MAX) {
		errno = ENAMETOOLONG;
		ret = KP_ERRNO;
		goto failure;
	}
	needle.safe = &safe;

	store = RB_FIND(storage, &storage, &needle);
	if (store == NULL) {
		errno = ENOENT;
		ret = KP_ERRNO;
		goto failure;
	}

	kp_agent_safe_free(agent, store->safe);
	RB_REMOVE(storage, &storage, store);

	if (silent) {
		return KP_SUCCESS;
	} else {
		return kp_agent_send(agent, KP_MSG_DISCARD, &result, sizeof(bool));
	}

failure:
	kp_agent_error(agent, ret);
	return ret;
}

kp_error_t
kp_agent_search(struct kp_agent *agent, const char *name)
{
	kp_error_t ret;
	struct kp_store needle, *store;
	struct kp_agent_safe safe;
	struct kp_unsafe unsafe = KP_UNSAFE_INIT;

	if (strlcpy(safe.name, name, PATH_MAX) > PATH_MAX) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}
	needle.safe = &safe;

	store = RB_FIND(storage, &storage, &needle);
	if (store == NULL) {
		errno = ENOENT;
		ret = KP_ERRNO;
		goto failure;
	}

	if (strlcpy(unsafe.name, store->safe->name, PATH_MAX) >= PATH_MAX) {
		errno = ENOMEM;
		ret = KP_ERRNO;
		goto failure;
	}
	if (strlcpy(unsafe.password, store->safe->password, KP_PASSWORD_MAX_LEN)
	    >= KP_PASSWORD_MAX_LEN) {
		errno = ENOMEM;
		ret = KP_ERRNO;
		goto failure;
	}
	if (strlcpy(unsafe.metadata, store->safe->metadata, KP_METADATA_MAX_LEN)
	    >= KP_METADATA_MAX_LEN) {
		errno = ENOMEM;
		ret = KP_ERRNO;
		goto failure;
	}

	return kp_agent_send(agent, KP_MSG_SEARCH, &unsafe,
	                         sizeof(struct kp_unsafe));

failure:
	kp_agent_error(agent, ret);
	return ret;
}

static int
store_cmp(struct kp_store *a, struct kp_store *b)
{
	return strncmp(a->safe->name, b->safe->name, PATH_MAX);
}

RB_GENERATE_STATIC(storage, kp_store, tree, store_cmp);
