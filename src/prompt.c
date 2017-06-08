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

#include <stdarg.h>
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

#define PASSWORD_PROMPT         "[kickpass] %s password: "
#define PASSWORD_CONFIRM_PROMPT "[kickpass] confirm: "


kp_error_t
kp_askpass(struct kp_ctx *ctx, bool confirm, char *password, const char *fmt, va_list ap)
{
	kp_error_t ret = KP_SUCCESS;
	char *prompt_fmt = NULL, *prompt = NULL;
	char *askpass = NULL;
	char *output = NULL;
	size_t len = 0;
	pid_t pid;
	int pipefd[2], status;
	FILE *fout = NULL;
	void (*sigchld_handler)(int);

	if ((askpass = getenv("KP_ASKPASS")) == NULL) {
		askpass = "ssh-askpass";
	}

	if (fflush(stdout) != 0) {
		return KP_ERRNO;
	}

	if (pipe(pipefd) < 0) {
		return KP_ERRNO;
	}

	sigchld_handler = signal(SIGCHLD, SIG_DFL);

	if ((pid = fork()) < 0) {
		ret = KP_ERRNO;
		goto out;
	}

	if (pid == 0) {
		close(pipefd[0]);

		if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
			kp_err(KP_ERRNO, "read stdout of %s", askpass);
		}

		if (asprintf(&prompt_fmt, PASSWORD_PROMPT, fmt) < 0) {
			kp_err(KP_ERRNO, "cannot build prompt");
		}

		if (vasprintf(&prompt, prompt_fmt, ap) < 0) {
			kp_err(KP_ERRNO, "cannot build prompt");
		}

		execlp(askpass, askpass, prompt, (char *)NULL);

		free(prompt);
		close(pipefd[1]);
		kp_err(KP_ERRNO, "cannot execute %s", askpass);
	}

	close(pipefd[1]);

	if ((fout = fdopen(pipefd[0], "r")) == NULL) {
		ret = KP_ERRNO;
		kp_warn(ret, "cannot read password");
		goto out;
	}

	if (getline(&output, &len, fout) < 0) {
		ret = KP_ERRNO;
		goto out;
	}

	while (waitpid(pid, &status, 0) < 0) {
		if (errno != EINTR)
			goto out;
	}

	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		goto out;
	}

	output[strcspn(output, "\r\n")] = '\0';
	if (strlcpy(password, output, KP_PASSWORD_MAX_LEN)
	    >= KP_PASSWORD_MAX_LEN) {
		errno = ENAMETOOLONG;
		ret = KP_ERRNO;
		kp_warn(ret, "cannot read password");
		goto out;
	}


out:
	fclose(fout);
	free(output);
	signal(SIGCHLD, sigchld_handler);
	return ret;
}

kp_error_t
kp_readpass(struct kp_ctx *ctx, bool confirm, char *password, const char *fmt, va_list ap)
{
	kp_error_t ret = KP_SUCCESS;
	char *prompt_fmt = NULL, *prompt = NULL;
	char *confirmation = NULL;

	assert(fmt);
	assert(password);

	if (asprintf(&prompt_fmt, PASSWORD_PROMPT, fmt) < 0) {
		kp_err(KP_ERRNO, "cannot build prompt");
	}

	if (vasprintf(&prompt, prompt_fmt, ap) < 0) {
		kp_err(KP_ERRNO, "cannot build prompt");
	}

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
	free(prompt_fmt);
	free(prompt);
	sodium_free(confirmation);

	return ret;
}
