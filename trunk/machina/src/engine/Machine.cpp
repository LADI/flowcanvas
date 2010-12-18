/* This file is part of Machina.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
 *
 * Machina is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Machina is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <cstdlib>

#include "raul/SharedPtr.hpp"
#include "redlandmm/Model.hpp"
#include "redlandmm/World.hpp"

#include "machina/Edge.hpp"
#include "machina/Machine.hpp"
#include "machina/MidiAction.hpp"
#include "machina/Node.hpp"

#include "LearnRequest.hpp"

using namespace std;
using namespace Raul;

namespace Machina {


Machine::Machine(TimeUnit unit)
	: _active_nodes(MAX_ACTIVE_NODES, SharedPtr<Node>())
	, _is_activated(false)
	, _is_finished(false)
	, _time(unit, 0, 0)
{
}


/** Copy a Machine.
 *
 * Creates a deep copy which is the 'same' machine, but with
 * fresh state (deactivated, rewound)
 */
Machine::Machine(const Machine& copy)
	: Raul::Stateful() // don't copy RDF ID
	, _active_nodes(MAX_ACTIVE_NODES, SharedPtr<Node>())
	, _is_activated(false)
	, _is_finished(false)
	, _time(copy.time())
	, _sink(copy._sink)
{
	map< SharedPtr<Node>, SharedPtr<Node> > replacements;

	for (Nodes::const_iterator n = copy._nodes.begin(); n != copy._nodes.end(); ++n) {
		SharedPtr<Machina::Node> node(new Machina::Node(*n->get()));
		_nodes.push_back(node);
		replacements[*n] = node;
	}

	for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n) {
		for (Node::Edges::const_iterator e = (*n)->edges().begin(); e != (*n)->edges().end(); ++e) {
			(*e)->set_tail(*n);
			(*e)->set_head(replacements[(*e)->head()]);
			assert((*e)->head());
		}
	}
}


Machine&
Machine::operator=(const Machine& other)
{
	_active_nodes = std::vector< SharedPtr<Node> >(MAX_ACTIVE_NODES, SharedPtr<Node>());
	_is_activated = false;
	_is_finished = false;
	_time = other._time;
	_pending_learn = SharedPtr<LearnRequest>();
	_sink = other._sink;
	_nodes.clear();

	map< SharedPtr<Node>, SharedPtr<Node> > replacements;

	for (Nodes::const_iterator n = other._nodes.begin(); n != other._nodes.end(); ++n) {
		SharedPtr<Machina::Node> node(new Machina::Node(*n->get()));
		_nodes.push_back(node);
		replacements[*n] = node;
	}

	for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n) {
		for (Node::Edges::const_iterator e = (*n)->edges().begin(); e != (*n)->edges().end(); ++e) {
			(*e)->set_tail(*n);
			(*e)->set_head(replacements[(*e)->head()]);
			assert((*e)->head());
		}
	}

	return *this;
}


/** Set the MIDI sink to be used for executing MIDI actions.
 *
 * MIDI actions will silently do nothing unless this call is passed an
 * existing Raul::MIDISink before running.
 */
void
Machine::set_sink(SharedPtr<Raul::MIDISink> sink)
{
	_sink = sink;
}


/** Always returns a node, unless there are none */
SharedPtr<Node>
Machine::random_node()
{
	if (_nodes.empty())
		return SharedPtr<Node>();

	size_t i = rand() % _nodes.size();

	// FIXME: O(n) worst case :(
	for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n, --i)
		if (i == 0)
			return *n;

	return SharedPtr<Node>();
}


/** May return NULL even if edges exist (with low probability) */
SharedPtr<Edge>
Machine::random_edge()
{
	SharedPtr<Node> tail = random_node();

	for (size_t i = 0; i < _nodes.size() && tail->edges().empty(); ++i)
		tail = random_node();

	return tail ? tail->random_edge() : SharedPtr<Edge>();
}


void
Machine::add_node(SharedPtr<Node> node)
{
	assert(_nodes.find(node) == _nodes.end());
	_nodes.push_back(node);
}


void
Machine::remove_node(SharedPtr<Node> node)
{
	_nodes.erase(_nodes.find(node));

	for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n)
		(*n)->remove_edges_to(node);
}


/** Exit all active states and reset time to 0.
 */
void
Machine::reset(Raul::TimeStamp time)
{
	if (!_is_finished) {
		for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n) {
			SharedPtr<Node> node = (*n);

			if (node->is_active())
				node->exit(_sink.lock(), time);

			assert(! node->is_active());
		}

		for (size_t i=0; i < MAX_ACTIVE_NODES; ++i)
			_active_nodes.at(i).reset();
	}

	_time = TimeStamp(_time.unit(), 0, 0);
	_is_finished = false;
}


/** Return the active Node with the earliest exit time.
 */
SharedPtr<Node>
Machine::earliest_node() const
{
	SharedPtr<Node> earliest;

	for (size_t i=0; i < MAX_ACTIVE_NODES; ++i) {
		SharedPtr<Node> node = _active_nodes.at(i);

		if (node) {
			assert(node->is_active());
			if (!earliest || node->exit_time() < earliest->exit_time()) {
				earliest = node;
			}
		}
	}

	return earliest;
}


/** Enter a state at the current _time.
 *
 * Returns true if node was entered, or false if the maximum active nodes has been reached.
 */
