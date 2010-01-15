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

static uint32_t string_type;
static uint32_t vec_type;

class Sample;
typedef LV2::Plugin<
	Sample,
	LV2::Ext::UriMap<true>,
	LV2::Ext::MessageContext<false>
> SampleBase;

class Sample : public SampleBase
{
public:
	Sample(double rate, const char* bundle, const LV2::Feature* const* features)
		: SampleBase(3)
	{
		string_type = uri_to_id(NULL, LV2_OBJECT_URI "#String");
		vec_type    = uri_to_id(NULL, LV2_OBJECT_URI "#Vector");
	}

	static uint32_t message_run(LV2_Handle  instance,
	                            const void* valid_inputs,
	                            void*       valid_outputs)
	{
		Sample*     me = reinterpret_cast<Sample*>(instance);
		LV2_Object* control = me->p<LV2_Object>(0);
		LV2_Object* respond = me->p<LV2_Object>(1);
		//LV2_Object* buffer  = me->p<LV2_Object>(2);

		if (control->type == string_type) {
			cout << "Sample command: " << control->body << endl;
			respond->type = string_type;
			respond->size = 3;
			strncpy((char*)respond->body, "OK", 3);
		}

		return 0;
	}
};

static const unsigned plugin_class = Sample::register_class(IMUM_URI "/sample");
