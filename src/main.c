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
#include "log.h"

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
static void       usage(void);
static kp_error_t help(struct kp_ctx *, int, char **);

struct cmd {
	const char    *name;
	struct kp_cmd *cmd;
};

#define CMD_COUNT (sizeof(cmds)/sizeof(cmds[0]))

static struct kp_cmd kp_cmd_help = {
	.main  = help,
	.usage = NULL,
	.opts  = "help <command>",
	.desc  = "Print help for given command",
};

static struct cmd cmds[] = {
	/* kp_cmd_help */
	{ "help",    &kp_cmd_help },
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
			usage();
			return KP_SUCCESS;
		default:
			kp_warnx(KP_EINPUT, "unknown option %c", opt);
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
 * Find command
 */
static struct kp_cmd *
find_command(const char *command)
{
	const struct cmd *cmd;

	qsort(cmds, CMD_COUNT, sizeof(struct cmd), cmd_sort);
	cmd = bsearch(command, cmds, CMD_COUNT, sizeof(struct cmd), cmd_cmp);

	if (!cmd)
		kp_errx(KP_EINPUT, "unknown command %s", command);

	return cmd->cmd;
}

/*
 * Call given command and let it parse its own arguments.
 */
static kp_error_t
command(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret;
	struct kp_cmd *cmd;

	if (optind >= argc)
		kp_errx(KP_EINPUT, "missing command");

	/* Test for help first so we don't mess with cmds */
	if (strncmp(argv[optind], "help", 4) == 0) {
		cmd = &kp_cmd_help;
	} else {
		cmd = find_command(argv[optind]);
	}

	optind++;

	/* Only init and help cannot load main config */
	if (cmd != &kp_cmd_init && cmd != &kp_cmd_help) {
		kp_prompt_password("master", false, (char *)ctx->password);
		if ((ret = kp_load(ctx)) != KP_SUCCESS) {
			kp_warn(ret, "cannot unlock workspace");
			return ret;
		}
	}

	return cmd->main(ctx, argc, argv);
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

static kp_error_t
help(struct kp_ctx *ctx, int argc, char **argv)
{
	extern char *__progname;
	struct kp_cmd *cmd;

	if (optind >= argc) {
		usage();
		return KP_EINPUT;
	}

	cmd = find_command(argv[optind]);

	printf("usage: %s %s\n"
	       "\n", __progname, cmd->opts);
	if (cmd->usage) cmd->usage();

	return KP_SUCCESS;
}

/*
 * Print global usage by calling each command own usage function.
 */
static void
usage(void)
{
	int i, opts_max_len = 0;
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
		size_t opts_len = strlen(cmds[i].cmd->opts);
		if (opts_len > opts_max_len) opts_max_len = opts_len;
	}

	opts_max_len++;

	for (i = 0; i < CMD_COUNT; i++) {
		if (cmds[i-1].cmd == cmds[i].cmd) continue;
		printf("    %*s%s\n", -opts_max_len, cmds[i].cmd->opts, cmds[i].cmd->desc);
	}
}
