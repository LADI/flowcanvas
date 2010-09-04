/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_EVENTS_SENDPORTVALUE_HPP
#define INGEN_EVENTS_SENDPORTVALUE_HPP

#include "raul/Atom.hpp"
#include "engine/Event.hpp"
#include "engine/types.hpp"

namespace Ingen {

class PortImpl;

namespace Events {


/** A special event used internally to send port values from the audio thread.
 *
 * This is created in the audio thread (using in-place new on a preallocated
 * buffer) for processing in the post processing thread (unlike normal events
 * which are created in the pre-processor thread then run through the audio
 * thread).  This event's job is done entirely in post_process.
 *
 * This only works for control ports right now.
 *
 * \ingroup engine
 */
class SendPortValue : public Event
{
public:
	inline SendPortValue(
			Engine&           engine,
			SampleCount       timestamp,
			PortImpl*         port,
			bool              omni,
			uint32_t          voice_num,
			const Raul::Atom& value)
		: Event(engine, SharedPtr<Request>(), timestamp)
		, _port(port)
		, _omni(omni)
		, _voice_num(voice_num)
		, _value(value)
	{
	}

	inline SendPortValue& operator=(const SendPortValue& ev) {
		_port = ev._port;
		_omni = ev._omni;
		_voice_num = ev._voice_num;
		_value = ev._value;
		return *this;
	}

	void post_process();

private:
	PortImpl*  _port;
	bool       _omni;
	uint32_t   _voice_num;
	Raul::Atom _value;
};


} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_SENDPORTVALUE_HPP
