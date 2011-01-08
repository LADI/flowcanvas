/* This file is part of Lolep.
 * Copyright (C) 2010 David Robillard <http://drobilla.net>
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <cassert>
#include <set>
#include <sstream>

#include <raptor.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom-helpers.h"
#include "lv2/lv2plug.in/ns/ext/contexts/contexts.h"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"

#include "LV2Extensions.hpp"
#include "LV2Plugin.hpp"
#include "lolep.hpp"

#define NS_XSD "http://www.w3.org/2001/XMLSchema#"

using namespace std;

class RDFRead;
typedef LV2::Plugin<
	RDFRead,
	LV2::Ext::UriMap<true>,
	LV2::Ext::UriUnmap<true>,
	LV2::Ext::MessageContext<false>,
	LV2::Ext::ResizePort<true>
> RDFReadBase;

#define FOREACH(T_i, i, col)  for (T_i i = col.begin();  (i) != col.end();  ++(i))
#define FOREACHP(T_i, i, col) for (T_i i = col->begin(); (i) != col->end(); ++(i))

/** ID of an Object (Resource or Blank).
 * This struct is binary compatible with the complete header of an LV2_Object,
 * so the ObjectID keys and LV2_Atom values of the model's maps can be compared.
 */
struct ObjectID {
	ObjectID() { init(0, 0, 0); }
	explicit ObjectID(uint32_t t, uint32_t c, uint32_t i) { init(t, c, i); }

	inline void init(uint32_t t, uint32_t c, uint32_t i) {
		assert(t != 1);
		type    = t;
		size    = sizeof(LV2_Atom) + sizeof(LV2_Object);
		context = c;
		id      = i;
	}

	inline bool operator<(const ObjectID& rhs) const {
		return (type < rhs.type)
			|| ((type == rhs.type)
			    && ((context < rhs.context)
			        || ((context == rhs.context)
			            && (id < rhs.id))));
	};

	uint16_t type;    // LV2_Atom   field 0
	uint16_t size;    // LV2_Atom   field 1
	uint32_t context; // LV2_Object field 0
	uint32_t id;      // LV2_Object field 1
};

class RDFRead : public RDFReadBase
{
public:
	typedef std::map<std::string, ObjectID>    BlankIDs;
	typedef std::map<ObjectID, uint32_t>       BlankRefs;
	typedef std::multimap<uint32_t, LV2_Atom*> Properties;
	typedef std::map<ObjectID, Properties*>    Objects;

	struct Model {
		Model(RDFRead* instance) : me(instance), next_id(1) {}

		ObjectID blank_id(const char* id_str, uint32_t context) {
			BlankIDs::iterator i = blank_ids.find(id_str);
			if (i != blank_ids.end()) {
				return i->second;
			} else {
				ObjectID id(me->atom_Blank, context, next_id++);
				blank_ids.insert(make_pair(id_str, id));
				return id;
			}
		}
		
		void abbreviate() {
			FOREACH(BlankRefs::const_iterator, r, blank_refs) {
				if (r->second == 1) {
					Objects::iterator b = blanks.find(r->first);
					inline_blanks.insert(*b);
					blanks.erase(b);
				}
			}
		}
		
		bool single_object() const { return resources.size() + blanks.size() == 1; }
		
		size_t object_atom_size(const Properties* properties) const {
			size_t size = sizeof(LV2_Atom) + sizeof(LV2_Object);
			FOREACHP (Properties::const_iterator, p, properties) {
				size += sizeof(uint32_t); // key
				if (p->second->type == me->atom_BlankID) {
					ObjectID blank_id(me->atom_Blank, 0, ((ObjectID*)p->second)->id);
					Objects::const_iterator i = inline_blanks.find(blank_id);
					if (i != inline_blanks.end()) {
						size += lv2_atom_pad_size(object_atom_size(i->second));
						continue;
					}
				}
				size += sizeof(LV2_Atom) + lv2_atom_pad_size(p->second->size);
			}
			return size;
		}

		size_t atom_size() const {
			size_t size = 0;
			FOREACH (Objects::const_iterator, i, resources)
				size += object_atom_size(i->second);
			FOREACH (Objects::const_iterator, i, blanks)
				size += object_atom_size(i->second);
			return size;
		}

		void write_property(uint8_t** head, Properties::const_iterator p) const {
			// Write key
			LV2_Atom_Property* property = *(LV2_Atom_Property**)head;
			property->key = p->first;
			*head += sizeof(uint32_t);

			// Write inline Blank (if applicable)
			if (p->second->type == me->atom_BlankID) {
				ObjectID blank_id(me->atom_Blank, 0, ((ObjectID*)p->second)->id);
				Objects::const_iterator i = inline_blanks.find(blank_id);
				if (i != inline_blanks.end()) {
					write_object(head, me->atom_Blank, i);
					return;
				}
			}

			// Write value
			const size_t value_size = sizeof(LV2_Atom) + p->second->size;
			memcpy(*head, p->second, value_size);
			*head += lv2_atom_pad_size(value_size);
		}
		
