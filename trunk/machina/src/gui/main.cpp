/* This file is part of Machina.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
#include <signal.h>
#include <iostream>
#include <string>
#include <libgnomecanvasmm.h>
#include "redlandmm/World.hpp"
#include "machina/Engine.hpp"
#include "machina/Loader.hpp"
#include "machina/Machine.hpp"
#include "machina/SMFDriver.hpp"
#include "MachinaGUI.hpp"

#ifdef HAVE_JACK
#include "machina/JackDriver.hpp"
#endif

using namespace std;
using namespace Machina;


int
main(int argc, char** argv)
{
	if ( ! Glib::thread_supported())
		Glib::thread_init();

	Redland::World rdf_world;

	SharedPtr<Machina::Machine> machine;

	// Load machine, if given
#if 0
	if (argc >= 2) {
		const string filename = argv[1];
		cout << "Building machine from MIDI file " << filename << endl;
		SharedPtr<Machina::SMFDriver> file_driver(new Machina::SMFDriver());

		if (argc >= 3) {
			float q = strtof(argv[2], NULL);
			cout << "Quantization: " << q << endl;
			machine = file_driver->learn(filename, q);
		} else {
			cout << "No quantization." << endl;
			machine = file_driver->learn(filename);
		}

		if (!machine) {
			cout << "Not a MIDI file.  Attempting to load as Machina file." << endl;
			machine = Loader(rdf_world).load(filename);
		}
	}
#endif

	if (!machine)
		machine = SharedPtr<Machine>(new Machine(TimeUnit(TimeUnit::BEATS, 19200)));

	// Build engine
	SharedPtr<Driver> driver;
#ifdef HAVE_JACK
	driver = SharedPtr<Driver>(new JackDriver(machine));
	((JackDriver*)driver.get())->attach("machina");
#endif
	if (!driver)
		driver = SharedPtr<Driver>(new SMFDriver(machine));

	SharedPtr<Engine> engine(new Engine(driver, rdf_world));

	Gnome::Canvas::init();
	Gtk::Main app(argc, argv);

	driver->activate();
	MachinaGUI gui(engine);

	app.run(*gui.window());

	return 0;
}


