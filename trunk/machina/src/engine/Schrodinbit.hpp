/* This file is part of Machina.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef SCHRODINBIT_HPP
#define SCHRODINBIT_HPP


/** A flag which becomes false when it's value is observed
 */
class Schrodinbit {
public:
	Schrodinbit() : _flag(false) {}

	inline operator bool() {
		const bool ret = _flag;
		_flag = false;
		return ret;
	}

	inline bool operator=(bool flag) {
		_flag = flag;
		return flag;
	}

private:
	bool _flag;
};


#endif // SCHRODINBIT_HPP

