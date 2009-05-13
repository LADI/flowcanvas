/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include <sstream>
#include "ingen-config.h"
#include "raul/Path.hpp"
#include "raul/Atom.hpp"
#include "PluginModel.hpp"
#include "PatchModel.hpp"
#include "PluginUI.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {

#ifdef HAVE_SLV2
SLV2World   PluginModel::_slv2_world   = NULL;
SLV2Plugins PluginModel::_slv2_plugins = NULL;
#endif

Redland::World* PluginModel::_rdf_world = NULL;


PluginModel::PluginModel(const URI& uri, const URI& type_uri)
	: ResourceImpl(uri)
	, _type(type_from_uri(_rdf_world->prefixes().qualify(type_uri.str())))
{
	Glib::Mutex::Lock lock(_rdf_world->mutex());
	assert(_rdf_world);
	set_property("rdf:type", Raul::Atom(Raul::Atom::URI, this->type_uri()));
#ifdef HAVE_SLV2
	SLV2Value plugin_uri = slv2_value_new_uri(_slv2_world, uri.c_str());
	_slv2_plugin = slv2_plugins_get_by_uri(_slv2_plugins, plugin_uri);
	slv2_value_free(plugin_uri);
#endif
	if (_type == Internal)
		set_property("doap:name", Raul::Atom(uri.substr(uri.find_last_of("#") + 1).c_str()));
}


string
PluginModel::default_node_symbol() const
{
	return Raul::Path::nameify(symbol());
}


string
PluginModel::human_name()
{
	const Atom& name_atom = get_property("doap:name");
	if (name_atom.type() == Atom::STRING)
		return name_atom.get_string();

#ifdef HAVE_SLV2
	if (_slv2_plugin) {
		SLV2Value name = slv2_plugin_get_name(_slv2_plugin);
		string ret = slv2_value_as_string(name);
		slv2_value_free(name);
		set_property("doap:name", Raul::Atom(Raul::Atom::STRING, ret.c_str()));
		return ret;
	}
#endif

	return default_node_symbol();
}


string
PluginModel::port_human_name(uint32_t index) const
{
#ifdef HAVE_SLV2
	if (_slv2_plugin) {
		Glib::Mutex::Lock lock(_rdf_world->mutex());
		SLV2Port port = slv2_plugin_get_port_by_index(_slv2_plugin, index);
		SLV2Value name = slv2_port_get_name(_slv2_plugin, port);
		string ret = slv2_value_as_string(name);
		slv2_value_free(name);
		return ret;
	}
#endif
	return "";
}


#ifdef HAVE_SLV2
bool
PluginModel::has_ui() const
{
	Glib::Mutex::Lock lock(_rdf_world->mutex());

	SLV2Value gtk_gui_uri = slv2_value_new_uri(_slv2_world,
		"http://lv2plug.in/ns/extensions/ui#GtkUI");

	SLV2UIs uis = slv2_plugin_get_uis(_slv2_plugin);

	if (slv2_values_size(uis) > 0)
		for (unsigned i=0; i < slv2_uis_size(uis); ++i)
			if (slv2_ui_is_a(slv2_uis_get_at(uis, i), gtk_gui_uri))
				return true;

	return false;
}


SharedPtr<PluginUI>
PluginModel::ui(Ingen::Shared::World* world, SharedPtr<NodeModel> node) const
{
	if (_type != LV2)
		return SharedPtr<PluginUI>();

	SharedPtr<PluginUI> ret = PluginUI::create(world, node, _slv2_plugin);
	return ret;
}


const string&
PluginModel::icon_path() const
{
	if (_icon_path == "" && _type == LV2) {
		Glib::Mutex::Lock lock(_rdf_world->mutex());
		_icon_path = get_lv2_icon_path(_slv2_plugin);
	}

	return _icon_path;
}


/** RDF world mutex must be held by the caller */
string
PluginModel::get_lv2_icon_path(SLV2Plugin plugin)
{
	string result;
	SLV2Value svg_icon_pred = slv2_value_new_uri(_slv2_world,
		"http://ll-plugins.nongnu.org/lv2/namespace#svgIcon");

	SLV2Values paths = slv2_plugin_get_value(plugin, svg_icon_pred);

	if (slv2_values_size(paths) > 0) {
		SLV2Value value = slv2_values_get_at(paths, 0);
		if (slv2_value_is_uri(value))
			result = slv2_uri_to_path(slv2_value_as_string(value));
		slv2_values_free(paths);
	}

	slv2_value_free(svg_icon_pred);
	return result;
}
#endif


const string
PluginModel::string_property(const std::string& name) const
{
	const Raul::Atom& atom = get_property(name);
	if (atom.type() == Raul::Atom::STRING)
		return atom.get_string();
	else
		return "";
}

} // namespace Client
} // namespace Ingen
