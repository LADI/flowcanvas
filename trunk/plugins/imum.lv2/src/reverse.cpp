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

static uint32_t string_type;

class Reverse;
typedef LV2::Plugin<
	Reverse,
	LV2::Ext::UriMap<true>,
	LV2::Ext::MessageContext<false>
> ReverseBase;

class Reverse : public ReverseBase
{
public:
	Reverse(double rate, const char* bundle, const LV2::Feature* const* features)
		: ReverseBase(2)
	{
		string_type = uri_to_id(NULL, LV2_OBJECT_URI "#String");
	}

	static uint32_t message_run(LV2_Handle  instance,
	                            const void* valid_inputs,
	                            void*       valid_outputs)
	{
		Reverse*    me = reinterpret_cast<Reverse*>(instance);
		LV2_Object* in = me->p<LV2_Object>(0);
		if (in->type == string_type) {
			cout << "Reverse: " << (char*)(in + 1) << endl;
		} else {
			cout << "Reverse unknown type " << in->type << " @ " << in << endl;
		}
		return 0;
	}
};

static const unsigned plugin_class = Reverse::register_class(IMUM_URI "/reverse");
