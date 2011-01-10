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

#ifndef MACHINA_LEARNREQUEST_HPP
#define MACHINA_LEARNREQUEST_HPP

#include "raul/Maid.hpp"
#include "raul/SharedPtr.hpp"

#include "machina/types.hpp"

#include "MidiAction.hpp"
#include "Node.hpp"

namespace Machina {

class Node;
class MidiAction;


/** A request to MIDI learn a certain node.
 */
class LearnRequest : public Raul::Deletable {
public:
	static SharedPtr<LearnRequest>
	create(SharedPtr<Raul::Maid> maid, SharedPtr<Node> node);

	void start(double q, Raul::TimeStamp time);
	void finish(TimeStamp time);

	inline bool started() const { return _started; }

	const SharedPtr<Node>&       node()         { return _node; }
	const SharedPtr<MidiAction>& enter_action() { return _enter_action; }
	const SharedPtr<MidiAction>& exit_action()  { return _exit_action; }

private:
	LearnRequest(SharedPtr<Raul::Maid> maid, SharedPtr<Node> node);

	bool                  _started;
	TimeStamp             _start_time;
	double                _quantization;
	SharedPtr<Node>       _node;
	SharedPtr<MidiAction> _enter_action;
	SharedPtr<MidiAction> _exit_action;
};


} // namespace Machina

#endif // MACHINA_LEARNREQUEST_HPP
