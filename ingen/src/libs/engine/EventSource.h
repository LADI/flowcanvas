/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#ifndef EVENTSOURCE_H
#define EVENTSOURCE_H

namespace Om {

class Event;
class QueuedEvent;


/** Source for events to run in the audio thread.
 *
 * The AudioDriver gets events from an EventSource in the process callback
 * (realtime audio thread) and executes them, then they are sent to the
 * PostProcessor and finalised (post-processing thread).
 *
 * There are two distinct classes of events - "queued" and "stamped".  Queued
 * events are events that require non-realtime pre-processing before being
 * executed in the process thread.  Stamped events are timestamped realtime
 * events that require no pre-processing and can be executed immediately
 * (with sample accuracy).
 */
class EventSource
{
public:

	virtual ~EventSource() {}

	virtual Event* pop_earliest_queued_before(const samplecount time) = 0;
	
	virtual Event* pop_earliest_stamped_before(const samplecount time) = 0;

protected:
	EventSource() {}
};


} // namespace Om

#endif // EVENTSOURCE_H
