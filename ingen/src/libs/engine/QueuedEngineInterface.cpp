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

#include "QueuedEngineInterface.h"
#include "QueuedEventSource.h"
#include "events.h"
#include "Om.h"
#include "util/Queue.h"
#include "OmApp.h"
#include "AudioDriver.h"

namespace Om {

QueuedEngineInterface::QueuedEngineInterface(size_t queued_size, size_t stamped_size)
: QueuedEventSource(queued_size, stamped_size)
, _responder(CountedPtr<Responder>(new Responder())) // NULL responder
{
}


/** Set the Responder to send responses to commands with, once the commands
 * are preprocessed and ready to be executed (or not).
 *
 * Ownership of @a responder is taken.
 */
void
QueuedEngineInterface::set_responder(CountedPtr<Responder> responder)
{
	_responder = responder;
}


void
QueuedEngineInterface::disable_responses()
{
	static CountedPtr<Responder> null_responder(new Responder());
	//cerr << "DISABLE\n";
	set_responder(null_responder);
}


/* *** EngineInterface implementation below here *** */


void
QueuedEngineInterface::register_client(ClientKey key, CountedPtr<ClientInterface> client)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new RegisterClientEvent(_responder, timestamp, key, client));
}


void
QueuedEngineInterface::unregister_client(ClientKey key)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new UnregisterClientEvent(_responder, timestamp, key));
}



// Engine commands
void
QueuedEngineInterface::load_plugins()
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new LoadPluginsEvent(_responder, timestamp));

}


void
QueuedEngineInterface::activate()    
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new ActivateEvent(_responder, timestamp));
}


void
QueuedEngineInterface::deactivate()  
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new DeactivateEvent(_responder, timestamp));
}


void
QueuedEngineInterface::quit()        
{
	_responder->respond_ok();
	om->quit();
}


		
// Object commands

void
QueuedEngineInterface::create_patch(const string& path,
                                    uint32_t      poly)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new CreatePatchEvent(_responder, timestamp, path, poly));

}


void QueuedEngineInterface::create_port(const string& path,
                                        const string& data_type,
                                        bool          direction)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new AddPortEvent(_responder, timestamp, path, data_type, direction));
}


void
QueuedEngineInterface::create_node(const string& path,
                                   const string& plugin_type,
                                   const string& plugin_uri,
                                   bool          polyphonic)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	// FIXME: ew
	
	Plugin* plugin = new Plugin();
	plugin->set_type(plugin_type);
	plugin->uri(plugin_uri);

	push_queued(new AddNodeEvent(_responder, timestamp, path, plugin, polyphonic));
}


void
QueuedEngineInterface::create_node(const string& path,
                                   const string& plugin_type,
                                   const string& plugin_lib,
                                   const string& plugin_label,
                                   bool          polyphonic)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	// FIXME: ew
	
	Plugin* plugin = new Plugin();
	plugin->set_type(plugin_type);
	plugin->lib_name(plugin_lib);
	plugin->plug_label(plugin_label);

	push_queued(new AddNodeEvent(_responder, timestamp, path, plugin, polyphonic));
}

void
QueuedEngineInterface::rename(const string& old_path,
                              const string& new_name)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new RenameEvent(_responder, timestamp, old_path, new_name));
}


void
QueuedEngineInterface::destroy(const string& path)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new DestroyEvent(_responder, timestamp, this, path));
}


void
QueuedEngineInterface::clear_patch(const string& patch_path)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new ClearPatchEvent(_responder, timestamp, patch_path));
}


void
QueuedEngineInterface::enable_patch(const string& patch_path)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new EnablePatchEvent(_responder, timestamp, patch_path));
}


void
QueuedEngineInterface::disable_patch(const string& patch_path)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new DisablePatchEvent(_responder, timestamp, patch_path));
}


void
QueuedEngineInterface::connect(const string& src_port_path,
                               const string& dst_port_path)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new ConnectionEvent(_responder, timestamp, src_port_path, dst_port_path));

}


void
QueuedEngineInterface::disconnect(const string& src_port_path,
                                  const string& dst_port_path)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new DisconnectionEvent(_responder, timestamp, src_port_path, dst_port_path));
}


void
QueuedEngineInterface::disconnect_all(const string& node_path)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new DisconnectNodeEvent(_responder, timestamp, node_path));
}


void
QueuedEngineInterface::set_port_value(const string& port_path,
                                      float         value)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_stamped(new SetPortValueEvent(_responder, timestamp, port_path, value));
}


void
QueuedEngineInterface::set_port_value(const string& port_path,
                                      uint32_t      voice,
                                      float         value)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_stamped(new SetPortValueEvent(_responder, timestamp, voice, port_path, value));
}


void
QueuedEngineInterface::set_port_value_queued(const string& port_path,
                                             float         value)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new SetPortValueQueuedEvent(_responder, timestamp, port_path, value));
}


void
QueuedEngineInterface::set_program(const string& node_path,
                                   uint32_t      bank,
                                   uint32_t      program)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new DSSIProgramEvent(_responder, timestamp, node_path, bank, program));
}


void
QueuedEngineInterface::midi_learn(const string& node_path)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new MidiLearnEvent(_responder, timestamp, node_path));
}


void
QueuedEngineInterface::set_metadata(const string& path,
                                    const string& predicate,
                                    const string& value)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new SetMetadataEvent(_responder, timestamp, path, predicate, value));
}


// Requests //

void
QueuedEngineInterface::ping()
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new PingQueuedEvent(_responder, timestamp));
}


void
QueuedEngineInterface::request_port_value(const string& port_path)
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new RequestPortValueEvent(_responder, timestamp, port_path));
}


void
QueuedEngineInterface::request_plugins()
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new RequestPluginsEvent(_responder, timestamp));
}


void
QueuedEngineInterface::request_all_objects()
{
	const samplecount timestamp = om->audio_driver()->time_stamp();
	push_queued(new RequestAllObjectsEvent(_responder, timestamp));
}


} // namespace Om

