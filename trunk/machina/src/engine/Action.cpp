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

#include "redlandmm/World.hpp"
#include "redlandmm/Model.hpp"
#include "machina/Action.hpp"

namespace Machina {

void
Action::write_state(Redland::Model& model)
{
	using namespace Raul;

	model.add_statement(rdf_id(model.world()),
			Redland::Node(model.world(), Redland::Node::RESOURCE, "rdf:type"),
			Redland::Node(model.world(), Redland::Node::RESOURCE, "machina:Action"));
}


} // namespace Machina

