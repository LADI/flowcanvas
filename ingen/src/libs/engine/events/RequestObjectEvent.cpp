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

#include "RequestObjectEvent.h"
#include <string>
#include "Responder.h"
#include "Engine.h"
#include "interface/ClientInterface.h"
#include "TypedPort.h"
#include "ObjectStore.h"
#include "ClientBroadcaster.h"
#include "Patch.h"
#include "Node.h"
#include "ObjectSender.h"

using std::string;

namespace Ingen {


RequestObjectEvent::RequestObjectEvent(Engine& engine, CountedPtr<Responder> responder, SampleCount timestamp, const string& path)
: QueuedEvent(engine, responder, timestamp),
  m_path(path),
  m_object(NULL)
{
}


void
RequestObjectEvent::pre_process()
{
	m_client = _engine.broadcaster()->client(_responder->client_key());
	m_object = _engine.object_store()->find(m_path);

	QueuedEvent::pre_process();
}


void
RequestObjectEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);
	assert(_time >= start && _time <= end);
}


void
RequestObjectEvent::post_process()
{
	if (!m_object) {
		_responder->respond_error("Unable to find object requested.");
	
	} else if (m_client) {	
		Patch* const patch = dynamic_cast<Patch*>(m_object);
		if (patch) {
			_responder->respond_ok();
			ObjectSender::send_patch(m_client.get(), patch, true);
			return;
		}
		
		Node* const node = dynamic_cast<Node*>(m_object);
		if (node) {
			_responder->respond_ok();
			ObjectSender::send_node(m_client.get(), node, true);
			return;
		}
		
		Port* const port = dynamic_cast<Port*>(m_object);
		if (port) {
			_responder->respond_ok();
			ObjectSender::send_port(m_client.get(), port);
			return;
		}
		
		_responder->respond_error("Object of unknown type requested.");

	} else {
		_responder->respond_error("Unable to find client to send object.");
	}
}


} // namespace Ingen
