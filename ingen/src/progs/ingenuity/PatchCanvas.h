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

#ifndef OMPATCHBAYAREA_H
#define OMPATCHBAYAREA_H

#include <string>
#include <flowcanvas/FlowCanvas.h>
#include "util/CountedPtr.h"
#include "util/Path.h"
#include "ConnectionModel.h"
#include "PatchModel.h"

using std::string;
using namespace LibFlowCanvas;

using LibFlowCanvas::Port;
using Ingen::Client::ConnectionModel;
using Ingen::Client::PatchModel;
using Ingen::Client::NodeModel;
using Ingen::Client::PortModel;
using Ingen::Client::MetadataMap;

namespace Ingenuity {
	
class NodeModule;


/** Patch canvas widget.
 *
 * \ingroup Ingenuity
 */
class PatchCanvas : public LibFlowCanvas::FlowCanvas
{
public:
	PatchCanvas(CountedPtr<PatchModel> patch, int width, int height);
	
	NodeModule* find_module(const string& name)
		{ return (NodeModule*)FlowCanvas::get_module(name); }

	void add_node(CountedPtr<NodeModel> nm);
	void remove_node(CountedPtr<NodeModel> nm);
	void add_port(CountedPtr<PortModel> pm);
	void remove_port(CountedPtr<PortModel> pm);
	void connection(CountedPtr<ConnectionModel> cm);
	void disconnection(const Path& src_port_path, const Path& dst_port_path);

	void get_new_module_location(double& x, double& y);

	void destroy_selected();

	void show_menu(GdkEvent* event)
	{ m_menu->popup(event->button.button, event->button.time); }
	
private:
	string generate_port_name(const string& base);
	void menu_add_port(const string& name, const string& type, bool is_output);
	void menu_load_plugin();
	void menu_new_patch();
	void menu_load_patch();
	
	MetadataMap get_initial_data();

	void build_canvas();

	bool canvas_event(GdkEvent* event);
	
	void connect(const LibFlowCanvas::Port* src_port, const LibFlowCanvas::Port* dst_port);
	void disconnect(const LibFlowCanvas::Port* src_port, const LibFlowCanvas::Port* dst_port);

	CountedPtr<PatchModel> m_patch;

	int m_last_click_x;
	int m_last_click_y;
	
	Gtk::Menu*      m_menu;
	Gtk::MenuItem*  m_menu_add_audio_input;
	Gtk::MenuItem*  m_menu_add_audio_output;
	Gtk::MenuItem*  m_menu_add_control_input;
	Gtk::MenuItem*  m_menu_add_control_output;
	Gtk::MenuItem*  m_menu_add_midi_input;
	Gtk::MenuItem*  m_menu_add_midi_output;
	Gtk::MenuItem*  m_menu_load_plugin;
	Gtk::MenuItem*  m_menu_load_patch;
	Gtk::MenuItem*  m_menu_new_patch;
};


} // namespace Ingenuity

#endif // OMPATCHBAYAREA_H