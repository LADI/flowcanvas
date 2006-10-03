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

#include "DeprecatedSerializer.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <algorithm>
#include "PatchModel.h"
#include "NodeModel.h"
#include "ConnectionModel.h"
#include "PortModel.h"
#include "PresetModel.h"
#include "ModelEngineInterface.h"
#include "PluginModel.h"
#include "util/Path.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <utility> // for pair, make_pair
#include <cassert>
#include <cstring>
#include <string>
#include <cstdlib>  // for atof
#include <cmath>

using std::string; using std::vector; using std::pair;
using std::cerr; using std::cout; using std::endl;

namespace Ingen {
namespace Client {

	
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
DeprecatedSerializer::find_file(const string& filename, const string& additional_path)
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
			cerr << "[DeprecatedSerializer] Could not find patch file " << full_patch_path << endl;
		}
	}

	return "";
}


string
DeprecatedSerializer::translate_load_path(const string& path)
{
	std::map<string,string>::iterator t = _load_path_translations.find(path);
	
	if (t != _load_path_translations.end()) {
		return (*t).second;
	} else {
		assert(Path::is_valid(path));
		return path;
	}
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
DeprecatedSerializer::load_patch(const string& filename,
	                       const string& parent_path,
	                       const string& name,
	                       size_t        poly,
	                       MetadataMap   initial_data,
	                       bool          existing)
{
	cerr << "[DeprecatedSerializer] Loading patch " << filename << "" << endl;

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
}


/** Build a NodeModel given a pointer to a Node in a patch file.
 */
bool
DeprecatedSerializer::load_node(const Path& parent, xmlDocPtr doc, const xmlNodePtr node)
{
	xmlChar* key;
	xmlNodePtr cur = node->xmlChildrenNode;
	
	string path = "";
	bool   polyphonic = false;

	string plugin_uri;
	
	string plugin_type;  // deprecated
	string library_name; // deprecated
	string plugin_label; // deprecated

	MetadataMap initial_data;

	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"name"))) {
			path = parent.base() + Path::nameify((char*)key);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"polyphonic"))) {
			polyphonic = !strcmp((char*)key, "true");
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"type"))) {
			plugin_type = (const char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"library-name"))) {
			library_name = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"plugin-label"))) {
			plugin_label = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"plugin-uri"))) {
			plugin_uri = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"port"))) {
			cerr << "FIXME: load port\n";
#if 0
			xmlNodePtr child = cur->xmlChildrenNode;
			
			string port_name;
			float user_min = 0.0;
			float user_max = 0.0;
			
			while (child != NULL) {
				key = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
				
				if ((!xmlStrcmp(child->name, (const xmlChar*)"name"))) {
					port_name = Path::nameify((char*)key);
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"user-min"))) {
					user_min = atof((char*)key);
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"user-max"))) {
					user_max = atof((char*)key);
				}
				
				xmlFree(key);
				key = NULL; // Avoid a (possible?) double free
		
				child = child->next;
			}

			assert(path.length() > 0);
			assert(Path::is_valid(path));

			// FIXME: /nasty/ assumptions
			CountedPtr<PortModel> pm(new PortModel(Path(path).base() + port_name,
					PortModel::CONTROL, PortModel::INPUT, PortModel::NONE,
					0.0, user_min, user_max));
			//pm->set_parent(nm);
			nm->add_port(pm);
#endif

		// DSSI hacks.  Stored in the patch files as special elements, but sent to
		// the engine as normal metadata with specially formatted key/values.  Not
		// sure if this is the best way to go about this, but it's the least damaging
		// right now
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"dssi-program"))) {
			cerr << "FIXME: load dssi program\n";
#if 0
			xmlNodePtr child = cur->xmlChildrenNode;
			
			string bank;
			string program;
			
			while (child != NULL) {
				key = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
				
				if ((!xmlStrcmp(child->name, (const xmlChar*)"bank"))) {
					bank = (char*)key;
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"program"))) {
					program = (char*)key;
				}
				
				xmlFree(key);
				key = NULL; // Avoid a (possible?) double free
				child = child->next;
			}
			nm->set_metadata("dssi-program", Atom(bank.append("/").append(program).c_str()));
