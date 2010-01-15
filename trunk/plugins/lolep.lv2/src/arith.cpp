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
#include <iostream>
#include "object.lv2/object.h"
#include "contexts.lv2/contexts.h"
#include "uri-map.lv2/uri-map.h"
#include "lolep.hpp"
#include "LV2Plugin.hpp"
#include "LV2Extensions.hpp"

using namespace std;

static uint32_t int_type;
static uint32_t float_type;
static uint32_t vec_type;

template<typename R, typename A, typename B>
struct SumOp { static inline R op(const A* a, const B* b) { return (*a) + (*b); } };

template<typename R, typename A, typename B>
struct DiffOp { static inline R op(const A* a, const B* b) { return (*a) - (*b); } };

template<typename R, typename A, typename B>
struct ProdOp { static inline R op(const A* a, const B* b) { return (*a) * (*b); } };

template<template<typename R, typename A, typename B> class Op>
class Arith : public LV2::Plugin<
		Arith<Op>,
		LV2::Ext::UriMap<true>,
		LV2::Ext::MessageContext<false>,
		LV2::Ext::ResizePort<true> >
{
public:
	typedef LV2::Plugin<
			Arith<Op>,
			LV2::Ext::UriMap<true>,
			LV2::Ext::MessageContext<false>,
			LV2::Ext::ResizePort<true> >
		Base;

	Arith(double rate, const char* bundle, const LV2::Feature* const* features)
		: Base(2)
	{
		int_type   = Base::uri_to_id(NULL, LV2_OBJECT_URI "#Int32");
		float_type = Base::uri_to_id(NULL, LV2_OBJECT_URI "#Float32");
		vec_type   = Base::uri_to_id(NULL, LV2_OBJECT_URI "#Vector");
	}

	template<typename T>
	static void set_output_type(LV2_Handle instance, LV2_Object* out, uint32_t type) {
		Base::resize_port(instance, 2, sizeof(LV2_Object) + sizeof(T));
		out->type = type;
		out->size = sizeof(T);
	}

	static uint32_t message_run(LV2_Handle  instance,
	                            const void* valid_inputs,
	                            void*       valid_outputs)
	{
		Arith<Op>*  me  = reinterpret_cast<Arith<Op>*>(instance);
		//LV2_Object* a   = me->Base::p<LV2_Object>(0);
		//LV2_Object* b   = me->Base::p<LV2_Object>(1);
		//LV2_Object* out = me->Base::p<LV2_Object>(2);
		LV2_Object* a   = reinterpret_cast<LV2_Object*&>(me->m_ports[0]);
		LV2_Object* b   = reinterpret_cast<LV2_Object*&>(me->m_ports[1]);
		LV2_Object* out = reinterpret_cast<LV2_Object*&>(me->m_ports[2]);

		if (a->type == int_type) {
			if (b->type == int_type) { // int + int => int
				set_output_type<int32_t>(instance, out, int_type);
				*(int32_t*)out->body = Op<int32_t,int32_t,int32_t>::op(
						(int32_t*)a->body, (int32_t*)b->body);
			} else if (b->type == float_type) { // int + float => float
				set_output_type<float>(instance, out, float_type);
				*(float*)out->body = Op<float,int32_t,float>::op(
						(int32_t*)a->body, (float*)b->body);
			} else {
				cout << "Unknown type for b (a is int)" << endl;
			}
		} else if (a->type == float_type) {
			if (b->type == int_type) { // float + int => float
				set_output_type<float>(instance, out, float_type);
				Base::resize_port(instance, 2, sizeof(LV2_Object) + sizeof(float));
				*(float*)out->body = Op<float,float,int32_t>::op(
						(float*)a->body, (int32_t*)b->body);
			} else if (b->type == float_type) { // float + float => float
				set_output_type<float>(instance, out, float_type);
				Base::resize_port(instance, 2, sizeof(LV2_Object) + sizeof(float));
				*(float*)out->body = Op<float,float,float>::op(
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

static const unsigned sum_class        = Arith<SumOp>::register_class(IMUM_URI "/sum");
static const unsigned difference_class = Arith<DiffOp>::register_class(IMUM_URI "/difference");
static const unsigned product_class    = Arith<ProdOp>::register_class(IMUM_URI "/product");
