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

#ifndef KP_STORAGE_H
#define KP_STORAGE_H

#include "kickpass.h"

#include "safe.h"

/**
 * Abstract kickpass storage context
 */
struct kp_storage;

kp_error_t kp_storage_init(struct kp_ctx *ctx, struct kp_storage **storage);
kp_error_t kp_storage_fini(struct kp_storage *storage);

kp_error_t kp_storage_get_engine(struct kp_storage *storage, char *engine, size_t dstsize);
kp_error_t kp_storage_get_version(struct kp_storage *storage, char *version, size_t dstsize);

kp_error_t kp_storage_save(struct kp_storage *storage, struct kp_safe *safe);

#endif /* KP_STORAGE_H */
