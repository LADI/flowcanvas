/* This file is part of Lolep.
 * Copyright (C) 2010 David Robillard <http://drobilla.net>
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
#include <iostream>
#include <ctype.h>
#include <stdio.h>
#include <raptor.h>
#include "lv2/http/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/http/lv2plug.in/ns/ext/atom/atom-helpers.h"
#include "contexts.lv2/contexts.h"
#include "uri-map.lv2/uri-map.h"
#include "lolep.hpp"
#include "LV2Plugin.hpp"
#include "LV2Extensions.hpp"

using namespace std;

class RDFWrite;
typedef LV2::Plugin<
	RDFWrite,
	LV2::Ext::UriMap<true>,
	LV2::Ext::UriUnmap<true>,
	LV2::Ext::MessageContext<false>,
	LV2::Ext::ResizePort<true>
> RDFWriteBase;

class RDFWrite : public RDFWriteBase
{
public:
	RDFWrite(double rate, const char* bundle, const LV2::Feature* const* features)
		: RDFWriteBase(2)
		, atom_Blank(uri_to_id(NULL,    LV2_ATOM_URI "#Blank"))
		, atom_BlankID(uri_to_id(NULL,  LV2_ATOM_URI "#BlankID"))
		, atom_Bool(uri_to_id(NULL,     LV2_ATOM_URI "#Bool"))
		, atom_Float(uri_to_id(NULL,    LV2_ATOM_URI "#Float32"))
		, atom_ID(uri_to_id(NULL,       LV2_ATOM_URI "#ID"))
		, atom_Int32(uri_to_id(NULL,    LV2_ATOM_URI "#Int32"))
		, atom_Model(uri_to_id(NULL,    LV2_ATOM_URI "#Model"))
		, atom_Resource(uri_to_id(NULL, LV2_ATOM_URI "#Resource"))
		, atom_String(uri_to_id(NULL,   LV2_ATOM_URI "#String"))
		, atom_Tuple(uri_to_id(NULL,    LV2_ATOM_URI "#Tuple"))
	{
	}

	static void serialize_object(RDFWrite* me, raptor_serializer* serializer, LV2_Atom* atom) {
		LV2_Object*      object = (LV2_Object*)atom->body;
		raptor_statement statement;

		const char*  pat     = "bXXXXXXXXXXXXXXX"; ///< Blank node ID pattern
		const size_t pat_len = strlen(pat);

		// Set subject
		if (atom->type == me->atom_Blank) {
			statement.subject_type = RAPTOR_IDENTIFIER_TYPE_ANONYMOUS;
			statement.subject      = malloc(pat_len + 1);
			snprintf((char*)statement.subject, pat_len + 1, "b%u", object->id);
		} else {
			assert(atom->type == me->atom_Resource);
			statement.subject_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;
			statement.subject      = raptor_new_uri((const uint8_t*)me->id_to_uri(NULL, object->id));
		}

		// Write properties
		statement.predicate_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;
		LV2_OBJECT_FOREACH(atom, i) {
			LV2_Atom_Property* p = lv2_object_iter_get(i);
			statement.predicate = raptor_new_uri((const uint8_t*)me->id_to_uri(NULL, p->key));

			if (p->value.type == me->atom_ID) {
				statement.object_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;
				statement.object = raptor_new_uri((const uint8_t*)me->id_to_uri(NULL,
					*(uint32_t*)p->value.body));

				raptor_serialize_statement(serializer, &statement);
			} else if (p->value.type == me->atom_Resource) {
				LV2_Object* value_obj = (LV2_Object*)p->value.body;
				statement.object_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;
				statement.object = raptor_new_uri((const uint8_t*)me->id_to_uri(NULL, value_obj->id));
				raptor_serialize_statement(serializer, &statement);

				serialize_object(me, serializer, &p->value);
			} else if (p->value.type == me->atom_Blank) {
				LV2_Object* value_obj = (LV2_Object*)p->value.body;
				statement.object_type = RAPTOR_IDENTIFIER_TYPE_ANONYMOUS;
				statement.object      = malloc(pat_len + 1);
				snprintf((char*)statement.object, pat_len + 1, "b%u", value_obj->id);
				raptor_serialize_statement(serializer, &statement);

				serialize_object(me, serializer, &p->value);
			} else if (p->value.type == me->atom_BlankID) {
				printf("XXXXXXXXXXXXXX BLANK ID VALUE\n");
			} else {
				printf("XXXXXXXXXXXXXX UNKNOWN VALUE TYPE %d %s\n",
				       p->value.type, me->id_to_uri(NULL, p->value.type));
			}
		}
	}
		
	static uint32_t message_run(LV2_Handle  instance,
	                            const void* valid_inputs,
	                            void*       valid_outputs)
	{
		RDFWrite* me  = reinterpret_cast<RDFWrite*>(instance);
		LV2_Atom* in  = me->p<LV2_Atom>(0);
		LV2_Atom* out = me->p<LV2_Atom>(1);

		out->type = 0;
		out->size = 0;
		lv2_contexts_unset_port_valid(valid_outputs, 1);

		if (in->type != me->atom_Blank && in->type != me->atom_Resource) {
			fprintf(stderr, "Unknown input type\n");
			out->type = 0;
			out->size = 0;
			return 0;
		}
		
		raptor_serializer* serializer = raptor_new_serializer("turtle");
		
		void*  txt     = NULL;
		size_t txt_len = 0;

		raptor_serialize_start_to_string(serializer, NULL, &txt, &txt_len);
		serialize_object(me, serializer, in);
		raptor_serialize_end(serializer);

		resize_port(me, 1, sizeof(LV2_Atom) + sizeof(LV2_Atom_String) + txt_len + 1);
		
		out = me->p<LV2_Atom>(1);

		out->type = me->atom_String;
		out->size = sizeof(LV2_Atom_String) + txt_len + 1;
		
		LV2_Atom_String* str = (LV2_Atom_String*)out->body;
		str->lang = 0;
		memcpy(str->str, txt, txt_len + 1);
		
		lv2_contexts_set_port_valid(valid_outputs, 1);

		return 0;
	}
	
	const uint32_t atom_Blank;
	const uint32_t atom_BlankID;
	const uint32_t atom_Bool;
	const uint32_t atom_Float;
	const uint32_t atom_ID;
	const uint32_t atom_Int32;
	const uint32_t atom_Model;
	const uint32_t atom_Resource;
	const uint32_t atom_String;
	const uint32_t atom_Tuple;
};

static const unsigned plugin_class = RDFWrite::register_class(LOLEP_URI "/rdf-write");
