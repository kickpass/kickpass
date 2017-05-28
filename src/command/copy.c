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

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "kickpass.h"

#include "command.h"
#include "copy.h"
#include "prompt.h"
#include "safe.h"
#include "log.h"

static kp_error_t copy(struct kp_ctx *ctx, int argc, char **argv);

struct kp_cmd kp_cmd_copy = {
	.main  = copy,
	.usage = NULL,
	.opts  = "copy <safe>",
	.desc  = "Copy a password (first line of safe) into X clipboard",
};

kp_error_t
copy(struct kp_ctx *ctx, int argc, char **argv)
{
	kp_error_t ret;
	struct kp_safe safe;
	Display *display;
	Window window;
	Atom XA_CLIPBOARD, XA_TARGETS;
	bool replied = false;
	size_t password_len;

	if (argc - optind != 1) {
		ret = KP_EINPUT;
		kp_warn(ret, "missing safe name");
		return ret;
	}

	if ((ret = kp_safe_init(ctx, &safe, argv[optind])) != KP_SUCCESS) {
		kp_warn(ret, "cannot init %s", argv[optind]);
		return ret;
	}

	if ((ret = kp_safe_open(ctx, &safe, 0)) != KP_SUCCESS) {
		kp_warn(ret, "cannot open %s", argv[optind]);
		goto out;
	}

	password_len = strlen(safe.password);

	display = XOpenDisplay(NULL);

	XA_CLIPBOARD     = XInternAtom(display, "CLIPBOARD", True);
	XA_TARGETS       = XInternAtom(display, "TARGETS", True);

	window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1, 1, 0, 0, 0);
	XSelectInput(display, window, PropertyChangeMask);
	XSetSelectionOwner(display, XA_PRIMARY, window, CurrentTime);
	XSetSelectionOwner(display, XA_CLIPBOARD, window, CurrentTime);

	if (daemon(0, 0) != 0) {
		ret = KP_ERRNO;
		kp_warn(ret, "cannot daemonize");
		goto out;
	}

	do {
		XEvent event;
		XSelectionRequestEvent *request;
		XSelectionEvent reply;
		Atom possibleTargets[] = {
			XInternAtom(display, "STRING", True),
			XInternAtom(display, "TEXT", True),
			XInternAtom(display, "COMPOUND_TEXT", True),
			XInternAtom(display, "UTF8_STRING", True),
			XInternAtom(display, "text/plain", False),
			XInternAtom(display, "text/plain;charset=utf-8", False),
		};

		XNextEvent(display, &event);

		if (event.type != SelectionRequest) {
		       continue;
		}

		request = &event.xselectionrequest;

		reply.type       = SelectionNotify;
		reply.send_event = True;
		reply.display    = display;
		reply.requestor  = request->requestor;
		reply.selection  = request->selection;
		reply.property   = request->property;
		reply.target     = None;
		reply.time       = request->time;

		if (request->target == XA_TARGETS) {

			XChangeProperty(display, request->requestor,
					request->property, XA_ATOM, 32,
					PropModeReplace,
					(unsigned char *) possibleTargets, 3);
		} else {
			bool supported = false;
			for (int i = 0;
			     i < sizeof(possibleTargets)/sizeof(possibleTargets[0]);
			     i++) {
				supported |= (request->target == possibleTargets[i]);
			}

			if (supported) {
				XChangeProperty(display, request->requestor,
						request->property, request->target,
						8, PropModeReplace,
						(unsigned char *)safe.password,
						password_len);
				replied = true;
			} else {
				char *name = XGetAtomName(display,
				                          request->target);
				kp_warn(KP_EINPUT, "don't know what to answer to %s",
				        name);
				if (name) {
					XFree(name);
				}
				reply.property = None;
				replied = true;
			}
		}

		XSendEvent(display, event.xselectionrequest.requestor, 0, 0,
				(XEvent *)&reply);
		XSync(display, False);


	} while (!replied);

	XCloseDisplay(display);

out:
	if ((ret = kp_safe_close(ctx, &safe)) != KP_SUCCESS) {
		kp_warn(ret, "cannot cleanly close safe"
			"clear text password might have leaked");
		return ret;
	}

	return KP_SUCCESS;
}
