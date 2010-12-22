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

#ifndef MACHINA_RECORDER_HPP
#define MACHINA_RECORDER_HPP

#include "raul/Slave.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/EventRingBuffer.hpp"

#include "machina/Machine.hpp"

namespace Machina {

class MachineBuilder;


class Recorder : public Raul::Slave {
public:
	Recorder(size_t buffer_size, TimeUnit unit, double q, bool step);

	inline void write(Raul::TimeStamp time, size_t size, const unsigned char* buf) {
		_record_buffer.write(time, size, buf);
	}

	SharedPtr<Machine> finish();

private:
	virtual void _whipped();

	TimeUnit                  _unit;
	Raul::EventRingBuffer     _record_buffer;
	SharedPtr<MachineBuilder> _builder;
};


} // namespace Machina

#endif // MACHINA_RECORDER_HPP
