/* This file is part of Lolep.
 * Copyright (C) 2009 Dave Robillard <http://drobilla.net>
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
#include <string.h>
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
static uint32_t string_type;
static uint32_t vec_type;

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
		bool_type   = uri_to_id(NULL, LV2_ATOM_URI "#Bool");
		float_type  = uri_to_id(NULL, LV2_ATOM_URI "#Float32");
		int_type    = uri_to_id(NULL, LV2_ATOM_URI "#Int32");
		string_type = uri_to_id(NULL, LV2_ATOM_URI "#String");
		vec_type    = uri_to_id(NULL, LV2_ATOM_URI "#Vector");
	}

	template <typename T>
	static void set_output_type(LV2_Handle instance, LV2_Atom* out, uint32_t type) {
		out->type = type;
		out->size = sizeof(T);
	}

	static bool is_int(const char* str) {
		// Skip sign if present
		uint32_t i = (str[0] == '-' || str[0] == '+') ? 1 : 0;

		// Ensure all characters are digits
		for (; str[i] != '\0'; ++i)
			if (!isdigit(str[i]))
				return false;

		return true;
	}

	static bool is_float(const char* str) {
		// Skip sign if present
		uint32_t i = (str[0] == '-' || str[0] == '+') ? 1 : 0;

		// Ensure all pre-decimal-point characters are digits
		for (; str[i] != '\0'; ++i) {
			if (str[i] == '.') {
				++i;
				break;
			} else if (!isdigit(str[i])) {
				return false;
			}
		}

		// Ensure all post-decimal-point characters are digits
		for (; str[i] != '\0'; ++i)
			if (!isdigit(str[i]))
				return false;

		return true;
	}

	static uint32_t message_run(LV2_Handle  instance,
	                            const void* valid_inputs,
	                            void*       valid_outputs)
	{
		Parse*    me  = reinterpret_cast<Parse*>(instance);
		LV2_Atom* in  = me->p<LV2_Atom>(0);
		LV2_Atom* out = me->p<LV2_Atom>(1);
		char*     str = (char*)in->body;

		if (in->type != string_type) {
			out->type = 0;
			out->size = 0;
			return 0;
		}

		if (!strcmp(str, "true")) {
			set_output_type<int32_t>(instance, out, bool_type);
			*(int32_t*)out->body = 1;
		} else if (!strcmp(str, "false")) {
			set_output_type<int32_t>(instance, out, bool_type);
			*(int32_t*)out->body = 0;
		} else if (!strcmp(str, "null")) {
			out->type = 0;
			out->size = 0;
		} else if (is_int(str)) {
			sscanf(str, "%d", (int32_t*)out->body);
			set_output_type<int32_t>(instance, out, int_type);
		} else if (is_float(str)) {
			sscanf(str, "%f", (float*)out->body);
			set_output_type<float>(instance, out, float_type);
		} else if (str[0] == '"') {
			size_t len = strlen(str);
			if (str[len-1] == '"') {
				resize_port(instance, 1, sizeof(LV2_Atom) + len - 1);
				out->type = string_type;
				out->size = len - 2;
				memcpy(out->body, str + 1, len - 2);
				((char*)out->body)[len - 2] = '\0';
			}
		} else {
			out->type = 0;
			out->size = 0;
			return 0;
		}

		lv2_contexts_set_port_valid(valid_outputs, 1);
		return 0;
	}
};

static const unsigned plugin_class = Parse::register_class(IMUM_URI "/parse");
