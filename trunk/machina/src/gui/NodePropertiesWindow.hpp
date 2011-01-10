/* This file is part of Machina.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
 *
 * Machina is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Machina is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Machina.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NODEPROPERTIESWINDOW_HPP
#define NODEPROPERTIESWINDOW_HPP

#include <gtkmm.h>

#include <libglademm/xml.h>

#include <raul/SharedPtr.hpp>

namespace Machina { namespace Client { class ClientObject; } }

class NodePropertiesWindow : public Gtk::Dialog
{
public:
	static void present(Gtk::Window* parent, SharedPtr<Machina::Client::ClientObject> node);

private:
	friend class Gnome::Glade::Xml;
	NodePropertiesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	~NodePropertiesWindow();

	void set_node(SharedPtr<Machina::Client::ClientObject> node);

	void apply_clicked();
	void cancel_clicked();
	void ok_clicked();

	static NodePropertiesWindow* _instance;

	SharedPtr<Machina::Client::ClientObject> _node;

	Gtk::SpinButton* _note_spinbutton;
	Gtk::SpinButton* _duration_spinbutton;
	Gtk::Button*     _apply_button;
	Gtk::Button*     _cancel_button;
	Gtk::Button*     _ok_button;
};


#endif // NODEPROPERTIESWINDOW_HPP
