/* This file is part of Imum.
 * Copyright (C) 2009 Dave Robillard <http://drobilla.net>
 *
 * Imum is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Imum is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <algorithm>
#include <iostream>
#include "object.lv2/object.h"
#include "contexts.lv2/contexts.h"
#include "uri-map.lv2/uri-map.h"
#include "imum.hpp"
#include "LV2Plugin.hpp"
#include "LV2Extensions.hpp"

using namespace std;

static uint32_t int_type;
static uint32_t float_type;
static uint32_t vec_type;

class Sum;
typedef LV2::Plugin<
	Sum,
	LV2::Ext::UriMap<true>,
	LV2::Ext::MessageContext<false>,
	LV2::Ext::ResizePort<true>
> SumBase;

class Sum : public SumBase
{
public:
	Sum(double rate, const char* bundle, const LV2::Feature* const* features)
		: SumBase(2)
	{
		int_type   = uri_to_id(NULL, LV2_OBJECT_URI "#Int32");
		float_type = uri_to_id(NULL, LV2_OBJECT_URI "#Float32");
		vec_type   = uri_to_id(NULL, LV2_OBJECT_URI "#Vector");
	}

	template <typename R, typename A, typename B>
	static inline R sum(A* a, B* b) {
		cout << *a << " + " << *b << endl;
		return *a + *b;
	}

	template <typename T>
	static void set_output_type(LV2_Handle instance, LV2_Object* out, uint32_t type) {
		resize_port(instance, 2, sizeof(LV2_Object) + sizeof(T));
		out->type = type;
		out->size = sizeof(T);
	}

	static uint32_t message_run(LV2_Handle  instance,
	                            const void* valid_inputs,
	                            void*       valid_outputs)
	{
		Sum*        me  = reinterpret_cast<Sum*>(instance);
		LV2_Object* a   = me->p<LV2_Object>(0);
		LV2_Object* b   = me->p<LV2_Object>(1);
		LV2_Object* out = me->p<LV2_Object>(2);

		if (a->type == int_type) {
			if (b->type == int_type) { // int + int => int
				set_output_type<int32_t>(instance, out, int_type);
				*(int32_t*)out->body = sum<int32_t,int32_t,int32_t>(
						(int32_t*)a->body, (int32_t*)b->body);
			} else if (b->type == float_type) { // int + float => float
				set_output_type<float>(instance, out, float_type);
				*(float*)out->body = sum<float,int32_t,float>(
						(int32_t*)a->body, (float*)b->body);
			} else {
				cout << "Unknown type for b (a is int)" << endl;
			}
		} else if (a->type == float_type) {
			if (b->type == int_type) { // float + int => float
				set_output_type<float>(instance, out, float_type);
				resize_port(instance, 2, sizeof(LV2_Object) + sizeof(float));
				*(float*)out->body = sum<float,float,int32_t>(
						(float*)a->body, (int32_t*)b->body);
			} else if (b->type == float_type) { // float + float => float
				set_output_type<float>(instance, out, float_type);
				resize_port(instance, 2, sizeof(LV2_Object) + sizeof(float));
				*(float*)out->body = sum<float,float,float>(
						(float*)a->body, (float*)b->body);
			} else {
				cout << "Unknown type for b (a is float)" << endl;
			}
		} else {
			cout << "Unknown type for a" << endl;
		}


		lv2_contexts_set_port_valid(valid_outputs, 2);

		return 0;
	}
};

static const unsigned plugin_class = Sum::register_class(IMUM_URI "/sum");
