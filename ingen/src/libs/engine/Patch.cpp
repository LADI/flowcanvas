/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include "ThreadManager.h"
#include "Node.h"
#include "Patch.h"
#include "Plugin.h"
#include "Port.h"
#include "Connection.h"
#include "DuplexPort.h"

using std::cerr; using std::cout; using std::endl;

namespace Ingen {


Patch::Patch(const string& path, size_t poly, Patch* parent, SampleRate srate, size_t buffer_size, size_t internal_poly) 
: NodeBase(new Plugin(Plugin::Patch, "ingen:patch"), path, poly, parent, srate, buffer_size),
  _internal_poly(internal_poly),
  _process_order(NULL),
  _process(false)
{
	assert(internal_poly >= 1);

	//_plugin->plug_label("om_patch");
	//_plugin->name("Ingen Patch");

	//std::cerr << "Creating patch " << _name << ", poly = " << poly
	//	<< ", internal poly = " << internal_poly << std::endl;
}


Patch::~Patch()
{
	assert(!_activated);
	
	for (Raul::List<Connection*>::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		delete (*i);
		delete _connections.erase(i);
	}

	for (Raul::List<Node*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
		assert(!(*i)->activated());
		delete (*i);
		delete _nodes.erase(i);
	}

	delete _process_order;
}


void
Patch::activate()
{
	NodeBase::activate();

	for (Raul::List<Node*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->activate();
	
	assert(_activated);
}


void
Patch::deactivate()
{
	if (_activated) {
	
		NodeBase::deactivate();
	
		for (Raul::List<Node*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
			if ((*i)->activated())
				(*i)->deactivate();
			assert(!(*i)->activated());
		}
	}
	assert(!_activated);
}


void
Patch::disable()
{
	// Write output buffers to 0
	/*for (Raul::List<InternalNode*>::iterator i = _bridge_nodes.begin(); i != _bridge_nodes.end(); ++i) {
	  assert((*i)->as_port() != NULL);
	  if ((*i)->as_port()->port_info()->is_output())
	  (*i)->as_port()->clear_buffers();*/
	for (Raul::List<Port*>::iterator i = _output_ports.begin(); i != _output_ports.end(); ++i)
		(*i)->clear_buffers();

	_process = false;
}


/** Run the patch for the specified number of frames.
 * 
 * Calls all Nodes in the order _process_order specifies.
 */
void
Patch::process(SampleCount nframes, FrameTime start, FrameTime end)
{
	if (_process_order == NULL || !_process)
		return;

	// FIXME: This is far too slow, too much checking every cycle
	
	// Prepare input ports for nodes to consume
	for (Raul::List<Port*>::iterator i = _input_ports.begin(); i != _input_ports.end(); ++i)
		(*i)->process(nframes, start, end);

	// Run all nodes (consume input ports)
	for (size_t i=0; i < _process_order->size(); ++i) {
		// Could be a gap due to a node removal event (see RemoveNodeEvent.cpp)
		// Yes, this is ugly
		if (_process_order->at(i) != NULL)
			_process_order->at(i)->process(nframes, start, end);
	}
	
	// Prepare output ports (for caller to consume)
	for (Raul::List<Port*>::iterator i = _output_ports.begin(); i != _output_ports.end(); ++i)
		(*i)->process(nframes, start, end);
}

	
void
Patch::set_buffer_size(size_t size)
{
	NodeBase::set_buffer_size(size);
	assert(_buffer_size == size);
	
	for (Raul::List<Node*>::iterator j = _nodes.begin(); j != _nodes.end(); ++j)
		(*j)->set_buffer_size(size);
}


void
Patch::add_to_store(ObjectStore* store)
{
	// Add self and ports
	NodeBase::add_to_store(store);

	// Add nodes
	for (Raul::List<Node*>::iterator j = _nodes.begin(); j != _nodes.end(); ++j)
		(*j)->add_to_store(store);
}


void
Patch::remove_from_store()
{
	// Remove self and ports
	NodeBase::remove_from_store();

	// Remove nodes
	for (Raul::List<Node*>::iterator j = _nodes.begin(); j != _nodes.end(); ++j)
		(*j)->remove_from_store();
}


// Patch specific stuff


void
Patch::add_node(Raul::ListNode<Node*>* ln)
{
	assert(ln != NULL);
	assert(ln->elem() != NULL);
	assert(ln->elem()->parent_patch() == this);
	assert(ln->elem()->poly() == _internal_poly || ln->elem()->poly() == 1);
	
	_nodes.push_back(ln);
}


/** Remove a node.
 * Realtime Safe.  Preprocessing thread.
 */
Raul::ListNode<Node*>*
Patch::remove_node(const string& name)
{
	for (Raul::List<Node*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		if ((*i)->name() == name)
			return _nodes.erase(i);
	
	return NULL;
}


/** Remove a connection.  Realtime safe.
 */
Raul::ListNode<Connection*>*
Patch::remove_connection(const Port* src_port, const Port* dst_port)
{
	bool found = false;
	Raul::ListNode<Connection*>* connection = NULL;
	for (Raul::List<Connection*>::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		if ((*i)->src_port() == src_port && (*i)->dst_port() == dst_port) {
			connection = _connections.erase(i);
			found = true;
		}
	}

	if ( ! found)
		cerr << "WARNING:  [Patch::remove_connection] Connection not found !" << endl;

	return connection;
}

#if 0
/** Remove a bridge_node.  Realtime safe.
 */
Raul::ListNode<InternalNode*>*
Patch::remove_bridge_node(const InternalNode* node)
{
	bool found = false;
	Raul::ListNode<InternalNode*>* bridge_node = NULL;
	for (Raul::List<InternalNode*>::iterator i = _bridge_nodes.begin(); i != _bridge_nodes.end(); ++i) {
		if ((*i) == node) {
			bridge_node = _bridge_nodes.remove(i);
			found = true;
		}
	}

	if ( ! found)
		cerr << "WARNING:  [Patch::remove_bridge_node] InternalNode not found !" << endl;

	return bridge_node;
}
#endif


size_t
Patch::num_ports() const
{
	ThreadID context = ThreadManager::current_thread_id();

	if (context == THREAD_PROCESS)
		return NodeBase::num_ports();
	else
		return _input_ports.size() + _output_ports.size();
}


/** Create a port.  Not realtime safe.
 */
Port*
Patch::create_port(const string& name, DataType type, size_t buffer_size, bool is_output)
{
	if (type == DataType::UNKNOWN) {
		cerr << "[Patch::create_port] Unknown port type " << type.uri() << endl;
		return NULL;
	}

	assert( !(type == DataType::UNKNOWN) );

	return new DuplexPort(this, name, 0, _poly, type, buffer_size, is_output);
}


/** Remove port from ports list used in pre-processing thread.
 *
 * Port is not removed from ports array for process thread (which could be
 * simultaneously running).
 *
 * Realtime safe.  Preprocessing thread only.
 */
Raul::ListNode<Port*>*
Patch::remove_port(const string& name)
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);

	bool found = false;
	Raul::ListNode<Port*>* ret = NULL;
	for (Raul::List<Port*>::iterator i = _input_ports.begin(); i != _input_ports.end(); ++i) {
		if ((*i)->name() == name) {
			ret = _input_ports.erase(i);
			found = true;
		}
	}

	if (!found)
	for (Raul::List<Port*>::iterator i = _output_ports.begin(); i != _output_ports.end(); ++i) {
		if ((*i)->name() == name) {
			ret = _output_ports.erase(i);
			found = true;
		}
	}

	if ( ! found)
		cerr << "WARNING:  [Patch::remove_port] Port not found !" << endl;

	return ret;
}


Raul::Array<Port*>*
Patch::build_ports_array() const
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);

	Raul::Array<Port*>* const result = new Raul::Array<Port*>(_input_ports.size() + _output_ports.size());

	size_t i = 0;
	
	for (Raul::List<Port*>::const_iterator p = _input_ports.begin(); p != _input_ports.end(); ++p,++i)
		result->at(i) = *p;
	
	for (Raul::List<Port*>::const_iterator p = _output_ports.begin(); p != _output_ports.end(); ++p,++i)
		result->at(i) = *p;

	return result;
}


