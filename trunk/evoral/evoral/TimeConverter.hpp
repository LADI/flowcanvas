/* This file is part of Evoral.
 * Copyright (C) 2009 Dave Robillard <http://drobilla.net>
 * Copyright (C) 2009 Paul Davis
 *
 * Evoral is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Evoral is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef EVORAL_TIME_CONVERTER_HPP
#define EVORAL_TIME_CONVERTER_HPP

namespace Evoral {

/** A bidirectional converter between two different time units.
 *
 * Think of the conversion method names as if they are written in-between
 * the two template parameters (i.e. "A <name> B").
 */
template<typename A, typename B>
class TimeConverter {
public:
	virtual ~TimeConverter() {}

	/** Convert A time to B time (A to B) */
	virtual B to(A a) const = 0;

	/** Convert B time to A time (A from B) */
	virtual A from(B b) const = 0;
};


/** A stub TimeConverter that simple statically casts between types. */
template<typename A, typename B>
class IdentityConverter : public TimeConverter<A,B> {
	B to(A a)   const { return static_cast<B>(a); }
	A from(B b) const { return static_cast<A>(b); }
};


} // namespace Evoral

#endif // EVORAL_TIME_CONVERTER_HPP
