/* This file is part of redlandmm.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include <iostream>
#include <sstream>
#include <cassert>
#include <cstring>
#include <map>
#include "redlandmm/World.hpp"
#include "redlandmm/Model.hpp"
#include "redlandmm/Node.hpp"

#define U(x) ((const unsigned char*)(x))

using namespace std;

namespace Redland {


//static const char* const RDF_LANG = "rdfxml-abbrev";
static const char* const RDF_LANG = "turtle";


/** Create an empty in-memory RDF model.
 */
Model::Model(World& world)
	: _world(world)
	, _base(world, Node::RESOURCE, ".")
	, _serialiser(NULL)
{
	Glib::Mutex::Lock lock(world.mutex());
	_storage = librdf_new_storage(_world.world(), "trees", NULL, NULL);
	if (!_storage)
		_storage = librdf_new_storage(_world.world(), "hashes", NULL, "hash-type='memory'");
	_c_obj = librdf_new_model(_world.world(), _storage, NULL);
}


/** Load a model from a URI (local file or remote).
 */
Model::Model(World& world, const Glib::ustring& data_uri, Glib::ustring base_uri)
	: _world(world)
	, _base(world, Node::RESOURCE, base_uri == "" ? "." : base_uri)
	, _serialiser(NULL)
{
	Glib::Mutex::Lock lock(world.mutex());
	_storage = librdf_new_storage(_world.world(), "trees", NULL, NULL);
	if (!_storage)
		_storage = librdf_new_storage(_world.world(), "hashes", NULL, "hash-type='memory'");
	_c_obj = librdf_new_model(_world.world(), _storage, NULL);

	librdf_uri* uri = librdf_new_uri(world.world(), (const unsigned char*)data_uri.c_str());

	if (uri) {
		librdf_parser* parser = librdf_new_parser(world.world(), "guess", NULL, NULL);
		// FIXME: locale kludges to work around librdf bug
		char* locale = strdup(setlocale(LC_NUMERIC, NULL));
		setlocale(LC_NUMERIC, "POSIX");
		librdf_parser_parse_into_model(parser, uri, _base.get_uri(), _c_obj);
		setlocale(LC_NUMERIC, locale);
		free(locale);
		librdf_free_parser(parser);
	} else {
		cerr << "Unable to create URI " << data_uri << endl;
	}

	if (uri)
		librdf_free_uri(uri);

	/*cout << endl << "Loaded model from " << data_uri << ":" << endl;
	serialize_to_file_handle(stdout);
	cout << endl << endl;*/
}


/** Load a model from a string.
 */
Model::Model(World& world, const char* str, size_t len, Glib::ustring base_uri)
	: _world(world)
	, _base(world, Node::RESOURCE, base_uri == "" ? "/" : base_uri)
	, _serialiser(NULL)
{
	Glib::Mutex::Lock lock(world.mutex());
	_storage = librdf_new_storage(_world.world(), "trees", NULL, NULL);
	if (!_storage)
		_storage = librdf_new_storage(_world.world(), "hashes", NULL, "hash-type='memory'");
	_c_obj = librdf_new_model(_world.world(), _storage, NULL);

	librdf_parser* parser = librdf_new_parser(world.world(), "turtle", NULL, NULL);
	char* locale = strdup(setlocale(LC_NUMERIC, NULL));
	setlocale(LC_NUMERIC, "POSIX");
	librdf_parser_parse_counted_string_into_model(parser, (const unsigned char*)str, len,
			_base.get_uri(), _c_obj);
	setlocale(LC_NUMERIC, locale);
	free(locale);
	librdf_free_parser(parser);
}


Model::~Model()
{
	Glib::Mutex::Lock lock(_world.mutex());

	if (_serialiser)
		librdf_free_serializer(_serialiser);

	librdf_free_model(_c_obj);
	librdf_free_storage(_storage);
}


void
Model::set_base_uri(const Glib::ustring& uri)
{
	if (uri == "")
		return;

	Glib::Mutex::Lock lock(_world.mutex());

	assert(uri.find(":") != string::npos);
	//assert(uri.substr(uri.find(":")+1).find(":") == string::npos);
	cout << "[Model] Base URI = " << uri << endl;
	_base = Node(_world, Node::RESOURCE, uri);
}


void
Model::setup_prefixes()
{
	assert(_serialiser);

	for (Namespaces::const_iterator i = _world.prefixes().begin(); i != _world.prefixes().end(); ++i) {
		librdf_serializer_set_namespace(_serialiser,
			librdf_new_uri(_world.world(), U(i->second.c_str())), i->first.c_str());
	}

	/* Don't write @base directive */
	librdf_serializer_set_feature(_serialiser,
			librdf_new_uri(_world.world(), U("http://feature.librdf.org/raptor-writeBaseURI")),
			Node(_world, Node::LITERAL, "0").get_node());

	/* Write relative URIs wherever possible */
	librdf_serializer_set_feature(_serialiser,
			librdf_new_uri(_world.world(), U("http://feature.librdf.org/raptor-relativeURIs")),
			Node(_world, Node::LITERAL, "1").get_node());
}


/** Begin a serialization to a C file handle.
 *
 * This must be called before any write methods.
 */
void
Model::serialise_to_file_handle(FILE* fd)
{
	Glib::Mutex::Lock lock(_world.mutex());

	_serialiser = librdf_new_serializer(_world.world(), RDF_LANG, NULL, NULL);

	setup_prefixes();
	librdf_serializer_serialize_model_to_file_handle(
            _serialiser, fd, _base.get_uri(), _c_obj);
	librdf_free_serializer(_serialiser);
	_serialiser = NULL;
}


/** Begin a serialization to a file.
 *
 * \a uri must be a local (file:) URI.
 *
 * This must be called before any write methods.
 */
