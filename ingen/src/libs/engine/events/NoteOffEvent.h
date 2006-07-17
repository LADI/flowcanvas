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

#ifndef NOTEOFFEVENT_H
#define NOTEOFFEVENT_H

#include "Event.h"
#include "types.h"
#include <string>
using std::string;

namespace Om {

class Node;


/** A note off event.
 *
 * \ingroup engine
 */
class NoteOffEvent : public Event
{
public:
	NoteOffEvent(CountedPtr<Responder> responder, samplecount timestamp, Node* node, uchar note_num);
	NoteOffEvent(CountedPtr<Responder> responder, samplecount timestamp, const string& node_path, uchar note_num);
	
	void execute(samplecount offset);
	void post_process();

private:
	Node*  m_node;
	string m_node_path;
	uchar  m_note_num;
};


} // namespace Om

#endif // NOTEOFFEVENT_H