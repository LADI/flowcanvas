/* This file is part of Machina.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Machina is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Machina is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "wafconfig.h"

#include <cmath>
#include <sstream>
#include <fstream>
#include <limits.h>
#include <pthread.h>
#include <libgnomecanvasmm.h>
#include <libglademm/xml.h>
#include "redlandmm/Model.hpp"
#include "machina/Machine.hpp"
#include "machina/Mutation.hpp"
#include "machina/SMFDriver.hpp"
#include "GladeXml.hpp"
#include "MachinaGUI.hpp"
#include "MachinaCanvas.hpp"
#include "NodeView.hpp"
#include "EdgeView.hpp"

#ifdef HAVE_EUGENE
#include "machina/Evolver.hpp"
#endif

using namespace Machina;


MachinaGUI::MachinaGUI(SharedPtr<Machina::Engine> engine)
	: _refresh(false)
	, _evolve(false)
	, _unit(TimeUnit::BEATS, 19200)
	, _engine(engine)
	, _maid(new Raul::Maid(32))
{
	_canvas = boost::shared_ptr<MachinaCanvas>(new MachinaCanvas(this, 1600*2, 1200*2));

	Glib::RefPtr<Gnome::Glade::Xml> xml = GladeXml::create();

	xml->get_widget("machina_win", _main_window);
	xml->get_widget("about_win", _about_window);
	xml->get_widget("help_dialog", _help_dialog);
	xml->get_widget("toolbar", _toolbar);
	xml->get_widget("open_menuitem", _menu_file_open);
	xml->get_widget("save_menuitem", _menu_file_save);
	xml->get_widget("save_as_menuitem", _menu_file_save_as);
	xml->get_widget("quit_menuitem", _menu_file_quit);
	xml->get_widget("import_midi_menuitem", _menu_import_midi);
	xml->get_widget("export_midi_menuitem", _menu_export_midi);
	xml->get_widget("export_graphviz_menuitem", _menu_export_graphviz);
	xml->get_widget("view_time_edges_menuitem", _menu_view_time_edges);
	xml->get_widget("view_toolbar_menuitem", _menu_view_toolbar);
	xml->get_widget("view_labels_menuitem", _menu_view_labels);
	xml->get_widget("help_about_menuitem", _menu_help_about);
	xml->get_widget("help_help_menuitem", _menu_help_help);
	xml->get_widget("canvas_scrolledwindow", _canvas_scrolledwindow);
	xml->get_widget("clock_checkbutton", _clock_checkbutton);
	xml->get_widget("bpm_spinbutton", _bpm_spinbutton);
	xml->get_widget("quantize_checkbutton", _quantize_checkbutton);
	xml->get_widget("quantize_spinbutton", _quantize_spinbutton);
	xml->get_widget("record_but", _record_button);
	xml->get_widget("stop_but", _stop_button);
	xml->get_widget("play_but", _play_button);
	xml->get_widget("zoom_normal_but", _zoom_normal_button);
	xml->get_widget("zoom_full_but", _zoom_full_button);
	xml->get_widget("arrange_but", _arrange_button);
	xml->get_widget("load_target_but", _load_target_button);
	xml->get_widget("evolve_but", _evolve_button);
	xml->get_widget("mutate_but", _mutate_button);
	xml->get_widget("compress_but", _compress_button);
	xml->get_widget("add_node_but", _add_node_button);
	xml->get_widget("remove_node_but", _remove_node_button);
	xml->get_widget("adjust_node_but", _adjust_node_button);
	xml->get_widget("add_edge_but", _add_edge_button);
	xml->get_widget("remove_edge_but", _remove_edge_button);
	xml->get_widget("adjust_edge_but", _adjust_edge_button);
	
	_canvas_scrolledwindow->add(*_canvas);
	_canvas_scrolledwindow->signal_event().connect(sigc::mem_fun(this,
				&MachinaGUI::scrolled_window_event));
	_canvas->scroll_to(static_cast<int>(_canvas->width()/2 - 320),
	                       static_cast<int>(_canvas->height()/2 - 240)); // FIXME: hardcoded

	_canvas_scrolledwindow->property_hadjustment().get_value()->set_step_increment(10);
	_canvas_scrolledwindow->property_vadjustment().get_value()->set_step_increment(10);
	
	_record_button->signal_toggled().connect(sigc::mem_fun(this, &MachinaGUI::record_toggled));
	_stop_button->signal_clicked().connect(sigc::mem_fun(this, &MachinaGUI::stop_clicked));
	_play_button->signal_toggled().connect(sigc::mem_fun(this, &MachinaGUI::play_toggled));

	_zoom_normal_button->signal_clicked().connect(sigc::bind(
		sigc::mem_fun(this, &MachinaGUI::zoom), 1.0));
	
	_zoom_full_button->signal_clicked().connect(
		sigc::mem_fun(_canvas.get(), &MachinaCanvas::zoom_full));

	_arrange_button->signal_clicked().connect(
		sigc::mem_fun(this, &MachinaGUI::arrange));

	_menu_file_open->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_file_open));
	_menu_file_save->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_file_save));
	_menu_file_save_as->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_file_save_as));
	_menu_file_quit->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_file_quit));
	_menu_import_midi->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_import_midi));
	_menu_export_midi->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_export_midi));
	_menu_export_graphviz->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_export_graphviz));
	_menu_view_toolbar->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::show_toolbar_toggled));
	_menu_view_labels->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::show_labels_toggled));
	_menu_help_about->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_help_about));
	_menu_help_help->signal_activate().connect(
		sigc::mem_fun(this, &MachinaGUI::menu_help_help));
	_clock_checkbutton->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::tempo_changed));
	_quantize_checkbutton->signal_toggled().connect(
		sigc::mem_fun(this, &MachinaGUI::quantize_changed));
	_quantize_spinbutton->signal_changed().connect(
		sigc::mem_fun(this, &MachinaGUI::quantize_changed));
	
	_mutate_button->signal_clicked().connect(sigc::bind(
		sigc::mem_fun(this, &MachinaGUI::random_mutation), SharedPtr<Machine>()));
	_compress_button->signal_clicked().connect(sigc::hide_return(sigc::bind(
		sigc::mem_fun(this, &MachinaGUI::mutate), SharedPtr<Machine>(), 0)));
	_add_node_button->signal_clicked().connect(sigc::bind(
		sigc::mem_fun(this, &MachinaGUI::mutate), SharedPtr<Machine>(), 1));
	_remove_node_button->signal_clicked().connect(sigc::bind(
		sigc::mem_fun(this, &MachinaGUI::mutate), SharedPtr<Machine>(), 2));
	_adjust_node_button->signal_clicked().connect(sigc::bind(
		sigc::mem_fun(this, &MachinaGUI::mutate), SharedPtr<Machine>(), 3));
	_add_edge_button->signal_clicked().connect(sigc::bind(
		sigc::mem_fun(this, &MachinaGUI::mutate), SharedPtr<Machine>(), 4));
	_remove_edge_button->signal_clicked().connect(sigc::bind(
		sigc::mem_fun(this, &MachinaGUI::mutate), SharedPtr<Machine>(), 5));
	_adjust_edge_button->signal_clicked().connect(sigc::bind(
		sigc::mem_fun(this, &MachinaGUI::mutate), SharedPtr<Machine>(), 6));

	connect_widgets();
		
	_canvas->show();

	_main_window->present();

	_clock_checkbutton->set_active(true);
	_quantize_checkbutton->set_active(false);
	update_toolbar();

	// Idle callback to drive the maid (collect garbage)
	Glib::signal_timeout().connect(
		sigc::bind_return(
			sigc::mem_fun(_maid.get(), &Raul::Maid::cleanup),
			true),
		1000);
	
	// Idle callback to update node states
	Glib::signal_timeout().connect(sigc::mem_fun(this, &MachinaGUI::idle_callback), 100);
	
#ifdef HAVE_EUGENE
	_load_target_button->signal_clicked().connect(sigc::mem_fun(this, &MachinaGUI::load_target_clicked));
	_evolve_button->signal_clicked().connect(sigc::mem_fun(this, &MachinaGUI::evolve_toggled));
	Glib::signal_timeout().connect(sigc::mem_fun(this, &MachinaGUI::evolve_callback), 1000);
#else
	_evolve_button->set_sensitive(false);
	_load_target_button->set_sensitive(false);
#endif
	
	_canvas->build(engine->machine(), _menu_view_labels->get_active());
}


MachinaGUI::~MachinaGUI() 
{
}


#ifdef HAVE_EUGENE
bool
MachinaGUI::evolve_callback() 
{
	if (_evolve) {
		if (_evolver->improvement()) {
			//_engine->driver()->set_machine(_evolver->best());
			_canvas->build(_evolver->best(), _menu_view_labels->get_active());
		}
	}

	return true;
}
#endif


bool
MachinaGUI::idle_callback() 
{
	const bool show_labels = _menu_view_labels->get_active();

	for (ItemList::iterator i = _canvas->items().begin(); i != _canvas->items().end(); ++i) {
		const SharedPtr<NodeView> nv = PtrCast<NodeView>(*i);
		if (nv && nv->node()->changed())
			nv->update_state(show_labels);
	}

	return true;
}


bool
MachinaGUI::scrolled_window_event(GdkEvent* event)
{
	if (event->type == GDK_KEY_PRESS) {
		if (event->key.keyval == GDK_Delete) {
			
			ItemList selection = _canvas->selected_items();
			_canvas->clear_selection();

			for (ItemList::iterator i = selection.begin();
					i != selection.end(); ++i) {
				SharedPtr<NodeView> view = PtrCast<NodeView>(*i);
				if (view) {
					machine()->remove_node(view->node());
					_canvas->remove_item(view);
				}
			}

			return true;
		}
	}
	
	return false;
}


void
MachinaGUI::arrange()
{
	_canvas->arrange(_menu_view_time_edges->get_active());
}


void
MachinaGUI::load_target_clicked()
{
	Gtk::FileChooserDialog dialog(*_main_window,
			"Load MIDI file for evolution", Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.set_local_only(false);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
	
	Gtk::FileFilter filt;
	filt.add_pattern("*.mid");
	filt.set_name("MIDI Files");
	dialog.set_filter(filt);

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK)
		_target_filename = dialog.get_filename();
}


#ifdef HAVE_EUGENE
void
MachinaGUI::evolve_toggled()
{
	if (_evolve_button->get_active()) {
		_evolver = SharedPtr<Evolver>(new Evolver(_unit, _target_filename, _engine->machine()));
		_evolve = true;
		stop_clicked();
		_engine->driver()->set_machine(SharedPtr<Machine>());
		_evolver->start();
	} else {
		_evolver->stop();
		_evolve = false;
		SharedPtr<Machine> new_machine = SharedPtr<Machine>(new Machine(*_evolver->best()));
		_engine->driver()->set_machine(new_machine);
		_canvas->build(new_machine, _menu_view_labels->get_active());
		new_machine->activate();
		_engine->driver()->activate();
	}
}
#endif


void
MachinaGUI::random_mutation(SharedPtr<Machine> machine)
{
	if (!machine)
		machine = _engine->machine();

	mutate(machine, machine->nodes().size() < 2 ? 1 : rand() % 7);
}


void
MachinaGUI::mutate(SharedPtr<Machine> machine, unsigned mutation)
{
	if (!machine)
		machine = _engine->machine();

	using namespace Mutation;

	switch (mutation) {
		case 0:
			Compress().mutate(*machine.get());
			_canvas->build(machine, _menu_view_labels->get_active());
			break;
		case 1:
			AddNode().mutate(*machine.get());
			_canvas->build(machine, _menu_view_labels->get_active());
			break;
		case 2:
			RemoveNode().mutate(*machine.get());
			_canvas->build(machine, _menu_view_labels->get_active());
			break;
		case 3:
			AdjustNode().mutate(*machine.get());
			idle_callback(); // update nodes
			break;
		case 4:
			AddEdge().mutate(*machine.get());
			_canvas->build(machine, _menu_view_labels->get_active());
			break;
		case 5:
			RemoveEdge().mutate(*machine.get());
			_canvas->build(machine, _menu_view_labels->get_active());
			break;
		case 6:
			AdjustEdge().mutate(*machine.get());
			_canvas->update_edges();
			break;
		default: throw;
	}
}


void
MachinaGUI::update_toolbar()
{
	_record_button->set_active(_engine->driver()->recording());
	_play_button->set_active(_engine->machine()->is_activated());
	_bpm_spinbutton->set_sensitive(_clock_checkbutton->get_active());
	_quantize_spinbutton->set_sensitive(_quantize_checkbutton->get_active());
}


void
MachinaGUI::quantize_changed()
{
	if (_quantize_checkbutton->get_active()) {
		_engine->set_quantization(1/(double)_quantize_spinbutton->get_value_as_int());
	} else {
		_engine->set_quantization(0.0);
	}
	update_toolbar();
}


void
MachinaGUI::tempo_changed()
{
	_engine->set_bpm(_bpm_spinbutton->get_value_as_int());
}


void
MachinaGUI::zoom(double z)
{
	_canvas->set_zoom(z);
}


/** Update the sensitivity status of menus to reflect the present.
 */
