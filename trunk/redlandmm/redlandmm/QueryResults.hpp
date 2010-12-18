/* This file is part of redlandmm.
 * Copyright (C) 2010 David Robillard <http://drobilla.net>
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

#ifndef REDLANDMM_QUERY_RESULTS_HPP
#define REDLANDMM_QUERY_RESULTS_HPP

#include <redland.h>

#include "redlandmm/World.hpp"
#include "redlandmm/Node.hpp"

namespace Redland {

class QueryResults {
public:
	inline QueryResults(World& world, librdf_query_results* c_obj)
		: _world(world)
		, _c_obj(c_obj)
	{}

	inline ~QueryResults() {
		if (_c_obj)
			librdf_free_query_results(_c_obj);
	}
	
	inline bool finished() const { return !_c_obj || librdf_query_results_finished(_c_obj); }
	inline void next()           { librdf_query_results_next(_c_obj); }

	inline Node get(int offset) {
		return Node(_world, librdf_query_results_get_binding_value(_c_obj, offset));
	}

	inline Node get(const char* name) {
		return Node(_world, librdf_query_results_get_binding_value_by_name(_c_obj, name));
	}
		
private:
	World&                _world;
	librdf_query_results* _c_obj;
};

} // namespace Redland

#endif // REDLANDMM_QUERY_RESULTS_HPP
