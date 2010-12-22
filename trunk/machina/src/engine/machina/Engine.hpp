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

#ifndef MACHINA_ENGINE_HPP
#define MACHINA_ENGINE_HPP

#include <string>

#include <glibmm/ustring.h>

#include "raul/SharedPtr.hpp"

#include "machina/Driver.hpp"
#include "machina/Loader.hpp"

namespace Machina {

class Machine;


class Engine {
public:
	Engine(SharedPtr<Driver> driver, Redland::World& rdf_world)
		: _driver(driver)
		, _rdf_world(rdf_world)
		, _loader(_rdf_world)
	{
	}

	Redland::World& rdf_world() { return _rdf_world; }

	static SharedPtr<Driver> new_driver(const std::string& name,
	                                    SharedPtr<Machine> machine);

	SharedPtr<Driver>  driver()  { return _driver; }
	SharedPtr<Machine> machine() { return _driver->machine(); }

	SharedPtr<Machine> load_machine(const Glib::ustring& uri);
	SharedPtr<Machine> import_machine(const Glib::ustring& uri);
	SharedPtr<Machine> import_midi(const Glib::ustring& uri, double q, Raul::TimeDuration d);

	void set_bpm(double bpm);
	void set_quantization(double beat_fraction);

private:
	SharedPtr<Driver> _driver;
	Redland::World&   _rdf_world;
	Loader            _loader;
};


} // namespace Machina

#endif // MACHINA_ENGINE_HPP