void
MachinaGUI::connect_widgets()
{
}

using namespace std;


void
MachinaGUI::menu_file_quit() 
{
	_main_window->hide();
}


void
MachinaGUI::menu_file_open() 
{
	Gtk::FileChooserDialog dialog(*_main_window, "Open Machine", Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.set_local_only(false);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
	
	Gtk::FileFilter filt;
	filt.add_pattern("*.machina.ttl");
	filt.set_name("Machina Machines (Turtle/RDF)");
	dialog.set_filter(filt);

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK) {
		SharedPtr<Machina::Machine> new_machine = _engine->import_machine(dialog.get_uri());
		if (new_machine) {
			_canvas->destroy();
			_canvas->build(new_machine, _menu_view_labels->get_active());
			_save_uri = dialog.get_uri();
		}
	}
}


void
MachinaGUI::menu_file_save() 
{
	if (_save_uri == "" || _save_uri.substr(0, 5) != "file:") {
		menu_file_save_as();
	} else {
		if (!raptor_uri_uri_string_is_file_uri((const unsigned char*)_save_uri.c_str()))
			menu_file_save_as();
		
		Redland::Model model(_engine->rdf_world());
		model.set_base_uri(_save_uri);
		machine()->write_state(model);
		model.serialise_to_file(_save_uri);
	}
}


