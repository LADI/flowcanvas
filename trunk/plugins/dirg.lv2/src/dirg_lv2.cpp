/* This file is part of Dirg.
 * Copyright (C) 2010 David Robillard <http://drobilla.net>
 *
 * Dirg is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Dirg is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <ctype.h>
#include <stdio.h>

#include <boost/thread.hpp>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom-helpers.h"
#include "lv2/lv2plug.in/ns/ext/contexts/contexts.h"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"

#include "LV2Extensions.hpp"
#include "LV2Plugin.hpp"
#include "UI.hpp"
#include "dirg_internal.hpp"

#define DIRG_URI     "http://drobilla.net/plugins/dirg"
#define LV2_MIDI_URI "http://lv2plug.in/ns/ext/midi"

class Dirg;
typedef LV2::Plugin<
	Dirg,
	LV2::Ext::UriMap<true>,
	LV2::Ext::UriUnmap<true>,
	LV2::Ext::MessageContext<false>,
	LV2::Ext::RequestRunFeature<true>,
	LV2::Ext::ResizePort<true>
> DirgBase;

class Dirg : public DirgBase
{
public:
	Dirg(double rate, const char* bundle, const LV2::Feature* const* features)
		: DirgBase(2)
		, atom_Blank(uri_to_id(NULL,         LV2_ATOM_URI "#Blank"))
		, atom_Resource(uri_to_id(NULL,      LV2_ATOM_URI "#Resource"))
		, atom_Bool(uri_to_id(NULL,          LV2_ATOM_URI "#Bool"))
		, atom_ID(uri_to_id(NULL,            LV2_ATOM_URI "#ID"))
		, atom_Int32(uri_to_id(NULL,         LV2_ATOM_URI "#Int32"))
		, atom_Object(uri_to_id(NULL,        LV2_ATOM_URI "#Object"))
		, atom_String(uri_to_id(NULL,        LV2_ATOM_URI "#String"))
		, ctx_MessageContext(uri_to_id(NULL, LV2_CONTEXTS_URI "#MessageContext"))
		, dirg_Press(uri_to_id(NULL,         DIRG_URI "#Press"))
		, dirg_column(uri_to_id(NULL,        DIRG_URI "#column"))
		, dirg_group(uri_to_id(NULL,         DIRG_URI "#group"))
		, dirg_row(uri_to_id(NULL,           DIRG_URI "#row"))
		, rdf_type(uri_to_id(NULL, "http://www.w3.org/1999/02/22-rdf-syntax-ns#type"))
	{
		web_ui = dirg_new_web_ui(state, bundle);
		pad_ui = dirg_new_launchpad_ui(state, bundle);
	}

	~Dirg() {
		web_ui.reset();
		pad_ui.reset();
	}

	void activate() {
		if (web_ui) {
			web_ui->button_pressed.connect(sigc::mem_fun(this, &Dirg::button_pressed));
			web_ui->activate();
		}
		if (pad_ui) {
			pad_ui->button_pressed.connect(sigc::mem_fun(this, &Dirg::button_pressed));
			pad_ui->activate();
		}
	}

	void deactivate() {
		web_ui->deactivate();
		pad_ui->deactivate();
	}
	
	uint32_t message_run(const void* valid_inputs,
	                     void*       valid_outputs)
	{
		boost::mutex::scoped_lock lock(mutex);

		const LV2_Atom* const in = p<LV2_Atom>(0);

		// Read input messages and update UI state accordingly
		if (in->type == atom_Resource || in->type == atom_Blank) {
			LV2_Object_Query q[] = {
				{ rdf_type,    NULL },
				{ dirg_group,  NULL },
				{ dirg_row,    NULL },
				{ dirg_column, NULL },
				{ 0, NULL }
			};

			lv2_object_query(in, q);

			if (q[0].value->type == atom_ID && *(uint32_t*)q[0].value->body == dirg_Press) {
				const LV2_Atom* group = q[1].value;
				const LV2_Atom* row   = q[2].value;
				const LV2_Atom* col   = q[3].value;
				if (group->type == atom_Int32 && row->type == atom_Int32 && col->type == atom_Int32) {
					toggle_button_state(ButtonID((ButtonGroup)*(int32_t*)group->body,
					                             *(int32_t*)row->body,
					                             *(int32_t*)col->body));
				}
			}
		}

		static const size_t uri_property_size = sizeof(LV2_Atom_Property) + sizeof(int32_t);
		size_t atom_size = sizeof(LV2_Atom) + sizeof(LV2_Object) + uri_property_size;
		for (Presses::const_iterator i = presses.begin(); i != presses.end(); ++i)
			atom_size += uri_property_size * 3; // group, row, col

		if (!resize_port(this, 1, atom_size)) {
			cerr << "Failed to resize output port to " << atom_size << "bytes" << endl;
			return 0;
		}

		LV2_Atom* out = p<LV2_Atom>(1);
		out->type = atom_Blank;
		out->size = sizeof(LV2_Object);

		LV2_Object* obj = (LV2_Object*)out->body;
		obj->context = 0;
		obj->id      = 0;

		// [] rdf:type dirg:Press
		lv2_atom_append_property(
			out, rdf_type,
			atom_ID, sizeof(uint32_t), (const uint8_t*)&dirg_Press);

		for (Presses::const_iterator i = presses.begin(); i != presses.end(); ++i) {
			const int32_t group = i->group;
			const int32_t row   = i->row;
			const int32_t col   = i->col;
			lv2_atom_append_property(
				out, dirg_group,
				atom_Int32, sizeof(int32_t), (const uint8_t*)&group);
			lv2_atom_append_property(
				out, dirg_row,
				atom_Int32, sizeof(int32_t), (const uint8_t*)&row);
			lv2_atom_append_property(
				out, dirg_column,
				atom_Int32, sizeof(int32_t), (const uint8_t*)&col);
		}

		assert(sizeof(LV2_Atom) + out->size == atom_size);

		lv2_contexts_set_port_valid(valid_outputs, 1);

		presses.clear();

		return 0;
	}

	void button_pressed(ButtonID id) {
		boost::mutex::scoped_lock lock(mutex);

		// Push request for message thread and request execution
		presses.push_back(id);
		request_run(ctx_MessageContext);

		toggle_button_state(id);
	}

	void toggle_button_state(ButtonID id) {
		const ButtonState& but = state.get(id);
		const float        val = (but.value == 0.0f) ? 1.0f : 0.0f;
		state.set_colour(id, but.hue, val);
		if (web_ui)
			web_ui->set_colour(id, but.hue, val);
		if (pad_ui)
			pad_ui->set_colour(id, but.hue, val);
	}

	const uint32_t atom_Blank;
	const uint32_t atom_Resource;
	const uint32_t atom_Bool;
	const uint32_t atom_ID;
	const uint32_t atom_Int32;
	const uint32_t atom_Object;
	const uint32_t atom_String;
	const uint32_t ctx_MessageContext;
	const uint32_t dirg_Press;
	const uint32_t dirg_column;
	const uint32_t dirg_group;
	const uint32_t dirg_row;
	const uint32_t rdf_type;

	typedef std::vector<ButtonID> Presses;

	boost::mutex mutex;
	Presses      presses;

	PadState state;
	SPtr<UI> web_ui;
	SPtr<UI> pad_ui;
};

static const unsigned plugin_class = Dirg::register_class(DIRG_URI);
