/* This file is part of redlandmm.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#include <cassert>
#include <cstring>
#include <iostream>

#include "redlandmm/Model.hpp"
#include "redlandmm/Query.hpp"
#include "redlandmm/QueryResults.hpp"

#define CUC(x) ((const unsigned char*)(x))

using namespace std;

namespace Redland {


boost::shared_ptr<QueryResults>
Query::run(World& world, Model& model, Glib::ustring base_uri_str) const
{
	Glib::Mutex::Lock lock(world.mutex());

	//cout << "\n**************** QUERY *******************\n";
	//cout << _query << endl;
	//cout << "******************************************\n\n";

	if (base_uri_str == "")
		base_uri_str = model.base_uri().to_c_string();

	librdf_uri* base_uri = librdf_new_uri(world.world(),
				CUC(base_uri_str.c_str()));

	librdf_query* q = librdf_new_query(world.world(), "sparql",
		NULL, CUC(_query.c_str()), base_uri);

	if (!q) {
		cerr << "Unable to create query:" << endl << _query << endl;
		return boost::shared_ptr<QueryResults>();
	}

	// FIXME: locale kludges to work around librdf bug
	char* locale = strdup(setlocale(LC_NUMERIC, NULL));
	setlocale(LC_NUMERIC, "POSIX");

	librdf_query_results* c_results = librdf_query_execute(q, model._c_obj);
	if (!c_results) {
		cerr << "Failed query:" << endl << _query << endl;
		free(locale);
		return boost::shared_ptr<QueryResults>();
	}

	boost::shared_ptr<QueryResults> results(new QueryResults(world, c_results));

	setlocale(LC_NUMERIC, locale);
	free(locale);

	librdf_free_query(q);

	if (base_uri)
		librdf_free_uri(base_uri);

	return results;
}


} // namespace Redland

