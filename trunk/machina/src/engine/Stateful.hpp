/* This file is part of Machina
 * Copyright (C) 2007-2010 David Robillard <http://drobilla.net>
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

#ifndef MACHINA_STATEFUL_HPP
#define MACHINA_STATEFUL_HPP

#include <stdint.h>

#include "redlandmm/World.hpp"
#include "redlandmm/Model.hpp"

#include "machina/types.hpp"

namespace Raul { class Atom; }

namespace Machina {

class URIs;

class Stateful {
public:
	Stateful();

	virtual ~Stateful() {}

	virtual void set(URIInt key, const Raul::Atom& value) {}
	virtual void write_state(Redland::Model& model) = 0;

	uint64_t             id() const { return _id; }
	const Redland::Node& rdf_id(Redland::World& world) const;

	static uint64_t next_id() { return _next_id++; }

protected:
	Stateful(uint64_t id) : _id(id) {}

private:
	static uint64_t _next_id;

	uint64_t              _id;
	mutable Redland::Node _rdf_id;
};

class StatefulKey : public Stateful {
public:
	StatefulKey(uint64_t id) : Stateful(id) {}

	void write_state(Redland::Model& model) {}
};

} // namespace Machina

#endif // MACHINA_STATEFUL_HPP
