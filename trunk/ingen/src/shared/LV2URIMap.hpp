/* This file is part of Ingen.
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef INGEN_SHARED_LV2URIMAP_HPP
#define INGEN_SHARED_LV2URIMAP_HPP

#include <boost/utility.hpp>
#include <raul/URI.hpp>
#include "uri-map.lv2/uri-map.h"
#include "ingen-config.h"
#include "LV2Features.hpp"

namespace Ingen {
namespace Shared {


/** Implementation of the LV2 URI Map extension
 */
class LV2URIMap : public boost::noncopyable, public LV2Features::Feature {
public:
	LV2URIMap();

	SharedPtr<LV2_Feature> feature(Node*) {
		return SharedPtr<LV2_Feature>(&uri_map_feature, NullDeleter<LV2_Feature>);
	}

	uint32_t uri_to_id(const char* map, const char* uri);

private:
	static uint32_t uri_map_uri_to_id(LV2_URI_Map_Callback_Data callback_data,
	                                  const char*               map,
	                                  const char*               uri);

	LV2_Feature         uri_map_feature;
	LV2_URI_Map_Feature uri_map_feature_data;

public:
	struct Quark : public Raul::URI {
		Quark(const char* str);
		uint32_t id;
	};

	const Quark ctx_context;
	const Quark ctx_AudioContext;
	const Quark ctx_MessageContext;
	const Quark doap_name;
	const Quark ingen_LADSPAPlugin;
	const Quark ingen_Internal;
	const Quark ingen_Node;
	const Quark ingen_Patch;
	const Quark ingen_Port;
	const Quark ingen_broadcast;
	const Quark ingen_enabled;
	const Quark ingen_polyphonic;
	const Quark ingen_polyphony;
	const Quark ingen_selected;
	const Quark ingen_value;
	const Quark ingenui_canvas_x;
	const Quark ingenui_canvas_y;
	const Quark lv2_Plugin;
	const Quark lv2_index;
	const Quark lv2_maximum;
	const Quark lv2_minimum;
	const Quark lv2_name;
	const Quark lv2_symbol;
	const Quark lv2_toggled;
	const Quark midi_event;
	const Quark object_class_bool;
	const Quark object_class_float32;
	const Quark object_class_int32;
	const Quark object_class_string;
	const Quark object_class_vector;
	const Quark object_transfer;
	const Quark rdf_instanceOf;
	const Quark rdf_type;
	const Quark string_transfer;
	const Quark ui_format_events;
};


} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_LV2URIMAP_HPP
