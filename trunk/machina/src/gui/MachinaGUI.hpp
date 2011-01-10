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

#ifndef MACHINA_GUI_HPP
#define MACHINA_GUI_HPP

#include <string>
#include <libgnomecanvasmm.h>

#include "raul/Maid.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/TimeStamp.hpp"

#include "machina-config.h"

using namespace std;

namespace Machina {
class Machine;
class Engine;
class Evolver;
class Controller;
namespace Client { class ClientModel; class ClientObject; }
}

class MachinaCanvas;

class MachinaGUI
{
public:
	MachinaGUI(SharedPtr<Machina::Engine> engine);
	~MachinaGUI();

	boost::shared_ptr<MachinaCanvas>   canvas() { return _canvas; }
	boost::shared_ptr<Machina::Engine> engine() { return _engine; }

	SharedPtr<Raul::Maid> maid() { return _maid; }

	Gtk::Window* window() { return _main_window; }

	void attach();
	void quit() { _main_window->hide(); }

	SharedPtr<Machina::Controller> controller() { return _controller; }

	inline void queue_refresh() { _refresh = true; }

	void on_new_object(SharedPtr<Machina::Client::ClientObject> object);
	void on_erase_object(SharedPtr<Machina::Client::ClientObject> object);

	SharedPtr<Machina::Client::ClientModel> client_model() { return _client_model; }

protected:
	void connect_widgets();

	void menu_file_quit();
	void menu_file_open();
	void menu_file_save();
	void menu_file_save_as();
	void menu_import_midi();
	void menu_export_midi();
	void menu_export_graphviz();
	void show_toolbar_toggled();
	void show_labels_toggled();
	void menu_help_about();
	void menu_help_help();
	void arrange();
	void load_target_clicked();

	void random_mutation(SharedPtr<Machina::Machine> machine);
	void mutate(SharedPtr<Machina::Machine> machine, unsigned mutation);
	void zoom(double z);
	void update_toolbar();

	bool scrolled_window_event(GdkEvent* ev);
	bool idle_callback();

#ifdef HAVE_EUGENE
	void evolve_toggled();
	bool evolve_callback();
#endif

	void record_toggled();
	void stop_clicked();
	void play_toggled();

	void quantize_changed();
	void tempo_changed();

	bool _refresh;
	bool _evolve;

	string _save_uri;
	string _target_filename;

	Raul::TimeUnit _unit;

	SharedPtr<MachinaCanvas>                _canvas;
	SharedPtr<Machina::Engine>              _engine;
	SharedPtr<Machina::Client::ClientModel> _client_model;
	SharedPtr<Machina::Controller>          _controller;
	
	SharedPtr<Raul::Maid>       _maid;
	SharedPtr<Machina::Evolver> _evolver;

	Gtk::Main* _gtk_main;

	Gtk::Window*           _main_window;
	Gtk::Dialog*           _help_dialog;
	Gtk::AboutDialog*      _about_window;
	Gtk::Toolbar*          _toolbar;
	Gtk::MenuItem*         _menu_file_open;
	Gtk::MenuItem*         _menu_file_save;
	Gtk::MenuItem*         _menu_file_save_as;
	Gtk::MenuItem*         _menu_file_quit;
	Gtk::MenuItem*         _menu_import_midi;
	Gtk::MenuItem*         _menu_export_midi;
	Gtk::MenuItem*         _menu_export_graphviz;
	Gtk::MenuItem*         _menu_help_about;
	Gtk::CheckMenuItem*    _menu_view_labels;
	Gtk::CheckMenuItem*    _menu_view_time_edges;
	Gtk::CheckMenuItem*    _menu_view_toolbar;
	Gtk::MenuItem*         _menu_help_help;
	Gtk::ScrolledWindow*   _canvas_scrolledwindow;
	Gtk::TextView*         _status_text;
	Gtk::Expander*         _messages_expander;
	Gtk::ToggleToolButton* _step_record_checkbutton;
	Gtk::CheckButton*      _clock_checkbutton;
	Gtk::SpinButton*       _bpm_spinbutton;
	Gtk::CheckButton*      _quantize_checkbutton;
	Gtk::SpinButton*       _quantize_spinbutton;
	Gtk::ToggleToolButton* _record_button;
	Gtk::ToolButton*       _stop_button;
	Gtk::ToggleToolButton* _play_button;
	Gtk::ToolButton*       _zoom_normal_button;
	Gtk::ToolButton*       _zoom_full_button;
	Gtk::ToolButton*       _arrange_button;
	Gtk::ToolButton*       _load_target_button;
	Gtk::Toolbar*          _evolve_toolbar;
	Gtk::ToggleToolButton* _evolve_button;
	Gtk::ToolButton*       _mutate_button;
	Gtk::ToolButton*       _compress_button;
	Gtk::ToolButton*       _add_node_button;
	Gtk::ToolButton*       _remove_node_button;
	Gtk::ToolButton*       _adjust_node_button;
	Gtk::ToolButton*       _add_edge_button;
	Gtk::ToolButton*       _remove_edge_button;
	Gtk::ToolButton*       _adjust_edge_button;
};

#endif // MACHINA_GUI_HPP
