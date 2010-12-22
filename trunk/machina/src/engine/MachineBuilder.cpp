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

#include <algorithm>

#include "raul/midi_events.h"
#include "raul/SharedPtr.hpp"
#include "raul/Quantizer.hpp"

#include "machina/Edge.hpp"
#include "machina/Machine.hpp"
#include "machina/MidiAction.hpp"
#include "machina/Node.hpp"

#include "MachineBuilder.hpp"

using namespace std;
using namespace Raul;

namespace Machina {


MachineBuilder::MachineBuilder(SharedPtr<Machine> machine, double q, bool step)
	: _quantization(q)
	, _time(machine->time().unit()) // = 0
	, _machine(machine)
	, _initial_node(new Node(_time, true)) // duration 0
	, _connect_node(_initial_node)
	, _connect_node_end_time(_time) // = 0
	, _step(step)
{
}


void
MachineBuilder::reset()
{
	_time = TimeStamp(_machine->time().unit()); // = 0
	_initial_node = SharedPtr<Node>(new Node(_time, true)); // duration 0
	_connect_node = _initial_node;
	_connect_node_end_time = _time; // = 0
}


bool
MachineBuilder::is_delay_node(SharedPtr<Node> node) const
{
	return !node->enter_action() && !node->exit_action();
}


/** Set the duration of a node, with quantization.
 */
void
MachineBuilder::set_node_duration(SharedPtr<Node> node, Raul::TimeDuration d) const
{
	if (_step) {
		node->set_duration(TimeStamp(d.unit(), 1, 0));
		return;
	}
	
	Raul::TimeStamp q_dur = Quantizer::quantize(TimeStamp(d.unit(), _quantization), d);

	// Never quantize a note to duration 0
	if (q_dur.is_zero() && ( node->enter_action() || node->exit_action() ))
		q_dur = _quantization; // Round up

	node->set_duration(q_dur);
}


/** Connect two nodes, inserting a delay node between them if necessary.
 *
 * If a delay node is added to the machine, it is returned.
 */
SharedPtr<Node>
MachineBuilder::connect_nodes(SharedPtr<Machine> m,
                              SharedPtr<Node>    tail, Raul::TimeStamp tail_end_time,
                              SharedPtr<Node>    head, Raul::TimeStamp head_start_time)
{
	assert(tail != head);
	assert(head_start_time >= tail_end_time);

	SharedPtr<Node> delay_node;

	if (is_delay_node(tail) && tail->edges().size() == 0) {
		// Tail is a delay node, just accumulate the time difference into it
		set_node_duration(tail, tail->duration() + head_start_time - tail_end_time);
		tail->add_edge(SharedPtr<Edge>(new Edge(tail, head)));
	} else if (_step || (head_start_time == tail_end_time)) {
		// Connect directly
		tail->add_edge(SharedPtr<Edge>(new Edge(tail, head)));
	} else {
		// Need to actually create a delay node
		delay_node = SharedPtr<Node>(new Node(head_start_time - tail_end_time));
		tail->add_edge(SharedPtr<Edge>(new Edge(tail, delay_node)));
		delay_node->add_edge(SharedPtr<Edge>(new Edge(delay_node, head)));
		m->add_node(delay_node);
	}

	return delay_node;
}


void
MachineBuilder::event(Raul::TimeStamp time_offset,
                      size_t          ev_size,
                      unsigned char*  buf)
{
	const Raul::TimeUnit unit = time_offset.unit();
	Raul::TimeStamp t = _time + time_offset;

	if (ev_size == 0)
		return;

	if ((buf[0] & 0xF0) == MIDI_CMD_NOTE_ON) {

		SharedPtr<Node> node(new Node(TimeStamp(unit)));
		node->set_enter_action(SharedPtr<Action>(new MidiAction(ev_size, buf)));

		SharedPtr<Node> this_connect_node;
		Raul::TimeStamp this_connect_node_end_time(unit);

		// If currently polyphonic, use a poly node with no successors as connect node
		// Results in patterns closest to what a human would choose
		if ( ! _poly_nodes.empty()) {
			for (PolyList::iterator j = _poly_nodes.begin(); j != _poly_nodes.end(); ++j) {
				if (j->second->edges().empty()) {
					this_connect_node = j->second;
					this_connect_node_end_time = j->first + j->second->duration();
					break;
				}
			}
		}

		// Currently monophonic, or didn't find a poly node, so use _connect_node
		// which is maintained below on note off events.
		if ( ! this_connect_node) {
			this_connect_node          = _connect_node;
			this_connect_node_end_time = _connect_node_end_time;
		}

		SharedPtr<Node> delay_node = connect_nodes(_machine,
				this_connect_node, this_connect_node_end_time, node, t);

		if (delay_node) {
		  _connect_node = delay_node;
		  _connect_node_end_time = t;
		}

		node->enter(SharedPtr<Raul::MIDISink>(), t);
		_active_nodes.push_back(node);

	} else if ((buf[0] & 0xF0) == MIDI_CMD_NOTE_OFF) {

		for (ActiveList::iterator i = _active_nodes.begin(); i != _active_nodes.end(); ++i) {
			SharedPtr<MidiAction> action = PtrCast<MidiAction>((*i)->enter_action());
			if (!action)
				continue;

			const size_t         ev_size = action->event_size();
			const unsigned char* ev      = action->event();

			if (ev_size == 3 && (ev[0] & 0xF0) == MIDI_CMD_NOTE_ON
					&& (ev[0] & 0x0F) == (buf[0] & 0x0F) // same channel
					&& ev[1] == buf[1]) // same note
			{
				SharedPtr<Node> resolved = *i;

				resolved->set_exit_action(SharedPtr<Action>(new MidiAction(ev_size, buf)));
				set_node_duration(resolved, t - resolved->enter_time());

				// Last active note
				if (_active_nodes.size() == 1) {

					_connect_node_end_time = t;

					// Finish a polyphonic section
					if (_poly_nodes.size() > 0) {

						_connect_node = SharedPtr<Node>(new Node(TimeStamp(unit)));
						_machine->add_node(_connect_node);

						connect_nodes(_machine, resolved, t, _connect_node, t);

						for (PolyList::iterator j = _poly_nodes.begin(); j != _poly_nodes.end(); ++j) {
							_machine->add_node(j->second);
							if (j->second->edges().size() == 0)
								connect_nodes(_machine, j->second, j->first + j->second->duration(),
										_connect_node, t);
						}
						_poly_nodes.clear();

						_machine->add_node(resolved);

					// Just monophonic
					} else {

						// Trim useless delay node if possible (these appear after poly sections)
						if (is_delay_node(_connect_node) && _connect_node->duration().is_zero()
								&& _connect_node->edges().size() == 1
								&& (*_connect_node->edges().begin())->head() == resolved) {

							_connect_node->edges().clear();
							assert(_connect_node->edges().empty());
							_connect_node->set_enter_action(resolved->enter_action());
							_connect_node->set_exit_action(resolved->exit_action());
							resolved->set_enter_action(SharedPtr<Action>());
							resolved->set_exit_action(SharedPtr<Action>());
							set_node_duration(_connect_node, resolved->duration());
							resolved = _connect_node;
							if (_machine->nodes().find(_connect_node) == _machine->nodes().end())
								_machine->add_node(_connect_node);

						} else {
							_connect_node = resolved;
							_machine->add_node(resolved);
						}
					}

				// Polyphonic, add this state to poly list
				} else {
					_poly_nodes.push_back(make_pair(resolved->enter_time(), resolved));
					_connect_node = resolved;
					_connect_node_end_time = t;
				}

				if (resolved->is_active())
					resolved->exit(SharedPtr<Raul::MIDISink>(), t);

				_active_nodes.erase(i);

				break;
			}
		}

	}
}


/** Finish the constructed machine and prepare it for use.
 * Resolve any stuck notes, quantize, etc.
 */
void
MachineBuilder::resolve()
{
	// Resolve stuck notes
	if ( ! _active_nodes.empty()) {
		for (list<SharedPtr<Node> >::iterator i = _active_nodes.begin(); i != _active_nodes.end(); ++i) {
			cerr << "WARNING:  Resolving stuck note." << endl;
			SharedPtr<MidiAction> action = PtrCast<MidiAction>((*i)->enter_action());
			if (!action)
				continue;

			const size_t         ev_size = action->event_size();
			const unsigned char* ev      = action->event();
			if (ev_size == 3 && (ev[0] & 0xF0) == MIDI_CMD_NOTE_ON) {
				unsigned char note_off[3] = { ((MIDI_CMD_NOTE_OFF & 0xF0) | (ev[0] & 0x0F)), ev[1], 0x40 };
				(*i)->set_exit_action(SharedPtr<Action>(new MidiAction(3, note_off)));
				set_node_duration((*i), _time - (*i)->enter_time());
				(*i)->exit(SharedPtr<Raul::MIDISink>(), _time);
				_machine->add_node((*i));
			}
		}
		_active_nodes.clear();
	}

	// Add initial note if necessary
	if (_machine->nodes().size() > 0
			&& _machine->nodes().find(_initial_node) == _machine->nodes().end())
		_machine->add_node(_initial_node);

}


SharedPtr<Machine>
MachineBuilder::finish()
{
	resolve();

	return _machine;
}


} // namespace Machina
