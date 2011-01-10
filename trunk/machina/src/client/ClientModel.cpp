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

#include "ClientModel.hpp"

namespace Machina {
namespace Client {

SharedPtr<ClientObject>
ClientModel::find(uint64_t id)
{
	SharedPtr<ClientObject> key(new ClientObjectKey(id));
	Objects::iterator i = _objects.find(key);
	if (i != _objects.end())
		return *i;
	else
		return SharedPtr<ClientObject>();
}

void
ClientModel::new_object(SharedPtr<ClientObject> object)
{
	_objects.insert(object);
	signal_new_object.emit(object);
}

void
ClientModel::erase_object(uint64_t id)
{
	SharedPtr<ClientObject> key(new ClientObjectKey(id));
	Objects::iterator i = _objects.find(key);
	if (i == _objects.end())
		return;

	signal_erase_object.emit(*i);
	(*i)->set_view(SharedPtr<ClientObject::View>());
	_objects.erase(i);
}

void
ClientModel::property(uint64_t id, URIInt key, const Raul::Atom& value)
{
	SharedPtr<ClientObject> object = find(id);
	if (object)
		object->set(key, value);
}

}
}
