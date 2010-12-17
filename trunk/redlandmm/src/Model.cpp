/* This file is part of redlandmm.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#define CUC(x) ((const unsigned char*)(x))

using namespace std;

namespace Redland {


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

	librdf_uri* uri = librdf_new_uri(world.world(), CUC(data_uri.c_str()));

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
Model::Model(World& world, const char* str, size_t len, Glib::ustring base_uri, const std::string lang)
	: _world(world)
	, _base(world, Node::RESOURCE, base_uri == "" ? "/" : base_uri)
	, _serialiser(NULL)
{
	Glib::Mutex::Lock lock(world.mutex());

	_storage = librdf_new_storage(_world.world(), "trees", NULL, NULL);
	if (!_storage)
		_storage = librdf_new_storage(_world.world(), "hashes", NULL, "hash-type='memory'");

	_c_obj = librdf_new_model(_world.world(), _storage, NULL);

	librdf_parser* parser = librdf_new_parser(world.world(), lang.c_str(), NULL, NULL);
	librdf_parser_parse_counted_string_into_model(
		parser, CUC(str), len,_base.get_uri(), _c_obj);
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
			librdf_new_uri(_world.world(), CUC(i->second.c_str())), i->first.c_str());
	}

	/* Don't write @base directive */
	librdf_serializer_set_feature(_serialiser,
			librdf_new_uri(_world.world(), CUC("http://feature.librdf.org/raptor-writeBaseURI")),
			Node(_world, Node::LITERAL, "0").get_node());

	/* Write relative URIs wherever possible */
	librdf_serializer_set_feature(_serialiser,
			librdf_new_uri(_world.world(), CUC("http://feature.librdf.org/raptor-relativeURIs")),
			Node(_world, Node::LITERAL, "1").get_node());
}


/** Begin a serialization to a C file handle.
 *
 * This must be called before any write methods.
 */
void
Model::serialise_to_file_handle(FILE* fd, const char* lang)
{
	Glib::Mutex::Lock lock(_world.mutex());

	_serialiser = librdf_new_serializer(_world.world(), lang, NULL, NULL);

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
Model::serialise_to_file(const Glib::ustring& uri_str, const char* lang)
{
	Glib::Mutex::Lock lock(_world.mutex());

	librdf_uri* uri = librdf_new_uri(_world.world(), CUC(uri_str.c_str()));
	if (uri && librdf_uri_is_file_uri(uri)) {
		_serialiser = librdf_new_serializer(_world.world(), lang, NULL, NULL);
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
Model::serialise_to_string(const char* lang)
{
	Glib::Mutex::Lock lock(_world.mutex());

	_serialiser = librdf_new_serializer(_world.world(), lang, NULL, NULL);
	setup_prefixes();

	unsigned char* c_str = librdf_serializer_serialize_model_to_string(
			_serialiser, _base.get_uri(), _c_obj);

	librdf_free_serializer(_serialiser);
	_serialiser = NULL;

	return (char*)(c_str);
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

	assert(subject.get_node());
	if (!object.get_node()) {
		cerr << "WARNING: Object node is nil, statement skipped" << endl;
		return;
	}

	const string predicate_uri = _world.expand_uri(predicate_id);
	librdf_node* predicate = librdf_new_node_from_uri_string(_world.world(),
			CUC(predicate_uri.c_str()));

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


} // namespace Redland

