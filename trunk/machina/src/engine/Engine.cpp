/* This file is part of Machina.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
 *
 * Machina is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Machina is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Machina.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glibmm/ustring.h>

#include "machina-config.h"
#include "machina/Engine.hpp"
#include "machina/Loader.hpp"
#include "machina/Machine.hpp"
#include "SMFDriver.hpp"
#ifdef HAVE_JACK
#include "JackDriver.hpp"
#endif

namespace Machina {

Engine::Engine(SharedPtr<Driver> driver, Redland::World& rdf_world)
	: _driver(driver)
	, _rdf_world(rdf_world)
	, _loader(_rdf_world)
{
}


SharedPtr<Driver>
Engine::new_driver(const std::string& name, SharedPtr<Machine> machine)
{
	#ifdef HAVE_JACK
	if (name == "jack") {
		JackDriver* driver = new JackDriver(machine);
		driver->attach("machina");
		return SharedPtr<Driver>(driver);
	}
	#endif
	if (name == "smf")
		return SharedPtr<Driver>(new SMFDriver(machine->time().unit()));

	std::cerr << "Error: Unknown driver type `" << name << "'" << std::endl;
	return SharedPtr<Driver>();
}

/** Load the machine at @a uri, and run it (replacing current machine).
 * Safe to call while engine is processing.
 */
SharedPtr<Machine>
Engine::load_machine(const Glib::ustring& uri)
{
	SharedPtr<Machine> machine = _loader.load(uri);
	SharedPtr<Machine> old_machine;
	if (machine) {
		old_machine = _driver->machine(); // Keep a reference to old machine...
		machine->activate();
		_driver->set_machine(machine); // Switch driver to new machine
	}

	// .. and drop it in this thread (to prevent deallocation in the RT thread)

	return machine;
}

/** Build a machine from the MIDI at @a uri, and run it (replacing current machine).
 * Safe to call while engine is processing.
 */
SharedPtr<Machine>
Engine::load_machine_midi(const Glib::ustring& uri, double q, Raul::TimeDuration dur)
{
	SharedPtr<SMFDriver> file_driver(new SMFDriver(dur.unit()));
	SharedPtr<Machine>   machine = file_driver->learn(uri, q, dur);
	SharedPtr<Machine>   old_machine;
	if (machine) {
		old_machine = _driver->machine(); // Keep a reference to old machine...
		machine->activate();
		_driver->set_machine(machine); // Switch driver to new machine
	}

	// .. and drop it in this thread (to prevent deallocation in the RT thread)

	return machine;
}

void
Engine::import_machine(SharedPtr<Machine> machine)
{
	machine->activate();
	_driver->machine()->nodes().append(machine->nodes());
	// FIXME: thread safe?
	// FIXME: announce
}

void
Engine::export_midi(const Glib::ustring& filename, Raul::TimeDuration dur)
{
	SharedPtr<Machine>            machine = _driver->machine();
	SharedPtr<Machina::SMFDriver> file_driver(new Machina::SMFDriver(dur.unit()));

	const bool activated = _driver->is_activated();
	if (activated)
		_driver->deactivate(); // FIXME: disable instead

	machine->set_sink(file_driver->writer());
	file_driver->writer()->start(filename, TimeStamp(dur.unit(), 0.0));
	file_driver->run(machine, dur);
	machine->set_sink(_driver);
	machine->reset(machine->time());
	file_driver->writer()->finish();

	if (activated)
		_driver->activate();
}

void
Engine::set_bpm(double bpm)
{
	_driver->set_bpm(bpm);
}


void
Engine::set_quantization(double q)
{
	_driver->set_quantization(q);
}


} // namespace Machina


