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

#include "raul/Quantizer.hpp"
#include "LearnRequest.hpp"

namespace Machina {


LearnRequest::LearnRequest(SharedPtr<Raul::Maid> maid, SharedPtr<Node> node)
	: _started(false)
	, _start_time(TimeUnit(TimeUnit::BEATS, 19200), 0, 0) // irrelevant
	, _quantization(0) // irrelevant
	, _node(node)
	, _enter_action(new MidiAction(4, NULL))
	, _exit_action(new MidiAction(4, NULL))
{
	maid->manage(_enter_action);
	maid->manage(_exit_action);
}

SharedPtr<LearnRequest>
LearnRequest::create(SharedPtr<Raul::Maid> maid, SharedPtr<Node> node)
{
	SharedPtr<LearnRequest> ret(new LearnRequest(maid, node));
	maid->manage(ret);
	return ret;
}

void
LearnRequest::start(double q, Raul::TimeStamp time)
{
	_started = true;
	_start_time = time;
	_quantization = q;
}

/** Add the learned actions to the node */
void
LearnRequest::finish(TimeStamp time)
{
	_node->set_enter_action(_enter_action);
	_node->set_exit_action(_exit_action);

	//TimeDuration duration = Raul::Quantizer::quantize(_quantization, time - _start_time);

	//_node->set_duration(duration);
}


}
