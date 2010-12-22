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

#ifndef MACHINA_EDGEVIEW_H
#define MACHINA_EDGEVIEW_H

#include "flowcanvas/Connection.hpp"

namespace Machina { class Edge; }
class NodeView;


class EdgeView : public FlowCanvas::Connection {
public:
	EdgeView(SharedPtr<FlowCanvas::Canvas> canvas,
	         SharedPtr<NodeView>           src,
	         SharedPtr<NodeView>           dst,
	         SharedPtr<Machina::Edge>      edge);

	void show_label(bool show);
	void update();

	virtual double length_hint() const;

private:
	bool on_event(GdkEvent* ev);

	SharedPtr<Machina::Edge> _edge;
};


#endif // MACHINA_EDGEVIEW_H
