/* LVZ - A C++ interface for writing LV2 plugins.
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <list>
#include <map>
#include <string>
#include <cassert>
#include <iostream>
#include <fstream>
#include <dlfcn.h>
#include "audioeffectx.h"
#include "AEffEditor.hpp"

using namespace std;

#define NS_LV2CORE "http://lv2plug.in/ns/lv2core#"

// VST is so incredibly awful.  Just.. wow.
#define MAX_NAME_LENGTH 1024
char name_buf[MAX_NAME_LENGTH];


struct Record {
	Record(const string& n) : base_name(n) {}
	string base_name;
	typedef list<string> UIs;
	UIs uis;
};

typedef std::map<string, Record> Manifest;
Manifest manifest;
typedef std::map<string, Record> GUIManifest;
GUIManifest gui_manifest;


string
symbolify(const char* name)
{
	string str(name);

	// Like This -> Like_This
	for (size_t i=0; i < str.length(); ++i)
		if (str[i] == ' ')
			str[i] = '_';

	str[0] = std::tolower(str[0]);

	// LikeThis -> like_this
	for (size_t i=1; i < str.length(); ++i)
		if (str[i] >= 'A' && str[i] <= 'Z'
				&& str[i-1] >= 'a' && str[i-1] <= 'z'
				&& ((i == str.length() - 1) || (str[i+1] <= 'a' && str[i+1] >= 'Z'))
				&& (!(str[i-1] == 'd' && str[i] == 'B'))
				&& (!(str[i-1] == 'F' && str[i] == 'X'))
				&& (!(str[i-1] == 'D' && str[i] == 'C')))
			str = str.substr(0, i) + '_' + str.substr(i);

	// To lowercase, and skip invalids
	for (size_t i=1; i < str.length(); ) {
		if (std::isalpha(str[i]) || std::isdigit(str[i])) {
			str[i] = std::tolower(str[i]);
			++i;
		} else if (str[i-1] != '_') {
			str[i] = '_';
			++i;
		} else {
			str = str.substr(0, i) + str.substr(i+1);
		}
	}

	return str;
}


void
write_plugin(AudioEffectX* effect, const string& lib_file_name)
{
	const string base_name = lib_file_name.substr(0, lib_file_name.find_last_of("."));
	const string data_file_name = base_name + ".ttl";

	fstream os(data_file_name.c_str(), ios::out);
	effect->getProductString(name_buf);

	os << "@prefix : <http://lv2plug.in/ns/lv2core#> ." << endl;
	os << "@prefix doap: <http://usefulinc.com/ns/doap#> ." << endl << endl;
	os << "<" << effect->getURI() << ">" << endl;
	os << "\t:symbol \"" << effect->getUniqueID() << "\" ;" << endl;
	os << "\tdoap:name \"" << name_buf << "\" ;" << endl;
	os << "\tdoap:license <http://usefulinc.com/doap/licenses/gpl> ;" << endl;
	os << "\t:pluginProperty :hardRTCapable";

	uint32_t num_params     = effect->getNumParameters();
	uint32_t num_audio_ins  = effect->getNumInputs();
	uint32_t num_audio_outs = effect->getNumOutputs();
	uint32_t num_ports      = num_params + num_audio_ins + num_audio_outs;

	if (num_ports > 0)
		os << " ;" << endl << "\t:port [" << endl;
	else
		os << " ." << endl;

	uint32_t idx = 0;

	for (uint32_t i = idx; i < num_params; ++i, ++idx) {
		effect->getParameterName(i, name_buf);
		os << "\t\ta :InputPort, :ControlPort ;" << endl;
		os << "\t\t:index" << " " << idx << " ;" << endl;
		os << "\t\t:name \"" << name_buf << "\" ;" << endl;
		os << "\t\t:symbol \"" << symbolify(name_buf) << "\" ;" << endl;
		os << "\t\t:default " << effect->getParameter(i) << " ;" << endl;
		os << "\t\t:minimum 0.0 ;" << endl;
		os << "\t\t:maximum 1.0 ;" << endl;
		os << ((idx == num_ports - 1) ? "\t] ." : "\t] , [") << endl;
	}

	for (uint32_t i = 0; i < num_audio_ins; ++i, ++idx) {
		os << "\t\ta :InputPort, :AudioPort ;" << endl;
		os << "\t\t:index" << " " << idx << " ;" << endl;
		os << "\t\t:symbol \"in" << i+1 << "\" ;" << endl;
		os << "\t\t:name \"Input " << i+1 << "\" ;" << endl;
		os << ((idx == num_ports - 1) ? "\t] ." : "\t] , [") << endl;
	}

	for (uint32_t i = 0; i < num_audio_outs; ++i, ++idx) {
		os << "\t\ta :OutputPort, :AudioPort ;" << endl;
		os << "\t\t:index " << idx << " ;" << endl;
		os << "\t\t:symbol \"out" << i+1 << "\" ;" << endl;
		os << "\t\t:name \"Output " << i+1 << "\" ;" << endl;
		os << ((idx == num_ports - 1) ? "\t] ." : "\t] , [") << endl;
	}

	os.close();

	Manifest::iterator i = manifest.find(effect->getURI());
	if (i != manifest.end()) {
		i->second.base_name = base_name;
	} else {
		manifest.insert(std::make_pair(effect->getURI(), Record(base_name)));
	}
}


void
write_gui(AEffEditor* gui, const string& lib_file_name)
{
	const string base_name = lib_file_name.substr(0, lib_file_name.find_last_of("."));
	assert(gui_manifest.find(gui->getURI()) == gui_manifest.end());
	gui_manifest.insert(std::make_pair(gui->getURI(), Record(base_name)));
	Manifest::iterator plugin_record = manifest.find(lib_file_name);
	if (plugin_record != manifest.end()) {
		plugin_record->second.uis.push_back(gui->getPluginURI());
	}
	Manifest::iterator i = manifest.find(gui->getPluginURI());
	if (i != manifest.end()) {
		i->second.uis.push_back(gui->getURI());
	} else {
		Record r("ERRNOBASE");
		r.uis.push_back(gui->getURI());
		manifest.insert(std::make_pair(gui->getPluginURI(), r));
	}
}


void
write_manifest(ostream& os)
{
	os << "@prefix : <http://lv2plug.in/ns/lv2core#> ." << endl;
	os << "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> ." << endl;
	os << "@prefix uiext: <http://lv2plug.in/ns/extensions/ui#> ." << endl << endl;
	for (Manifest::iterator i = manifest.begin(); i != manifest.end(); ++i) {
		Record& r = i->second;
		os << "<" << i->first << "> a :Plugin ;" << endl;
		os << "\trdfs:seeAlso <" << r.base_name << ".ttl> ;" << endl;
		os << "\t:binary <" << r.base_name << ".so> ";
		for (Record::UIs::iterator j = r.uis.begin(); j != r.uis.end(); ++j)
			os << ";" << endl << "\tuiext:ui <" << *j << "> ";
		os << "." << endl << endl;
	}

	for (GUIManifest::iterator i = gui_manifest.begin(); i != gui_manifest.end(); ++i) {
		Record& r = i->second;
		os << "<" << i->first << "> a uiext:GtkUI ;" << endl;
		os << "\trdfs:seeAlso <" << r.base_name << ".ttl> ;" << endl;
		os << "\tuiext:binary <" << r.base_name << ".so> ." << endl << endl;
	}
}


int
main(int argc, char** argv)
{
	if (argc == 0) {
		cout << "Usage: gendata [PLUGINLIB1] [PLUGINLIB2]..." << endl;
		cout << "Each argument is a path to a LVZ plugin library." << endl;
		cout << "For each library an LV2 data file with the same name" << endl;
		cout << "will be output containing the data for that plugin." << endl;
		cout << "A manifest of the plugins found is written to stdout" << endl;
		return 1;
	}

	typedef AudioEffectX* (*new_effect_func)();
	typedef AEffEditor* (*new_gui_func)();
	typedef AudioEffectX* (*plugin_uri_func)();

	new_effect_func constructor     = NULL;
	new_gui_func    gui_constructor = NULL;
	AudioEffectX*   effect          = NULL;
	AEffEditor*     gui             = NULL;

	for (int i = 1; i < argc; ++i) {
		void* handle = dlopen(argv[i], RTLD_LAZY);
		if (handle == NULL) {
			cerr << "ERROR: " << argv[i] << ": " << dlerror() << " (ignoring)" << endl;
			continue;
		}

		string lib_path = argv[i];
		size_t last_slash = lib_path.find_last_of("/");
		if (last_slash != string::npos)
			lib_path = lib_path.substr(last_slash + 1);

		constructor = (new_effect_func)dlsym(handle, "lvz_new_audioeffectx");
		if (constructor != NULL) {
			effect = constructor();
			write_plugin(effect, lib_path);
		}

		gui_constructor = (new_gui_func)dlsym(handle, "lvz_new_aeffeditor");
		if (gui_constructor != NULL) {
			gui = gui_constructor();
			write_gui(gui, lib_path);
		}

		if (constructor == NULL && gui_constructor == NULL) {
			cerr << "ERROR: " << argv[i] << ": not an LVZ plugin library, ignoring" << endl;
		}

		dlclose(handle);
	}

	write_manifest(cout);

	return 0;
}

