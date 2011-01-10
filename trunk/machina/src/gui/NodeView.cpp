/* This file is part of Machina.
 * Copyright (C) 2007-2010 David Robillard <http://drobilla.net>
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

#include "machina/Controller.hpp"
#include "machina/URIs.hpp"
#include "machina/types.hpp"

#include "client/ClientModel.hpp"

#include "MachinaCanvas.hpp"
#include "MachinaGUI.hpp"
#include "NodePropertiesWindow.hpp"
#include "NodeView.hpp"

using namespace std;
using Machina::URIs;

NodeView::NodeView(Gtk::Window*                             window,
                   SharedPtr<FlowCanvas::Canvas>            canvas,
                   SharedPtr<Machina::Client::ClientObject> node,
                   double                                   x,
                   double                                   y)
	: FlowCanvas::Ellipse(canvas, "", x, y, 20, 20, false)
	, _window(window)
	, _node(node)
	, _default_border_color(_border_color)
	, _old_color(_color)
{
	signal_clicked.connect(
		sigc::mem_fun(this, &NodeView::handle_click));

	node->signal_property.connect(
		sigc::mem_fun(this, &NodeView::on_property));
}

void
NodeView::on_double_click(GdkEventButton*)
{
	NodePropertiesWindow::present(_window, _node);
}

bool
NodeView::node_is(Machina::URIInt key)
{
	const Raul::Atom& value = _node->get(key);
	return value.type() == Raul::Atom::BOOL && value.get_bool();
}

void
NodeView::handle_click(GdkEventButton* event)
{
	if (event->state & GDK_CONTROL_MASK) {
		SharedPtr<MachinaCanvas> canvas = PtrCast<MachinaCanvas>(_canvas.lock());
		if (event->button == 1) {
			canvas->app()->controller()->set_property(
					_node->id(),
					URIs::instance().machina_initial,
					!node_is(URIs::instance().machina_initial));
		} else if (event->button == 3) {
			canvas->app()->controller()->set_property(
					_node->id(),
					URIs::instance().machina_selector,
					!node_is(URIs::instance().machina_selector));
		}
	}
}

static std::string
midi_note_name(uint8_t num)
{
	static const char* notes[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

	const uint8_t octave = num / 12;
	num -= octave * 12;
	
	static char str[8];
	snprintf(str, sizeof(str), "%s%d", notes[num], octave);
	return str;
}

void
NodeView::show_label(bool show)
{
	if (show) {
		if (_enter_action) {
			Raul::Atom note_number = _enter_action->get(URIs::instance().machina_note_number);
			if (note_number.is_valid()) {
				set_name(midi_note_name(note_number.get_int32()));
				return;
			}
		}
	}
	
	set_name("");
}

/// Dash style for selector node outlines
static ArtVpathDash*
selector_dash()
{
	static ArtVpathDash* selector_dash = NULL;

	if (!selector_dash) {
		selector_dash = new ArtVpathDash();
		selector_dash->n_dash = 2;
		selector_dash->dash = art_new(double, 2);
		selector_dash->dash[0] = 8;
		selector_dash->dash[1] = 8;
	}

	return selector_dash;
}

void
NodeView::set_selected(bool selected)
{
	Ellipse::set_selected(selected);
	if (!selected)
		_ellipse.property_dash() = node_is(URIs::instance().machina_selector) ? selector_dash() : 0;
}

void
NodeView::on_property(Machina::URIInt key, const Raul::Atom& value)
{
	static const uint32_t active_color        = 0x408040FF;
	static const uint32_t active_border_color = 0x00FF00FF;

	if (key == URIs::instance().machina_selector) {
		_ellipse.property_dash() = value.get_bool() ? selector_dash() : 0;
	} else if (key == URIs::instance().machina_initial) {
		set_border_width(value.get_bool() ? 4.0 : 1.0);
	} else if (key == URIs::instance().machina_active) {
		if (value.get_bool()) {
			if (_color != active_color) {
				_old_color = _color;
				set_base_color(active_color);
				set_border_color(active_border_color);
			}
		} else if (_color == active_color) {
			set_base_color(_old_color);
			set_border_color(_default_border_color);
		}
	} else if (key == URIs::instance().machina_enter_action) {
		const uint64_t action_id = value.get_int32();
		SharedPtr<MachinaCanvas> canvas = PtrCast<MachinaCanvas>(_canvas.lock());
		_enter_action_connection.disconnect();
		_enter_action = canvas->app()->client_model()->find(action_id);
		_enter_action_connection = _enter_action->signal_property.connect(
			sigc::mem_fun(this, &NodeView::on_action_property));
	} else {
		cout << "Unknown property " << key << endl;
	}
}

void
NodeView::on_action_property(Machina::URIInt key, const Raul::Atom& value)
{
	if (key == URIs::instance().machina_note_number)
		show_label(true);
}
