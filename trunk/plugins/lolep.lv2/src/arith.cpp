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
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/contexts/contexts.h"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
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
		LV2::Ext::ResizePort<true> >
{
public:
	typedef LV2::Plugin<
			Arith<Op>,
			LV2::Ext::UriMap<true>,
			LV2::Ext::ResizePort<true> >
		Base;

	Arith(double rate, const char* bundle, const LV2::Feature* const* features)
		: Base(2)
	{
		int_type   = Base::uri_to_id(NULL, LV2_ATOM_URI "#Int32");
		float_type = Base::uri_to_id(NULL, LV2_ATOM_URI "#Float32");
		vec_type   = Base::uri_to_id(NULL, LV2_ATOM_URI "#Vector");
	}

	template<typename T>
	void set_output_type(LV2_Atom* out, uint32_t type) {
		Base::resize_port(reinterpret_cast<LV2_Handle>(this),
				2, sizeof(LV2_Atom) + sizeof(T));
		out->type = type;
		out->size = sizeof(T);
	}

	void run(uint32_t sample_count) {
		LV2_Atom* a   = reinterpret_cast<LV2_Atom*&>(this->m_ports[0]);
		LV2_Atom* b   = reinterpret_cast<LV2_Atom*&>(this->m_ports[1]);
		LV2_Atom* out = reinterpret_cast<LV2_Atom*&>(this->m_ports[2]);

		if (a->type == int_type) {
			if (b->type == int_type) { // int (+) int => int
				std::cout << "int + int" << std::endl;
				set_output_type<int32_t>(out, int_type);
				*(int32_t*)out->body = Op<int32_t,int32_t,int32_t>::op(
						(int32_t*)a->body, (int32_t*)b->body);
			} else if (b->type == float_type) { // int (+) float => float
				std::cout << "int + float" << std::endl;
				set_output_type<float>(out, float_type);
				*(float*)out->body = Op<float,int32_t,float>::op(
						(int32_t*)a->body, (float*)b->body);
			} else {
				out->type = 0;
				out->size = 0;
				//cout << "Unknown type for b (a is int)" << endl;
			}
		} else if (a->type == float_type) {
			if (b->type == int_type) { // float (+) int => float
				std::cout << "float + int" << std::endl;
				set_output_type<float>(out, float_type);
				*(float*)out->body = Op<float,float,int32_t>::op(
						(float*)a->body, (int32_t*)b->body);
			} else if (b->type == float_type) { // float (+) float => float
				std::cout << "float + float" << std::endl;
				set_output_type<float>(out, float_type);
				*(float*)out->body = Op<float,float,float>::op(
						(float*)a->body, (float*)b->body);
			} else {
				cout << "Unknown type for b (a is float)" << endl;
			}
		} else {
			out->type = 0;
			out->size = 0;
			//cout << "Unknown type for a" << endl;
		}
	}
};

static const unsigned sum_class        = Arith<SumOp>::register_class(LOLEP_URI "/sum");
static const unsigned difference_class = Arith<DiffOp>::register_class(LOLEP_URI "/difference");
static const unsigned product_class    = Arith<ProdOp>::register_class(LOLEP_URI "/product");
