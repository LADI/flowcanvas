/* This file is part of Machina.
 * Copyright (C) 2010 David Robillard <http://drobilla.net>
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

#include "client/ClientModel.hpp"
#include "client/ClientObject.hpp"
#include "machina/Controller.hpp"
#include "machina/Engine.hpp"
#include "machina/Machine.hpp"
#include "machina/Updates.hpp"
#include "machina/Machine.hpp"

#include "Edge.hpp"

namespace Machina {

Controller::Controller(SharedPtr<Engine> engine, Client::ClientModel& client_model)
	: _engine(engine)
	, _client_model(client_model)
	, _updates(new UpdateBuffer(4096))
{
	_engine->driver()->set_update_sink(_updates);
}

uint64_t
Controller::create(const Client::ClientObject& properties)
{
	TimeDuration dur(_engine->machine()->time().unit(),
	                 properties.get(URIs::instance().machina_duration).get_float());
	SharedPtr<Machina::Node> node(new Machina::Node(dur));
	SharedPtr<Client::ClientObject> obj(new Client::ClientObject(properties, node->id()));
	_objects.insert(node);
	_client_model.new_object(obj);
	_engine->machine()->add_node(node);
	return node->id();
}

void
Controller::announce(SharedPtr<Machine> machine)
{
	for (Machina::Machine::Nodes::const_iterator n = machine->nodes().begin();
	     n != machine->nodes().end(); ++n) {
		
		SharedPtr<Machina::Client::ClientObject> obj(new Machina::Client::ClientObject((*n)->id()));
		obj->set(URIs::instance().rdf_type, "machina:Node");
		obj->set(URIs::instance().machina_duration, Raul::Atom(float((*n)->duration().to_double())));
		obj->set(URIs::instance().machina_canvas_x, 0.0f);
		obj->set(URIs::instance().machina_canvas_y, 0.0f);

		_objects.insert(*n);
		_client_model.new_object(obj);
	}

	for (Machina::Machine::Nodes::const_iterator n = machine->nodes().begin();
	     n != machine->nodes().end(); ++n) {
		for (Machina::Node::Edges::const_iterator e = (*n)->edges().begin();
		     e != (*n)->edges().end(); ++e) {
			_objects.insert(*e);
			
			SharedPtr<Client::ClientObject> eobj(new Client::ClientObject((*e)->id()));
			eobj->set(URIs::instance().rdf_type, "machina:Edge");
			eobj->set(URIs::instance().machina_probability, (*e)->probability());
			eobj->set(URIs::instance().machina_tail_id, (int32_t)(*n)->id());
			eobj->set(URIs::instance().machina_head_id, (int32_t)(*e)->head()->id());
	
			_client_model.new_object(eobj);
		}
	}
}

SharedPtr<Stateful>
Controller::find(uint64_t id)
{
	SharedPtr<StatefulKey> key(new StatefulKey(id));
	Objects::iterator i = _objects.find(key);
	if (i != _objects.end())
		return *i;
	return SharedPtr<Stateful>();
}

void
Controller::learn(SharedPtr<Raul::Maid> maid, uint64_t node_id)
{
	SharedPtr<Machina::Node> node = PtrCast<Machina::Node>(find(node_id));
	if (node)
		_engine->machine()->learn(maid, node);
	else
		std::cerr << "Failed to find node " << node_id << " for learn" << std::endl;
}

void
Controller::set_property(uint64_t object_id, URIInt key, const Raul::Atom& value)
{
	SharedPtr<Stateful> object = find(object_id);
	if (object) {
		object->set(key, value);
		_client_model.property(object_id, key, value);
	}
}

uint64_t
Controller::connect(uint64_t tail_id, uint64_t head_id)
{
	SharedPtr<Machina::Node> tail = PtrCast<Machina::Node>(find(tail_id));
	SharedPtr<Machina::Node> head = PtrCast<Machina::Node>(find(head_id));
	
	SharedPtr<Machina::Edge> edge(new Machina::Edge(tail, head));
	tail->add_edge(edge);
	_objects.insert(edge);

	SharedPtr<Client::ClientObject> obj(new Client::ClientObject(/**this,*/ edge->id()));
	obj->set(URIs::instance().rdf_type, "machina:Edge");
	obj->set(URIs::instance().machina_probability, 1.0f);
	obj->set(URIs::instance().machina_tail_id, (int32_t)tail->id());
	obj->set(URIs::instance().machina_head_id, (int32_t)head->id());
	
	_client_model.new_object(obj);

	return edge->id();
}

void
Controller::disconnect(uint64_t tail_id, uint64_t head_id)
{
	SharedPtr<Machina::Node> tail = PtrCast<Machina::Node>(find(tail_id));
	SharedPtr<Machina::Node> head = PtrCast<Machina::Node>(find(head_id));

	SharedPtr<Edge> edge = tail->remove_edge_to(head);
	if (edge)
		_client_model.erase_object(edge->id());
	else
		Raul::error << "Edge not found" << std::endl;
}

void
Controller::erase(uint64_t id)
{
	SharedPtr<StatefulKey> key(new StatefulKey(id));
	Objects::iterator i = _objects.find(key);
	if (i == _objects.end()) {
		return;
	}

	SharedPtr<Node> node = PtrCast<Node>(*i);
	if (node) {
		_engine->machine()->remove_node(node);
	}

	_client_model.erase_object((*i)->id());
	_objects.erase(i);
}

void
Controller::process_updates()
{
	const uint32_t read_space = _updates->read_space();

	uint64_t   subject;
	URIInt     key;
	Raul::Atom value;
	for (uint32_t i = 0; i < read_space;) {
		i += read_set(_updates, &subject, &key, &value);
		SharedPtr<Machina::Client::ClientObject> obj = _client_model.find(subject);
		if (obj) {
			obj->set(key, value);
		} else {
			SharedPtr<Client::ClientObject> obj(new Client::ClientObject(subject));
			obj->set(key, value);
			_client_model.new_object(obj);
		}
	}
}

}
