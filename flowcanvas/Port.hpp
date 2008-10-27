/* This file is part of FlowCanvas.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * FlowCanvas is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * FlowCanvas is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef FLOWCANVAS_PORT_HPP
#define FLOWCANVAS_PORT_HPP

#include <string>
#include <list>
#include <vector>
#include <inttypes.h>
#include <libgnomecanvasmm.h>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <flowcanvas/Connectable.hpp>

namespace FlowCanvas {
	
class Connection;
class Module;


static const int PORT_LABEL_SIZE = 8000; // in thousandths of a point


/** A port on a Module.
 *
 * This is a group that contains both the label and rectangle for a port.
 *
 * \ingroup FlowCanvas
 */
class Port : public Gnome::Canvas::Group, public Connectable
{
public:
	Port(boost::shared_ptr<Module> module,
	     const std::string&        name,
	     bool                      is_input,
	     uint32_t                  color);
	
	virtual ~Port();
	
	void disconnect_all();
	
	virtual Gnome::Art::Point src_connection_point();
	virtual Gnome::Art::Point dst_connection_point(const Gnome::Art::Point& src);
	
	boost::weak_ptr<Module> module() const { return _module; }
	
	void set_fill_color(uint32_t c) { _rect->property_fill_color_rgba() = c; }
	
	void set_selected(bool b);
	bool selected() const { return _selected; }

	void set_highlighted(bool highlight,
	                     bool highlight_parent=true,
	                     bool highlight_connections=true,
	                     bool raise_connections=true);
	
	void zoom(float z);
	
	void popup_menu(guint button, guint32 activate_time) {
		if ( ! _menu)
			create_menu();
		if (_menu)
			_menu->popup(button, activate_time);
	}

	virtual void create_menu();
	void         set_menu(Gtk::Menu* m);

	Gtk::Menu* menu() const           { return _menu; }

	double width() const { return _width; }
	void   set_width(double w);
	
	double border_width() const { return _border_width; }
	void   set_border_width(double w);

	const std::string& name() const { return _name; }
	virtual void       set_name(const std::string& n);
	
	bool     is_input()  const { return _is_input; }
	bool     is_output() const { return !_is_input; }
	uint32_t color()     const { return _color; }
	double   height()    const { return _height; }

	virtual bool is_toggled() const { return _toggled; }
	virtual void set_toggled(bool b) { _toggled = b; }
	virtual void toggle(bool signal=true);

	virtual void set_control(float value, bool signal=true);
	virtual void set_control_min(float min) { _control_min = min; set_control(_control_value, false);  }
	virtual void set_control_max(float max) { _control_max = max; set_control(_control_value, false);  }
	
	float control_value() { return _control_value; }
	float control_min()   { return _control_min; }
	float control_max()   { return _control_max; }
	
	void show_control();
	void hide_control();

	inline bool operator==(const std::string& name) { return (_name == name); }

	sigc::signal<void>       signal_renamed;
	sigc::signal<void,float> signal_control_changed;

protected:
	friend class Canvas;

	void on_menu_hide();

	boost::weak_ptr<Module> _module;
	std::string             _name;
	bool                    _is_input;
	double                  _width;
	double                  _height;
	double                  _border_width;
	uint32_t                _color;
	bool                    _selected;

	bool  _toggled;
	float _control_value;
	float _control_min;
	float _control_max;
	
	Gnome::Canvas::Text* _label;
	Gnome::Canvas::Rect* _rect;
	Gnome::Canvas::Rect* _control_rect;
	Gtk::Menu*           _menu;
};

typedef std::vector<boost::shared_ptr<Port> > PortVector;


} // namespace FlowCanvas

#endif // FLOWCANVAS_PORT_HPP
