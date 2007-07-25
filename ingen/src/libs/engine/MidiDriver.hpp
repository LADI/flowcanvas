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

#ifndef MIDIDRIVER_H
#define MIDIDRIVER_H

#include "types.hpp"
#include "Driver.hpp"
#include <iostream>
#include "MidiBuffer.hpp"

namespace Ingen {


/** Midi driver abstract base class.
 *
 * \ingroup engine
 */
class MidiDriver : public Driver
{
public:
	MidiDriver() : Driver(DataType::MIDI) {}

	/** Prepare events (however neccessary) for the specified block (realtime safe) */
	virtual void prepare_block(const SampleCount block_start, const SampleCount block_end) = 0;
};



/** Dummy MIDIDriver.
 *
 * Not abstract, all functions are dummies.  One of these will be allocated and
 * "used" if no working MIDI driver is loaded.  (Doing it this way as opposed to
 * just making MidiDriver have dummy functions makes sure any existing MidiDriver
 * derived class actually implements the required functions).
 *
 * \ingroup engine
 */
class DummyMidiDriver : public MidiDriver
{
public:
	DummyMidiDriver() {
		std::cout << "[DummyMidiDriver] Started Dummy MIDI driver." << std::endl;
	}
	
	~DummyMidiDriver() {}

	void activate()   {}
	void deactivate() {}
	
	bool is_activated() const { return false; }
	bool is_enabled()   const { return false; }
	
	void enable()  {}
	void disable() {}
	
	DriverPort* create_port(DuplexPort* patch_port) { return NULL; }
	
	void        add_port(DriverPort* port)    {}
	DriverPort* remove_port(const Raul::Path& path) { return NULL; }
	
	void prepare_block(const SampleCount block_start, const SampleCount block_end) {}
};



} // namespace Ingen

#endif // MIDIDRIVER_H