/* This file is part of redlandmm.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * redlandmm is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * redlandmm is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef REDLANDMM_QUERY_HPP
#define REDLANDMM_QUERY_HPP

#include <map>
#include <list>
#include <glibmm/ustring.h>
#include <redlandmm/World.hpp>
#include <redlandmm/Namespaces.hpp>

namespace Redland {

class World;
class Model;


/** SPARQL query.
 *
 * Automatically handles things like prepending prefixes, etc.
 */
class Query {
public:
	typedef std::map<std::string, Node> Bindings; // FIXME: order?  better to use int
	typedef std::list<Bindings>         Results;

	Query(World& world, Glib::ustring query)
	{
		Glib::Mutex::Lock lock(world.mutex());

		// Prepend prefix header
		for (Namespaces::const_iterator i = world.prefixes().begin();
				i != world.prefixes().end(); ++i) {
			_query += "PREFIX ";
			_query += i->first + ": <" + i->second + ">\n";
		}
		_query += "\n";
		_query += query;
	}

	Results run(World& world, Model& model, const Glib::ustring base_uri="") const;

	const Glib::ustring& string() const { return _query; };

private:
	Glib::ustring _query;
};


} // namespace Redland

#endif // REDLANDMM_QUERY_HPP
