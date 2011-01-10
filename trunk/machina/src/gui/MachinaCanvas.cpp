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

#include <map>

#include "raul/log.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/TimeStamp.hpp"

#include "machina/Engine.hpp"
#include "machina/Controller.hpp"
#include "client/ClientObject.hpp"
#include "client/ClientModel.hpp"

#include "EdgeView.hpp"
#include "MachinaCanvas.hpp"
#include "MachinaGUI.hpp"
#include "NodeView.hpp"

using namespace Raul;
using namespace FlowCanvas;
using namespace Machina;

MachinaCanvas::MachinaCanvas(MachinaGUI* app, int width, int height)
	: Canvas(width, height)
	, _app(app)
	, _selector_dash(NULL)
{
	grab_focus();
}

void
MachinaCanvas::node_clicked(WeakPtr<NodeView> item, GdkEventButton* event)
{
	SharedPtr<NodeView> node = PtrCast<NodeView>(item.lock());
	if (!node)
		return;

	if (event->state & GDK_CONTROL_MASK)
		return;

	if (event->button == 2) { // Middle click: learn
		_app->controller()->learn(_app->maid(), node->node()->id());
		return;

	} else if (event->button == 3) { // Right click: connect/disconnect
		SharedPtr<NodeView> last = _last_clicked.lock();

		if (last) {
			if (node != last) {
				if (get_connection(last, node))
					action_disconnect(last, node);
				else
					action_connect(last, node);
			}

			last->set_default_base_color();
			_last_clicked.reset();

		} else {
			_last_clicked = node;
			node->set_base_color(0xFF0000FF);
		}
	}
}

bool
MachinaCanvas::canvas_event(GdkEvent* event)
{
	if (event->type == GDK_BUTTON_RELEASE
			&& event->button.button == 3
			&& !(event->button.state & (GDK_CONTROL_MASK))) {

		action_create_node(event->button.x, event->button.y);
		return true;

	} else {
		return Canvas::canvas_event(event);
	}
}

void
MachinaCanvas::on_new_object(SharedPtr<Client::ClientObject> object)
{
	const Machina::URIs& uris = URIs::instance();
	const Raul::Atom&    type = object->get(uris.rdf_type);
	if (type == "machina:Node") {
		SharedPtr<NodeView> view(
			new NodeView(_app->window(), shared_from_this(), object,
			             object->get(uris.machina_canvas_x).get_float(),
			             object->get(uris.machina_canvas_y).get_float()));

		//if ( ! node->enter_action() && ! node->exit_action() )
		//	view->set_base_color(0x101010FF);

		view->signal_clicked.connect(
			sigc::bind<0>(sigc::mem_fun(this, &MachinaCanvas::node_clicked),
			              WeakPtr<NodeView>(view)));

		object->set_view(view);
		add_item(view);

	} else if (type == "machina:Edge") {
		SharedPtr<Machina::Client::ClientObject> tail = _app->client_model()->find(
			object->get(uris.machina_tail_id).get_int32());
		SharedPtr<Machina::Client::ClientObject> head = _app->client_model()->find(
			object->get(uris.machina_head_id).get_int32());

		SharedPtr<NodeView> tail_view = PtrCast<NodeView>(tail->view());
		SharedPtr<NodeView> head_view = PtrCast<NodeView>(head->view());

		SharedPtr<EdgeView> view(
			new EdgeView(shared_from_this(), tail_view, head_view, object));

		tail_view->add_connection(view);
		head_view->add_connection(view);

		object->set_view(view);
		add_connection(view);

	} else {
		Raul::error << "Unknown object type " << type << std::endl;
	}
}

void
MachinaCanvas::on_erase_object(SharedPtr<Client::ClientObject> object)
{
	const Raul::Atom& type = object->get(URIs::instance().rdf_type);
	if (type == "machina:Node") {
		SharedPtr<NodeView> view = PtrCast<NodeView>(object->view());
		if (view) {
			remove_item(view);
		}
	} else if (type == "machina:Edge") {
		SharedPtr<EdgeView> view = PtrCast<EdgeView>(object->view());
		if (view) {
			remove_connection(view->source().lock(), view->dest().lock());
		}
	} else {
		Raul::error << "Unknown object type " << type << std::endl;
	}
}

void
MachinaCanvas::action_create_node(double x, double y)
{
	Machina::Client::ClientObject obj(0);
	obj.set(URIs::instance().rdf_type, "machina:Node");
	obj.set(URIs::instance().machina_canvas_x, Raul::Atom((float)x));
	obj.set(URIs::instance().machina_canvas_y, Raul::Atom((float)y));
	obj.set(URIs::instance().machina_duration, Raul::Atom((float)1.0));
	_app->controller()->create(obj);
}

void
MachinaCanvas::action_connect(boost::shared_ptr<NodeView> src,
                              boost::shared_ptr<NodeView> head)
{
	_app->controller()->connect(src->node()->id(), head->node()->id());
}

void
MachinaCanvas::action_disconnect(boost::shared_ptr<NodeView> src,
                                 boost::shared_ptr<NodeView> head)
{
	_app->controller()->disconnect(src->node()->id(), head->node()->id());
}
