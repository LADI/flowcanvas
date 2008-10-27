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

#ifndef FLOWCANVAS_ELLIPSE_HPP
#define FLOWCANVAS_ELLIPSE_HPP

#include <string>
#include <map>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <libgnomecanvasmm.h>
#include <flowcanvas/Connectable.hpp>
#include <flowcanvas/Item.hpp>

namespace FlowCanvas {
	
class Canvas;


/** A named circle (possibly).
 *
 * Unlike a Module, this doesn't contain ports, but is directly Connectable itself
 * (think your classic circles 'n' lines diagram, ala FSM).
 *
 * \ingroup FlowCanvas
 */
class Ellipse : public Item, public Connectable
{
public:
	Ellipse(boost::shared_ptr<Canvas> canvas,
	        const std::string&        name,
	        double                    x,
	        double                    y,
	        double                    x_radius,
	        double                    y_radius,
	        bool                      show_title = true);

	virtual ~Ellipse();
	
	Gnome::Art::Point src_connection_point() {
		return Gnome::Art::Point(property_x(), property_y());
	}
	
	Gnome::Art::Point dst_connection_point(const Gnome::Art::Point& src);

	void add_connection(boost::shared_ptr<Connection> c);

	bool point_is_within(double x, double y);

	void zoom(double z);
	void resize();
	
	virtual void move(double dx, double dy);
	virtual void move_to(double x, double y);

	virtual void load_location()  {}
	virtual void store_location() {}
	
	virtual void set_name(const std::string& n);

	void set_width(double w);
	
	void set_height(double h);

	double border_width() const { return _border_width; }
	void   set_border_width(double w);

	void select_tick();
	void set_selected(bool b);
	
	void set_highlighted(bool b);
	void set_border_color(uint32_t c);
	void set_base_color(uint32_t c);
	void set_default_base_color();

protected:
	bool is_within(const Gnome::Canvas::Rect& rect);

	double _border_width;
	bool   _title_visible;

	Gnome::Canvas::Ellipse _ellipse;
	Gnome::Canvas::Text*   _label;
};


} // namespace FlowCanvas

#endif // FLOWCANVAS_ELLIPSE_HPP
