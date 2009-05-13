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

#include <cassert>
#include <iostream>
#include <unistd.h>
#include "raul/AtomLiblo.hpp"
#include "interface/ClientInterface.hpp"
#include "EngineStore.hpp"
#include "NodeImpl.hpp"
#include "OSCClientSender.hpp"
#include "PatchImpl.hpp"
#include "PluginImpl.hpp"
#include "PortImpl.hpp"
#include "util.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {


/*! \page client_osc_namespace Client OSC Namespace Documentation
 *
 * <p>NOTE: this comment doesn't really apply any longer.. sort of.
 * but maybe it still should.. maybe.  so it remains...</p>
 *
 * <p>These are all the messages sent from the engine to the client.
 * Communication takes place over two distinct bands: control band and
 * notification band.</p>
 * <p>The control band is where clients send commands, and receive a simple
 * response, either OK or an error.</p>
 * <p>All notifications of engine state (ie new nodes) are sent over the
 * notification band <em>which is seperate from the control band</em>.  The
 * reasoning behind this is that many clients may be connected at the same
 * time - a client may receive notifications that are not a direct consequence
 * of some message it sent.</p>
 * <p>The notification band can be thought of as a stream of events representing
 * the changing engine state.  For example, It is possible for a client to send
 * commands and receive aknowledgements, and not listen to the notification band
 * at all; or (in the near future anyway) for a client to use UDP for the control
 * band (for speed), and TCP for the notification band (for reliability and
 * order guarantees).</p>
 * \n\n
 */


/** \page client_osc_namespace
 * \n
 * <h2>Control Band</h2>
 */

/** \page client_osc_namespace
 * <p> \b /ingen/ok - Respond to a successful user command
 * \arg \b response-id (int) - Request ID this is a response to
 * </p> \n \n
 */
void
OSCClientSender::response_ok(int32_t id)
{
	if (!_enabled)
		return;

	if (lo_send(_address, "/ingen/ok", "i", id) < 0) {
		cerr << "Unable to send ok " << id << "! ("
			<< lo_address_errstr(_address) << ")" << endl;
	}
}


/** \page client_osc_namespace
 * <p> \b /ingen/response - Respond to a user command
 * \arg \b response-id (int) - Request ID this is a response to
 * \arg \b message (string) - Error message (natural language text)
 * </p> \n \n
 */
void
OSCClientSender::response_error(int32_t id, const std::string& msg)
{
	if (!_enabled)
		return;

	if (lo_send(_address, "/ingen/error", "is", id, msg.c_str()) < 0) {
		cerr << "Unable to send error " << id << "! ("
			<< lo_address_errstr(_address) << ")" << endl;
	}
}


/** \page client_osc_namespace
 * \n
 * <h2>Notification Band</h2>
 */


/** \page client_osc_namespace
 * <p> \b /ingen/error - Notification that an error has occurred
 * \arg \b message (string) - Error message (natural language text) \n\n
 * \li This is for notification of errors that aren't a direct response to a
 * user command, ie "unexpected" errors.</p> \n \n
 */
void
OSCClientSender::error(const std::string& msg)
{
	send("/ingen/error", "s", msg.c_str(), LO_ARGS_END);
}


/** \page client_osc_namespace
 * <p> \b /ingen/new_node - Notification of a new node's creation.
 * \arg \b plug-uri (string) - URI of the plugin new node is an instance of
 * \arg \b path (string) - Path of the new node
 * \arg \b polyphonic (boolean) - Node is polyphonic\n\n
 * \li New nodes are sent as a bundle.  The first message in the bundle will be
 * this one (/ingen/new_node), followed by a series of /ingen/new_port commands,
 * followed by /ingen/new_node_end. </p> \n \n
 */
void OSCClientSender::new_node(const Path& node_path,
                               const URI&  plugin_uri)
{
	send("/ingen/new_node", "ss", node_path.c_str(), plugin_uri.c_str(), LO_ARGS_END);
}



