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

#include <sys/types.h>
#include <sys/wait.h>

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include <readpassphrase.h>
#include <signal.h>
#include <sodium.h>
#include <string.h>
#include <unistd.h>

#include "kickpass.h"
#include "safe.h"
#include "log.h"

kp_error_t
kp_askpass(struct kp_ctx *ctx, const char *type, bool confirm, char *password)
{
	char *askpass;
	pid_t pid, ret;
	size_t len;
	int p[2], status;
	void (*osigchld)(int);

	if ((askpass = getenv("KP_ASKPASS")) == NULL) {
		return KP_EINPUT;
	}

	if (fflush(stdout) != 0) {
		return KP_ERRNO;
	}
	if (askpass == NULL) {
		return KP_EINTERNAL;
	}
	if (pipe(p) < 0) {
		return KP_ERRNO;
	}
	osigchld = signal(SIGCHLD, SIG_DFL);
	if ((pid = fork()) < 0) {
		signal(SIGCHLD, osigchld);
		return KP_ERRNO;
	}
	if (pid == 0) {
		close(p[0]);
		if (dup2(p[1], STDOUT_FILENO) < 0) {
			kp_warn(KP_ERRNO, "kp_asskpass: dup2");
			exit(1);
		}
		execlp(askpass, askpass, type, (char *)NULL);
		kp_warn(KP_ERRNO, "kp_asskpass: exec(%s)", askpass);
		exit(1);
	}
	close(p[1]);

	len = 0;
	do {
		ssize_t r = read(p[0], password + len,
		    KP_PASSWORD_MAX_LEN - 1 - len);
		if (r == -1 && errno == EINTR) {
			continue;
		}
		if (r <= 0)
			break;
		len += r;
	} while (KP_PASSWORD_MAX_LEN - 1 - len > 0);
	password[len] = '\0';
	close(p[0]);

	while ((ret = waitpid(pid, &status, 0)) < 0) {
		if (errno != EINTR)
			break;
	}

	signal(SIGCHLD, osigchld);
	if (ret == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		memset(password, 0, KP_PASSWORD_MAX_LEN);
		return KP_ERRNO;
	}
	password[strcspn(password, "\r\n")] = '\0';
	return KP_SUCCESS;
}

kp_error_t
kp_readpass(struct kp_ctx *ctx, const char *type, bool confirm, char *password)
{
	kp_error_t ret = KP_SUCCESS;
	char *prompt = NULL;
	size_t prompt_size;
	char *confirmation = NULL;

	assert(type);
	assert(password);

#define PASSWORD_PROMPT         "[kickpass] %s password: "
#define PASSWORD_CONFIRM_PROMPT "[kickpass] confirm: "

	/* Prompt is PASSWORD_PROMPT - '%s' + type + '\0' */
	prompt_size = strlen(PASSWORD_PROMPT) - 2 + strlen(type) + 1;
	prompt = malloc(prompt_size);
	if (!prompt) {
		errno = ENOMEM;
		ret = KP_ERRNO;
		kp_warn(ret, "memory error");
		goto out;
	}

	snprintf(prompt, prompt_size, PASSWORD_PROMPT, type);

	if (readpassphrase(prompt, password, KP_PASSWORD_MAX_LEN,
				RPP_ECHO_OFF | RPP_REQUIRE_TTY) == NULL) {
		ret = KP_ERRNO;
		kp_warn(ret, "cannot read password");
		goto out;
	}

	if (confirm) {
		confirmation = sodium_malloc(KP_PASSWORD_MAX_LEN);
		if (!confirmation) {
			errno = ENOMEM;
			ret = KP_ERRNO;
			kp_warn(ret, "memory error");
			goto out;
		}

		if (readpassphrase(PASSWORD_CONFIRM_PROMPT, confirmation,
					KP_PASSWORD_MAX_LEN,
					RPP_ECHO_OFF | RPP_REQUIRE_TTY)
				== NULL) {
			ret = KP_ERRNO;
			kp_warn(ret, "cannot read password");
			goto out;
		}

		if (strncmp(password, confirmation, KP_PASSWORD_MAX_LEN) != 0) {
			ret = KP_EINPUT;
			kp_warn(ret, "mismatching password");
			goto out;
		}
	}

out:
	free(prompt);
	sodium_free(confirmation);

	return ret;
}