bool
Machine::enter_node(SharedPtr<Raul::MIDISink> sink, SharedPtr<Node> node)
{
	assert(!node->is_active());

	/* FIXME: Would be best to use the MIDI note here as a hash key, at least
	 * while all actions are still MIDI notes... */
	size_t index = (rand() % MAX_ACTIVE_NODES);
	for (size_t i=0; i < MAX_ACTIVE_NODES; ++i) {
		if (_active_nodes.at(index) == NULL) {
			node->enter(sink, _time);
			assert(node->is_active());
			_active_nodes.at(index) = node;
			return true;
		}
		index = (index + 1) % MAX_ACTIVE_NODES;
	}

	// If we get here, ran out of active node spots.  Don't enter node
	return false;
}



/** Exit an active node at the current _time.
 */
void
Machine::exit_node(SharedPtr<Raul::MIDISink> sink, SharedPtr<Node> node)
{
	node->exit(sink, _time);
	assert(!node->is_active());

	for (size_t i=0; i < MAX_ACTIVE_NODES; ++i)
		if (_active_nodes.at(i) == node)
			_active_nodes.at(i).reset();

	// Activate successors to this node
	// (that aren't aready active right now)

	if (node->is_selector()) {

		const double rand_normal = rand() / (double)RAND_MAX; // [0, 1]
		double range_min = 0;

		for (Node::Edges::const_iterator s = node->edges().begin();
				s != node->edges().end(); ++s) {

			if (!(*s)->head()->is_active()
					&& rand_normal > range_min
					&& rand_normal < range_min + (*s)->probability()) {

				enter_node(sink, (*s)->head());
				break;

			} else {
				range_min += (*s)->probability();
			}
		}

	} else {

		for (Node::Edges::const_iterator e = node->edges().begin();
				e != node->edges().end(); ++e) {

			const double rand_normal = rand() / (double)RAND_MAX; // [0, 1]

			if (rand_normal <= (*e)->probability()) {
				SharedPtr<Node> head = (*e)->head();

				if ( ! head->is_active())
					enter_node(sink, head);
			}
		}

	}
}


/** Run the machine for a (real) time slice.
 *
 * Returns the duration of time the machine actually ran.
 *
 * Caller can check is_finished() to determine if the machine still has any
 * active states.  If not, time() will return the exact time stamp the
 * machine actually finished on (so it can be restarted immediately
 * with sample accuracy if necessary).
 */
uint32_t
Machine::run(const Raul::TimeSlice& time)
{
	if (_is_finished)
		return 0;

	SharedPtr<Raul::MIDISink> sink = _sink.lock();

	const TimeStamp cycle_end_frames = time.start_ticks() + time.length_ticks();
	const TimeStamp cycle_end = time.ticks_to_beats(cycle_end_frames);

	assert(_is_activated);

	// Initial run, enter all initial states
	if (_time.is_zero()) {
		bool entered = false;
		if ( ! _nodes.empty()) {
			for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n) {
				if ((*n)->is_active())
					(*n)->exit(sink, _time);

				if ((*n)->is_initial()) {
					if (enter_node(sink, (*n)))
						entered = true;
				}
			}
		}
		if (!entered) {
			_is_finished = true;
			return 0;
		}
	}

	while (true) {

		SharedPtr<Node> earliest = earliest_node();

		if (!earliest) {
			// No more active states, machine is finished
#ifndef NDEBUG
			for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n)
				assert( ! (*n)->is_active());
#endif
			_is_finished = true;
			break;

		} else if (time.beats_to_ticks(earliest->exit_time()) < cycle_end_frames) {
			// Earliest active state ends this cycle
			_time = earliest->exit_time();
			exit_node(sink, earliest);

		} else {
			// Earliest active state ends in the future, done this cycle
			_time = cycle_end;
			break;
		}

	}

	return time.beats_to_ticks(_time).ticks() - time.start_ticks().ticks();
}


/** Push a node onto the learn stack.
 *
 * NOT realtime (actions are allocated here).
 */
void
Machine::learn(SharedPtr<Raul::Maid> maid, SharedPtr<Node> node)
{
	_pending_learn = LearnRequest::create(maid, node);
}


void
Machine::write_state(Redland::Model& model)
{
	using namespace Raul;

	model.world().add_prefix("machina", "http://drobilla.net/ns/machina#");

	model.add_statement(model.base_uri(),
			Redland::Node(model.world(), Redland::Node::RESOURCE, "rdf:type"),
			Redland::Node(model.world(), Redland::Node::RESOURCE, "machina:Machine"));

	size_t count = 0;

	for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n) {

		(*n)->write_state(model);

		if ((*n)->is_initial()) {
			model.add_statement(model.base_uri(),
					Redland::Node(model.world(), Redland::Node::RESOURCE, "machina:initialNode"),
					(*n)->id());
		} else {
			model.add_statement(model.base_uri(),
					Redland::Node(model.world(), Redland::Node::RESOURCE, "machina:node"),
					(*n)->id());
		}
	}

	count = 0;

	for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n) {

		for (Node::Edges::const_iterator e = (*n)->edges().begin();
			e != (*n)->edges().end(); ++e) {

			(*e)->write_state(model);

			model.add_statement(model.base_uri(),
				Redland::Node(model.world(), Redland::Node::RESOURCE, "machina:edge"),
				(*e)->id());
		}

	}
}

} // namespace Machina