void
MachinaGUI::menu_file_save_as() 
{
	Gtk::FileChooserDialog dialog(*_main_window, "Save Machine",
			Gtk::FILE_CHOOSER_ACTION_SAVE);
	
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);	
	
	if (_save_uri.length() > 0)
		dialog.set_uri(_save_uri);
	
	const int result = dialog.run();
	
	assert(result == Gtk::RESPONSE_OK
			|| result == Gtk::RESPONSE_CANCEL
			|| result == Gtk::RESPONSE_NONE);
	
	if (result == Gtk::RESPONSE_OK) {	
		string filename = dialog.get_filename();

		if (filename.length() < 13 || filename.substr(filename.length()-12) != ".machina.ttl")
			filename += ".machina.ttl";

		Glib::ustring uri = Glib::filename_to_uri(filename);
			
		bool confirm = false;
		std::fstream fin;
		fin.open(filename.c_str(), std::ios::in);
		if (fin.is_open()) {  // File exists
			string msg = "A file named \"";
			msg += filename + "\" already exists.\n\nDo you want to replace it?";
			Gtk::MessageDialog confirm_dialog(dialog,
				msg, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
			if (confirm_dialog.run() == Gtk::RESPONSE_YES)
				confirm = true;
			else
				confirm = false;
		} else {  // File doesn't exist
			confirm = true;
		}
		fin.close();
		
		if (confirm) {
			Redland::Model model(_engine->rdf_world());
			_save_uri = uri;
			model.set_base_uri(_save_uri);
			_engine->machine()->write_state(model);
			model.serialise_to_file(_save_uri);
		}
	}
}