		void write_object(uint8_t** head, uint32_t type, Objects::const_iterator i) const {
			// Write Atom header
			LV2_Atom* const atom = *(LV2_Atom**)head;
			atom->type = type;
			*head += sizeof(LV2_Atom);

			// Write Object header
			LV2_Object* const object = (LV2_Object* const)atom->body;
			object->context = 0;
			object->id = i->first.id;
			*head += sizeof(LV2_Object);
			
			FOREACHP (Properties::const_iterator, p, i->second) {
				write_property(head, p); 
			}
			atom->size = *head - (uint8_t*)object;
		}

		void write(uint8_t** head) const {
			FOREACH (Objects::const_iterator, i, resources)
				write_object(head, me->atom_Resource, i);
			FOREACH (Objects::const_iterator, i, blanks)
				write_object(head, me->atom_Blank, i);
		}

		RDFRead*  me;            ///< Parent plugin instance
		uint32_t  next_id;       ///< Next blank node ID
		BlankIDs  blank_ids;     ///< Map of string blank IDs to numeric blank IDs
		BlankRefs blank_refs;    ///< Count of references to blanks in triple objects
		Objects   resources;     ///< Resources (named atom:Objects)
		Objects   blanks;        ///< Blanks (anonymous atom:Objects)
		Objects   inline_blanks; ///< Inline Blanks (populated in abbreviate())
	};

	RDFRead(double rate, const char* bundle, const LV2::Feature* const* features)
		: RDFReadBase(2)
		, atom_Blank(uri_to_id(NULL,    LV2_ATOM_URI "#Blank"))
		, atom_BlankID(uri_to_id(NULL,  LV2_ATOM_URI "#BlankID"))
		, atom_Bool(uri_to_id(NULL,     LV2_ATOM_URI "#Bool"))
		, atom_Float(uri_to_id(NULL,    LV2_ATOM_URI "#Float32"))
		, atom_ID(uri_to_id(NULL,       LV2_ATOM_URI "#ID"))
		, atom_Int32(uri_to_id(NULL,    LV2_ATOM_URI "#Int32"))
		, atom_Model(uri_to_id(NULL,    LV2_ATOM_URI "#Model"))
		, atom_Resource(uri_to_id(NULL, LV2_ATOM_URI "#Resource"))
		, atom_String(uri_to_id(NULL,   LV2_ATOM_URI "#String"))
		, atom_Tuple(uri_to_id(NULL,    LV2_ATOM_URI "#Tuple"))
	{
	}

