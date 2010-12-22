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

#ifndef MACHINA_DRIVER_HPP
#define MACHINA_DRIVER_HPP

#include "raul/MIDISink.hpp"

namespace Machina {

class Machine;


class Driver : public Raul::MIDISink {
public:
	Driver(SharedPtr<Machine> machine) : _machine(machine) {}
	virtual ~Driver() {}

	SharedPtr<Machine> machine() { return _machine; }
	virtual void set_machine(SharedPtr<Machine> machine) { _machine = machine; }

	virtual void set_bpm(double bpm) = 0;
	virtual void set_quantization(double q) = 0;

	virtual void activate() {}
	virtual void deactivate() {}

	virtual void stop() {}

	virtual bool recording() { return false; }
	virtual void start_record(bool step) {}
	virtual void finish_record() {}

protected:
	SharedPtr<Machine> _machine;
};


} // namespace Machina

#endif // MACHINA_JACKDRIVER_HPP