void
MachinaGUI::menu_import_midi()
{
	Gtk::FileChooserDialog dialog(*_main_window, "Learn from MIDI file",
			Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
	
	Gtk::FileFilter filt;
	filt.add_pattern("*.mid");
	filt.set_name("MIDI Files");
	dialog.set_filter(filt);
	
	Gtk::HBox* extra_widget = Gtk::manage(new Gtk::HBox());
	Gtk::SpinButton* length_sb = Gtk::manage(new Gtk::SpinButton());
	length_sb->set_increments(1, 10);
	length_sb->set_range(0, INT_MAX);
	length_sb->set_value(0);
	extra_widget->pack_start(*Gtk::manage(new Gtk::Label("")), true, true);
	extra_widget->pack_start(*Gtk::manage(new Gtk::Label("Maximum Length (0 = unlimited): ")), false, false);
	extra_widget->pack_start(*length_sb, false, false); 
	dialog.set_extra_widget(*extra_widget);
	extra_widget->show_all();
	

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK) {
		SharedPtr<Machina::SMFDriver> file_driver(new Machina::SMFDriver());
		
		double length_dbl = length_sb->get_value_as_int();
		Raul::TimeStamp length(_unit, length_dbl);

		SharedPtr<Machina::Machine> machine = file_driver->learn(dialog.get_filename(), 0.0, length);
		
		if (machine) {
			dialog.hide();
			machine->activate();
			machine->reset(machine->time());
			_canvas->build(machine, _menu_view_labels->get_active());
			_engine->driver()->set_machine(machine);
		} else {
			Gtk::MessageDialog msg_dialog(dialog, "Error loading MIDI file",
					false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			msg_dialog.run();
		} 
	}
}


void
MachinaGUI::menu_export_midi()
{
	Gtk::FileChooserDialog dialog(*_main_window, "Export to a MIDI file",
			Gtk::FILE_CHOOSER_ACTION_SAVE);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
	
	Gtk::FileFilter filt;
	filt.add_pattern("*.mid");
	filt.set_name("MIDI Files");
	dialog.set_filter(filt);

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK) {
		SharedPtr<Machina::SMFDriver> file_driver(new Machina::SMFDriver());
		_engine->driver()->deactivate();
		const SharedPtr<Machina::Machine> m = _engine->machine();
		m->set_sink(file_driver->writer());
		file_driver->writer()->start(dialog.get_filename(), TimeStamp(_unit, 0.0));
		file_driver->run(m, TimeStamp(_unit, 32.0)); // TODO: solve halting problem
		m->set_sink(_engine->driver());
		m->reset(m->time());
		file_driver->writer()->finish();
		_engine->driver()->activate();
	}
}


