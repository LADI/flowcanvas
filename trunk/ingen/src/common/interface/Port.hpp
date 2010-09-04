/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_INTERFACE_PORT_HPP
#define INGEN_INTERFACE_PORT_HPP

#include <set>
#include <stdint.h>
#include "GraphObject.hpp"
#include "PortType.hpp"

namespace Raul { class Atom; }

namespace Ingen {
namespace Shared {


/** A Port on a Node.
 *
 * Purely virtual (except for the destructor).
 *
 * \ingroup interface
 */
class Port : public virtual GraphObject
{
public:
	typedef std::set<Shared::PortType> PortTypes;

	virtual const PortTypes& types() const = 0;

	inline bool is_a(PortType type) const { return types().find(type) != types().end(); }

	virtual bool supports(const Raul::URI& value_type) const = 0;

	virtual uint32_t          index()    const = 0;
	virtual bool              is_input() const = 0;
	virtual const Raul::Atom& value()    const = 0;
};


} // namespace Shared
} // namespace Ingen

#endif // INGEN_INTERFACE_PORT_HPP
