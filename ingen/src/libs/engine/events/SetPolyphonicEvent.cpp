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

#include <raul/Maid.hpp>
#include "SetPolyphonicEvent.hpp"
#include "Responder.hpp"
#include "Engine.hpp"
#include "Patch.hpp"
#include "ClientBroadcaster.hpp"
#include "util.hpp"
#include "ObjectStore.hpp"
#include "Port.hpp"
#include "Node.hpp"
#include "Connection.hpp"
#include "QueuedEventSource.hpp"

namespace Ingen {


SetPolyphonicEvent::SetPolyphonicEvent(Engine& engine, SharedPtr<Responder> responder, FrameTime time, QueuedEventSource* source, const string& patch_path, bool poly)
: QueuedEvent(engine, responder, time, true, source),
  _patch_path(patch_path),
  _patch(NULL),
  _poly(poly)
{
}


void
SetPolyphonicEvent::pre_process()
{
	/*
	_patch = _engine.object_store()->find_patch(_patch_path);
	 if (_patch && _poly > _patch->internal_poly())
		 _patch->prepare_internal_poly(_poly);
*/
	QueuedEvent::pre_process();
}


void
SetPolyphonicEvent::execute(SampleCount nframes, FrameTime start, FrameTime end)
{
	QueuedEvent::execute(nframes, start, end);
/*
	if (_patch)
		_patch->apply_internal_poly(*_engine.maid(), _poly);
*/	
	_source->unblock();
}


void
SetPolyphonicEvent::post_process()
{
	/*
	if (_patch)
		_responder->respond_ok();
	else
		_responder->respond_error("Unable to find patch");
		*/
}


} // namespace Ingen
