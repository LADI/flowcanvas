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

#include "Engine.h"	
#include "config.h"
#include "tuning.h"
#include <sys/mman.h>
#include <iostream>
#include <unistd.h>
#include "Event.h"
#include "util/Queue.h"
#include "JackAudioDriver.h"
#include "NodeFactory.h"
#include "OSCReceiver.h"
#include "ClientBroadcaster.h"
#include "Patch.h"
#include "ObjectStore.h"
#include "MaidObject.h"
#include "Maid.h"
#include "MidiDriver.h"
#include "QueuedEventSource.h"
#include "PostProcessor.h"
#include "CreatePatchEvent.h"
#include "EnablePatchEvent.h"
#ifdef HAVE_JACK_MIDI
#include "JackMidiDriver.h"
#endif
#ifdef HAVE_ALSA_MIDI
#include "AlsaMidiDriver.h"
#endif
#ifdef HAVE_LASH
#include "LashDriver.h"
#endif
using std::cout; using std::cerr; using std::endl;

namespace Ingen {


Engine* Engine::m_instance = NULL;


void
Engine::instantiate(const char* port, AudioDriver* audio_driver)
{
	assert(!m_instance);
	m_instance = new Engine(port, audio_driver);
}


Engine::Engine(const char* port, AudioDriver* audio_driver)
: m_audio_driver( (audio_driver) ? audio_driver : new JackAudioDriver() ),
  m_osc_receiver(new OSCReceiver(pre_processor_queue_size, port)),
#ifdef HAVE_JACK_MIDI
  m_midi_driver(new JackMidiDriver(((JackAudioDriver*)m_audio_driver)->jack_client())),
#elif HAVE_ALSA_MIDI
  m_midi_driver(new AlsaMidiDriver()),
#else
  m_midi_driver(new DummyMidiDriver()),
#endif
  m_post_processor(new PostProcessor(post_processor_queue_size)),
  m_maid(new Maid(maid_queue_size)),
  m_client_broadcaster(new ClientBroadcaster()),
  m_object_store(new ObjectStore()),
  m_node_factory(new NodeFactory()),
#ifdef HAVE_LASH
  m_lash_driver(new LashDriver()),
#else 
  m_lash_driver(NULL),
#endif
  m_quit_flag(false),
  m_activated(false)
{
	m_osc_receiver->activate();
}


Engine::~Engine()
{
	deactivate();

	for (Tree<GraphObject*>::iterator i = m_object_store->objects().begin();
			i != m_object_store->objects().end(); ++i) {
		if ((*i)->parent() == NULL)
			delete (*i);
	}
	
	delete m_object_store;
	delete m_client_broadcaster;
	delete m_osc_receiver;
	delete m_node_factory;
	delete m_midi_driver;
	delete m_audio_driver;
	
	delete m_maid;

	munlockall();
}


/* driver() template specializations.
 * Due to the lack of RTTI, this needs to be implemented manually like this.
 * If more types/drivers start getting added, it may be worth it to enable
 * RTTI and put all the drivers into a map with typeid's as the key.  That's
 * more elegant and extensible, but this is faster and simpler - for now.
 */
template<>
Driver<MidiMessage>* Engine::driver<MidiMessage>() { return m_midi_driver; }
template<>
Driver<Sample>* Engine::driver<Sample>() { return m_audio_driver; }


int
Engine::main()
{
	// Loop until quit flag is set (by OSCReceiver)
	while ( ! m_quit_flag) {
		nanosleep(&main_rate, NULL);
#ifdef HAVE_LASH
		// Process any pending LASH events
		if (lash_driver->enabled())
			lash_driver->process_events();
#endif
		// Run the maid (garbage collector)
		m_maid->cleanup();
	}
	cout << "[Main] Done main loop." << endl;
	
	if (m_activated)
		deactivate();

	sleep(1);
	cout << "[Main] Exiting..." << endl;
	
	return 0;
}


void
Engine::activate()
{
	if (m_activated)
		return;
	
	// Create root patch
	CreatePatchEvent create_ev(CountedPtr<Responder>(new Responder()), 0, "/", 1);
	create_ev.pre_process();
	create_ev.execute(0);
	create_ev.post_process();
	EnablePatchEvent enable_ev(CountedPtr<Responder>(new Responder()), 0, "/");
	enable_ev.pre_process();
	enable_ev.execute(0);
	enable_ev.post_process();

	assert(m_audio_driver->root_patch() != NULL);

	m_audio_driver->activate();
#ifdef HAVE_ALSA_MIDI
	m_midi_driver->activate();
#endif
	
	m_post_processor->start();

	m_activated = true;
}


void
Engine::deactivate()
{
	if (!m_activated)
		return;
	
	m_audio_driver->root_patch()->process(false);
	m_audio_driver->root_patch()->deactivate();

	/*for (Tree<GraphObject*>::iterator i = m_object_store->objects().begin();
			i != m_object_store->objects().end(); ++i)
		if ((*i)->as_node() != NULL && (*i)->as_node()->parent() == NULL)
			(*i)->as_node()->deactivate();*/
	
	if (m_midi_driver != NULL)
		m_midi_driver->deactivate();
	
	m_osc_receiver->deactivate();
	m_audio_driver->deactivate();

	// Finalize any lingering events (unlikely)
	m_post_processor->whip();
	m_post_processor->stop();

	m_activated = false;
}


} // namespace Ingen