void
MachinaGUI::menu_export_graphviz()
{
	Gtk::FileChooserDialog dialog(*_main_window, "Export to a GraphViz DOT file",
			Gtk::FILE_CHOOSER_ACTION_SAVE);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

	const int result = dialog.run();

	if (result == Gtk::RESPONSE_OK)
		_canvas->render_to_dot(dialog.get_filename());
}


void
MachinaGUI::show_toolbar_toggled()
{
	if (_menu_view_toolbar->get_active())
		_toolbar->show();
	else
		_toolbar->hide();
}


void
MachinaGUI::show_labels_toggled()
{
	const bool show = _menu_view_labels->get_active();
	
	for (ItemList::iterator i = _canvas->items().begin(); i != _canvas->items().end(); ++i) {
		const SharedPtr<NodeView> nv = PtrCast<NodeView>(*i);
		if (nv)
			nv->show_label(show);
	}

	for (ConnectionList::iterator c = _canvas->connections().begin();
			c != _canvas->connections().end(); ++c) {
		const SharedPtr<EdgeView> ev = PtrCast<EdgeView>(*c);
		if (ev)
			ev->show_label(show);
	}
}


void
MachinaGUI::menu_help_about() 
{
	_about_window->set_transient_for(*_main_window);
	_about_window->show();
}


void
MachinaGUI::menu_help_help() 
{
	_help_dialog->set_transient_for(*_main_window);
	_help_dialog->run();
	_help_dialog->hide();
}


void
MachinaGUI::record_toggled()
{
	if (_record_button->get_active() && ! _engine->driver()->recording()) {
		_engine->driver()->start_record();
	} else if (_engine->driver()->recording()) {
		_engine->driver()->finish_record();
		_canvas->build(_engine->machine(), _menu_view_labels->get_active());
		update_toolbar();
	}
}


void
MachinaGUI::stop_clicked()
{
	_play_button->set_active(false);

	if (_engine->driver()->recording()) {
		_engine->driver()->stop();
		_engine->machine()->deactivate();
		_canvas->build(_engine->machine(), _menu_view_labels->get_active());
	} else {
		_engine->driver()->stop();
		_engine->machine()->deactivate();
	}

	update_toolbar();
}


void
MachinaGUI::play_toggled()
{
	if (_play_button->get_active())
		_engine->machine()->activate();
	else
		_engine->machine()->deactivate();
}


