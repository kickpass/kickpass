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
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "error.h"
#include "kpagent.h"

#define SOCKET_BACKLOG 128

kp_error_t
kp_agent_socket(char *socket_path, bool listening, int *sock)
{
	struct sockaddr_un sunaddr;
	*sock = -1;

	memset(&sunaddr, 0, sizeof(sunaddr));
	sunaddr.sun_family = AF_UNIX;
	if (strlcpy(sunaddr.sun_path, socket_path, sizeof(sunaddr.sun_path))
			>= sizeof(sunaddr.sun_path)) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	if ((*sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		return KP_ERRNO;
	}

	if (listening) {
		if (bind(*sock, (struct sockaddr *)&sunaddr,
					sizeof(sunaddr)) < 0) {
			return KP_ERRNO;
		}

		if (listen(*sock, SOCKET_BACKLOG) < 0) {
			return KP_ERRNO;
		}
	} else {
		if (connect(*sock, (struct sockaddr *)&sunaddr,
					sizeof(sunaddr)) < 0) {
			return KP_ERRNO;
		}
	}

	return KP_SUCCESS;
}
