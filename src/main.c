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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sodium.h>
#include <unistd.h>

#include "kickpass.h"

#include "command.h"
#include "kpagent.h"
#include "log.h"
#include "prompt.h"
#include "safe.h"

/* commands */
#ifdef HAS_X11
#include "command/copy.h"
#endif /* HAS_X11 */
#include "command/create.h"
#include "command/delete.h"
#include "command/edit.h"
#include "command/init.h"
#include "command/list.h"
#include "command/cat.h"
#include "command/rename.h"
#include "command/agent.h"
#include "command/open.h"

static int        cmd_search(const void *, const void *);
static int        cmd_sort(const void *, const void *);
static kp_error_t command(struct kp_ctx *, int, char **);
static kp_error_t help(struct kp_ctx *, int, char **);
static kp_error_t parse_opt(struct kp_ctx *, int, char **);
static kp_error_t setup_prompt(struct kp_ctx *);
static kp_error_t show_version(struct kp_ctx *);
static kp_error_t usage(void);

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

	/* kp_cmd_cat */
	{ "cat",     &kp_cmd_cat },
	{ "show",    &kp_cmd_cat },

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

	/* kp_cmd_agent */
	{ "agent",   &kp_cmd_agent },

	/* kp_cmd_open */
	{ "open",   &kp_cmd_open },
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
	char *socket_path = NULL;

	kp_init(&ctx);

	if ((ret = parse_opt(&ctx, argc, argv)) != KP_SUCCESS) {
		goto out;
	}

	if ((ret = setup_prompt(&ctx)) != KP_SUCCESS) {
		goto out;
	}

	/* Try to connect to agent */
	if ((socket_path = getenv(KP_AGENT_SOCKET_ENV)) != NULL) {
		if ((ret = kp_agent_init(&ctx.agent, socket_path)) != KP_SUCCESS) {
			kp_warn(ret, "cannot connect to agent socket %s", socket_path);
			return ret;
		}

		if ((ret = kp_agent_connect(&ctx.agent)) != KP_SUCCESS) {
			kp_warn(ret, "cannot connect to agent socket %s", socket_path);
			return ret;
		}
	}

	ret = command(&ctx, argc, argv);

out:
	kp_fini(&ctx);

	return ret;
}

/*
 * Setup best prompt for password.
 */
static kp_error_t
setup_prompt(struct kp_ctx *ctx)
{
	int fd;
	const char *tty;

	tty = ctermid(NULL);
	fd = open(tty, O_RDWR);
	if (isatty(fd)) {
		ctx->password_prompt = kp_readpass;
	} else {
		ctx->password_prompt = kp_askpass;
	}

	close(fd);
	return KP_SUCCESS;
}

/*
 * Parse global argument
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
			return usage();
		default:
			kp_warnx(KP_EINPUT, "unknown option %c", opt);
			return KP_EINPUT;
		}
	}

	return KP_SUCCESS;
}

static int
cmd_search(const void *k, const void *e)
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
	cmd = bsearch(command, cmds, CMD_COUNT, sizeof(struct cmd), cmd_search);

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

	return cmd->main(ctx, argc, argv);
}

static kp_error_t
show_version(struct kp_ctx *ctx)
{
	printf("KickPass version %s\n", kp_version_string());

	return KP_EXIT;
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
static kp_error_t
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

	return KP_EXIT;
}
