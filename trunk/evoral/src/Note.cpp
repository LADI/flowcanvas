/* This file is part of Evoral.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
 * Copyright (C) 2000-2008 Paul Davis
 *
 * Evoral is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Evoral is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <iostream>
#include <limits>
#include "evoral/Note.hpp"

namespace Evoral {

template<typename Time>
Note<Time>::Note(uint8_t chan, Time t, Time l, uint8_t n, uint8_t v)
	// FIXME: types?
	: _on_event(0xDE, t, 3, NULL, true)
	, _off_event(0xAD, t + l, 3, NULL, true)
{
	assert(chan < 16);

	_on_event.buffer()[0] = MIDI_CMD_NOTE_ON + chan;
	_on_event.buffer()[1] = n;
	_on_event.buffer()[2] = v;

	_off_event.buffer()[0] = MIDI_CMD_NOTE_OFF + chan;
	_off_event.buffer()[1] = n;
	_off_event.buffer()[2] = 0x40;

	assert(time() == t);
	assert(musical_time_equal (length(), l));
	assert(note() == n);
	assert(velocity() == v);
	assert(_on_event.channel() == _off_event.channel());
	assert(channel() == chan);
}


template<typename Time>
Note<Time>::Note(const Note<Time>& copy)
	: _on_event(copy._on_event, true)
	, _off_event(copy._off_event, true)
{
	assert(_on_event.buffer());
	assert(_off_event.buffer());
	/*
	assert(copy._on_event.size == 3);
	_on_event.buffer = _on_event_buffer;
	memcpy(_on_event_buffer, copy._on_event_buffer, 3);

	assert(copy._off_event.size == 3);
	_off_event.buffer = _off_event_buffer;
	memcpy(_off_event_buffer, copy._off_event_buffer, 3);
	*/

	assert(time() == copy.time());
	assert(end_time() == copy.end_time());
	assert(note() == copy.note());
	assert(velocity() == copy.velocity());
	assert(length() == copy.length());
	assert(_on_event.channel() == _off_event.channel());
	assert(channel() == copy.channel());
}

template<typename Time>
Note<Time>::~Note()
{
}

template<typename Time>
const Note<Time>&
Note<Time>::operator=(const Note<Time>& other)
{
	_on_event = other._on_event;
	_off_event = other._off_event;

	assert(time() == other.time());
	assert(end_time() == other.end_time());
	assert(note() == other.note());
	assert(velocity() == other.velocity());
	assert(length() == other.length());
	assert(_on_event.channel() == _off_event.channel());
	assert(channel() == other.channel());

	return *this;
}

template class Note<Evoral::MusicalTime>;

} // namespace Evoral

