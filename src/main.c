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

#include <getopt.h>
#include <gpgme.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "kickpass_config.h"
#include "storage.h"

static int parse_opt(int argc, char **argv);
static int show_version(void);

int
main(int argc, char **argv)
{

	if (parse_opt(argc, argv) != 0) error("cannot parse command line arguments");

	return EXIT_SUCCESS;
}

static int
parse_opt(int argc, char **argv)
{
	int opt;
	static struct option longopts[] = {
		{ "version", no_argument, NULL, 'v' },
		{ NULL,      0,           NULL, 0   },
	};

	while ((opt = getopt_long(argc, argv, "v", longopts, NULL)) != -1) {
		switch (opt) {
		case 'v':
			show_version();
			exit(EXIT_SUCCESS);
		}
	}

	return KP_SUCCESS;
}

static int
show_version(void)
{
	struct kp_storage_ctx *ctx;
	char storage_engine[10], storage_version[10];

	printf("KickPass version %d.%d.%d\n",
			KICKPASS_VERSION_MAJOR,
			KICKPASS_VERSION_MINOR,
			KICKPASS_VERSION_PATCH);

	kp_storage_init(&ctx);
	kp_storage_get_engine(ctx, storage_engine, sizeof(storage_engine));
	kp_storage_get_version(ctx, storage_version, sizeof(storage_version));
	printf("storage engine %s %s\n", storage_engine, storage_version);

	return KP_SUCCESS;
}
