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

#include "raul/Atom.hpp"
#include "raul/SharedPtr.hpp"

#include "machina/types.hpp"
#include "machina/Updates.hpp"

namespace Machina {

static inline void
write_atom(SharedPtr<UpdateBuffer> buf,
           const Raul::Atom&       value)
{
	const Raul::Atom::Type type = value.type();
	buf->write(sizeof(type), &type);
	int32_t ival;
	float   fval;
	size_t  szval;
	switch (value.type()) {
	case Raul::Atom::INT:
		ival = value.get_int32();
		buf->write(sizeof(ival), &ival);
		break;
	case Raul::Atom::FLOAT:
		ival = value.get_float();
		buf->write(sizeof(fval), &fval);
		break;
	case Raul::Atom::BOOL:
		ival = value.get_bool() ? 1 : 0;
		buf->write(sizeof(ival), &ival);
		break;
	case Raul::Atom::STRING:
		szval = value.data_size();
		buf->write(sizeof(size_t), &szval);
		buf->write(value.data_size(), value.get_string());
		break;
	case Raul::Atom::URI:
		szval = value.data_size();
		buf->write(sizeof(size_t), &szval);
		buf->write(value.data_size(), value.get_uri());
		break;
	default:
		assert(false);
	}
}

uint32_t
read_atom(SharedPtr<UpdateBuffer> buf,
          Raul::Atom*             value)
{
	Raul::Atom::Type type;
	buf->read(sizeof(type), &type);

	int32_t ival;
	float   fval;
	char*   sval;
	size_t  val_size;
	switch (type) {
	case Raul::Atom::INT:
		val_size = sizeof(ival);
		buf->read(val_size, &ival);
		*value = Raul::Atom(ival);
		break;
	case Raul::Atom::FLOAT:
		val_size = sizeof(fval);
		buf->read(val_size, &fval);
		*value = Raul::Atom(fval);
		break;
	case Raul::Atom::BOOL:
		val_size = sizeof(ival);
		buf->read(val_size, &ival);
		assert(ival == 0 || ival == 1);
		*value = Raul::Atom(bool(ival));
		break;
	case Raul::Atom::STRING:
		buf->read(sizeof(val_size), &val_size);
		sval = (char*)malloc(val_size);
		buf->read(val_size, sval);
		val_size += sizeof(val_size);
		*value = Raul::Atom(sval);
		free(sval);
		break;
	case Raul::Atom::URI:
		buf->read(sizeof(val_size), &val_size);
		sval = (char*)malloc(val_size);
		buf->read(val_size, sval);
		val_size += sizeof(val_size);
		*value = Raul::Atom(Raul::Atom::URI, sval);
		free(sval);
		break;
	default:
		assert(false);
	}

	return sizeof(type) + val_size;
}

void
write_set(SharedPtr<UpdateBuffer> buf,
          uint64_t                subject,
          URIInt                  key,
          const Raul::Atom&       value)
{
	const uint32_t update_type = UPDATE_SET;
	buf->write(sizeof(update_type), &update_type);
	buf->write(sizeof(subject), &subject);
	buf->write(sizeof(key), &key);
	write_atom(buf, value);
}

uint32_t
read_set(SharedPtr<UpdateBuffer> buf,
         uint64_t*               subject,
         URIInt*                 key,
         Raul::Atom*             value)
{
	uint32_t update_type;
	buf->read(sizeof(update_type), &update_type);
	assert(update_type == UPDATE_SET);
	buf->read(sizeof(*subject), subject);
	buf->read(sizeof(*key), key);

	const uint32_t value_size = read_atom(buf, value);
	return sizeof(update_type) + sizeof(*subject) + sizeof(*key) + value_size;
}

}
