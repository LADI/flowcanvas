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

#include "atom.lv2/atom.h"
#include "atom.lv2/atom-helpers.h"
#include "contexts.lv2/contexts.h"
#include "uri-map.lv2/uri-map.h"
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
		, msg_diff(uri_to_id(NULL,  LV2_MESSAGE_URI "#diff"))
		, msg_set(uri_to_id(NULL, LV2_MESSAGE_URI "#set"))
		, msg_unset(uri_to_id(NULL, LV2_MESSAGE_URI "#unset"))
		, rdf_type(uri_to_id(NULL, "http://www.w3.org/1999/02/22-rdf-syntax-ns#type"))
	{
	}

	static uint32_t message_run(LV2_Handle  instance,
	                            const void* valid_inputs,
	                            void*       valid_outputs)
	{
		Object*   me  = reinterpret_cast<Object*>(instance);
		LV2_Atom* in  = me->p<LV2_Atom>(0);
		LV2_Atom* out = me->p<LV2_Atom>(1);

		if (lv2_atom_is_a(in, me->rdf_type, me->atom_URIInt, me->atom_Object, me->msg_set)) {
			printf("SET:\n");
			for (LV2_Atom_Object_Iter i = lv2_atom_object_get_iter((LV2_Atom_Property*)in->body);
			     !lv2_atom_object_iter_is_end(in, i);
			     i = lv2_atom_object_iter_next(i)) {
				LV2_Atom_Property* prop = lv2_atom_object_iter_get(i);
				printf("\t%u :: %u\n", prop->predicate, prop->object.type);
			}

			out->type = me->msg_diff;
			out->size = 0;
			lv2_contexts_set_port_valid(valid_outputs, 1);
		} else if (lv2_atom_is_a(in, me->rdf_type, me->atom_URIInt, me->atom_Object, me->msg_unset)) {
			printf("UNSET:\n");
			for (LV2_Atom_Object_Iter i = lv2_atom_object_get_iter((LV2_Atom_Property*)in->body);
			     !lv2_atom_object_iter_is_end(in, i);
			     i = lv2_atom_object_iter_next(i)) {
				LV2_Atom_Property* prop = lv2_atom_object_iter_get(i);
				printf("\t%u :: %u\n", prop->predicate, prop->object.type);
			}

			out->type = me->msg_diff;
			out->size = 0;
			lv2_contexts_set_port_valid(valid_outputs, 1);
		} else {
			printf("Unknown message type %u\n", in->type);
		}

		out->type = 0;
		out->size = 0;
		lv2_contexts_unset_port_valid(valid_outputs, 1);
		return 0;
	}

	uint32_t atom_Object;
	uint32_t atom_URIInt;
	uint32_t msg_diff;
	uint32_t msg_set;
	uint32_t msg_unset;
	uint32_t rdf_type;

	std::map<uint32_t, LV2_Atom*> _map;
};

static const unsigned plugin_class = Object::register_class(LOLEP_URI "/object");
