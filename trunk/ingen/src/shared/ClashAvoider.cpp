/* This file is part of Ingen.
 * Copyright (C) 2008-2009 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <cstdio>
#include <sstream>
#include "raul/log.hpp"
#include "ClashAvoider.hpp"
#include "Store.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {


const URI
ClashAvoider::map_uri(const Raul::URI& in)
{
	if (Path::is_path(in))
		return map_path(in.str());
	else
		return in;
}


const Path
ClashAvoider::map_path(const Raul::Path& in)
{
	debug << "MAP PATH: " << in;

	unsigned offset = 0;
	bool has_offset = false;
	const size_t pos = in.find_last_of('_');
	if (pos != string::npos && pos != (in.length()-1)) {
		const std::string trailing = in.substr(pos + 1);
		has_offset = (sscanf(trailing.c_str(), "%u", &offset) > 0);
	}

	debug << "OFFSET: " << offset << endl;

	// Path without _n suffix
	Path base_path = in;
	if (has_offset)
		base_path = base_path.substr(0, base_path.find_last_of('_'));

	debug << "BASE: " << base_path << endl;

	SymbolMap::iterator m = _symbol_map.find(in);
	if (m != _symbol_map.end()) {
		debug << " (1) " << m->second << endl;
		return m->second;
	} else {
		typedef std::pair<SymbolMap::iterator, bool> InsertRecord;

		// See if parent is mapped
		Path parent = in.parent();
		do {
			debug << "CHECK: " << parent << endl;
			SymbolMap::iterator p = _symbol_map.find(parent);
			if (p != _symbol_map.end()) {
				const Path mapped = p->second.base() + in.substr(parent.base().length());
				InsertRecord i = _symbol_map.insert(make_pair(in, mapped));
				debug << " (2) " << i.first->second << endl;
				return i.first->second;
			}
			parent = parent.parent();
		} while (!parent.is_root());

		// No clash, use symbol unmodified
		if (!exists(in) && _symbol_map.find(in) == _symbol_map.end()) {
			InsertRecord i = _symbol_map.insert(make_pair(in, in));
			assert(i.second);
			debug << " (3) " << i.first->second << endl;;
			return i.first->second;

		// Append _2 _3 etc until an unused symbol is found
		} else {
			while (true) {
				Offsets::iterator o = _offsets.find(base_path);
				if (o != _offsets.end()) {
					offset = ++o->second;
				} else {
					string parent_str = in.parent().base();
					parent_str = parent_str.substr(0, parent_str.find_last_of("/"));
					if (parent_str.empty())
						parent_str = "/";
					debug << "PARENT: " << parent_str << endl;
				}

				if (offset == 0)
					offset = 2;

				std::stringstream ss;
				ss << base_path << "_" << offset;
				if (!exists(ss.str())) {
					const string name = (base_path.length() > 1) ? base_path.symbol() : "_";
					string str = ss.str();
					InsertRecord i = _symbol_map.insert(make_pair(in, str));
					debug << "HIT: offset = " << offset << ", str = " << str << endl;
					offset = _store.child_name_offset(in.parent(), name, false);
					_offsets.insert(make_pair(base_path, offset));
					debug << " (4) " << i.first->second << endl;;
					return i.first->second;
				} else {
					debug << "MISSED OFFSET: " << in << " => " << ss.str() << endl;
					if (o != _offsets.end())
						offset = ++o->second;
					else
						++offset;
				}
			}
		}
	}
}


bool
ClashAvoider::exists(const Raul::Path& path) const
{
	bool exists = (_store.find(path) != _store.end());
	if (exists)
		return true;

	if (_also_avoid)
		return (_also_avoid->find(path) != _also_avoid->end());
	else
		return false;
}


void
ClashAvoider::put(const Raul::URI&                    path,
                  const Shared::Resource::Properties& properties)
{
	_target.put(map_uri(path), properties);
}


void
ClashAvoider::delta(const Raul::URI&                    path,
                    const Shared::Resource::Properties& remove,
                    const Shared::Resource::Properties& add)
{
	_target.delta(map_uri(path), remove, add);
}


void
ClashAvoider::move(const Raul::Path& old_path,
                   const Raul::Path& new_path)
{
	_target.move(map_path(old_path), map_path(new_path));
}


void
ClashAvoider::connect(const Raul::Path& src_port_path,
                      const Raul::Path& dst_port_path)
{
	_target.connect(map_path(src_port_path), map_path(dst_port_path));
}


void
ClashAvoider::disconnect(const Raul::Path& src_port_path,
                         const Raul::Path& dst_port_path)
{
	_target.disconnect(map_path(src_port_path), map_path(dst_port_path));
}


void
ClashAvoider::set_property(const Raul::URI&  subject,
                           const Raul::URI&  predicate,
                           const Raul::Atom& value)
{
	_target.set_property(map_uri(subject), predicate, value);
}


void
ClashAvoider::del(const Raul::Path& path)
{
	_target.del(map_path(path));
}


} // namespace Shared
} // namespace Ingen
