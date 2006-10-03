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

#include "RequestPluginEvent.h"
#include <string>
#include "Responder.h"
#include "Engine.h"
#include "interface/ClientInterface.h"
#include "TypedPort.h"
#include "ObjectStore.h"
#include "ClientBroadcaster.h"
#include "NodeFactory.h"
#include "Plugin.h"

using std::string;

namespace Ingen {


RequestPluginEvent::RequestPluginEvent(Engine& engine, CountedPtr<Responder> responder, SampleCount timestamp, const string& uri)
: QueuedEvent(engine, responder, timestamp),
  m_uri(uri),
  m_plugin(NULL)
{
}


void
RequestPluginEvent::pre_process()
{
	m_client = _engine.broadcaster()->client(_responder->client_key());
	m_plugin = _engine.node_factory()->plugin(m_uri);

	QueuedEvent::pre_process();
}


void
RequestPluginEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);
	assert(_time >= start && _time <= end);
}


void
RequestPluginEvent::post_process()
{
	if (!m_plugin) {
		_responder->respond_error("Unable to find plugin requested.");
	
	} else if (m_client) {

		_responder->respond_ok();
		assert(m_plugin->uri() == m_uri);
		m_client->new_plugin(m_uri, m_plugin->name());

	} else {
		_responder->respond_error("Unable to find client to send plugin.");
	}
}


} // namespace Ingen
