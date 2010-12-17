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

#ifndef REDLANDMM_WORLD_HPP
#define REDLANDMM_WORLD_HPP

#include <set>
#include <stdexcept>
#include <string>

#include <stdint.h>

#include <boost/utility.hpp>
#include <glibmm/thread.h>
#include <redland.h>

#include "redlandmm/Wrapper.hpp"
#include "redlandmm/Namespaces.hpp"

namespace Redland {


/** Library state
 */
class World : public boost::noncopyable, public Wrapper<librdf_world> {
public:
	inline World()
		: _next_blank_id(0)
	{
		if (!Glib::thread_supported())
			Glib::thread_init();
		
		_mutex = new Glib::Mutex();
		
		_c_obj = librdf_new_world();
		librdf_world_open(_c_obj);
		
		add_prefix("rdf", "http://www.w3.org/1999/02/22-rdf-syntax-ns#");
	}

	inline ~World() {
		Glib::Mutex::Lock lock(*_mutex);
		librdf_free_world(_c_obj);
	}
	
	inline uint64_t blank_id() { return _next_blank_id++; }

	void add_prefix(const std::string& prefix, const std::string& uri) {
		_prefixes[prefix] = uri;
	}

	std::string expand_uri(const std::string& uri) const {
		if (uri.find(":") == std::string::npos)
			return uri;

		for (Namespaces::const_iterator i = _prefixes.begin(); i != _prefixes.end(); ++i)
			if (uri.substr(0, i->first.length()+1) == i->first + ":")
				return i->second + uri.substr(i->first.length()+1);

		return uri;
	}
	
	std::string qualify(const std::string& uri) const {
		return _prefixes.qualify(uri);
	}

	const Namespaces& prefixes() const { return _prefixes; }

	librdf_world* world() { return _c_obj; }

	Glib::Mutex& mutex() { return *_mutex; }

private:
	void setup_prefixes();

	Glib::Mutex* _mutex;
	Namespaces   _prefixes;

	std::set<std::string> _blank_ids;
	uint64_t              _next_blank_id;
};


} // namespace Redland

#endif // REDLANDMM_WORLD_HPP
