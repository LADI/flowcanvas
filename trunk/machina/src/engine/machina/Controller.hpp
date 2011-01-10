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

#ifndef MACHINA_CONTROLLER_HPP
#define MACHINA_CONTROLLER_HPP

#include <stdint.h>

#include <set>

#include "raul/SharedPtr.hpp"
#include "raul/RingBuffer.hpp"

#include "machina/types.hpp"
#include "machina/URIs.hpp"

#include "Stateful.hpp"

namespace Raul { class Atom; class Maid; }

namespace Machina {

class Engine;
class Machine;
class Stateful;

namespace Client { class ClientModel; class ClientObject; }

class Controller {
public:
	Controller(SharedPtr<Engine> engine, Client::ClientModel& client_model);

	uint64_t create(const Client::ClientObject& obj);
	uint64_t connect(uint64_t tail_id, uint64_t head_id);
	void     set_property(uint64_t object_id, URIInt key, const Raul::Atom& value);
	void     learn(SharedPtr<Raul::Maid> maid, uint64_t node_id);
	void     disconnect(uint64_t tail_id, uint64_t head_id);
	void     erase(uint64_t id);

	void announce(SharedPtr<Machine> machine);

	void process_updates();

private:
	SharedPtr<Stateful> find(uint64_t id);

	struct StatefulComparator {
		inline bool operator()(SharedPtr<Stateful> lhs, SharedPtr<Stateful> rhs) const {
			return lhs->id() < rhs->id();
		}
	};
	
	typedef std::set<SharedPtr<Stateful>, StatefulComparator> Objects;
	Objects _objects;

	SharedPtr<Engine>    _engine;
	Client::ClientModel& _client_model;

	SharedPtr<UpdateBuffer> _updates;
};

}

#endif // MACHINA_CONTROLLER_HPP
