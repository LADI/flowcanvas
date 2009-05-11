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

#ifndef PATCHCANVAS_H
#define PATCHCANVAS_H

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include "ingen-config.h"
#include "flowcanvas/Canvas.hpp"
#include "flowcanvas/Module.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/Path.hpp"
#include "client/ConnectionModel.hpp"
#include "interface/GraphObject.hpp"
#include "NodeModule.hpp"

using namespace FlowCanvas;
using namespace Ingen::Shared;

using std::string;
using FlowCanvas::Port;
using Ingen::Client::ConnectionModel;
using Ingen::Client::NodeModel;
using Ingen::Client::PortModel;

namespace Ingen {

namespace Client { class PatchModel; }
using Ingen::Client::PatchModel;

namespace GUI {
	
class NodeModule;


/** Patch canvas widget.
 *
 * \ingroup GUI
 */
class PatchCanvas : public FlowCanvas::Canvas
{
public:
	PatchCanvas(SharedPtr<PatchModel> patch, int width, int height);
	
	virtual ~PatchCanvas() {}

	void build();
	void arrange(bool use_length_hints);
	void show_human_names(bool show);
	void show_port_names(bool show);
	bool show_port_names() const { return _show_port_names; }

	void add_plugin(SharedPtr<PluginModel> pm);
	void add_node(SharedPtr<NodeModel> nm);
	void remove_node(SharedPtr<NodeModel> nm);
	void add_port(SharedPtr<PortModel> pm);
	void remove_port(SharedPtr<PortModel> pm);
	void connection(SharedPtr<ConnectionModel> cm);
	void disconnection(SharedPtr<ConnectionModel> cm);

	void get_new_module_location(double& x, double& y);

	void destroy_selection();
	void copy_selection();
	void paste();
	void select_all();

	void show_menu(GdkEvent* event);
	
	bool canvas_key_event(GdkEventKey* event);
	
private:
	enum ControlType { NUMBER, BUTTON };
	void menu_add_control(ControlType type);
	
	void generate_port_name(
			const string& sym_base,  string& sym,
			const string& name_base, string& name);

	void menu_add_port(
			const string& sym_base, const string& name_base,
			const string& type, bool is_output);
	
	void menu_load_plugin();
	void menu_new_patch();
	void menu_load_patch();
	void load_plugin(WeakPtr<PluginModel> plugin);

	void build_menus();

	void build_internal_menu();
	void build_classless_menu();

#ifdef HAVE_SLV2
	void build_plugin_menu();
	size_t build_plugin_class_menu(
			Gtk::Menu*        menu,
			SLV2PluginClass   plugin_class,
			SLV2PluginClasses classes);
#endif
	
	GraphObject::Properties get_initial_data();

	bool canvas_event(GdkEvent* event);
	
	SharedPtr<FlowCanvas::Port> get_port_view(SharedPtr<PortModel> port);

	void connect(boost::shared_ptr<FlowCanvas::Connectable> src,
	             boost::shared_ptr<FlowCanvas::Connectable> dst);

	void disconnect(boost::shared_ptr<FlowCanvas::Connectable> src,
	                boost::shared_ptr<FlowCanvas::Connectable> dst);

	SharedPtr<PatchModel> _patch;

	typedef std::map<SharedPtr<ObjectModel>, SharedPtr<FlowCanvas::Module> > Views;
	Views _views;

	int _last_click_x;
	int _last_click_y;
	int _paste_count;

	typedef std::multimap<const std::string, Gtk::Menu*> ClassMenus;
	ClassMenus _class_menus;
	
	bool            _refresh_menu;
	bool            _human_names;
	bool            _show_port_names;
	Gtk::Menu*      _menu;
	Gtk::Menu*      _internal_menu;
	Gtk::Menu*      _classless_menu;
	Gtk::Menu*      _plugin_menu;
	/*Gtk::MenuItem*  _menu_add_number_control;
	Gtk::MenuItem*  _menu_add_button_control;*/
	Gtk::MenuItem*  _menu_add_audio_input;
	Gtk::MenuItem*  _menu_add_audio_output;
	Gtk::MenuItem*  _menu_add_control_input;
	Gtk::MenuItem*  _menu_add_control_output;
	Gtk::MenuItem*  _menu_add_event_input;
	Gtk::MenuItem*  _menu_add_event_output;
	Gtk::MenuItem*  _menu_load_plugin;
	Gtk::MenuItem*  _menu_load_patch;
	Gtk::MenuItem*  _menu_new_patch;
};


} // namespace GUI
} // namespace Ingen

#endif // PATCHCANVAS_H
