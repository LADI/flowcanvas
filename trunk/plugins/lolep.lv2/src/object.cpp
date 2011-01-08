/* This file is part of Lolep.
 * Copyright (C) 2009 David Robillard <http://drobilla.net>
 *
 * Lolep is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Lolep is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <ctype.h>
#include <stdio.h>

#include <algorithm>
#include <iostream>
#include <map>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom-helpers.h"
#include "lv2/lv2plug.in/ns/ext/contexts/contexts.h"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
#include "lolep.hpp"
#include "LV2Plugin.hpp"
#include "LV2Extensions.hpp"

using namespace std;

#define LV2_MESSAGE_URI "http://lv2plug.in/ns/ext/message"

class Object;
typedef LV2::Plugin<
	Object,
	LV2::Ext::UriMap<true>,
	LV2::Ext::MessageContext<false>
> ObjectBase;

class Object : public ObjectBase
{
public:
	Object(double rate, const char* bundle, const LV2::Feature* const* features)
		: ObjectBase(1)
		, atom_Object(uri_to_id(NULL, LV2_ATOM_URI "#Object"))
		, atom_URIInt(uri_to_id(NULL, LV2_ATOM_URI "#URIInt"))
		, msg_diff(uri_to_id(NULL,    LV2_MESSAGE_URI "#diff"))
		, msg_key(uri_to_id(NULL,     LV2_MESSAGE_URI "#key"))
		, msg_set(uri_to_id(NULL,     LV2_MESSAGE_URI "#set"))
		, msg_unset(uri_to_id(NULL,   LV2_MESSAGE_URI "#unset"))
		, msg_value(uri_to_id(NULL,   LV2_MESSAGE_URI "#value"))
		, rdf_type(uri_to_id(NULL, "http://www.w3.org/1999/02/22-rdf-syntax-ns#type"))
	{
	}

	~Object()
	{
		for (Map::iterator i = _map.begin(); i != _map.end(); ++i)
			free(i->second);
	}
	
	uint32_t message_run(const void* valid_inputs,
	                     void*       valid_outputs)
	{
		LV2_Atom* in  = p<LV2_Atom>(0);
		LV2_Atom* out = p<LV2_Atom>(1);

		out->type = 0;
		out->size = 0;

		if (lv2_atom_is_a(in, rdf_type, atom_URIInt, atom_Object, msg_set)) {
			LV2_Object_Query q[] = {
				{ msg_key,   NULL },
				{ msg_value, NULL },
				{ 0, NULL }
			};
			
			if (lv2_object_query(in, q)) {
				const LV2_Atom* key   = q[0].value;
				const LV2_Atom* value = q[1].value;
				
				if (key->type != atom_URIInt) {
					fprintf(stderr, "error: Key is not a URI\n");
					return 0;
				}

				const size_t copy_size = sizeof(LV2_Atom) + value->size;
				LV2_Atom*    copy      = (LV2_Atom*)malloc(copy_size);
				memcpy(copy, value, copy_size);

				printf("SET %u\n", *(uint32_t*)key->body);
				_map.insert(make_pair(*(uint32_t*)key->body, copy));

				out->type = msg_diff;
				lv2_contexts_set_port_valid(valid_outputs, 1);
				return 0;
			} else {
				fprintf(stderr, "error: received invalid set message\n");
			}
		} else if (lv2_atom_is_a(in, rdf_type, atom_URIInt, atom_Object, msg_unset)) {
			LV2_Object_Query q[] = {
				{ msg_key, NULL },
				{ 0, NULL }
			};
			
			if (lv2_object_query(in, q)) {
				const LV2_Atom* key   = q[0].value;

				if (key->type != atom_URIInt) {
					fprintf(stderr, "error: Key is not a URI\n");
					return 0;
				}

				printf("UNSET %u\n", *(uint32_t*)key->body);
				_map.erase(*(uint32_t*)key->body);

				out->type = msg_diff;
				lv2_contexts_set_port_valid(valid_outputs, 1);
				return 0;
			} else {
				fprintf(stderr, "error: received invalid unset message\n");
			}
		}

		lv2_contexts_unset_port_valid(valid_outputs, 1);
		return 0;
	}

	uint32_t atom_Object;
	uint32_t atom_URIInt;
	uint32_t msg_diff;
	uint32_t msg_key;
	uint32_t msg_set;
	uint32_t msg_unset;
	uint32_t msg_value;
	uint32_t rdf_type;

	typedef std::map<uint32_t, LV2_Atom*> Map;
	Map _map;
};

static const unsigned plugin_class = Object::register_class(LOLEP_URI "/object");
