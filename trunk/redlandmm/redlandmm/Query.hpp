/* This file is part of redlandmm.
 * Copyright (C) 2007-2010 David Robillard <http://drobilla.net>
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

#include <iostream>

#include <boost/shared_ptr.hpp>

#include <glibmm/ustring.h>

#include "redlandmm/Namespaces.hpp"
#include "redlandmm/QueryResults.hpp"
#include "redlandmm/World.hpp"
#include "redlandmm/Model.hpp"

namespace Redland {

class World;

/** SPARQL query.
 *
 * Automatically handles things like prepending prefixes, etc.
 */
class Query {
public:
	inline Query(World& world, Glib::ustring query) {
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

	inline boost::shared_ptr<QueryResults>
	run(World& world, Model& model, Glib::ustring base_uri_str="") const {
		Glib::Mutex::Lock lock(world.mutex());

		//std::cout << "QUERY {" << endl << _query << endl << "}" << std::endl;

		if (base_uri_str == "")
			base_uri_str = model.base_uri().to_c_string();

		librdf_uri* base_uri = librdf_new_uri(
			world.world(), (const unsigned char*)base_uri_str.c_str());

		librdf_query* q = librdf_new_query(
			world.world(), "sparql", NULL, (const unsigned char*)_query.c_str(), base_uri);

		if (!q) {
			std::cerr << "Unable to create query:" << std::endl << _query << std::endl;
			return boost::shared_ptr<QueryResults>();
		}

		librdf_query_results* c_results = librdf_query_execute(q, model.c_obj());
		if (!c_results) {
			std::cerr << "Failed query:" << std::endl << _query << std::endl;
			return boost::shared_ptr<QueryResults>();
		}

		boost::shared_ptr<QueryResults> results(new QueryResults(world, c_results));

		librdf_free_query(q);

		return results;
	}

	inline const Glib::ustring& string() const { return _query; }

private:
	Glib::ustring _query;
};

} // namespace Redland

#endif // REDLANDMM_QUERY_HPP

