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

#ifndef DESTROYEVENT_H
#define DESTROYEVENT_H

#include "raul/Path.h"
#include "QueuedEvent.h"
#include <string>

using std::string;

template<typename T> class Array;
template<typename T> class ListNode;
template<typename T> class TreeNode;

namespace Ingen {

class GraphObject;
class Patch;
class Node;
class Plugin;
class DisconnectNodeEvent;
class DisconnectPortEvent;


/** An event to remove and delete a Node.
 *
 * \ingroup engine
 */
class DestroyEvent : public QueuedEvent
{
public:
	DestroyEvent(Engine& engine, SharedPtr<Responder> responder, FrameTime timestamp, QueuedEventSource* source, const string& path, bool block = true);
	DestroyEvent(Engine& engine, SharedPtr<Responder> responder, FrameTime timestamp, QueuedEventSource* source, Node* node, bool block = true);
	~DestroyEvent();

	void pre_process();
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();

private:
	Path                    m_path;
	Node*                   m_node; 
	ListNode<Node*>*        m_patch_listnode;
	TreeNode<GraphObject*>* m_store_treenode;
	Array<Node*>*           m_process_order; // Patch's new process order
	DisconnectNodeEvent*    m_disconnect_event;
};


} // namespace Ingen

#endif // DESTROYEVENT_H
