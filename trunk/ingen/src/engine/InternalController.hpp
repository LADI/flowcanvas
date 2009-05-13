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

#ifndef MIDICONTROLNODE_H
#define MIDICONTROLNODE_H

#include <string>
#include "NodeBase.hpp"

namespace Ingen {

class InputPort;
class OutputPort;


/** MIDI control input node.
 *
 * Creating one of these nodes is how a user makes "MIDI Bindings".  Note that
 * this node will always be monophonic, the poly parameter is ignored.
 *
 * \ingroup engine
 */
class ControllerNode : public NodeBase
{
public:
	ControllerNode(const std::string& path, bool polyphonic, PatchImpl* parent, SampleRate srate, size_t buffer_size);

	void process(ProcessContext& context);

	void control(ProcessContext& context, uint8_t control_num, uint8_t val, FrameTime time);

	void learn() { _learning = true; }

private:
	bool _learning;

	InputPort*  _midi_in_port;
	InputPort*  _param_port;
	InputPort*  _log_port;
	InputPort*  _min_port;
	InputPort*  _max_port;
	OutputPort* _audio_port;
};


} // namespace Ingen

#endif // MIDICONTROLNODE_H
