/* This file is part of Patchage.
 * Copyright (C) 2008-2009 David Robillard <http://drobilla.net>
 *
 * Patchage is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Patchage is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PATCHAGE_PORTID_HPP
#define PATCHAGE_PORTID_HPP

#include <cstring>
#include <iostream>

#include "patchage-config.h"

#ifdef PATCHAGE_LIBJACK
  #include <jack/jack.h>
#endif
#ifdef HAVE_ALSA
  #include <alsa/asoundlib.h>
#endif

#include "PatchagePort.hpp"

struct PortID {
	PortID() : type(NULL_PORT_ID) { memset(&id, 0, sizeof(id)); }
	PortID(const PortID& copy) : type(copy.type) {
		memcpy(&id, &copy.id, sizeof(id));
	}

	enum { NULL_PORT_ID, JACK_ID, ALSA_ADDR } type;

#ifdef PATCHAGE_LIBJACK
	PortID(jack_port_id_t jack_id, bool ign=false)
		: type(JACK_ID) { id.jack_id = jack_id; }
#endif

#ifdef HAVE_ALSA
	PortID(snd_seq_addr_t addr, bool in)
		: type(ALSA_ADDR) { id.alsa_addr = addr; id.is_input = in; }
#endif

	union {
#ifdef PATCHAGE_LIBJACK
		jack_port_id_t jack_id;
#endif
#ifdef HAVE_ALSA
		struct {
			snd_seq_addr_t alsa_addr;
			bool           is_input : 1;
		};
#endif
	} id;
};

static inline std::ostream&
operator<<(std::ostream& os, const PortID& id)
{
	switch (id.type) {
	case PortID::NULL_PORT_ID:
		return os << "(null)";
	case PortID::JACK_ID:
#ifdef PATCHAGE_LIBJACK
		return os << "jack:" << id.id.jack_id;
#endif
		break;
	case PortID::ALSA_ADDR:
#ifdef HAVE_ALSA
		return os << "alsa:" << (int)id.id.alsa_addr.client << ":" << (int)id.id.alsa_addr.port
		          << ":" << (id.id.is_input ? "in" : "out");
#endif
		break;
	}
	assert(false);
}

static inline bool
operator<(const PortID& a, const PortID& b)
{
	if (a.type != b.type)
		return a.type < b.type;

	switch (a.type) {
	case PortID::NULL_PORT_ID:
		return true;
	case PortID::JACK_ID:
#ifdef PATCHAGE_LIBJACK
		return a.id.jack_id < b.id.jack_id;
#endif
		break;
	case PortID::ALSA_ADDR:
#ifdef HAVE_ALSA
		if ((a.id.alsa_addr.client < b.id.alsa_addr.client)
		    || ((a.id.alsa_addr.client == b.id.alsa_addr.client)
		        && a.id.alsa_addr.port < b.id.alsa_addr.port)) {
			return true;
		} else if (a.id.alsa_addr.client == b.id.alsa_addr.client
		           && a.id.alsa_addr.port == b.id.alsa_addr.port) {
			return (a.id.is_input < b.id.is_input);
		} else {
			return false;
		}
#endif
		break;
	}
	assert(false);
}

#endif // PATCHAGE_PORTID_HPP

