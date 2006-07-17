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

#ifndef OMAPP_H
#define OMAPP_H

template<typename T> class Queue;
class Maid;

namespace Om {

class AudioDriver;
class MidiDriver;
class NodeFactory;
class OSCReceiver;
class ClientBroadcaster;
class Patch;
class ObjectStore;
class EventSource;
class PostProcessor;
class Event;
class QueuedEvent;
class LashDriver;
template <typename T> class Driver;


/** The main class for Om, the whole app lives in here
 *
 * This class should not exist.
 *
 * \ingroup engine
 */
class OmApp
{
public:
	OmApp(const char* const port, AudioDriver* audio_driver = 0);
	~OmApp();
	
	int main();
	
	/** Set the quit flag that should kill all threads and exit cleanly.
	 * Note that it will take some time. */
	void quit() { m_quit_flag = true; }
	
	void activate();
	void deactivate();


	AudioDriver*       audio_driver()       const { return m_audio_driver; }
	OSCReceiver*       osc_receiver()       const { return m_osc_receiver; }
	MidiDriver*        midi_driver()        const { return m_midi_driver; }
	PostProcessor*     post_processor()     const { return m_post_processor; }
	Maid*              maid()               const { return m_maid; }
	ClientBroadcaster* client_broadcaster() const { return m_client_broadcaster; }
	ObjectStore*       object_store()       const { return m_object_store; }
	NodeFactory*       node_factory()       const { return m_node_factory; }
	LashDriver*        lash_driver()        const { return m_lash_driver; }


	/** Return the active driver for the given (template parameter) type.
	 * This is a hook for BridgeNode.  See OmApp.cpp for specializations. */
	template<typename T> Driver<T>* driver();
	
private:
	// Prevent copies
	OmApp(const OmApp&);
	OmApp& operator=(const OmApp&);
	

	AudioDriver*       m_audio_driver;
	OSCReceiver*       m_osc_receiver;
	MidiDriver*        m_midi_driver;
	PostProcessor*     m_post_processor;
	Maid*              m_maid;
	ClientBroadcaster* m_client_broadcaster;
	ObjectStore*       m_object_store;
	NodeFactory*       m_node_factory;
	LashDriver*        m_lash_driver;
	
	bool m_quit_flag;
	bool m_activated;
};


} // namespace Om

#endif // OMAPP_H