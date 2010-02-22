/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include "raul/Array.hpp"
#include "raul/Atom.hpp"
#include "raul/List.hpp"
#include "raul/Maid.hpp"
#include "raul/Path.hpp"
#include "shared/LV2URIMap.hpp"
#include "ClientBroadcaster.hpp"
#include "ControlBindings.hpp"
#include "CreatePort.hpp"
#include "Driver.hpp"
#include "DuplexPort.hpp"
#include "Engine.hpp"
#include "EngineStore.hpp"
#include "PatchImpl.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "Request.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Events {

using namespace Shared;


CreatePort::CreatePort(
		Engine&                     engine,
		SharedPtr<Request>          request,
		SampleCount                 timestamp,
		const Raul::Path&           path,
		const Raul::URI&            type,
		bool                        is_output,
		const Resource::Properties& properties)
	: QueuedEvent(engine, request, timestamp, bool(request))
	, _path(path)
	, _type(type)
	, _is_output(is_output)
	, _data_type(type)
	, _patch(NULL)
	, _patch_port(NULL)
	, _driver_port(NULL)
	, _properties(properties)
{
	/* This is blocking because of the two different sets of Patch ports, the array used in the
	 * audio thread (inherited from NodeBase), and the arrays used in the pre processor thread.
	 * If two add port events arrive in the same cycle and the second pre processes before the
	 * first executes, bad things happen (ports are lost).
	 *
	 * TODO: fix this using RCU?
	 */

	if (_data_type == PortType::UNKNOWN)
		_error = UNKNOWN_TYPE;
}


void
CreatePort::pre_process()
{
	if (_error == UNKNOWN_TYPE || _engine.engine_store()->find_object(_path)) {
		QueuedEvent::pre_process();
		return;
	}

	_patch = _engine.engine_store()->find_patch(_path.parent());

	const LV2URIMap& uris = *_engine.world()->uris.get();

	if (_patch != NULL) {
		assert(_patch->path() == _path.parent());

		size_t buffer_size = _engine.buffer_factory()->default_buffer_size(_data_type);

		const uint32_t old_num_ports = (_patch->external_ports())
			? _patch->external_ports()->size()
			: 0;

		Shared::Resource::Properties::const_iterator index_i = _properties.find(uris.lv2_index);
		if (index_i->second.type() != Atom::INT
				|| index_i->second.get_int32() != static_cast<int32_t>(old_num_ports)) {
			QueuedEvent::pre_process();
			_error = BAD_INDEX;
			return;
		}

		Shared::Resource::Properties::const_iterator poly_i = _properties.find(uris.ingen_polyphonic);
		bool polyphonic = (poly_i != _properties.end() && poly_i->second.type() == Atom::BOOL
				&& poly_i->second.get_bool());

		_patch_port = _patch->create_port(*_engine.buffer_factory(), _path.symbol(), _data_type, buffer_size, _is_output, polyphonic);
		if (_patch->parent())
			_patch_port->set_property(uris.rdf_instanceOf, _patch_port->meta_uri());

		_patch_port->properties().insert(_properties.begin(), _properties.end());
		_patch_port->meta().properties().insert(_properties.begin(), _properties.end());

		assert(index_i->second == Atom((int)_patch_port->index()));

		if (_patch_port) {

			if (_is_output)
				_patch->add_output(new Raul::List<PortImpl*>::Node(_patch_port));
			else
				_patch->add_input(new Raul::List<PortImpl*>::Node(_patch_port));

			if (_patch->external_ports())
				_ports_array = new Raul::Array<PortImpl*>(old_num_ports + 1, *_patch->external_ports(), NULL);
			else
				_ports_array = new Raul::Array<PortImpl*>(old_num_ports + 1, NULL);

			_ports_array->at(old_num_ports) = _patch_port;
			_engine.engine_store()->add(_patch_port);

			if (!_patch->parent())
				_driver_port = _engine.driver()->create_port(
						dynamic_cast<DuplexPort*>(_patch_port));

			assert(_ports_array->size() == _patch->num_ports());

		} else {
			_error = CREATION_FAILED;
		}
	}
	QueuedEvent::pre_process();
}


void
CreatePort::execute(ProcessContext& context)
{
	QueuedEvent::execute(context);

	if (_patch_port) {
		_engine.maid()->push(_patch->external_ports());
		_patch->external_ports(_ports_array);
		_engine.control_bindings()->port_binding_changed(context, _patch_port);
	}

	if (_driver_port) {
		_engine.driver()->add_port(_driver_port);
	}

	if (_request)
		_request->unblock();
}


void
CreatePort::post_process()
{
	if (!_request)
		return;

	string msg;
	switch (_error) {
	case NO_ERROR:
		_request->respond_ok();
		_engine.broadcaster()->send_object(_patch_port, true);
		break;
	case BAD_INDEX:
		msg = string("Could not create port ") + _path.str() + " (Illegal index given)";
		_request->respond_error(msg);
		break;
	case UNKNOWN_TYPE:
		msg = string("Could not create port ") + _path.str() + " (Unknown type)";
		_request->respond_error(msg);
		break;
	case CREATION_FAILED:
		msg = string("Could not create port ") + _path.str() + " (Creation failed)";
		_request->respond_error(msg);
		break;
	}
}


} // namespace Ingen
} // namespace Events

