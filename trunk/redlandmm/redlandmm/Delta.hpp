/* This file is part of redlandmm.
 * Copyright (C) 2007-2010 David Robillard <http://drobilla.net>
 *
 * redlandmm is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * redlandmm is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef REDLANDMM_DELTA_HPP
#define REDLANDMM_DELTA_HPP

#include <map>
#include <set>
#include <string>
#include <utility>

#include <redland.h>

#include "redlandmm/Model.hpp"

namespace Redland {

/** A difference between two models */
class Delta {
public:
	inline Delta(const Model& from, const Model& to);

	inline void serialise(Model& model, const std::string& lang, const std::string& uri) const;

private:
	Model* _additions;
	Model* _removals;
};


inline Delta::Delta(const Model& from, const Model& to)
	: _additions(new Model(from.world()))
	, _removals(new Model(from.world()))
{
	assert(from.world().c_obj() == to.world().c_obj());
	librdf_model* a = const_cast<librdf_model*>(from.c_obj());
	librdf_model* b = const_cast<librdf_model*>(to.c_obj());

	// Statement is in a but not in b: deletion
	for (librdf_stream* s = librdf_model_as_stream(a);
			!librdf_stream_end(s); librdf_stream_next(s)) {
		librdf_statement* t = librdf_stream_get_object(s);
		if (!librdf_model_contains_statement(b, t))
			_removals->add_statement(t);
	}

	// Statement is in b but not in a: insertion
	for (librdf_stream* s = librdf_model_as_stream(b);
			!librdf_stream_end(s); librdf_stream_next(s)) {
		librdf_statement* t = librdf_stream_get_object(s);
		if (!librdf_model_contains_statement(a, t))
			_additions->add_statement(t);
	}
}

struct Change {
	inline Change(Node* c, bool addition, Node* n)
		: changeset(c)
		, additions(addition ? n : 0)
		, removals(addition  ? 0 : n)
	{}
	inline Node* get(bool addition) { return addition ? additions : removals; }

	Node* changeset;
	Node* additions;
	Node* removals;
};

typedef std::map<librdf_node*, Change*> ChangeSets;

static inline void
serialise_changes(World& world, Model& model, ChangeSets& changes,
		bool insertion, Node& root, librdf_stream* stream)
{
	typedef std::pair<Node*, Node*> Property;
	typedef std::set<Property>      Properties;
	for ( ; !librdf_stream_end(stream); librdf_stream_next(stream)) {
		librdf_statement*    t       = librdf_stream_get_object(stream);
		librdf_node*         s       = librdf_statement_get_subject(t);
		librdf_node*         p       = librdf_statement_get_predicate(t);
		librdf_node*         o       = librdf_statement_get_object(t);
		ChangeSets::iterator c       = changes.find(s);
		Change*              change  = 0;
		Node*                node    = 0;
		Node*                subject = new Node(world, s);
		if (subject->type() == Node::RESOURCE) {
			if (c != changes.end()) {
				change = c->second;
				Node** node_ptr = insertion ? &c->second->additions : &c->second->removals;
				if (!(node = *node_ptr))
					node = (*node_ptr = new Node(world));
			} else {
				node = new Node(world);
				change = new Change(new Node(world), insertion, node);
				changes.insert(std::make_pair(s, change));
			}

			Node* object = new Node(world, o);
			if (object->type() == Node::BLANK) {
				ChangeSets::iterator m = changes.find(o);
				if (m == changes.end()) {
					object = new Node(world);
					changes.insert(std::make_pair(o, new Change(NULL, insertion, object)));
				} else {
					object = m->second->get(insertion);
				}
			} else {
				if (insertion) {
					model.add_statement(*change->changeset, "cs:addition", *node);
				} else {
					model.add_statement(*change->changeset, "cs:removal", *node);
				}
			}

			model.add_statement(*change->changeset, "cs:subjectOfChange", *subject);
			model.add_statement(*node, Node(world, p), *object);
		} else {
			if (c == changes.end())
				c = changes.insert(std::make_pair(
					                   s,
					                   new Change(NULL, insertion, new Node(world)))).first;
			Node* subject = c->second->get(insertion);
			model.add_statement(*subject, Node(world, p), Node(world, o));
		}
	}
}

inline void
Delta::serialise(Model& model, const std::string& lang, const std::string& uri) const
{
	World& world = _additions->world();
	Node root(world, Node::RESOURCE, ".");
	world.add_prefix("cs", "http://purl.org/vocab/changeset/schema#");

	ChangeSets changes;

	serialise_changes(world, model, changes, true, root,
			librdf_model_as_stream(_additions->c_obj()));

	serialise_changes(world, model, changes, false, root,
			librdf_model_as_stream(_removals->c_obj()));
}

} // namespace Redland

#endif // REDLANDMM_DELTA_HPP
