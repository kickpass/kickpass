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

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include <readpassphrase.h>
#include <sodium.h>
#include <string.h>

#include "kickpass.h"
#include "safe.h"
#include "log.h"

#define PASSWORD_PROMPT         "[kickpass] %s password: "
#define PASSWORD_CONFIRM_PROMPT "[kickpass] confirm: "

kp_error_t
kp_prompt_password(const char *type, bool confirm, char *password)
{
	kp_error_t ret = KP_SUCCESS;
	char *prompt = NULL;
	size_t prompt_size;
	char *confirmation = NULL;

	assert(type);
	assert(password);

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
