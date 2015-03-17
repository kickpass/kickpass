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
#include <string.h>

#include "kickpass.h"

#include "command.h"
#include "storage.h"

/* commands */
#include "command/init.h"
#include "command/create.h"
#include "command/open.h"

static void       parse_opt(int, char **);
static int        cmd_cmp(const void *, const void *);
static int        cmd_sort(const void *, const void *);
static kp_error_t command(int, char **);
static void       show_version(void);
static void       usage(void);
static kp_error_t init_ws_path(struct kp_ctx *);

struct cmd {
	const char    *name;
	struct kp_cmd *cmd;
};

#define CMD_COUNT (sizeof(cmds)/sizeof(cmds[0]))

static struct cmd cmds[] = {
	/* kp_cmd_init */
	{ "init",   &kp_cmd_init },

	/* kp_cmd_create */
	{ "create", &kp_cmd_create },
	{ "new",    &kp_cmd_create },
	{ "insert", &kp_cmd_create },

	/* kp_cmd_open */
	{ "show",   &kp_cmd_open },
	{ "open",   &kp_cmd_open },
};

/*
 * Parse command line and call matching command.
 * Most command are aliased and parse their own arguments.
 */
int
main(int argc, char **argv)
{
	parse_opt(argc, argv);

	if (command(argc, argv) != KP_SUCCESS) {
		return EXIT_FAILURE;
	} else {
		return EXIT_SUCCESS;
	}
}

/*
 * Parse global argument and command name.
 */
static void
parse_opt(int argc, char **argv)
{
	int opt;
	static struct option longopts[] = {
		{ "version", no_argument, NULL, 'v' },
		{ "help",    no_argument, NULL, 'h' },
		{ NULL,      0,           NULL, 0   },
	};

	while ((opt = getopt_long(argc, argv, "vh", longopts, NULL)) != -1) {
		switch (opt) {
		case 'v':
			show_version();

		case 'h':
			usage();

		default:
			errx(KP_EINPUT, "unknown option %c", opt);
		}
	}
}

static int
cmd_cmp(const void *k, const void *e)
{
	return strcmp(k, ((struct cmd *)e)->name);
}

static int
cmd_sort(const void *a, const void *b)
{
	return strcmp(((struct cmd *)a)->name, ((struct cmd *)b)->name);
}

static kp_error_t
init_ws_path(struct kp_ctx *ctx)
{
	const char *home;

	home = getenv("HOME");
	if (!home)
		errx(KP_EINPUT, "cannot find $HOME environment variable");

	if (strlcpy(ctx->ws_path, home, PATH_MAX) >= PATH_MAX)
		errx(KP_ENOMEM, "memory error");

	if (strlcat(ctx->ws_path, "/" KP_PATH, PATH_MAX) >= PATH_MAX)
		errx(KP_ENOMEM, "memory error");

	return KP_SUCCESS;
}

/*
 * Call given command and let it parse its own arguments.
 */
static kp_error_t
command(int argc, char **argv)
{
	kp_error_t ret;
	struct kp_ctx ctx;
	const struct cmd *cmd;

	if (optind >= argc)
		errx(KP_EINPUT, "missing command");

	qsort(cmds, CMD_COUNT, sizeof(struct cmd), cmd_sort);
	cmd = bsearch(argv[optind], cmds, CMD_COUNT, sizeof(struct cmd), cmd_cmp);

	if (!cmd)
		errx(KP_EINPUT, "unknown command %s", argv[optind]);

	optind++;

	if ((ret = init_ws_path(&ctx)) != KP_SUCCESS) return ret;

	return cmd->cmd->main(&ctx, argc, argv);
}

static void
show_version(void)
{
	struct kp_storage *storage;
	struct kp_ctx ctx;
	char storage_engine[10], storage_version[10];

	printf("KickPass version %d.%d.%d\n",
			KICKPASS_VERSION_MAJOR,
			KICKPASS_VERSION_MINOR,
			KICKPASS_VERSION_PATCH);

	kp_storage_init(&ctx, &storage);
	kp_storage_get_engine(storage, storage_engine, sizeof(storage_engine));
	kp_storage_get_version(storage, storage_version, sizeof(storage_version));
	printf("storage engine %s %s\n", storage_engine, storage_version);

	exit(EXIT_SUCCESS);
}

/*
 * Print global usage by calling each command own usage function.
 */
static void
usage(void)
{
	int i;
	extern char *__progname;
	char usage[] =
		"usage: %s [-hv] <command> [<args>]\n"
		"\n"
		"options:\n"
		"    -h, --help     Display this help\n"
		"    -v, --version  Show %s version\n"
		"\n"
		"commands:\n";
	printf(usage, __progname, __progname);
	for (i = 0; i < CMD_COUNT; i++) {
		if (cmds[i-1].cmd == cmds[i].cmd) continue;
		if (cmds[i].cmd->usage) cmds[i].cmd->usage();
	}

	exit(EXIT_SUCCESS);
}
