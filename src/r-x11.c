/***
	This file is part of LockPassword
	Copyright (C) 2020-2021 Aleksandr D. Goncharov (Joursoir) <chat@joursoir.net>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
***/

#if defined(DISPLAY)

#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>

#include "constants.h"

static Atom X_utf8;

static void send_data(Display *dpy, XSelectionRequestEvent *sev,
	const char *data)
{
	XSelectionEvent ssev;
	Atom target = sev->target;
	char *an;

	an = XGetAtomName(dpy, target);
	if(an) {
		dbgprint("Request of type '%s'\n", an);
		XFree(an);
	}

	/* All of these should match the values of the request. */
	ssev.type = SelectionNotify;
	ssev.requestor = sev->requestor;
	ssev.selection = sev->selection;
	ssev.target = target;
	ssev.property = None; // means refusal
	ssev.time = sev->time;

	if(target == X_utf8) {
		an = XGetAtomName(dpy, sev->property);
		if(an) {
			dbgprint("Sending data, property '%s'\n", sev->requestor, an);
			XFree(an);
		}

		ssev.property = sev->property;
		XChangeProperty(dpy, sev->requestor, sev->property, X_utf8, 8,
			PropModeReplace, (unsigned char *)data, strlen(data));
	}
	else {
		dbgprint("No valid target. Refuse request.\n");
	}

	XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
}

int run_clipboard(const char *data)
{
	Display *dpy;
	Window owner, root;
	int screen;
	Atom sel;
	XEvent ev;
	XSelectionRequestEvent *sev;

	dpy = XOpenDisplay(NULL); // means use env $DISPLAY
	if(!dpy)
		errprint(1, "Open X display failed\n");

	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	owner = XCreateSimpleWindow(dpy, root, -10, -10, 1, 1, 0, 0, 0);

	sel = XInternAtom(dpy, "CLIPBOARD", False);
	X_utf8 = XInternAtom(dpy, "UTF8_STRING", False);

	XSetSelectionOwner(dpy, sel, owner, CurrentTime);
	for(;;)
	{
		XNextEvent(dpy, &ev);
		switch(ev.type) {
		case SelectionClear:
			dbgprint("Lost 'CLIPBOARD' ownership\n");
			return 2;
		case SelectionRequest:
			sev = (XSelectionRequestEvent *)&ev.xselectionrequest;
			dbgprint("SelectionRequest by 0x%lx\n", sev->requestor);

			send_data(dpy, sev, data);
		}
	}
}

#endif /* defined(DISPLAY) */
