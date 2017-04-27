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
#include <stdlib.h>
#include <string.h>

#include "kickpass.h"

#include "config.h"
#include "safe.h"

#define KP_CONFIG_SAFE_NAME ".config"
#define KP_CONFIG_TEMPLATE \
	"memlimit: %lu\n" \
	"opslimit: %llu\n" \

#define CONFIG(name, type) { .key = #name, \
                             .offset = (size_t)((const volatile void *)&((struct kp_ctx *)0)->cfg.name), \
                             .getter = type ## _config_getter, \
                             .setter = type ## _config_setter }
#define CONFIG_GET(config, ctx, type) ((type *)(&((char *)ctx)[config->offset]))

#define N_CONFIG (sizeof(configs)/sizeof(configs[0]))

struct config;

static int config_sort(const void *, const void *);
static int config_search(const void *, const void *);
static kp_error_t size_t_config_getter(struct config *, struct kp_ctx *, char **);
static kp_error_t size_t_config_setter(struct config *, struct kp_ctx *, char *);
static kp_error_t llu_config_getter(struct config *, struct kp_ctx *, char **);
static kp_error_t llu_config_setter(struct config *, struct kp_ctx *, char *);

struct config {
	char *key;
	size_t offset;
	kp_error_t (*getter)(struct config *, struct kp_ctx *, char **);
	kp_error_t (*setter)(struct config *, struct kp_ctx *, char *);
} configs[] = {
	CONFIG(memlimit, size_t),
	CONFIG(opslimit, llu),
};

kp_error_t
kp_cfg_create(struct kp_ctx *ctx)
{
	kp_error_t ret;
	struct kp_safe cfg_safe;

	if ((ret = kp_safe_create(ctx, &cfg_safe, KP_CONFIG_SAFE_NAME))
			!= KP_SUCCESS) {
		return ret;
	}

	snprintf(cfg_safe.password, KP_PASSWORD_MAX_LEN, "%s", "");
	snprintf(cfg_safe.metadata, KP_METADATA_MAX_LEN, KP_CONFIG_TEMPLATE,
	         ctx->cfg.memlimit, ctx->cfg.opslimit);

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
	char *line = NULL, *save_line = NULL;

	if ((ret = kp_safe_load(ctx, &cfg_safe, KP_CONFIG_SAFE_NAME))
			!= KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_safe_open(ctx, &cfg_safe, false)) != KP_SUCCESS) {
		return ret;
	}

	if (ctx->agent.connected) {
		if ((ret = kp_safe_store(ctx, &cfg_safe, 3600)) != KP_SUCCESS) {
			return ret;
		}
	}

	qsort(configs, N_CONFIG, sizeof(struct config), config_sort);

	line = strtok_r(cfg_safe.metadata, "\n", &save_line);
	while (line != NULL) {
		struct config *config;
		char *key, *value;

		key = strtok(line, ":");
		value = strtok(NULL, ":");

		config = bsearch(key, configs, N_CONFIG, sizeof(struct config), config_search);
		if (config == NULL) {
			continue;
		}

		config->setter(config, ctx, value);

		line = strtok_r(NULL, "\n", &save_line);
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

static int
config_sort(const void *a, const void *b)
{
	return strcmp(((struct config *)a)->key, ((struct config *)b)->key);
}

static int
config_search(const void *k, const void *e)
{
	return strcmp(k, ((struct config *)e)->key);
}

static kp_error_t
size_t_config_setter(struct config *config, struct kp_ctx *ctx, char *str_value)
{
	size_t *value = NULL;
	value = CONFIG_GET(config, ctx, size_t);

	*value = atol(str_value);

	return KP_SUCCESS;
}

static kp_error_t
size_t_config_getter(struct config *config, struct kp_ctx *ctx, char **str_value)
{
	size_t *value = NULL;
	value = CONFIG_GET(config, ctx, size_t);
	if (asprintf(str_value, "%lu", *value) < 0) {
		errno = ENOMEM;
		return KP_ERRNO;
	}

	return KP_SUCCESS;
}

static kp_error_t
llu_config_setter(struct config *config, struct kp_ctx *ctx, char *str_value)
{
	long long unsigned *value = NULL;
	value = CONFIG_GET(config, ctx, long long unsigned);

	*value = atoll(str_value);

	return KP_SUCCESS;
}

static kp_error_t
llu_config_getter(struct config *config, struct kp_ctx *ctx, char **str_value)
{
	long long unsigned *value = NULL;
	value = CONFIG_GET(config, ctx, long long unsigned);
	if (asprintf(str_value, "%llu", *value) < 0) {
		errno = ENOMEM;
		return KP_ERRNO;
	}

	return KP_SUCCESS;
}
