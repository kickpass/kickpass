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

#ifndef KP_LOG_H
#define KP_LOG_H

#include <stdarg.h>
#include <stdio.h>

#define LOGT(fmt, ...) kp_log(kp_log_trace, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) kp_log(kp_log_debug, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) kp_log(kp_log_info, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) kp_log(kp_log_warn, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) kp_log(kp_log_error, fmt, ##__VA_ARGS__)
#define LOGF(fmt, ...) kp_log(kp_log_fatal, fmt, ##__VA_ARGS__)

enum kp_log_lvl {
	kp_log_trace,
	kp_log_debug,
	kp_log_info,
	kp_log_warn,
	kp_log_error,
	kp_log_fatal,
};

static const char *kp_log_lvl[] = {
	"trace",
	"debug",
	"info",
	"warn",
	"error",
	"fatal",
};

static inline void
kp_vlog(enum kp_log_lvl lvl, char *fmt, va_list ap)
{
	char log[256];

	vsnprintf(log, 256, fmt, ap);
	printf("%-5s %s\n", kp_log_lvl[lvl], log);
}

static inline void
kp_log(enum kp_log_lvl lvl, char *fmt, ...)
__attribute__((format(printf, 2, 3)));

static inline void
kp_log(enum kp_log_lvl lvl, char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	kp_vlog(lvl, fmt, ap);
	va_end(ap);
}

#endif /* KP_LOG_H */
