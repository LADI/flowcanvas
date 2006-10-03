
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

#ifndef DIRECTRESPONDER_H
#define DIRECTRESPONDER_H

#include "util/CountedPtr.h"
#include "interface/ClientInterface.h"
#include "Responder.h"

namespace Ingen {


/** Responder for Direct clients (directly calls methods on a ClientInterface).
 */
class DirectResponder : public Responder
{
public:
	DirectResponder(CountedPtr<ClientInterface> client, int32_t id)
	: _client(client), _id(id)
	{}

	void set_id(int32_t id) { _id = id; }

	void respond_ok()                     { _client->response(_id, true, ""); }
	void respond_error(const string& msg) { _client->response(_id, false, msg); }

	CountedPtr<ClientInterface> client() { return _client; }

private:
	CountedPtr<ClientInterface> _client;
	int32_t                     _id;
};


} // namespace Ingen

#endif // DIRECTRESPONDER_H
