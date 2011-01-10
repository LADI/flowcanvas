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

#ifndef MACHINA_NODEVIEW_HPP
#define MACHINA_NODEVIEW_HPP

#include "flowcanvas/Ellipse.hpp"

#include "client/ClientObject.hpp"

#include "machina/types.hpp"

class NodeView
	: public FlowCanvas::Ellipse
	, public Machina::Client::ClientObject::View {
public:
	NodeView(Gtk::Window*                             window,
	         SharedPtr<FlowCanvas::Canvas>            canvas,
	         SharedPtr<Machina::Client::ClientObject> node,
	         double                                   x,
	         double                                   y);

	SharedPtr<Machina::Client::ClientObject> node() { return _node; }

	void show_label(bool show);

	void update_state(bool show_labels);

private:
	void handle_click(GdkEventButton* ev);
	void on_double_click(GdkEventButton* ev);
	void on_property(Machina::URIInt key, const Raul::Atom& value);
	void on_action_property(Machina::URIInt key, const Raul::Atom& value);
	void set_selected(bool selected);

	bool node_is(Machina::URIInt key);

	Gtk::Window*                             _window;
	SharedPtr<Machina::Client::ClientObject> _node;
	uint32_t                                 _default_border_color;
	uint32_t                                 _old_color;

	SharedPtr<Machina::Client::ClientObject> _enter_action;
	sigc::connection                         _enter_action_connection;
};


#endif // MACHINA_NODEVIEW_HPP
