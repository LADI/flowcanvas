/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility> // pair, make_pair
#include <cassert>
#include <cmath>
#include <cstdlib> // atof
#include <cstring>
#include <redland.h>
#include "Serializer.h"
#include "PatchModel.h"
#include "NodeModel.h"
#include "ConnectionModel.h"
#include "PortModel.h"
#include "PresetModel.h"
#include "ModelEngineInterface.h"
#include "PluginModel.h"
#include "util/Path.h"
#include "util/Atom.h"
#include "util/RedlandAtom.h"

#define U(x) ((const unsigned char*)(x))

using std::string; using std::vector; using std::pair;
using std::cerr; using std::cout; using std::endl;

namespace Ingen {
namespace Client {


Serializer::Serializer(CountedPtr<ModelEngineInterface> engine)
	: _world(librdf_new_world())
	, _serializer(0)
	, _patch_search_path(".")
	, _engine(engine)
{
	librdf_world_open(_world);

	//_prefixes["xsd"]       = "http://www.w3.org/2001/XMLSchema#";
	_prefixes["rdf"]       = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
	_prefixes["ingen"]     = "http://codeson.net/ns/ingen#";
	_prefixes["ingenuity"] = "http://codeson.net/ns/ingenuity#";
}


Serializer::~Serializer()
{

	librdf_free_world(_world);
}


void
Serializer::create()
{
	_serializer = librdf_new_serializer(_world, "rdfxml-abbrev", NULL, librdf_new_uri(_world, U(NS_INGEN())));
	
	setup_prefixes();
}

void
Serializer::destroy()
{
	librdf_free_serializer(_serializer);
	_serializer = NULL;
}


void
Serializer::setup_prefixes()
{
	for (map<string,string>::const_iterator i = _prefixes.begin(); i != _prefixes.end(); ++i) {
		librdf_serializer_set_namespace(_serializer,
			librdf_new_uri(_world, U(i->second.c_str())), i->first.c_str());
	}
}


/** Expands the prefix of URI, if the prefix is registered.
 *
 * If uri is not a valid URI, the empty string is returned (so invalid URIs won't be serialized).
 */
string
Serializer::expand_uri(const string& uri)
{
	// FIXME: slow, stupid
	for (map<string,string>::const_iterator i = _prefixes.begin(); i != _prefixes.end(); ++i)
		if (uri.substr(0, i->first.length()+1) == i->first + ":")
			return i->second + uri.substr(i->first.length()+1);

	/*librdf_uri* redland_uri = librdf_new_uri(_world, U(uri.c_str()));
	string ret = (redland_uri) ? uri : "";
	librdf_free_uri(redland_uri);
	return ret;*/

	// FIXME: find a correct way to validate a URI
	if (uri.find(":") == string::npos && uri.find("/") == string::npos)
		return "";
	else
		return uri;
}


/** Searches for the filename passed in the path, returning the full
 * path of the file, or the empty string if not found.
 *
 * This function tries to be as friendly a black box as possible - if the path
 * passed is an absolute path and the file is found there, it will return
 * that path, etc.
 *
 * additional_path is a list (colon delimeted as usual) of additional
 * directories to look in.  ie the directory the parent patch resides in would
 * be a good idea to pass as additional_path, in the case of a subpatch.
 */
string
Serializer::find_file(const string& filename, const string& additional_path)
{
	string search_path = additional_path + ":" + _patch_search_path;
	
	// Try to open the raw filename first
	std::ifstream is(filename.c_str(), std::ios::in);
	if (is.good()) {
		is.close();
		return filename;
	}
	
	string directory;
	string full_patch_path = "";
	
	while (search_path != "") {
		directory = search_path.substr(0, search_path.find(':'));
		if (search_path.find(':') != string::npos)
			search_path = search_path.substr(search_path.find(':')+1);
		else
			search_path = "";

		full_patch_path = directory +"/"+ filename;
		
		std::ifstream is;
		is.open(full_patch_path.c_str(), std::ios::in);
	
		if (is.good()) {
			is.close();
			return full_patch_path;
		} else {
			cerr << "[Serializer] Could not find patch file " << full_patch_path << endl;
		}
	}

	return "";
}


/** Save a patch from a PatchModel to a filename.
 *
 * The filename passed is the true filename the patch will be saved to (with no prefixing or anything
 * like that), and the patch_model's filename member will be set accordingly.
 *
 * FIXME: make this take a URI.  network loading for free.
 *
 * This will break if:
 * - The filename does not have an extension (ie contain a ".")
 * - The patch_model has no (Ingen) path
 */
void
Serializer::save_patch(CountedPtr<PatchModel> patch_model, const string& filename, bool recursive)
{
	librdf_storage* storage = librdf_new_storage(_world, "hashes", NULL, "hash-type='memory'");

	librdf_model* model = librdf_new_model(_world, storage, NULL);
	assert(model);

	const string uri = string("file://") + filename;
	add_patch_to_rdf(model, patch_model, uri);

	create();

	//librdf_serializer_serialize_model_to_file_handle(serializer, stdout, NULL, model);
	librdf_serializer_serialize_model_to_file(_serializer,
		filename.c_str(), NULL, model);

	destroy();

	librdf_storage_close(storage);
	librdf_free_storage(storage);
	librdf_free_model(model);
}


void
Serializer::add_resource_to_rdf(librdf_model* rdf,
		const string& subject_uri, const string& predicate_uri, const string& object_uri)
{

	librdf_node* subject   = librdf_new_node_from_uri_string(_world, U(subject_uri.c_str()));
	add_resource_to_rdf(rdf, subject, predicate_uri, object_uri);
}

void
Serializer::add_resource_to_rdf(librdf_model* rdf,
		librdf_node* subject, const string& predicate_uri, const string& object_uri)
{

	librdf_node* predicate = librdf_new_node_from_uri_string(_world, U(predicate_uri.c_str()));
	librdf_node* object    = librdf_new_node_from_uri_string(_world, U(object_uri.c_str()));

	librdf_model_add(rdf, subject, predicate, object);
}


void
Serializer::add_atom_to_rdf(librdf_model* rdf,
		const string& subject_uri, const string& predicate_uri, const Atom& atom)
{
	librdf_node* subject   = librdf_new_node_from_uri_string(_world, U(subject_uri.c_str()));
	librdf_node* predicate = librdf_new_node_from_uri_string(_world, U(predicate_uri.c_str()));
	librdf_node* object    = RedlandAtom::atom_to_node(_world, atom);

	librdf_model_add(rdf, subject, predicate, object);
}


void
Serializer::add_patch_to_rdf(librdf_model* rdf,
		CountedPtr<PatchModel> patch, const string& uri_unused)
{
	const string uri = "#";

	add_resource_to_rdf(rdf,
		uri.c_str(),
		NS_RDF("type"),
		NS_INGEN("Patch"));

	if (patch->path().name().length() > 0) {
		add_atom_to_rdf(rdf,
			uri.c_str(),
			NS_INGEN("name"),
			Atom(patch->path().name().c_str()));
	}

	add_atom_to_rdf(rdf,
		uri.c_str(),
		NS_INGEN("polyphony"),
		Atom((int)patch->poly()));

	for (NodeModelMap::const_iterator n = patch->nodes().begin(); n != patch->nodes().end(); ++n) {
		add_node_to_rdf(rdf, n->second, "#");
		add_resource_to_rdf(rdf, "#", NS_INGEN("node"), uri + n->second->path().name());
	}
	
	for (PortModelList::const_iterator p = patch->ports().begin(); p != patch->ports().end(); ++p) {
		add_port_to_rdf(rdf, *p, uri);
		add_resource_to_rdf(rdf, "#", NS_INGEN("port"), uri + (*p)->path().name());
	}

	for (ConnectionList::const_iterator c = patch->connections().begin(); c != patch->connections().end(); ++c) {
		add_connection_to_rdf(rdf, *c, uri);
	}
	
	//_engine->set_metadata(patch->path(), "uri", uri);
}


void
Serializer::add_node_to_rdf(librdf_model* rdf,
		CountedPtr<NodeModel> node, const string ns_prefix)
{
	const string node_uri = ns_prefix + node->path().name();

	add_resource_to_rdf(rdf,
		node_uri.c_str(),
		NS_RDF("type"),
		NS_INGEN("Node"));
	
	/*add_atom_to_rdf(rdf,
		node_uri_ref.c_str(),
		NS_INGEN("name"),
		Atom(node->path().name()));*/

	for (PortModelList::const_iterator p = node->ports().begin(); p != node->ports().end(); ++p) {
		add_port_to_rdf(rdf, *p, node_uri + "/");
		add_resource_to_rdf(rdf, node_uri, NS_INGEN("port"), node_uri + "/" + (*p)->path().name());
	}

	for (MetadataMap::const_iterator m = node->metadata().begin(); m != node->metadata().end(); ++m) {
		if (expand_uri(m->first) != "") {
			add_atom_to_rdf(rdf,
				node_uri.c_str(),
				expand_uri(m->first.c_str()).c_str(),
				m->second);
		}
	}
}


void
Serializer::add_port_to_rdf(librdf_model* rdf,
		CountedPtr<PortModel> port, const string ns_prefix)
{
	const string port_uri_ref = ns_prefix + port->path().name();

	add_resource_to_rdf(rdf,
		port_uri_ref.c_str(),
		NS_RDF("type"),
		NS_INGEN("Port"));
	
	for (MetadataMap::const_iterator m = port->metadata().begin(); m != port->metadata().end(); ++m) {
		if (expand_uri(m->first) != "") {
			add_atom_to_rdf(rdf,
				port_uri_ref.c_str(),
				expand_uri(m->first).c_str(),
				m->second);
		}
	}
}


void
Serializer::add_connection_to_rdf(librdf_model* rdf,
		CountedPtr<ConnectionModel> connection, const string ns_prefix)
{
	librdf_node* c = librdf_new_node(_world);

	const string src_port_rel_path = connection->src_port_path().substr(connection->patch_path().length());
	const string dst_port_rel_path = connection->dst_port_path().substr(connection->patch_path().length());

	librdf_statement* s = librdf_new_statement_from_nodes(_world, c,
		librdf_new_node_from_uri_string(_world, U(NS_INGEN("source"))),
		librdf_new_node_from_uri_string(_world, U((ns_prefix + src_port_rel_path).c_str())));
	librdf_model_add_statement(rdf, s);
	
	librdf_statement* d = librdf_new_statement_from_nodes(_world, c,
		librdf_new_node_from_uri_string(_world, U(NS_INGEN("destination"))),
		librdf_new_node_from_uri_string(_world, U((ns_prefix + dst_port_rel_path).c_str())));
	librdf_model_add_statement(rdf, d);
	
	add_resource_to_rdf(rdf, c, NS_RDF("type"), NS_INGEN("Connection"));
}


/** Load a patch in to the engine (and client) from a patch file.
 *
 * The name and poly from the passed PatchModel are used.  If the name is
 * the empty string, the name will be loaded from the file.  If the poly
 * is 0, it will be loaded from file.  Otherwise the given values will
 * be used.
 *
 * @param wait If true the patch will be checked for existence before
 * loading everything in to it (to prevent messing up existing patches
 * that exist at the path this one should load as).
 *
 * @param existing If true, the patch will be loaded into a currently
 * existing patch (ie a merging will take place).  Errors will result
 * if Nodes of conflicting names exist.
 *
 * @param parent_path Patch to load this patch as a child of (empty string to load
 * to the root patch)
 *
 * @param name Name of this patch (loaded/generated if the empty string)
 *
 * @param initial_data will be set last, so values passed there will override
 * any values loaded from the patch file.
 *
 * Returns the path of the newly created patch.
 */
string
Serializer::load_patch(const string& filename,
	                   const string& parent_path,
	                   const string& name,
	                   size_t        poly,
	                   MetadataMap   initial_data,
	                   bool          existing)
{
#if 0
	cerr << "[Serializer] Loading patch " << filename << "" << endl;

	Path path = "/"; // path of the new patch

	const bool load_name = (name == "");
	const bool load_poly = (poly == 0);
	
	if (initial_data.find("filename") == initial_data.end())
		initial_data["filename"] = Atom(filename.c_str()); // FIXME: URL?

	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if (!doc) {
		cerr << "Unable to parse patch file." << endl;
		return "";
	}

	xmlNodePtr cur = xmlDocGetRootElement(doc);

	if (!cur) {
		cerr << "Empty document." << endl;
		xmlFreeDoc(doc);
		return "";
	}

	if (xmlStrcmp(cur->name, (const xmlChar*) "patch")) {
		cerr << "File is not an Ingen patch file (root node != <patch>)" << endl;
		xmlFreeDoc(doc);
		return "";
	}

	xmlChar* key = NULL;
	cur = cur->xmlChildrenNode;

	// Load Patch attributes
	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"name"))) {
			if (load_name) {
				assert(key != NULL);
				if (parent_path != "")
					path = Path(parent_path).base() + Path::nameify((char*)key);
			}
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"polyphony"))) {
			if (load_poly) {
				poly = atoi((char*)key);
			}
		} else if (xmlStrcmp(cur->name, (const xmlChar*)"connection")
				&& xmlStrcmp(cur->name, (const xmlChar*)"node")
				&& xmlStrcmp(cur->name, (const xmlChar*)"subpatch")
				&& xmlStrcmp(cur->name, (const xmlChar*)"filename")
				&& xmlStrcmp(cur->name, (const xmlChar*)"preset")) {
			// Don't know what this tag is, add it as metadata without overwriting
			// (so caller can set arbitrary parameters which will be preserved)
			if (key)
				if (initial_data.find((const char*)cur->name) == initial_data.end())
					initial_data[(const char*)cur->name] = (const char*)key;
		}
		
		xmlFree(key);
		key = NULL; // Avoid a (possible?) double free

		cur = cur->next;
	}
	
	if (poly == 0)
		poly = 1;

	// Create it, if we're not merging
	if (!existing)
		_engine->create_patch_with_data(path, poly, initial_data);

	// Load nodes
	cur = xmlDocGetRootElement(doc)->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"node")))
			load_node(path, doc, cur);
			
		cur = cur->next;
	}

	// Load subpatches
	cur = xmlDocGetRootElement(doc)->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"subpatch"))) {
			load_subpatch(path, doc, cur);
		}
		cur = cur->next;
	}
	
	// Load connections
	cur = xmlDocGetRootElement(doc)->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"connection"))) {
			load_connection(path, doc, cur);
		}
		cur = cur->next;
	}
	
	
	// Load presets (control values)
	cerr << "FIXME: load preset\n";
	/*cur = xmlDocGetRootElement(doc)->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"preset"))) {
			load_preset(pm, doc, cur);
			assert(preset_model != NULL);
			if (preset_model->name() == "default")
				_engine->set_preset(pm->path(), preset_model);
		}
		cur = cur->next;
	}
	*/
	
	xmlFreeDoc(doc);
	xmlCleanupParser();

	// Done above.. late enough?
	//_engine->set_metadata_map(path, initial_data);

	if (!existing)
		_engine->enable_patch(path);

	_load_path_translations.clear();

	return path;
#endif
	return "/FIXME";
}

} // namespace Client
} // namespace Ingen