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

#ifndef MACHINA_CLIENTOBJECT_HPP
#define MACHINA_CLIENTOBJECT_HPP

#include <map>

#include <sigc++/sigc++.h>

#include "raul/Atom.hpp"
#include "raul/SharedPtr.hpp"

#include "machina/types.hpp"

namespace Machina {
namespace Client {

class ClientObject {
public:
	ClientObject(uint64_t id);
	ClientObject(const ClientObject& copy, uint64_t id);

	inline uint64_t id() const { return _id; }

	void              set(URIInt key, const Raul::Atom& value);
	const Raul::Atom& get(URIInt key) const;

	sigc::signal<void, URIInt, Raul::Atom> signal_property;

	class View {
	public:
		virtual ~View() {}
	};

	SharedPtr<View> view() const { return _view; }
	void set_view(SharedPtr<View> view) { _view = view; }
	
private:
	uint64_t        _id;
	SharedPtr<View> _view;

	typedef std::map<URIInt, Raul::Atom> Properties;
	Properties _properties;
};

class ClientObjectKey : public ClientObject {
public:
	ClientObjectKey(uint64_t id) : ClientObject(id) {}
};

}
}

#endif // MACHINA_CLIENTOBJECT_HPP
