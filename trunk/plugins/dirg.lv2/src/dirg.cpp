/* Dirg
 * Copyright (C) 2010 David Robillard <http://drobilla.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <math.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <set>

#include "binary_location.h"
#include "dirg_internal.hpp"
#include "UI.hpp"

using std::cout;
using std::cerr;
using std::endl;

sem_t    sem;
PadState state;
SPtr<UI> web_ui;
SPtr<UI> pad_ui;

static void
interrupted(int)
{
	cout << "dirg: Interrupted" << endl;
	sem_post(&sem);
}

static void
button_pressed(ButtonID id)
{
	const ButtonState& but = state.get(id);
	const float        val = (but.value == 0.0f) ? 1.0f : 0.0f;
	state.set_colour(id, but.hue, val);
	if (web_ui)
		web_ui->set_colour(id, but.hue, val);
	if (pad_ui)
		pad_ui->set_colour(id, but.hue, val);
}

static int
print_usage(int argc, char** argv)
{
	fprintf(stderr, "USAGE: %s [WWW_DIR]\n", argv[0]);
	return 1;
}

int
main(int argc, char** argv)
{
	if (argc > 2)
		return print_usage(argc, argv);

	std::string dir;
	if (argc == 2) {
		dir = argv[1];
	} else {
		char* binloc = binary_location();
		dir = binloc;
		free(binloc);
		size_t last_slash = dir.find_last_of('/');
		if (last_slash == std::string::npos)
			last_slash = dir.find_last_of('\\');
		if (last_slash == std::string::npos)
			return print_usage(argc, argv);

		dir = dir.substr(0, last_slash);
		printf("dirg: Bundle directory: %s\n", dir.c_str());
	}

	sem_init(&sem, 0, 0);
	signal(SIGINT,  interrupted);
	signal(SIGTERM, interrupted);

	web_ui = dirg_new_web_ui(state, dir.c_str());
	pad_ui = dirg_new_launchpad_ui(state, dir.c_str());

	SPtr<UI> p = web_ui;

	if (web_ui) {
		web_ui->button_pressed.connect(sigc::ptr_fun(button_pressed));
		web_ui->activate();
	}
	if (pad_ui) {
		pad_ui->button_pressed.connect(sigc::ptr_fun(button_pressed));
		pad_ui->activate();
	}
	
	cout << "(Press Ctrl-c to exit)" << endl;
	sem_wait(&sem);

	return 0;
}
