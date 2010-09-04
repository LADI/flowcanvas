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
#include "atom.lv2/atom.h"
#include "contexts.lv2/contexts.h"
#include "uri-map.lv2/uri-map.h"
#include "lolep.hpp"
#include "LV2Plugin.hpp"
#include "LV2Extensions.hpp"

using namespace std;

static uint32_t bool_type;
static uint32_t float_type;
static uint32_t int_type;
static uint32_t midi_type;
static uint32_t string_type;
static uint32_t vec_type;

class Print;
typedef LV2::Plugin<
	Print,
	LV2::Ext::UriMap<true>,
	LV2::Ext::MessageContext<false>,
	LV2::Ext::ResizePort<true>
> PrintBase;

class Print : public PrintBase
{
public:
	Print(double rate, const char* bundle, const LV2::Feature* const* features)
		: PrintBase(1)
	{
		bool_type   = uri_to_id(NULL, LV2_ATOM_URI "#Bool");
		float_type  = uri_to_id(NULL, LV2_ATOM_URI "#Float32");
		int_type    = uri_to_id(NULL, LV2_ATOM_URI "#Int32");
		midi_type   = uri_to_id(NULL, "http://lv2plug.in/ns/ext/midi#MidiEvent");
		string_type = uri_to_id(NULL, LV2_ATOM_URI "#String");
		vec_type    = uri_to_id(NULL, LV2_ATOM_URI "#Vector");
	}

	static uint32_t message_run(LV2_Handle  instance,
	                            const void* valid_inputs,
	                            void*       valid_outputs)
	{
		Print*    me = reinterpret_cast<Print*>(instance);
		LV2_Atom* in = me->p<LV2_Atom>(0);
		if (in->type == 0 && in->size == 0) {
			printf("null\n");
		} else if (in->type == bool_type) {
			if (*(int32_t*)in->body == 0) {
				printf("false\n");
			} else {
				printf("true\n");
			}
		} else if (in->type == string_type) {
			printf("\"%s\"\n", (char*)in->body);
		} else if (in->type == int_type) {
			printf("%d\n", *(int32_t*)in->body);
		} else if (in->type == float_type) {
			printf("%f\n", *(float*)in->body);
		} else if (in->type == vec_type) {
			printf("[ ... ]\n");
		} else if (in->type == midi_type) {
			printf("MIDI Message:");
			for (uint16_t i = 0; i < in->size; ++i) {
				printf(" %X", (unsigned)in->body[i]);
			}
			printf("\n");
		} else {
			printf("<LV2_Atom type=%d size=%d>\n", in->type, in->size);
		}
		return 0;
	}

};

static const unsigned plugin_class = Print::register_class(IMUM_URI "/print");