/** Find the process order for this Patch.
 *
 * The process order is a flat list that the patch will execute in order
 * when it's run() method is called.  Return value is a newly allocated list
 * which the caller is reponsible to delete.  Note that this function does
 * NOT actually set the process order, it is returned so it can be inserted
 * at the beginning of an audio cycle (by various Events).
 *
 * Not realtime safe.
 */
Raul::Array<Node*>*
Patch::build_process_order() const
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);

	cerr << "*********** Building process order for " << path() << endl;

	Raul::Array<Node*>* const process_order = new Raul::Array<Node*>(_nodes.size(), NULL);
	
	// FIXME: tweak algorithm so it just ends up like this and save the cost of iteration?
	for (Raul::List<Node*>::const_iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->traversed(false);
		
	// Traverse backwards starting at outputs
	//for (Raul::List<Port*>::const_iterator p = _output_ports.begin(); p != _output_ports.end(); ++p) {

		/*const Port* const port = (*p);
		for (Raul::List<Connection*>::const_iterator c = port->connections().begin();
				c != port->connections().end(); ++c) {
			const Connection* const connection = (*c);
			assert(connection->dst_port() == port);
			assert(connection->src_port());
			assert(connection->src_port()->parent_node());
			build_process_order_recursive(connection->src_port()->parent_node(), process_order);
		}*/
	//}
	
	for (Raul::List<Node*>::const_iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
		Node* const node = (*i);
		// Either a sink or connected to our output ports:
		if ( ( ! node->traversed()) && node->dependants()->size() == 0)
			build_process_order_recursive(node, process_order);
	}

	// Add any (disjoint) nodes that weren't hit by the traversal
	// FIXME: this shouldn't be necessary
	/*for (Raul::List<Node*>::const_iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
		Node* const node = (*i);
		if ( ! node->traversed()) {
			process_order->push_back(*i);
			node->traversed(true);
			cerr << "********** APPENDED DISJOINT NODE " << node->path() << endl;
		}
	}*/

	/*
	cerr << "----------------------------------------\n";
	for (size_t i=0; i < process_order->size(); ++i) {
		assert(process_order->at(i));
		cerr << process_order->at(i)->path() << endl;
	}
	cerr << "----------------------------------------\n";
	*/

	assert(process_order->size() == _nodes.size());

	return process_order;
}


/** Rename this Patch.
 *
 * This is responsible for updating the ObjectStore so the Patch can be
 * found at it's new path, as well as all it's children.
 */
void
Patch::set_path(const Path& new_path)
{
	const Path old_path = path();

	// Update nodes
	for (Raul::List<Node*>::iterator i = _nodes.begin(); i != _nodes.end(); ++i)
		(*i)->set_path(new_path.base() + (*i)->name());
	
	// Update self
	NodeBase::set_path(new_path);
}


} // namespace Ingen