#endif
			
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"dssi-configure"))) {
			cerr << "FIXME: load dssi configure\n";
#if 0
			xmlNodePtr child = cur->xmlChildrenNode;
			
			string dssi_key;
			string dssi_value;
			
			while (child != NULL) {
				key = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
				
				if ((!xmlStrcmp(child->name, (const xmlChar*)"key"))) {
					dssi_key = (char*)key;
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"value"))) {
					dssi_value = (char*)key;
				}
				
				xmlFree(key);
				key = NULL; // Avoid a (possible?) double free
		
				child = child->next;
			}
			nm->set_metadata(string("dssi-configure--").append(dssi_key), Atom(dssi_value.c_str()));
#endif		
		} else {  // Don't know what this tag is, add it as metadata
			if (key) {

				// Hack to make module-x and module-y set as floats
				char* endptr = NULL;
				float fval = strtof((const char*)key, &endptr);
				if (endptr != (char*)key && *endptr == '\0')
					initial_data[(const char*)cur->name] = Atom(fval);
				else
					initial_data[(const char*)cur->name] = Atom((const char*)key);
			}
		}
		xmlFree(key);
		key = NULL;

		cur = cur->next;
	}
	
	if (path == "") {
		cerr << "[DeprecatedSerializer] Malformed patch file (node tag has empty children)" << endl;
		cerr << "[DeprecatedSerializer] Node ignored." << endl;
		return false;
	}

	// Compatibility hacks for old patches that represent patch ports as nodes
	if (plugin_uri == "") {
		cerr << "WARNING: Loading deprecated Node.  Resave! " << path << endl;
		bool is_port = false;

		if (plugin_type == "Internal") {
			if (plugin_label == "audio_input") {
				_engine->create_port(path, "AUDIO", false);
				is_port = true;
			} else if (plugin_label == "audio_output") {
				_engine->create_port(path, "AUDIO", true);
				is_port = true;
			} else if (plugin_label == "control_input") {
				_engine->create_port(path, "CONTROL", false);
				is_port = true;
			} else if (plugin_label == "control_output" ) {
				_engine->create_port(path, "CONTROL", true);
				is_port = true;
			} else if (plugin_label == "midi_input") {
				_engine->create_port(path, "MIDI", false);
				is_port = true;
			} else if (plugin_label == "midi_output" ) {
				_engine->create_port(path, "MIDI", true);
				is_port = true;
			}
		}

		if (is_port) {
			const string old_path = path;
			const string new_path = Path::pathify(old_path);

			// Set up translations (for connections etc) to alias both the old
			// module path and the old module/port path to the new port path
			_load_path_translations[old_path] = new_path;
			_load_path_translations[old_path + "/in"] = new_path;
			_load_path_translations[old_path + "/out"] = new_path;

			path = new_path;

			_engine->set_metadata_map(path, initial_data);

			return CountedPtr<NodeModel>();

		} else {
			if (plugin_label  == "note_in") {
				plugin_uri = "ingen:note_node";
			} else if (plugin_label == "control_input") {
				plugin_uri = "ingen:control_node";
			} else if (plugin_label == "transport") {
				plugin_uri = "ingen:transport_node";
			} else if (plugin_label == "trigger_in") {
				plugin_uri = "ingen:trigger_node";
			} else {
				cerr << "WARNING: Unknown deprecated node (label " << plugin_label
					<< ")." << endl;
			}

			if (plugin_uri != "")
				_engine->create_node(path, plugin_uri, polyphonic);
			else
				_engine->create_node(path, plugin_type, library_name, plugin_label, polyphonic);
		
			_engine->set_metadata_map(path, initial_data);

			return true;
		}

	// Not deprecated
	} else {
		_engine->create_node(path, plugin_uri, polyphonic);
		_engine->set_metadata_map(path, initial_data);
		return true;
	}

	// (shouldn't get here)
}


