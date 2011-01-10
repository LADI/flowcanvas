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

#ifndef MACHINA_URIS_HPP
#define MACHINA_URIS_HPP

#include <stdint.h>

#include "raul/Atom.hpp"

#include "machina/types.hpp"

namespace Machina {

class URIs {
public:
	static void init() { _instance = new URIs(); }

	static inline const URIs& instance() { assert(_instance); return *_instance; }

	Raul::Atom machina_MidiAction_atom;

	URIInt machina_active;
	URIInt machina_canvas_x;
	URIInt machina_canvas_y;
	URIInt machina_duration;
	URIInt machina_enter_action;
	URIInt machina_exit_action;
	URIInt machina_head_id;
	URIInt machina_initial;
	URIInt machina_note_number;
	URIInt machina_probability;
	URIInt machina_selector;
	URIInt machina_tail_id;
	URIInt rdf_type;

private:
	URIs()
		: machina_MidiAction_atom(Raul::Atom::URI, "machina:MidiAction")
		, machina_active(1)
		, machina_canvas_x(2)
		, machina_canvas_y(3)
		, machina_duration(4)
		, machina_enter_action(11)
		, machina_exit_action(12)
		, machina_head_id(5)
		, machina_initial(6)
		, machina_note_number(13)
		, machina_probability(7)
		, machina_selector(8)
		, machina_tail_id(9)
		, rdf_type(10)
	{}

	static URIs* _instance;
};

} // namespace Machina

#endif // MACHINA_URIS_HPP
