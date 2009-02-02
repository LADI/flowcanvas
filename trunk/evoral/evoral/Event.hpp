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

#ifndef EVORAL_EVENT_HPP
#define EVORAL_EVENT_HPP

#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <assert.h>
#include "evoral/types.hpp"


/** If this is not defined, all methods of MidiEvent are RT safe
 * but MidiEvent will never deep copy and (depending on the scenario)
 * may not be usable in STL containers, signals, etc. 
 */
#define EVORAL_EVENT_ALLOC 1

namespace Evoral {


/** An event (much like a type generic jack_midi_event_t)
 *
 * Template parameter T is the type of the time stamp used for this event.
 */
template<typename T>
struct Event {
#ifdef EVORAL_EVENT_ALLOC
	Event(EventType type=0, T t=0, uint32_t s=0, uint8_t* b=NULL, bool alloc=false);
	
	/** Copy \a copy.
	 * 
	 * If \a alloc is true, the buffer will be copied and this method
	 * is NOT REALTIME SAFE.  Otherwise both events share a buffer and
	 * memory management semantics are the caller's problem.
	 */
	Event(const Event& copy, bool alloc);
	
	~Event();

	inline const Event& operator=(const Event& copy) {
		_type = copy._type;
		_time = copy._time;
		if (_owns_buf) {
			if (copy._buf) {
				if (copy._size > _size) {
					_buf = (uint8_t*)::realloc(_buf, copy._size);
				}
				memcpy(_buf, copy._buf, copy._size);
			} else {
				free(_buf);
				_buf = NULL;
			}
		} else {
			_buf = copy._buf;
		}

		_size = copy._size;
		return *this;
	}

	inline void shallow_copy(const Event& copy) {
		if (_owns_buf) {
			free(_buf);
			_buf = false;
			_owns_buf = false;
		}

		_type = copy._type;
		_time = copy._time;
		_size = copy._size;
		_buf  = copy._buf;
	}
	
	inline void set(uint8_t* buf, uint32_t size, T t) {
		if (_owns_buf) {
			if (_size < size) {
				_buf = (uint8_t*) ::realloc(_buf, size);
			}
			memcpy (_buf, buf, size);
		} else {
			_buf = buf;
		}

		_time = t;
		_size = size;
	}

	inline bool operator==(const Event& other) const {
		if (_type != other._type)
			return false;

		if (_time != other._time)
			return false;

		if (_size != other._size)
			return false;

		if (_buf == other._buf)
			return true;

		for (uint32_t i=0; i < _size; ++i)
			if (_buf[i] != other._buf[i])
				return false;

		return true;
	}
	
	inline bool operator!=(const Event& other) const { return ! operator==(other); }

	inline bool owns_buffer() const { return _owns_buf; }
	
	inline void set_buffer(uint32_t size, uint8_t* buf, bool own) {
		if (_owns_buf) {
			free(_buf);
			_buf = NULL;
		}
		_size     = size;
		_buf      = buf;
		_owns_buf = own;
	}

	inline void realloc(uint32_t size) {
		if (_owns_buf) {
			if (size > _size)
				_buf = (uint8_t*) ::realloc(_buf, size);
		} else {
			_buf = (uint8_t*) ::malloc(size);
			_owns_buf = true;
		}

		_size = size;
	}
	
	inline void clear() {
		_type = 0;
		_time = 0;
		_size = 0;
		_buf  = NULL;
	}

#else

	inline void set_buffer(uint8_t* buf) { _buf = buf; }

#endif // EVORAL_EVENT_ALLOC

	inline EventType   event_type()            const { return _type; }
	inline void        set_event_type(EventType t)   { _type = t; }
	inline T           time()                  const { return _time; }
	inline T&          time()                        { return _time; }
	inline uint32_t    size()                  const { return _size; }
	inline uint32_t&   size()                        { return _size; }

	inline const uint8_t* buffer()             const { return _buf; }
	inline uint8_t*&      buffer()                   { return _buf; }

protected:
	EventType _type; /**< Type of event (application relative, NOT MIDI 'type') */
	T         _time; /**< Sample index (or beat time) at which event is valid */
	uint32_t  _size; /**< Number of uint8_ts of data in \a buffer */
	uint8_t*  _buf;  /**< Raw MIDI data */

#ifdef EVORAL_EVENT_ALLOC
	bool _owns_buf; /**< Whether buffer is locally allocated */
#endif
};


} // namespace Evoral

#endif // EVORAL_EVENT_HPP

