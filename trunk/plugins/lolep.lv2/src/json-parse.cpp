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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atom.lv2/atom.h"
#include "contexts.lv2/contexts.h"
#include "uri-map.lv2/uri-map.h"
#include "lolep.hpp"
#include "json-c/json.h"
#include "LV2Plugin.hpp"
#include "LV2Extensions.hpp"

using namespace std;

static uint32_t array_type;
static uint32_t bool_type;
static uint32_t dict_type;
static uint32_t float_type;
static uint32_t int_type;
static uint32_t string_type;

class Parse;
typedef LV2::Plugin<
	Parse,
	LV2::Ext::UriMap<true>,
	LV2::Ext::MessageContext<false>,
	LV2::Ext::ResizePort<true>
> ParseBase;

class Parse : public ParseBase
{
public:
	Parse(double rate, const char* bundle, const LV2::Feature* const* features)
		: ParseBase(2)
	{
		array_type  = uri_to_id(NULL, LV2_ATOM_URI "#Array");
		bool_type   = uri_to_id(NULL, LV2_ATOM_URI "#Bool");
		dict_type   = uri_to_id(NULL, LV2_ATOM_URI "#Dict");
		float_type  = uri_to_id(NULL, LV2_ATOM_URI "#Float32");
		int_type    = uri_to_id(NULL, LV2_ATOM_URI "#Int32");
		string_type = uri_to_id(NULL, LV2_ATOM_URI "#String");
	}

	static void*
	append_blob(Parse* me, uint32_t index, uint16_t* offset, uint16_t size)
	{
		if (resize_port(me, index, (*offset) + size)) {
			void* ret = me->p<uint8_t>(index) + *offset;
			*offset += size;
			return ret;
		} else {
			cerr << "Failed to resize port " << index << " to " << size << "bytes" << endl;
			return NULL;
		}
	}

	static LV2_Atom*
	append_atom(Parse* me, uint32_t index, uint16_t* offset, uint16_t type, uint16_t size)
	{
		void* ptr = append_blob(me, index, offset, sizeof(LV2_Atom) + size);
		if (!ptr)
			return NULL;

		LV2_Atom* atom = (LV2_Atom*)ptr;
		atom->type = type;
		atom->size = size;
		return atom;
	}
	
	static uint16_t
	append_json_object(Parse* me, uint32_t index, uint16_t* offset, json_object* obj)
	{
		LV2_Atom* out = NULL;
		if (!obj) {
			out = append_atom(me, index, offset, 0, 0);
			return sizeof(LV2_Atom);
		}

		switch (json_object_get_type(obj)) {
		case json_type_null:
			out = append_atom(me, index, offset, 0, 0);
			break;
		case json_type_boolean:
			out = append_atom(me, index, offset, bool_type, sizeof(int32_t));
			*(int32_t*)out->body = json_object_get_boolean(obj) ? 1 : 0;
			break;
		case json_type_double:
			out = append_atom(me, index, offset, float_type, sizeof(float));
			*((float*)out->body) = json_object_get_double(obj);
			break;
		case json_type_int:
			out = append_atom(me, index, offset, int_type, sizeof(int32_t));
			*(int32_t*)out->body = json_object_get_int(obj);
			break;
		case json_type_string: {
			const char*  str = json_object_get_string(obj);
			const size_t len = strlen(str);
			out = append_atom(me, index, offset, string_type, len + 1);
			memcpy(out->body, str, len + 1);
			break;
		}
		case json_type_array: {
			// Append array header
			uint16_t out_offset = *offset;
			uint16_t out_size   = 0;
			append_atom(me, index, offset, array_type, 0);

			for (int i = 0; i < json_object_array_length(obj); ++i) {
				// Append element
				const uint16_t elem_size = append_json_object(
					me, index, offset, json_object_array_get_idx(obj, i));
				out_size += elem_size;
			}

			// Update size in header
			out = (LV2_Atom*)(me->p<uint8_t>(index) + out_offset);
			out->size = out_size;
			break;
		}
		case json_type_object: {
			// Append object header
			uint16_t out_offset = *offset;
			uint16_t out_size   = 0;
			out = append_atom(me, index, offset, dict_type, out_size);
			
			json_object_object_foreach(obj, key, val) {
				// Append key
				uint32_t* atom_key = (uint32_t*)append_blob(me, index, offset, sizeof(uint32_t));
				*atom_key = me->uri_to_id(NULL, key);
				out_size += sizeof(uint32_t);

				// Append value
				out_size += append_json_object(me, index, offset, val);
			}
			
			// Update size in header
			out = (LV2_Atom*)(me->p<uint8_t>(index) + out_offset);
			out->size = out_size;
			break;
		}
		}
		
		return sizeof(LV2_Atom) + out->size;
	}

	static uint32_t message_run(LV2_Handle  instance,
	                            const void* valid_inputs,
	                            void*       valid_outputs)
	{
		Parse*    me  = reinterpret_cast<Parse*>(instance);
		LV2_Atom* in  = me->p<LV2_Atom>(0);
		LV2_Atom* out = me->p<LV2_Atom>(1);
		char*     str = (char*)in->body;

		out->type = 0;
		out->size = 0;
		
		if (in->type != string_type)
			return 0;

		json_tokener* tok = json_tokener_new();
		json_object*  obj = json_tokener_parse_ex(tok, str, in->size);

		if (tok->err != json_tokener_success)
			cerr << "JSON parse error: " << json_tokener_errors[tok->err] << endl;
		
		if (tok->err == json_tokener_success && obj) {
			uint16_t offset = 0;
			append_json_object(me, 1, &offset, obj);
			lv2_contexts_set_port_valid(valid_outputs, 1);
		} else {
			lv2_contexts_unset_port_valid(valid_outputs, 1);
		}
		
		json_tokener_free(tok);
		return 0;
	}
};

static const unsigned plugin_class = Parse::register_class(LOLEP_URI "/json-parse");