	static void rdf_statement_handler(void* data, const raptor_statement* statement)
	{
		Model&   model = *reinterpret_cast<Model*>(data);
		RDFRead* me    = model.me;

		// Find properties map for subject
		Properties* properties = NULL;
		switch (statement->subject_type) {
		case RAPTOR_IDENTIFIER_TYPE_RESOURCE: {
			const char*     str = (const char*)raptor_uri_as_string((raptor_uri*)statement->subject);
			ObjectID        id(me->atom_Resource, 0, me->uri_to_id(NULL, str));
			Objects::iterator s = model.resources.find(id);
			if (s == model.resources.end())
				s = model.resources.insert(make_pair(id, new Properties())).first;
			properties = s->second;
			break;
		}
		case RAPTOR_IDENTIFIER_TYPE_ANONYMOUS: {
			const char*       str = (const char*)statement->subject;
			ObjectID          id  = model.blank_id(str, 0);
			Objects::iterator s = model.blanks.find(id);
			if (s == model.blanks.end())
				s = model.blanks.insert(make_pair(id, new Properties())).first;
			properties = s->second;
			break;
		}
		default:
			fprintf(stderr, "error: Subject is neither a resource or anonymous\n");
			return;
		}

		// Map predicate URI to ID
		uint32_t predicate = 0;
		switch (statement->predicate_type) {
		case RAPTOR_IDENTIFIER_TYPE_RESOURCE: {
			const char* str = (const char*)raptor_uri_as_string((raptor_uri*)statement->predicate);
			predicate = me->uri_to_id(NULL, str);
			break;
		}
		default:
			fprintf(stderr, "error: Predicate is not a resource\n");
			return;
		}

		// Copy object as an LV2_Object
		LV2_Atom* object = NULL;
		switch (statement->object_type) {
		case RAPTOR_IDENTIFIER_TYPE_RESOURCE: {
			const char*    str = (const char*)raptor_uri_as_string((raptor_uri*)statement->object);
			const uint32_t id  = me->uri_to_id(NULL, str);
			object = (LV2_Atom*)malloc(sizeof(LV2_Atom) + sizeof(uint32_t));
			object->type = me->atom_ID;
			object->size = sizeof(uint32_t);
			*(uint32_t*)object->body = id;
			break;
		}
		case RAPTOR_IDENTIFIER_TYPE_ANONYMOUS: {
			ObjectID id = model.blank_id((const char*)statement->object, 0);
			object = (LV2_Atom*)malloc(sizeof(LV2_Atom) + sizeof(LV2_Object));
			object->type = me->atom_BlankID;
			object->size = sizeof(uint32_t);
			LV2_Object* body = (LV2_Object*)object->body;
			body->context = 0;
			body->id = id.id;
			BlankRefs::iterator i = model.blank_refs.find(*(ObjectID*)object);
			if (i != model.blank_refs.end())
				++i->second;
			else
				model.blank_refs.insert(make_pair(id, 1));
			break;
		}
		case RAPTOR_IDENTIFIER_TYPE_LITERAL: {
			raptor_uri* datatype_uri = statement->object_literal_datatype;
			if (!strcmp((const char*)raptor_uri_as_string(datatype_uri), NS_XSD "integer")) {
				std::locale c_locale("C");
				std::stringstream ss((const char*)statement->object);
				ss.imbue(c_locale);
				int32_t i = 0;
				ss >> i;
				object = (LV2_Atom*)malloc(sizeof(LV2_Atom) + sizeof(int32_t));
				object->type = me->atom_Int32;
				object->size = sizeof(int32_t);
				*(int32_t*)object->body = i;
			} else if (!strcmp((const char*)raptor_uri_as_string(datatype_uri), NS_XSD "decimal")) {
				std::locale c_locale("C");
				std::stringstream ss((const char*)statement->object);
				ss.imbue(c_locale);
				float f = 0;
				ss >> f;
				object = (LV2_Atom*)malloc(sizeof(LV2_Atom) + sizeof(float));
				object->type = me->atom_Float;
				object->size = sizeof(float);
				*(float*)object->body = f;
			} else {
				const char*  str = (const char*)statement->object;
				const size_t len = strlen(str);
				object = (LV2_Atom*)malloc(sizeof(LV2_Atom) + sizeof(uint32_t) + len);
				object->type = me->atom_String;
				object->size = sizeof(uint32_t) + len + 1;
				LV2_Atom_String* lv2_str = (LV2_Atom_String*)object->body;
				lv2_str->lang = 0;
				memcpy(lv2_str->str, str, len + 1);
			}
			break;
		}
		default:
			fprintf(stderr, "error: Unknown object type\n");
			break;
		}

		properties->insert(make_pair(predicate, object));
	}

	uint32_t message_run(const void* valid_inputs,
	                     void*       valid_outputs)
	{
		LV2_Atom*   in  = p<LV2_Atom>(0);
		LV2_Atom*   out = p<LV2_Atom>(1);
		const char* str = (char*)in->body;

		out->type = 0;
		out->size = 0;
		lv2_contexts_unset_port_valid(valid_outputs, 1);
		
		if (in->type != atom_String)
			return 0;

		Model model(this);

		raptor_init();
		raptor_parser* p = raptor_new_parser("turtle");
		raptor_set_statement_handler(p, &model, rdf_statement_handler);
		raptor_uri* base_uri = raptor_new_uri((const uint8_t*)"http://exaple.org/base");
		raptor_start_parse(p, base_uri);
		raptor_parse_chunk(p, (const uint8_t*)str, in->size - 1, 1);
		raptor_free_uri(base_uri);
		raptor_free_parser(p);
		raptor_finish();

		model.abbreviate();

		const size_t atom_size = model.single_object()
			? model.atom_size()
			: sizeof(LV2_Atom) + model.atom_size(); // LV2_Model header
		
		if (!resize_port(this, 1, atom_size)) {
			cerr << "Failed to resize port " << 1 << " to " << atom_size << "bytes" << endl;
			return NULL;
		}
		
		out = this->p<LV2_Atom>(1);
		uint8_t* head = (uint8_t*)out;

		// Single top-level object, emit it directly
		if (model.single_object()) {
			model.write(&head);
			const size_t write_size = head - (uint8_t*)out;
			if (write_size != atom_size) {
				fprintf(stderr, "ERROR: write size %zu != %zu\n", write_size, atom_size);
				return 0;
			}

		// Several top-level objects, emit model
		} else {
			out->type = atom_Model;
			out->size = atom_size;
			*head += sizeof(LV2_Object);

			model.write(&head);
		}

		lv2_contexts_set_port_valid(valid_outputs, 1);
		
		return 0;
	}

	const uint32_t atom_Blank;
	const uint32_t atom_BlankID;
	const uint32_t atom_Bool;
	const uint32_t atom_Float;
	const uint32_t atom_ID;
	const uint32_t atom_Int32;
	const uint32_t atom_Model;
	const uint32_t atom_Resource;
	const uint32_t atom_String;
	const uint32_t atom_Tuple;
};
	
static const unsigned plugin_class = RDFRead::register_class(LOLEP_URI "/rdf-read");
