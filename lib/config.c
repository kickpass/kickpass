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

#include <stdio.h>

#include "kickpass.h"

#include "config.h"
#include "safe.h"

#define KP_CONFIG_SAFE_NAME ".config"

kp_error_t
kp_cfg_create(struct kp_ctx *ctx)
{
	kp_error_t ret;
	struct kp_safe cfg_safe;

	if ((ret = kp_safe_create(ctx, &cfg_safe, KP_CONFIG_SAFE_NAME))
			!= KP_SUCCESS) {
		return ret;
	}

	snprintf(cfg_safe.password, KP_PASSWORD_MAX_LEN, "%s", "dummy password file");
	snprintf(cfg_safe.metadata, KP_METADATA_MAX_LEN, "%s", "dummy config file");

	if ((ret = kp_safe_save(ctx, &cfg_safe)) != KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_safe_close(ctx, &cfg_safe)) != KP_SUCCESS) {
		return ret;
	}


	return KP_SUCCESS;
}

kp_error_t
kp_cfg_load(struct kp_ctx *ctx)
{
	kp_error_t ret;
	struct kp_safe cfg_safe;

	if ((ret = kp_safe_load(ctx, &cfg_safe, KP_CONFIG_SAFE_NAME))
			!= KP_SUCCESS) {
		return ret;
	}

	/* As there is no real config we just check we are able to open the
	 * config safe */
	if ((ret = kp_safe_open(ctx, &cfg_safe, false)) != KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_safe_close(ctx, &cfg_safe)) != KP_SUCCESS) {
		return ret;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_cfg_save(struct kp_ctx *ctx)
{
	/* Nothing to do for now */
	return KP_SUCCESS;
}
