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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MACHINA_MACHINEBUILDER_HPP
#define MACHINA_MACHINEBUILDER_HPP

#include <list>
#include "raul/SharedPtr.hpp"

namespace Machina {

class Machine;
class Node;


class MachineBuilder {
public:
	MachineBuilder(SharedPtr<Machine> machine,
	               double             quantization);

	void set_time(Raul::TimeStamp time) { _time = time; }

	void event(Raul::TimeStamp time_offset, size_t size, unsigned char* buf);

	void reset();
	void resolve();

	SharedPtr<Machine> finish();

private:
	bool is_delay_node(SharedPtr<Node> node) const;
	void set_node_duration(SharedPtr<Node> node, Raul::TimeDuration d) const;

	SharedPtr<Node>
	connect_nodes(SharedPtr<Machine> m,
                  SharedPtr<Node>    tail, Raul::TimeStamp tail_end_time,
     	          SharedPtr<Node>    head, Raul::TimeStamp head_start_time);

	typedef std::list<SharedPtr<Node> > ActiveList;
	ActiveList _active_nodes;

	typedef std::list<std::pair<Raul::TimeStamp, SharedPtr<Node> > > PolyList;
	PolyList _poly_nodes;

	double             _quantization;
	Raul::TimeStamp    _time;

	SharedPtr<Machine> _machine;
	SharedPtr<Node>    _initial_node;
	SharedPtr<Node>    _connect_node;
	Raul::TimeStamp    _connect_node_end_time;
};


} // namespace Machina

#endif // MACHINA_MACHINEBUILDER_HPP