bool
DeprecatedSerializer::load_subpatch(const Path& parent, xmlDocPtr doc, const xmlNodePtr subpatch)
{
	xmlChar *key;
	xmlNodePtr cur = subpatch->xmlChildrenNode;
	
	string name     = "";
	string filename = "";
	size_t poly     = 0;
	
	MetadataMap initial_data;

	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"name"))) {
			name = (const char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"polyphony"))) {
			poly = atoi((const char*)key);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"filename"))) {
			filename = (const char*)key;
		} else {  // Don't know what this tag is, add it as metadata
			if (key != NULL && strlen((const char*)key) > 0)
				initial_data[(const char*)cur->name] = Atom((const char*)key);
		}
		xmlFree(key);
		key = NULL;

		cur = cur->next;
	}

	// load_patch sets the passed metadata last, so values stored in the parent
	// will override values stored in the child patch file
	string path = load_patch(filename, parent, name, poly, initial_data, false);
	
	return false;
}


/** Build a ConnectionModel given a pointer to a connection in a patch file.
 */
bool
DeprecatedSerializer::load_connection(const Path& parent, xmlDocPtr doc, const xmlNodePtr node)
{
	xmlChar *key;
	xmlNodePtr cur = node->xmlChildrenNode;
	
	string source_node, source_port, dest_node, dest_port;
	
	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		
		if ((!xmlStrcmp(cur->name, (const xmlChar*)"source-node"))) {
			source_node = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"source-port"))) {
			source_port = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"destination-node"))) {
			dest_node = (char*)key;
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"destination-port"))) {
			dest_port = (char*)key;
		}
		
		xmlFree(key);
		key = NULL; // Avoid a (possible?) double free

		cur = cur->next;
	}

	if (source_node == "" || source_port == "" || dest_node == "" || dest_port == "") {
		cerr << "ERROR: Malformed patch file (connection tag has empty children)" << endl;
		cerr << "ERROR: Connection ignored." << endl;
		return false;
	}

	// Compatibility fixes for old (fundamentally broken) patches
	source_node = Path::nameify(source_node);
	source_port = Path::nameify(source_port);
	dest_node = Path::nameify(dest_node);
	dest_port = Path::nameify(dest_port);

	_engine->connect(
		translate_load_path(parent.base() + source_node +"/"+ source_port),
		translate_load_path(parent.base() + dest_node +"/"+ dest_port));
	
	return true;
}


/** Build a PresetModel given a pointer to a preset in a patch file.
 */
bool
DeprecatedSerializer::load_preset(const Path& parent, xmlDocPtr doc, const xmlNodePtr node)
{
	cerr << "FIXME: load preset\n";
#if 0
	xmlNodePtr cur = node->xmlChildrenNode;
	xmlChar* key;

	PresetModel* pm = new PresetModel(patch->path().base());
	
	while (cur != NULL) {
		key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

		if ((!xmlStrcmp(cur->name, (const xmlChar*)"name"))) {
			assert(key != NULL);
			pm->name((char*)key);
		} else if ((!xmlStrcmp(cur->name, (const xmlChar*)"control"))) {
			xmlNodePtr child = cur->xmlChildrenNode;
	
			string node_name = "", port_name = "";
			float val = 0.0;
			
			while (child != NULL) {
				key = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
				
				if ((!xmlStrcmp(child->name, (const xmlChar*)"node-name"))) {
					node_name = (char*)key;
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"port-name"))) {
					port_name = (char*)key;
				} else if ((!xmlStrcmp(child->name, (const xmlChar*)"value"))) {
					val = atof((char*)key);
				}
				
				xmlFree(key);
				key = NULL; // Avoid a (possible?) double free
		
				child = child->next;
			}

			// Compatibility fixes for old patch files
			node_name = Path::nameify(node_name);
			port_name = Path::nameify(port_name);
			
			if (port_name == "") {
				string msg = "Unable to parse control in patch file ( node = ";
				msg.append(node_name).append(", port = ").append(port_name).append(")");
				cerr << "ERROR: " << msg << endl;
				//m_client_hooks->error(msg);
			} else {
				// FIXME: temporary compatibility, remove any slashes from port name
				// remove this soon once patches have migrated
				string::size_type slash_index;
				while ((slash_index = port_name.find("/")) != string::npos)
					port_name[slash_index] = '-';
				pm->add_control(node_name, port_name, val);
			}
		}
		xmlFree(key);
		key = NULL;
		cur = cur->next;
	}
	if (pm->name() == "") {
		cerr << "Preset in patch file has no name." << endl;
		//m_client_hooks->error("Preset in patch file has no name.");
		pm->name("Unnamed");
	}

	return pm;
#endif
	return false;
}

} // namespace Client
} // namespace Ingen