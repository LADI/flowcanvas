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

#ifndef MACHINA_CANVAS_HPP_HPP
#define MACHINA_CANVAS_HPP_HPP

#include <string>
#include "raul/SharedPtr.hpp"
#include "raul/WeakPtr.hpp"
#include "flowcanvas/Canvas.hpp"

using namespace FlowCanvas;

class MachinaGUI;
class NodeView;

namespace Machina { namespace Client { class ClientObject; } }

class MachinaCanvas : public Canvas
{
public:
	MachinaCanvas(MachinaGUI* app, int width, int height);

	//void build(SharedPtr<const Machina::Machine> machine, bool show_labels);
	//void update_edges();

	void on_new_object(SharedPtr<Machina::Client::ClientObject> object);
	void on_erase_object(SharedPtr<Machina::Client::ClientObject> object);

	ArtVpathDash* selector_dash() { return _selector_dash; }

	MachinaGUI* app() { return _app; }

protected:
	bool canvas_event(GdkEvent* event);

	void node_clicked(WeakPtr<NodeView> item, GdkEventButton* ev);

private:
	//SharedPtr<NodeView> create_node_view(SharedPtr<Machina::Node> node);

	void action_create_node(double x, double y);

	void action_connect(SharedPtr<NodeView> port1,
	                    SharedPtr<NodeView> port2);

	void action_disconnect(SharedPtr<NodeView> port1,
	                       SharedPtr<NodeView> port2);

	MachinaGUI*       _app;
	ArtVpathDash*     _selector_dash;
	WeakPtr<NodeView> _last_clicked;
};


#endif // MACHINA_CANVAS_HPP_HPP
