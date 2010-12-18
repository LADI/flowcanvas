/* This file is part of Machina.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
 *
 * Machina is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Machina is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "machina-config.h"

#include <iostream>
#include <signal.h>
#include "machina/Action.hpp"
#include "machina/Edge.hpp"
#include "machina/Engine.hpp"
#include "machina/Machine.hpp"
#include "machina/MidiAction.hpp"
#include "machina/Node.hpp"
#ifdef HAVE_EUGENE
	#include "machina/Problem.hpp"
#endif

#include "JackDriver.hpp"

using namespace std;
using namespace Machina;


bool quit = false;


void
catch_int(int)
{
	signal(SIGINT, catch_int);
	signal(SIGTERM, catch_int);

	std::cout << "Interrupted" << std::endl;

	quit = true;
}


int
main(int argc, char** argv)
{
	if (argc != 2) {
		cout << "Usage: " << argv[0] << " FILE" << endl;
		return -1;
	}

	if ( ! Glib::thread_supported())
		Glib::thread_init();

	SharedPtr<JackDriver> driver(new JackDriver());

	Redland::World rdf_world;
	Engine engine(driver, rdf_world);

	/* FIXME: Would be nice if this could take URIs on the cmd line
	char* uri = (char*)calloc(6 + strlen(argv[1]), sizeof(char));
	strcpy(uri, "file:");
	strcat(uri, argv[1]);
	engine.load_machine(uri);
	free(uri);
	*/
	engine.load_machine(argv[1]);

	driver->attach("machina");

	signal(SIGINT, catch_int);
	signal(SIGTERM, catch_int);

	while (!quit)
		sleep(1);

	driver->detach();

	return 0;
}

