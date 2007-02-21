/* This file is part of Machina.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef MACHINA_NODE_HPP
#define MACHINA_NODE_HPP

#include <boost/utility.hpp>
#include <raul/SharedPtr.h>
#include <raul/List.h>
#include <raul/Stateful.h>
#include <raul/TimeSlice.h>
#include "Action.hpp"

namespace Machina {

class Edge;
using Raul::BeatCount;
using Raul::BeatTime;


/** A node is a state (as in a FSM diagram), or "note".
 *
 * It contains a action, as well as a duration and pointers to it's
 * successors (states/nodes that (may) follow it).
 *
 * Initial nodes do not have enter actions (since they are entered at
 * an undefined point in time <= 0).
 */
class Node : public Raul::Stateful, public boost::noncopyable {
public:
	typedef std::string ID;

	Node(BeatCount duration=0, bool initial=false);

	void add_enter_action(SharedPtr<Action> action);
	void remove_enter_action(SharedPtr<Action> action);
	
	void add_exit_action(SharedPtr<Action> action);
	void remove_exit_action(SharedPtr<Action> action);

	void enter(BeatTime time);
	void exit(BeatTime time);

	void add_outgoing_edge(SharedPtr<Edge> edge);
	void remove_outgoing_edge(SharedPtr<Edge> edge);

	void write_state(Raul::RDFWriter& writer);

	bool      is_initial() const        { return _is_initial; }
	void      set_initial(bool i)       { _is_initial = i; }
	bool      is_active() const         { return _is_active; }
	BeatTime  enter_time() const        { return _enter_time; }
	BeatTime  exit_time() const         { return _enter_time + _duration; }
	BeatCount duration()                { return _duration; }
	void      set_duration(BeatCount d) { _duration = d; }
	
	typedef Raul::List<SharedPtr<Edge> > EdgeList;
	const EdgeList& outgoing_edges() const { return _outgoing_edges; }
	
private:
	bool              _is_initial;
	bool              _is_active;
	BeatTime          _enter_time; ///< valid iff _is_active
	BeatCount         _duration;
	SharedPtr<Action> _enter_action;
	SharedPtr<Action> _exit_action;
	EdgeList          _outgoing_edges;
};


} // namespace Machina

#endif // MACHINA_NODE_HPP