/** \page client_osc_namespace
 * <p> \b /ingen/new_port - Notification of a new port's creation.
 * \arg \b path (string) - Path of new port
 * \arg \b index (integer) - Index (or sort key) of port on parent
 * \arg \b data-type (string) - Type of port (ingen:AudioPort, ingen:ControlPort, ingen:EventPort)
 * \arg \b direction ("is-output") (integer) - Direction of data flow (Input = 0, Output = 1)
 *
 * \li Note that in the event of loading a patch, this message could be
 * followed immediately by a control change, meaning the default-value is
 * not actually the current value of the port.
 * \li The minimum and maximum values are suggestions only, they are not
 * enforced in any way, and going outside them is perfectly fine.  Also note
 * that the port ranges in om_gtk are not these ones!  Those ranges are set
 * as variable.</p> \n \n
 */
void
OSCClientSender::new_port(const Path& path,
                          const URI&  type,
                          uint32_t          index,
                          bool              is_output)
{
	send("/ingen/new_port", "sisi", path.c_str(), index, type.c_str(), is_output, LO_ARGS_END);
}


/** \page client_osc_namespace
 * <p> \b /ingen/destroyed - Notification an object has been destroyed
 * \arg \b path (string) - Path of object (which no longer exists) </p> \n \n
 */
void
OSCClientSender::destroy(const Path& path)
{
	send("/ingen/destroyed", "s", path.c_str(), LO_ARGS_END);
}


/** \page client_osc_namespace
 * <p> \b /ingen/clear_patch - Notification a patch has been cleared (all children destroyed)
 * \arg \b path (string) - Path of patch (which is now empty)</p> \n \n
 */
void
OSCClientSender::clear_patch(const Path& patch_path)
{
	send("/ingen/clear_patch", "s", patch_path.c_str(), LO_ARGS_END);
}


/** \page client_osc_namespace
 * <p> \b /ingen/new_connection - Notification a new connection has been made.
 * \arg \b src-path (string) - Path of the source port
 * \arg \b dst-path (string) - Path of the destination port</p> \n \n
 */
void
OSCClientSender::connect(const Path& src_port_path, const Path& dst_port_path)
{
	send("/ingen/new_connection", "ss", src_port_path.c_str(), dst_port_path.c_str(), LO_ARGS_END);
}


/** \page client_osc_namespace
 * <p> \b /ingen/disconnection - Notification a connection has been unmade.
 * \arg \b src-path (string) - Path of the source port
 * \arg \b dst-path (string) - Path of the destination port</p> \n \n
 */
void
OSCClientSender::disconnect(const Path& src_port_path, const Path& dst_port_path)
{
	send("/ingen/disconnection", "ss", src_port_path.c_str(), dst_port_path.c_str(), LO_ARGS_END);
}


/** \page client_osc_namespace
 * <p> \b /ingen/set_variable - Notification of a variable.
 * \arg \b path (string) - Path of the object associated with variable (node, patch, or port)
 * \arg \b key (string)
 * \arg \b value (string)</p> \n \n
 */
void
OSCClientSender::set_variable(const URI& path, const URI& key, const Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_string(m, path.c_str());
	lo_message_add_string(m, key.c_str());
	AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_variable", m);
}


/** \page client_osc_namespace
 * <p> \b /ingen/set_property - Notification of a property.
 * \arg \b path (string) - Path of the object associated with property (node, patch, or port)
 * \arg \b key (string)
 * \arg \b value (string)</p> \n \n
 */
void
OSCClientSender::set_property(const URI& path, const URI& key, const Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_string(m, path.c_str());
	lo_message_add_string(m, key.c_str());
	AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_property", m);
}


/** \page client_osc_namespace
 * <p> \b /ingen/set_port_value - Notification the value of a port has changed
 * \arg \b path (string) - Path of port
 * \arg \b value (any) - New value of port </p> \n \n
 */
void
OSCClientSender::set_port_value(const Path& port_path, const Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_string(m, port_path.c_str());
	AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_port_value", m);
}


