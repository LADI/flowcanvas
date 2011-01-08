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

#include <math.h>

#include <algorithm>
#include <iostream>

#include <sigc++/sigc++.h>

#include "LaunchpadImpl.hpp"
#include "UI.hpp"
#include "dirg_internal.hpp"

using std::cerr;
using std::cout;
using std::endl;

struct LaunchpadUI : public UI {
	LaunchpadUI();
	~LaunchpadUI();

	void activate();
	void deactivate();

	void set_colour(ButtonID button, float hue, float value);

	void clear();
	void pressed(ButtonID button, bool down);

	SPtr<LaunchpadImpl> pad;
};

// TODO: Make this a loadable module with this function as entry point
SPtr<UI>
dirg_new_launchpad_ui(const PadState& state, const char* dir)
{
	return SPtr<UI>(new LaunchpadUI());
}

static uint8_t
colour_to_velocity(float hue, float value)
{
	if (value == 0.0f)
		return 0;

	const float g = 2.0f * (1.0 - hue);
	const float r = 2.0f * hue;

	const int r_level = std::min(static_cast<int>(lrintf(r * 3)), 3);
	const int g_level = std::min(static_cast<int>(lrintf(g * 3)), 3);
	assert(g_level >= 0);
	assert(g_level <= 3);
	assert(r_level >= 0);
	assert(r_level <= 3);

	const uint8_t velocity = 16 * g_level + r_level;
	assert((velocity & (1<<6)) == 0);
	return velocity;
}

LaunchpadUI::LaunchpadUI()
{
}

LaunchpadUI::~LaunchpadUI()
{
	if (pad)
		deactivate();
}

void
LaunchpadUI::activate()
{
	assert(!pad);
	pad = SPtr<LaunchpadImpl>(new LaunchpadImpl());

	cout << "Attempting to connect to Launchpad";
	for (int i = 0; !pad->isConnected() && i < 40; ++i) {
		usleep(100000);
		cout << ".";
	}
	cout << endl;

	if (!pad->isConnected()) {
		cerr << "Failed to connect to Launchpad" << endl;
		return;
	}

	clear();

	pad->getSignal().connect(sigc::mem_fun(this, &LaunchpadUI::pressed));
}

void
LaunchpadUI::deactivate()
{
	clear();
	pad.reset();
}

void
LaunchpadUI::set_colour(ButtonID button, float hue, float value)
{
	pad->setButton(button, colour_to_velocity(hue, value));
}

void
LaunchpadUI::clear()
{
	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; ++c) {
			pad->setButton(ButtonID(GRID, r, c), 0);
		}
	}
}

void
LaunchpadUI::pressed(ButtonID id, bool down)
{
	if (down) {
		const uint8_t vel = colour_to_velocity(1.0f, 1.0f);
		pad->setButton(id, vel);
	} else {
		button_pressed.emit(id);
	}
}
