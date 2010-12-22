/* This file is part of Machina.
 * Copyright (C) 2010 David Robillard <http://drobilla.net>
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

#include "machina/Stateful.hpp"

namespace Machina {

uint64_t Stateful::_next_id = 1;

Stateful::Stateful()
	: _id(_next_id++)
{
}


const Redland::Node&
Stateful::rdf_id(Redland::World& world) const
{
	if (!_rdf_id.is_valid()) {
		std::ostringstream ss;
		ss << "b" << _id;
		_rdf_id = Redland::Node(world, Redland::Node::BLANK, ss.str());
	}

	return _rdf_id;
}

} // namespace Machina
