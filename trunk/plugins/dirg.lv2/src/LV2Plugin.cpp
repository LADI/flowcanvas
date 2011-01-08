/* LV2Plugin - C++ wrapper class for LV2 plugins
 * Copyright (C) 2006-2007 Lars Luthman <lars.luthman@gmail.com>
 * Copyright (C) 2008-2010 David Robillard <d@drobilla.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA
 */

#include "LV2Plugin.hpp"

namespace LV2 {
	Descriptors& get_lv2_descriptors() {
		static Descriptors descriptors;
		return descriptors;
	}
}


extern "C" {
	const LV2_Descriptor* lv2_descriptor(uint32_t index) {
		const LV2::Descriptors& descriptors = LV2::get_lv2_descriptors();
		if (index < descriptors.size())
			return &descriptors[index];
		return NULL;
	}
}


