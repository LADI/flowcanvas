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
#include <cassert>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atom.lv2/atom.h"
#include "atom.lv2/atom-helpers.h"
#include "contexts.lv2/contexts.h"
#include "uri-map.lv2/uri-map.h"
#include "lolep.hpp"
#include "json-c/json.h"
#include "LV2Plugin.hpp"
#include "LV2Extensions.hpp"

using namespace std;

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
		, atom_Bool(uri_to_id(NULL,    LV2_ATOM_URI "#Bool"))
		, atom_Float(uri_to_id(NULL,   LV2_ATOM_URI "#Float32"))
		, atom_Int32(uri_to_id(NULL,   LV2_ATOM_URI "#Int32"))
		, atom_Object(uri_to_id(NULL,  LV2_ATOM_URI "#Object"))
		, atom_String(uri_to_id(NULL,  LV2_ATOM_URI "#String"))
		, atom_Tuple(uri_to_id(NULL,   LV2_ATOM_URI "#Tuple"))
		, atom_URIInt(uri_to_id(NULL,  LV2_ATOM_URI "#URIInt"))
	{
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
		assert((*offset % 4) == 0); // Atoms are 4 byte (32-bit) aligned
		const uint16_t blob_size = lv2_atom_pad_size(size);
		void* ptr = append_blob(me, index, offset, sizeof(LV2_Atom) + blob_size);
		if (!ptr)
			return NULL;

		LV2_Atom* atom = (LV2_Atom*)ptr;
		atom->type = type;
		atom->size = size;
		return atom;
	}
	
	static LV2_Atom*
	append_json_object(Parse* me, uint32_t index, uint16_t* offset, json_object* obj)
	{
		LV2_Atom* out = NULL;
		if (!obj) {
			out = append_atom(me, index, offset, 0, 0);
			return out;
		}

		switch (json_object_get_type(obj)) {
		case json_type_null:
			out = append_atom(me, index, offset, 0, 0);
			break;
		case json_type_boolean:
			out = append_atom(me, index, offset, me->atom_Bool, sizeof(int32_t));
			*(int32_t*)out->body = json_object_get_boolean(obj) ? 1 : 0;
			break;
		case json_type_double:
			out = append_atom(me, index, offset, me->atom_Float, sizeof(float));
			*((float*)out->body) = json_object_get_double(obj);
			break;
		case json_type_int:
			out = append_atom(me, index, offset, me->atom_Int32, sizeof(int32_t));
			*(int32_t*)out->body = json_object_get_int(obj);
			break;
		case json_type_string: {
			const char*  str = json_object_get_string(obj);
			const size_t len = strlen(str);
			if (str[0] == '<' && str[len - 1] == '>') { // URI
				char* uri = (char*)malloc(len - 1);
				memcpy(uri, str + 1, len - 2);
				uri[len - 2] = '\0';
				const uint32_t id = me->uri_to_id(NULL, uri);
				out = append_atom(me, index, offset, me->atom_URIInt, sizeof(uint32_t));
				memcpy(out->body, &id, sizeof(uint32_t));
			} else { // String
				out = append_atom(me, index, offset, me->atom_String, len + 1);
				memcpy(out->body, str, len + 1);
			}
			break;
		}
		case json_type_array: {
			// Append tuple header
			uint16_t out_offset = *offset;
			uint16_t out_size   = 0;
			append_atom(me, index, offset, me->atom_Tuple, 0);

			for (int i = 0; i < json_object_array_length(obj); ++i) {
				// Append element
				LV2_Atom* const elem = append_json_object(
					me, index, offset, json_object_array_get_idx(obj, i));
				out_size += sizeof(LV2_Atom) + lv2_atom_pad_size(elem->size);
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
			out = append_atom(me, index, offset, me->atom_Object, out_size);
			
			json_object_object_foreach(obj, key, val) {
				// Append key
				uint32_t* atom_key = (uint32_t*)append_blob(me, index, offset, sizeof(uint32_t));
				*atom_key = me->uri_to_id(NULL, key);
				out_size += sizeof(uint32_t);

				// Append value
				LV2_Atom* const value = append_json_object(me, index, offset, val);
				out_size += sizeof(LV2_Atom) + lv2_atom_pad_size(value->size);
			}
			
			// Update size in header
			out = (LV2_Atom*)(me->p<uint8_t>(index) + out_offset);
			out->size = out_size;
			break;
		}
		}

		return out;
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
		
		if (in->type != me->atom_String)
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

	const uint32_t atom_Bool;
	const uint32_t atom_Float;
	const uint32_t atom_Int32;
	const uint32_t atom_Object;
	const uint32_t atom_String;
	const uint32_t atom_Tuple;
	const uint32_t atom_URIInt;
};

static const unsigned plugin_class = Parse::register_class(LOLEP_URI "/json-parse");
