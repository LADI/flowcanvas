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

#include <algorithm>
#include <iostream>
#include <ctype.h>
#include <stdio.h>
#include "lv2/http/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/http/lv2plug.in/ns/ext/atom/atom-helpers.h"
#include "contexts.lv2/contexts.h"
#include "uri-map.lv2/uri-map.h"
#include "lolep.hpp"
#include "json-c/json.h"
#include "LV2Plugin.hpp"
#include "LV2Extensions.hpp"

using namespace std;

static uint32_t tuple_type;
static uint32_t bool_type;
static uint32_t object_type;
static uint32_t float_type;
static uint32_t int_type;
static uint32_t string_type;

class Print;
typedef LV2::Plugin<
	Print,
	LV2::Ext::UriMap<true>,
	LV2::Ext::UriUnmap<true>,
	LV2::Ext::MessageContext<false>,
	LV2::Ext::ResizePort<true>
> PrintBase;

class Print : public PrintBase
{
public:
	Print(double rate, const char* bundle, const LV2::Feature* const* features)
		: PrintBase(1)
	{
		tuple_type  = uri_to_id(NULL, LV2_ATOM_URI "#Tuple");
		bool_type   = uri_to_id(NULL, LV2_ATOM_URI "#Bool");
		object_type = uri_to_id(NULL, LV2_ATOM_URI "#Object");
		float_type  = uri_to_id(NULL, LV2_ATOM_URI "#Float32");
		int_type    = uri_to_id(NULL, LV2_ATOM_URI "#Int32");
		string_type = uri_to_id(NULL, LV2_ATOM_URI "#String");
	}

	static json_object* atom_to_object(Print* me, const LV2_Atom* atom)
	{
		json_object* obj = NULL;
		if (atom->type == bool_type) {
			obj = json_object_new_boolean(*(int32_t*)atom->body);
		} else if (atom->type == string_type) {
			obj = json_object_new_string((char*)atom->body);
		} else if (atom->type == int_type) {
			obj = json_object_new_int(*(int32_t*)atom->body);
		} else if (atom->type == float_type) {
			obj = json_object_new_double(*(float*)atom->body);
		} else if (atom->type == tuple_type) {
			obj = json_object_new_array();
			LV2_Atom* elem = (LV2_Atom*)atom->body;
			while (elem < (LV2_Atom*)(atom->body + atom->size)) {
				json_object_array_add(obj, atom_to_object(me, elem));
				elem = (LV2_Atom*)(elem->body + lv2_atom_pad_size(elem->size));
			}
		} else if (atom->type == object_type) {
			obj = json_object_new_object();
			for (LV2_Atom_Object_Iter i = lv2_atom_object_get_iter((LV2_Atom_Property*)atom->body);
			     !lv2_atom_object_iter_is_end(atom, i);
			     i = lv2_atom_object_iter_next(i)) {
				LV2_Atom_Property* prop    = lv2_atom_object_iter_get(i);
				const char*        key_str = me->id_to_uri(NULL, prop->predicate);
				if (key_str) {
					json_object_object_add(obj, key_str, atom_to_object(me, &prop->object));
				} else {
					fprintf(stderr, "Failed to unmap URI ID %d\n", prop->predicate);
				}
			}
		} else {
			obj = json_object_new_object(); // null
		}
		return obj;
	}
	
	static uint32_t message_run(LV2_Handle  instance,
	                            const void* valid_inputs,
	                            void*       valid_outputs)
	{
		Print*         me  = reinterpret_cast<Print*>(instance);
		LV2_Atom*      in  = me->p<LV2_Atom>(0);
		json_object*   obj = atom_to_object(me, in);
		const char*    str = json_object_to_json_string(obj);
		const uint16_t len = strlen(str);

		resize_port(me, 1, len + 1);
		LV2_Atom* out = me->p<LV2_Atom>(1);

		out->type = string_type;
		out->size = len + 1;
		memcpy(out->body, str, len + 1);
		lv2_contexts_set_port_valid(valid_outputs, 1);

		return 0;
	}
};

static const unsigned plugin_class = Print::register_class(LOLEP_URI "/json-serialise");
