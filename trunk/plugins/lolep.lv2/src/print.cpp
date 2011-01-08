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
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom-helpers.h"
#include "lv2/lv2plug.in/ns/ext/contexts/contexts.h"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
#include "lolep.hpp"
#include "LV2Plugin.hpp"
#include "LV2Extensions.hpp"

using namespace std;

#define LV2_MIDI_URI "http://lv2plug.in/ns/ext/midi"

class Print;
typedef LV2::Plugin<
	Print,
	LV2::Ext::UriMap<true>,
	LV2::Ext::UriUnmap<true>,
	LV2::Ext::MessageContext<false>
> PrintBase;

class Print : public PrintBase
{
public:
	Print(double rate, const char* bundle, const LV2::Feature* const* features)
		: PrintBase(1)
		, atom_Blank(uri_to_id(NULL,     LV2_ATOM_URI "#Blank"))
		, atom_Bool(uri_to_id(NULL,      LV2_ATOM_URI "#Bool"))
		, atom_Float(uri_to_id(NULL,     LV2_ATOM_URI "#Float32"))
		, atom_ID(uri_to_id(NULL,        LV2_ATOM_URI "#ID"))
		, atom_Int32(uri_to_id(NULL,     LV2_ATOM_URI "#Int32"))
		, atom_String(uri_to_id(NULL,    LV2_ATOM_URI "#String"))
		, atom_Tuple(uri_to_id(NULL,     LV2_ATOM_URI "#Tuple"))
		, atom_Vector(uri_to_id(NULL,    LV2_ATOM_URI "#Vector"))
		, midi_MidiEvent(uri_to_id(NULL, LV2_MIDI_URI "#MidiEvent"))
	{
	}

	void print(LV2_Atom* in)
	{
		if (in->type == 0 && in->size == 0) {
			printf("null");
		} else if (in->type == atom_ID) {
			printf("<%s>", id_to_uri(NULL, *(int32_t*)in->body));
		} else if (in->type == atom_Bool) {
			if (*(int32_t*)in->body == 0) {
				printf("false");
			} else {
				printf("true");
			}
		} else if (in->type == atom_String) {
			printf("\"%s\"", ((LV2_Atom_String*)in->body)->str);
		} else if (in->type == atom_Int32) {
			printf("%d", *(int32_t*)in->body);
		} else if (in->type == atom_Float) {
			printf("%f", *(float*)in->body);
		} else if (in->type == atom_Tuple) {
			printf("( ... )");
		} else if (in->type == atom_Vector) {
			printf("[ ... ]");
		} else if (in->type == atom_Blank) {
			printf("{\n");
			LV2_OBJECT_FOREACH(in, i) {
				LV2_Atom_Property* prop = lv2_object_iter_get(i);
				printf("  <%s> : ", id_to_uri(NULL, prop->key));
				print(&prop->value);
				printf(",\n");
			}
			printf("}");
		} else if (in->type == midi_MidiEvent) {
			printf("(MIDI ");
			for (uint16_t i = 0; i < in->size; ++i) {
				printf(" %X", (unsigned)in->body[i]);
			}
			printf(")");
		} else {
			printf("(Atom type: %d size: %d)", in->type, in->size);
		}
	}

	uint32_t message_run(const void* valid_inputs,
	                     void*       valid_outputs)
	{
		print(p<LV2_Atom>(0));
		printf("\n");
		return 0;
	}
	
	const uint32_t atom_Blank;
	const uint32_t atom_Bool;
	const uint32_t atom_Float;
	const uint32_t atom_ID;
	const uint32_t atom_Int32;
	const uint32_t atom_String;
	const uint32_t atom_Tuple;
	const uint32_t atom_Vector;
	const uint32_t midi_MidiEvent;
};

static const unsigned plugin_class = Print::register_class(LOLEP_URI "/print");
