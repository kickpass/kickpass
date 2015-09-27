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
#include <sodium.h>

#include "kickpass.h"

#include "command.h"
#include "safe.h"
#include "prompt.h"

/* commands */
#ifdef HAS_X11
#include "command/copy.h"
#endif /* HAS_X11 */
#include "command/create.h"
#include "command/delete.h"
#include "command/edit.h"
#include "command/init.h"
#include "command/list.h"
#include "command/open.h"
#include "command/rename.h"

static int        cmd_cmp(const void *, const void *);
static int        cmd_sort(const void *, const void *);
static kp_error_t command(struct kp_ctx *, int, char **);
static kp_error_t parse_opt(struct kp_ctx *, int, char **);
static kp_error_t show_version(struct kp_ctx *);
static kp_error_t usage(struct kp_ctx *);

struct cmd {
	const char    *name;
	struct kp_cmd *cmd;
};

#define CMD_COUNT (sizeof(cmds)/sizeof(cmds[0]))

static struct cmd cmds[] = {
	/* kp_cmd_init */
	{ "init",    &kp_cmd_init },

	/* kp_cmd_create */
	{ "create",  &kp_cmd_create },
	{ "new",     &kp_cmd_create },
	{ "insert",  &kp_cmd_create },

	/* kp_cmd_open */
	{ "show",    &kp_cmd_open },
	{ "open",    &kp_cmd_open },
	{ "cat",     &kp_cmd_open },

	/* kp_cmd_edit */
	{ "edit",    &kp_cmd_edit },

#ifdef HAS_X11
	/* kp_cmd_copy */
	{ "copy",    &kp_cmd_copy },
#endif /* HAS_X11 */

	/* kp_cmd_list */
	{ "ls",      &kp_cmd_list },
	{ "list",    &kp_cmd_list },

	/* kp_cmd_delete */
	{ "delete",  &kp_cmd_delete },
	{ "rm",      &kp_cmd_delete },
	{ "remove",  &kp_cmd_delete },
	{ "destroy", &kp_cmd_delete },

	/* kp_cmd_rename */
	{ "rename",  &kp_cmd_rename },
	{ "mv",      &kp_cmd_rename },
	{ "move",    &kp_cmd_rename },
};

/*
 * Parse command line and call matching command.
 * Most command are aliased and parse their own arguments.
 */
int
main(int argc, char **argv)
{
	int ret;
	struct kp_ctx ctx;

	kp_init(&ctx);

	ret = parse_opt(&ctx, argc, argv);

	kp_fini(&ctx);

	return ret;
}

/*
 * Parse global argument and command name.
 */
static kp_error_t
parse_opt(struct kp_ctx *ctx, int argc, char **argv)
{
	int opt;
	static struct option longopts[] = {
		{ "version", no_argument, NULL, 'v' },
		{ "help",    no_argument, NULL, 'h' },
		{ NULL,      0,           NULL, 0   },
	};

	while ((opt = getopt_long(argc, argv, "+vh", longopts, NULL)) != -1) {
		switch (opt) {
		case 'v':
			return show_version(ctx);
		case 'h':
			return usage(ctx);
		default:
			warnx("unknown option %c", opt);
			return KP_EINPUT;
		}
	}

	return command(ctx, argc, argv);
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

/*
 * Call given command and let it parse its own arguments.
 */
static kp_error_t
command(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret;
	const struct cmd *cmd;

	if (optind >= argc)
		errx(KP_EINPUT, "missing command");

	qsort(cmds, CMD_COUNT, sizeof(struct cmd), cmd_sort);
	cmd = bsearch(argv[optind], cmds, CMD_COUNT, sizeof(struct cmd), cmd_cmp);

	if (!cmd)
		errx(KP_EINPUT, "unknown command %s", argv[optind]);

	optind++;

	/* Only init cannot load main config */
	if (cmd->cmd != &kp_cmd_init) {
		char *master;
		master = sodium_malloc(KP_PASSWORD_MAX_LEN);
		if (!master) {
			warnx("memory error");
			return KP_ENOMEM;
		}
		kp_prompt_password("master", false, master);
		ret = kp_load(ctx, master);
		sodium_free(master);
		if (ret != KP_SUCCESS) {
			return ret;
		}
	}

	return cmd->cmd->main(ctx, argc, argv);
}

static kp_error_t
show_version(struct kp_ctx *ctx)
{
	printf("KickPass version %d.%d.%d\n",
			KICKPASS_VERSION_MAJOR,
			KICKPASS_VERSION_MINOR,
			KICKPASS_VERSION_PATCH);

	return KP_SUCCESS;
}

/*
 * Print global usage by calling each command own usage function.
 */
static kp_error_t
usage(struct kp_ctx *ctx)
{
	int i;
	extern char *__progname;
	char usage[] =
		"usage: %s [-hv] <command> [<cmd_opts>] [<args>]\n"
		"\n"
		"options:\n"
		"    -h, --help     Print this help\n"
		"    -v, --version  Print %s version\n"
		"\n"
		"commands:\n";
	printf(usage, __progname, __progname);
	for (i = 0; i < CMD_COUNT; i++) {
		if (cmds[i-1].cmd == cmds[i].cmd) continue;
		printf("    %-20s%s\n", cmds[i].cmd->opts, cmds[i].cmd->desc);
	}

	return KP_SUCCESS;
}
