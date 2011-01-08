/* Dirg
 * Copyright (C) 2010 David Robillard <http://drobilla.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef DIRG_INTERNAL_HPP
#define DIRG_INTERNAL_HPP

#include <stdint.h>
#include <string.h>

#include <cassert>

#include <tr1/memory>

#include <iostream>

class UI;

typedef uint8_t ButtonValue;

enum ButtonGroup {
	GRID     = 0, ///< 8x8 grid
	MARGIN_H = 1, ///< Horizontal margin (top 8 "live" buttons)
	MARGIN_V = 2  ///< Vertical margin (right 8 "send" buttons)
};

struct ButtonID {
	inline ButtonID(ButtonGroup g, int r, int c) : group(g), row(r), col(c) {}
	inline bool operator<(const ButtonID& other) const {
		return row < other.row || (row == other.row && col < other.col);
	}
	ButtonGroup group;
	int         row;
	int         col;
};

static inline std::ostream&
operator<<(std::ostream& os, const ButtonID& id) {
	os << id.group << ":" << id.row << ":" << id.col;
	return os;
}

struct ButtonState {
	inline ButtonState() : hue(0.0f), value(0.0f) {}
	float hue;
	float value;
};

static inline std::ostream&
operator<<(std::ostream& os, const ButtonState& but) {
	os << but.hue << "," << but.value;
	return os;
}

struct PadState {
	inline PadState() {
		memset(margin_h, sizeof(margin_h), '\0');
		memset(margin_v, sizeof(margin_v), '\0');
		memset(grid,     sizeof(grid),     '\0');
	}

	inline const ButtonState& get(ButtonID id) const {
		switch (id.group) {
		case GRID:
			return grid[id.row][id.col];
		case MARGIN_H:
			return margin_h[id.col];
		case MARGIN_V:
			return margin_v[id.row];
		default:
			assert(false);
		}
	}

	inline void set_colour(ButtonID id, float hue, float value) {
		ButtonState* state;
		switch (id.group) {
		case GRID:
			state = &grid[id.row][id.col];
			break;
		case MARGIN_H:
			state = &margin_h[id.col];
			break;
		case MARGIN_V:
			state = &margin_v[id.row];
			break;
		}
		state->hue   = hue;
		state->value = value;
	}

	ButtonState margin_h[8];
	ButtonState margin_v[8];
	ButtonState grid[8][8];
};

template<typename T>
struct SPtr : public std::tr1::shared_ptr<T> {
	SPtr()                                 : std::tr1::shared_ptr<T>()  {}
	explicit SPtr(T* t)                    : std::tr1::shared_ptr<T>(t) {}
	SPtr(const std::tr1::shared_ptr<T>& t) : std::tr1::shared_ptr<T>(t) {}
};

SPtr<UI>
dirg_new_launchpad_ui(const PadState& state, const char* dir);

SPtr<UI>
dirg_new_web_ui(const PadState& state, const char* dir);


#endif // DIRG_INTERNAL_HPP
