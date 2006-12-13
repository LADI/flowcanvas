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

#include <iostream>
#include <raptor.h>
#include "Loader.h"
#include "RDFQuery.h"
#include "ModelEngineInterface.h"

namespace Ingen {
namespace Client {


Loader::Loader(SharedPtr<ModelEngineInterface> engine, SharedPtr<Namespaces> namespaces)
	: _engine(engine)
	, _namespaces(namespaces)
{
	if (!_namespaces)
		_namespaces = SharedPtr<Namespaces>(new Namespaces());

	// FIXME: hack
	_namespaces->add("ingen", "http://codeson.net/ns/ingen#");
	_namespaces->add("ingenuity", "http://codeson.net/ns/ingenuity#");
}


/** Load (create) all objects from an RDF into the engine.
 *
 * @param filename Filename to load objects from.
 * @param parent Path of parent under which to load objects.
 * @return whether or not load was successful.
 */
bool
Loader::load(const Glib::ustring& filename,
             const Path&          parent,
			 Glib::ustring        patch_uri,
			 MetadataMap          data)
{
	// FIXME: this whole thing is a mess
	
	std::map<Glib::ustring, bool> created;

	// FIXME: kluge
	unsigned char* document_uri_str = raptor_uri_filename_to_uri_string(filename.c_str());
	Glib::ustring document_uri = (const char*)document_uri_str;
	//Glib::ustring document_uri = "file:///home/dave/code/codesonnet/ingen/src/progs/ingenuity/test2.ingen.ttl";

	if (patch_uri == "")
		patch_uri = "<>";

	cerr << "[Loader] Loading " << patch_uri << " from " << document_uri
		<< " under " << parent << endl;

	/* Get polyphony (mandatory) */

	// FIXME: polyphony datatype
	RDFQuery query(Glib::ustring(
		"SELECT DISTINCT ?poly \nFROM <") + document_uri + ">\nWHERE {\n\t" +
		patch_uri + " ingen:polyphony ?poly .\n"
		"}");

	RDFQuery::Results results = query.run(document_uri);

	if (results.size() == 0) {
		cerr << "[Loader] ERROR: No polyphony found!" << endl;
		return false;
	}

	size_t patch_poly = atoi(((*results.begin())["poly"]).c_str());
	
	
	/* Get name (if available) */

	query = RDFQuery(Glib::ustring(
		"SELECT DISTINCT ?name \nFROM <") + document_uri + ">\nWHERE {\n\t" +
		patch_uri + " ingen:name ?name .\n"
		"}");

	results = query.run(document_uri);
	
	string patch_name = string(filename.substr(filename.find_last_of("/")+1));
	Path patch_path = parent.base() + string(filename.substr(filename.find_last_of("/")+1));

	if (results.size() > 0)
		patch_path = parent.base() + string((*results.begin())["name"]);
	
	cerr << "************ PATCH: " << patch_path << ", poly = " << patch_poly << endl;

	_engine->create_patch(patch_path, patch_poly);


	/* Load nodes */
	
	query = RDFQuery(Glib::ustring(
		"SELECT DISTINCT ?name ?plugin ?floatkey ?floatval FROM <") + document_uri + "> WHERE {\n" +
		patch_uri + " ingen:node   ?node .\n"
		"?node        ingen:name   ?name ;\n"
		"             ingen:plugin ?plugin .\n"
		"OPTIONAL { ?node ?floatkey ?floatval . \n"
		"           FILTER ( datatype(?floatval) = xsd:decimal ) }\n"
		"}");

	results = query.run(document_uri);

	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		
		const Glib::ustring& name   = (*i)["name"];
		const Glib::ustring& plugin = (*i)["plugin"];
		
		if (created.find(name) == created.end()) {
			_engine->create_node(patch_path.base() + name, plugin, false);
			created[name] = true;
		}

		Glib::ustring floatkey = _namespaces->qualify((*i)["floatkey"]);
		Glib::ustring floatval = (*i)["floatval"];

		if (floatkey != "" && floatval != "") {
			const float val = atof(floatval.c_str());
			_engine->set_metadata(patch_path.base() + name, floatkey, Atom(val));
		}
	}
	
	created.clear();


	/* Load this patch's ports */
	
	query = RDFQuery(Glib::ustring(
		"SELECT DISTINCT ?port ?type ?name ?datatype ?floatkey ?floatval FROM <") + document_uri + "> WHERE {\n" +
		patch_uri + " ingen:port     ?port .\n"
		"?port        a              ?type ;\n"
		"             ingen:name     ?name ;\n"
		"             ingen:dataType ?datatype .\n"
		"OPTIONAL { ?port ?floatkey ?floatval . \n"
		"           FILTER ( datatype(?floatval) = xsd:decimal ) }\n"
		"}");

	results = query.run(document_uri);

	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const Glib::ustring& name     = (*i)["name"];
		const Glib::ustring& type     = _namespaces->qualify((*i)["type"]);
		const Glib::ustring& datatype = (*i)["datatype"];

		if (created.find(name) == created.end()) {
			//cerr << "TYPE: " << type << endl;
			bool is_output = (type == "ingen:OutputPort"); // FIXME: check validity
			_engine->create_port(patch_path.base() + name, datatype, is_output);
			created[name] = true;
		}

		Glib::ustring floatkey = _namespaces->qualify((*i)["floatkey"]);
		Glib::ustring floatval = (*i)["floatval"];

		if (floatkey != "" && floatval != "") {
			const float val = atof(floatval.c_str());
			_engine->set_metadata(patch_path.base() + name, floatkey, Atom(val));
		}
	}

	/* Load connections */

	// FIXME: path?
	query = RDFQuery(Glib::ustring(
		"SELECT DISTINCT ?srcnode ?src ?dstnode ?dst FROM <") + document_uri + "> WHERE {\n" +
		"?srcnoderef ingen:port ?srcref .\n"
		"?dstnoderef ingen:port ?dstref .\n"
		"?dstref ingen:connectedTo ?srcref .\n"
		"?srcref ingen:name ?src .\n"
		"?dstref ingen:name ?dst .\n"
		"OPTIONAL { ?srcnoderef ingen:name ?srcnode } .\n"
		"OPTIONAL { ?dstnoderef ingen:name ?dstnode }\n"
		"}\n");
	
	results = query.run(document_uri);
	
	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		Path src_node = patch_path;
		if ((*i).find("srcnode") != (*i).end())
			src_node = patch_path.base() + string((*i)["srcnode"]);
		Path src_port = src_node.base() + string((*i)["src"]);
		
		Path dst_node = patch_path;
		if ((*i).find("dstnode") != (*i).end())
			dst_node = patch_path.base() + string((*i)["dstnode"]);
		Path dst_port = dst_node.base() + string((*i)["dst"]);
		
		//cerr << "CONNECTION: " << src_port << " -> " << dst_port << endl;

		_engine->connect(src_port, dst_port);
	}
	
	// Set passed metadata last to override any loaded values
	for (MetadataMap::const_iterator i = data.begin(); i != data.end(); ++i)
		_engine->set_metadata(patch_path, i->first, i->second);

	return true;
}


} // namespace Client
} // namespace Ingen
