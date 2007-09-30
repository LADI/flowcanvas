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

#define __STDC_LIMIT_MACROS 1
#include <stdint.h>
#include <iostream>
#include "MidiBuffer.hpp"

using namespace std;

namespace Ingen {


/** Allocate a new MIDI buffer.
 * \a capacity is in bytes (not number of events).
 */
MidiBuffer::MidiBuffer(size_t capacity)
	: Buffer(DataType(DataType::MIDI), capacity)
	, _latest_stamp(0)
	, _joined_buf(NULL)
{
	if (capacity > UINT32_MAX) {
		cerr << "MIDI buffer size " << capacity << " too large, aborting." << endl;
		throw std::bad_alloc();
	}

	int ret = posix_memalign((void**)&_local_buf, 16, sizeof(LV2_MIDI));
	if (ret) {
		cerr << "Failed to allocate MIDI buffer.  Aborting." << endl;
		exit(EXIT_FAILURE);
	}
	
	ret = posix_memalign((void**)&_local_buf->data, 16, capacity);
	if (ret) {
		cerr << "Failed to allocate MIDI buffer contents.  Aborting." << endl;
		exit(EXIT_FAILURE);
	}
	
	_local_buf->capacity = (uint32_t)capacity;
	_buf = _local_buf;
	reset(0);

	//cerr << "Creating MIDI Buffer " << _buf << ", capacity = " << _buf->capacity << endl;
}

MidiBuffer::~MidiBuffer()
{
	free(_local_buf->data);
	free(_local_buf);
}


/** Use another buffer's data instead of the local one.
 *
 * This buffer will essentially be identical to @a buf after this call.
 */
bool
MidiBuffer::join(Buffer* buf)
{
	MidiBuffer* mbuf = dynamic_cast<MidiBuffer*>(buf);
	if (mbuf) {
		_position = mbuf->_position;
		_buf = mbuf->local_data();
		_joined_buf = mbuf;
		return false;
	} else {
		return false;
	}

	//assert(mbuf->size() == _size);
	
	_joined_buf = mbuf;

	return true;
}

	
void
MidiBuffer::unjoin()
{
	_joined_buf = NULL;
	_buf = _local_buf;
	reset(_this_nframes);
}


bool
MidiBuffer::is_joined_to(Buffer* buf) const
{
	MidiBuffer* mbuf = dynamic_cast<MidiBuffer*>(buf);
	if (mbuf)
		return (data() == mbuf->data());

	return false;
}


void
MidiBuffer::prepare_read(SampleCount nframes)
{
	rewind();
	_this_nframes = nframes;
}


void
MidiBuffer::prepare_write(SampleCount nframes)
{
	reset(nframes);
}
	
/** FIXME: parameters ignored */
void
MidiBuffer::copy(const Buffer* src_buf, size_t start_sample, size_t end_sample)
{
	MidiBuffer* src = (MidiBuffer*)src_buf;
	clear();
	src->rewind();
	const uint32_t frame_count = min(_this_nframes, src->this_nframes());
	double time;
	uint32_t size;
	unsigned char* data;
	while (src->increment() < frame_count) {
		src->get_event(&time, &size, &data);
		assert(data[0] >= 0x80);
		append(time, size, data);
	}
}

/** Increment the read position by one event.
 *
 * Returns the timestamp of the now current event, or this_nframes if
 * there are no events left.
 */
double
MidiBuffer::increment() const
{
	if (_position + sizeof(double) + sizeof(uint32_t) >= _buf->size) {
		_position = _buf->size;
		return _this_nframes; // hit end
	}

	_position += sizeof(double) + sizeof(uint32_t) +
		*(uint32_t*)(_buf->data + _position + sizeof(double));

	if (_position >= _buf->size) {
		_position = _buf->size;
		return _this_nframes;
	} else {
		return *(double*)(_buf->data + _position);
	}
}


/** Append a MIDI event to the buffer.
 *
 * \a timestamp must be >= the latest event in the buffer,
 * and < this_nframes()
 *
 * \return true on success
 */
bool
MidiBuffer::append(double               timestamp,
                   uint32_t             size,
                   const unsigned char* data)
{
	/*cerr << "Append midi " << size << " bytes @ " << timestamp << ":" << endl;
	for (uint32_t i=0; i < size; ++i) {
		fprintf(stderr, "( %X )", *((uint8_t*)data + i));
	}*/

	if (_buf->capacity - _buf->size < sizeof(double) + sizeof(uint32_t) + size)
		return false;
	
	assert(size > 0);
	assert(data[0] >= 0x80);
	assert(timestamp >= _latest_stamp);

	*(double*)(_buf->data + _buf->size) = timestamp;
	_buf->size += sizeof(double);
	*(uint32_t*)(_buf->data + _buf->size) = size;
	_buf->size += sizeof(uint32_t);
	memcpy(_buf->data + _buf->size, data, size);
	_buf->size += size;

	++_buf->event_count;
	_latest_stamp = timestamp;

	return true;
}


/** Read an event from the current position in the buffer
 * 
 * \return the timestamp for the read event, or this_nframes()
 * if there are no more events in the buffer.
 */
double
MidiBuffer::get_event(double*         timestamp, 
                      uint32_t*       size, 
                      unsigned char** data) const
{
	if (_position >= _buf->size) {
		_position = _buf->size;
		*timestamp = _this_nframes;
		*size = 0;
		*data = NULL;
		return _this_nframes;
	}

	*timestamp = *(double*)(_buf->data + _position);
	*size = *(uint32_t*)(_buf->data + _position + sizeof(double));
	*data = _buf->data + _position + sizeof(double) + sizeof(uint32_t);

	return *timestamp;
}


/** Clear, and merge \a a and \a b into this buffer.
 *
 * FIXME: This is slow.
 *
 * \return true if complete merge was successful
 */
bool
MidiBuffer::merge(const MidiBuffer& a, const MidiBuffer& b)
{
	// Die if a merge isn't necessary as it's expensive
	assert(a.size() > 0 && b.size() > 0);

	reset(_this_nframes);

	a.rewind();
	b.rewind();

	double         a_time;
	uint32_t       a_size;
	unsigned char* a_data;
	
	double         b_time;
	uint32_t       b_size;
	unsigned char* b_data;

	a.get_event(&a_time, &a_size, &a_data);
	b.get_event(&b_time, &b_size, &b_data);

	while (true) {
		if (a_data && (!b_data || (a_time < b_time))) {
			append(a_time, a_size, a_data);
			if (a.increment())
				a.get_event(&a_time, &a_size, &a_data);
			else
				a_data = NULL;
		} else if (b_data) {
			append(b_time, b_size, b_data);
			if (b.increment())
				b.get_event(&b_time, &b_size, &b_data);
			else
				b_data = NULL;
		} else {
			break;
		}
	}

	_latest_stamp = max(a_time, b_time);

	return true;
}


} // namespace Ingen