void
Model::serialise_to_file(const Glib::ustring& uri_str)
{
	Glib::Mutex::Lock lock(_world.mutex());

	librdf_uri* uri = librdf_new_uri(_world.world(), (const unsigned char*)uri_str.c_str());
	if (uri && librdf_uri_is_file_uri(uri)) {
		_serialiser = librdf_new_serializer(_world.world(), RDF_LANG, NULL, NULL);
		setup_prefixes();
		librdf_serializer_serialize_model_to_file(
                _serialiser, librdf_uri_to_filename(uri), _base.get_uri(), _c_obj);
		librdf_free_serializer(_serialiser);
		_serialiser = NULL;
	}
	librdf_free_uri(uri);
}


/** Begin a serialization to a string.
 *
 * This must be called before any write methods.
 *
 * The results of the serialization will be returned by the finish() method after
 * the desired objects have been serialized.
 */
char*
Model::serialise_to_string()
{
	Glib::Mutex::Lock lock(_world.mutex());

	_serialiser = librdf_new_serializer(_world.world(), RDF_LANG, NULL, NULL);
	setup_prefixes();

	unsigned char* c_str = librdf_serializer_serialize_model_to_string(
			_serialiser, _base.get_uri(), _c_obj);

	librdf_free_serializer(_serialiser);
	_serialiser = NULL;

	return (char*)c_str;
}


void
Model::add_statement(const Node& subject,
                     const Node& predicate,
                     const Node& object)
{
	Glib::Mutex::Lock lock(_world.mutex());

	assert(subject.get_node());
	assert(predicate.get_node());

	if (!object.get_node()) {
		cerr << "WARNING: Object node is nil, statement skipped" << endl;
		return;
	}

	librdf_statement* triple = librdf_new_statement_from_nodes(_world.world(),
			subject.get_node(), predicate.get_node(), object.get_node());

	librdf_model_add_statement(_c_obj, triple);
}


void
Model::add_statement(const Node&   subject,
                     const string& predicate_id,
                     const Node&   object)
{
	Glib::Mutex::Lock lock(_world.mutex());

	if (!object.get_node()) {
		cerr << "WARNING: Object node is nil, statement skipped" << endl;
		return;
	}

	const string predicate_uri = _world.expand_uri(predicate_id);
	librdf_node* predicate = librdf_new_node_from_uri_string(_world.world(),
			(const unsigned char*)predicate_uri.c_str());

	librdf_statement* triple = librdf_new_statement_from_nodes(_world.world(),
			subject.get_node(), predicate, object.get_node());

	librdf_model_add_statement(_c_obj, triple);
}


void
Model::add_statement(librdf_statement* statement)
{
	Glib::Mutex::Lock lock(_world.mutex());
	librdf_model_add_statement(_c_obj, statement);
}


#define NS_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_CS "http://purl.org/vocab/changeset/schema#"


Model::Delta::Delta(const Model& from, const Model& to)
	: _additions(new Model(from.world()))
	, _removals(new Model(from.world()))
{
	assert(from.world().c_obj() == to.world().c_obj());
	librdf_model* a = from._c_obj;
	librdf_model* b = to._c_obj;

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
	Change(Node* c, bool addition, Node* n)
		: changeset(c)
		, additions(addition ? n : 0)
		, removals(addition  ? 0 : n)
	{}
	Node* get(bool addition) { return addition ? additions : removals; }
	Node* changeset;
	Node* additions;
	Node* removals;
};

typedef std::map<librdf_node*, Change*> ChangeSets;

static void
serialise_changes(World& world, Model& model, ChangeSets& changes,
		bool insertion, Node& root, librdf_stream* stream)
{
	typedef pair<Node*, Node*> Property;
	typedef set<Property>      Properties;
	for ( ; !librdf_stream_end(stream); librdf_stream_next(stream)) {
		librdf_statement*    t = librdf_stream_get_object(stream);
		librdf_node*         s = librdf_statement_get_subject(t);
		librdf_node*         p = librdf_statement_get_predicate(t);
		librdf_node*         o = librdf_statement_get_object(t);
		ChangeSets::iterator c = changes.find(s);
		Change* change  = 0;
		Node*   node    = 0;
		Node*   subject = new Node(world, s);
		if (subject->type() == Node::RESOURCE) {
			if (c != changes.end()) {
				change = c->second;
				Node** node_ptr = insertion ? &c->second->additions : &c->second->removals;
				if (!(node = *node_ptr))
					node = (*node_ptr = new Node(world));
			} else {
				node = new Node(world);
				change = new Change(new Node(world), insertion, node);
				changes.insert(make_pair(s, change));
			}

			Node* object = new Node(world, o);
			if (object->type() == Node::BLANK) {
				ChangeSets::iterator m = changes.find(o);
				if (m == changes.end()) {
					object = new Node(world);
					changes.insert(make_pair(o, new Change(NULL, insertion, object)));
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
				c = changes.insert(make_pair(s, new Change(NULL, insertion, new Node(world)))).first;
			Node* subject = c->second->get(insertion);
			model.add_statement(*subject, Node(world, p), Node(world, o));
		}
	}
}


void
Model::Delta::serialise(Model& model, const std::string& lang, const std::string& uri) const
{
	World& world = _additions->world();
	Node root(world, Node::RESOURCE, ".");
	world.add_prefix("cs", NS_CS);

	ChangeSets changes;

	serialise_changes(world, model, changes, true, root,
			librdf_model_as_stream(_additions->c_obj()));

	serialise_changes(world, model, changes, false, root,
			librdf_model_as_stream(_removals->c_obj()));
}


void
Model::Delta::apply(const Model& from, Model& to) const
{
}


} // namespace Redland