/** \page client_osc_namespace
 * <p> \b /ingen/set_port_value - Notification the value of a port has changed
 * \arg \b path (string) - Path of port
 * \arg \b voice (int) - Voice which is set to this value
 * \arg \b value (any) - New value of port </p> \n \n
 */
void
OSCClientSender::set_voice_value(const Path& port_path, uint32_t voice, const Atom& value)
{
	lo_message m = lo_message_new();
	lo_message_add_string(m, port_path.c_str());
	AtomLiblo::lo_message_add_atom(m, value);
	send_message("/ingen/set_port_value", m);
}


/** \page client_osc_namespace
 * <p> \b /ingen/activity - Notification of "activity" (e.g. port message blinkenlights)
 * \arg \b path (string) - Path of object </p> \n \n
 */
void
OSCClientSender::activity(const Path& path)
{
	if (!_enabled)
		return;

	lo_send(_address, "/ingen/activity", "s", path.c_str(), LO_ARGS_END);
}


/** \page client_osc_namespace
 * <p> \b /ingen/plugin - Notification of the existance of a plugin
 * \arg \b uri (string) - URI of plugin (e.g. http://example.org/filtermatic)
 * \arg \b type (string) - Type of plugin (e.g. "lv2:Plugin", "ingen:LADSPAPlugin")
 * \arg \b symbol (string) - Valid symbol for plugin (default symbol for nodes) (e.g. "adsr")
 * \arg \b name (string) - Descriptive human-readable name of plugin (e.g. "ADSR Envelope")
 */
void
OSCClientSender::new_plugin(const URI&    uri,
                            const URI&    type_uri,
                            const Symbol& symbol)
{
	lo_message m = lo_message_new();
	lo_message_add_string(m, uri.c_str());
	lo_message_add_string(m, type_uri.c_str());
	lo_message_add_string(m, symbol.c_str());
	send_message("/ingen/plugin", m);
}


bool
OSCClientSender::new_object(const Shared::GraphObject* object)
{
#if 0
	using namespace Shared;

	const Patch* patch = dynamic_cast<const Patch*>(object);
	if (patch) {
		new_patch(patch->path(), patch->internal_polyphony());
		return true;
	}

	const Node* node = dynamic_cast<const Node*>(object);
	if (node) {
		new_node(node->path(), node->plugin()->uri());
		return true;
	}

	const Port* port = dynamic_cast<const Port*>(object);
	if (port) {
		new_port(port->path(), port->type().uri(), port->index(), !port->is_input());
		return true;
	}
#endif

	return false;
}


/** \page client_osc_namespace
 * <p> \b /ingen/new_patch - Notification of a new patch
 * \arg \b path (string) - Path of new patch
 * \arg \b poly (int) - Polyphony of new patch (\em not a boolean like new_node) </p> \n \n
 */
void
OSCClientSender::new_patch(const Path& path, uint32_t poly)
{
	send("/ingen/new_patch", "si", path.c_str(), poly, LO_ARGS_END);
}


/** \page client_osc_namespace
 * <p> \b /ingen/rename - Notification of an object's renaming
 * \arg \b old-path (string) - Old path of object
 * \arg \b new-path (string) - New path of object </p> \n \n
 */
void
OSCClientSender::rename(const Path& old_path, const Path& new_path)
{
	send("/ingen/rename", "ss", old_path.c_str(), new_path.c_str(), LO_ARGS_END);
}


/** Sends information about a program associated with a node.
 */
void
OSCClientSender::program_add(const Path& node_path, uint32_t bank, uint32_t program, const std::string& name)
{
	send("/ingen/program_add", "siis",
		node_path.c_str(), bank, program, name.c_str(), LO_ARGS_END);
}


void
OSCClientSender::program_remove(const Path& node_path, uint32_t bank, uint32_t program)
{
	send("/ingen/program_remove", "sii",
		node_path.c_str(), bank, program, LO_ARGS_END);
}


} // namespace Ingen
