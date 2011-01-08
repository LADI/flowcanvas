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

#ifndef DIRG_UI_HPP
#define DIRG_UI_HPP

#include <sigc++/sigc++.h>

#include "dirg_internal.hpp"

class UI : public sigc::trackable {
public:
	virtual ~UI() {}

	virtual void set_colour(ButtonID button, float hue, float value) = 0;

	virtual void activate()   = 0;
	virtual void deactivate() = 0;

	sigc::signal<void, ButtonID> button_pressed;
};

#endif // DIRG_UI_HPP
