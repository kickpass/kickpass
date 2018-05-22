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

#include <sys/stat.h>

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
kp_cfg_create(struct kp_ctx *ctx, const char *sub)
{
	kp_error_t ret;
	char path[PATH_MAX] = "";
	struct kp_safe cfg_safe;

	if (strlcpy(path , sub, PATH_MAX) >= PATH_MAX) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	if (strlcat(path , "/" KP_CONFIG_SAFE_NAME, PATH_MAX) >= PATH_MAX) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	if ((ret = kp_safe_init(ctx, &cfg_safe, path)) != KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_safe_open(ctx, &cfg_safe, KP_CREATE)) != KP_SUCCESS) {
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
kp_cfg_load(struct kp_ctx *ctx, const char *sub)
{
	kp_error_t ret;
	char path[PATH_MAX] = "";
	struct kp_safe cfg_safe;
	char *line = NULL, *save_line = NULL;

	if (strlcpy(path , sub, PATH_MAX) >= PATH_MAX) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	if (strlcat(path , "/" KP_CONFIG_SAFE_NAME, PATH_MAX) >= PATH_MAX) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	if ((ret = kp_safe_init(ctx, &cfg_safe, path)) != KP_SUCCESS) {
		return ret;
	}

	if ((ret = kp_safe_open(ctx, &cfg_safe, 0)) != KP_SUCCESS) {
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
		if (config != NULL) {
			config->setter(config, ctx, value);
		}

		line = strtok_r(NULL, "\n", &save_line);
	}

	if ((ret = kp_safe_close(ctx, &cfg_safe)) != KP_SUCCESS) {
		return ret;
	}

	return KP_SUCCESS;
}

kp_error_t
kp_cfg_save(struct kp_ctx *ctx, const char *sub)
{
	/* Nothing to do for now */
	return KP_SUCCESS;
}

kp_error_t
kp_cfg_find(struct kp_ctx *ctx, const char *path, char *cfg_path, size_t size)
{
	char *dir = NULL;
	struct stat stats;

	if (strlcpy(cfg_path, "/", size) >= size) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	if (strlcat(cfg_path, path, size) >= size) {
		errno = ENAMETOOLONG;
		return KP_ERRNO;
	}

	while ((dir = strrchr(cfg_path, '/')) != NULL) {
		char abspath[PATH_MAX] = "";

		dir[0] = '\0';

		if (strlcpy(abspath, ctx->ws_path, PATH_MAX) >= PATH_MAX) {
			errno = ENAMETOOLONG;
			return KP_ERRNO;
		}

		if (strlcat(abspath, "/", PATH_MAX) >= PATH_MAX) {
			errno = ENAMETOOLONG;
			return KP_ERRNO;
		}

		if (strlcat(abspath, cfg_path, PATH_MAX) >= PATH_MAX) {
			errno = ENAMETOOLONG;
			return KP_ERRNO;
		}

		if (strlcat(abspath, "/" KP_CONFIG_SAFE_NAME, PATH_MAX) >= PATH_MAX) {
			return KP_ERRNO;
		}

		if (stat(abspath, &stats) < 0) {
			if (errno == ENOENT) {
				continue;
			}
			return KP_ERRNO;
		} else {
			break;
		}
	}

	if (dir == NULL) {
		errno = ENOENT;
		return KP_ERRNO;
	}

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
