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

#ifndef PORT_H
#define PORT_H

#include <cstdlib>
#include <string>
#include "types.h"
#include "GraphObject.h"
#include "DataType.h"

using std::string;

namespace Om {

class Node;


/** A port on a Node.
 *
 * This is a non-template abstract base class, which basically exists so
 * things can pass around Port pointers and not have to worry about type,
 * templates, etc.
 *
 * \ingroup engine
 */
class Port : public GraphObject
{
public:
	virtual ~Port() {}

	void add_to_store();
	void remove_from_store();
	
	/** A port's parent is always a node, so static cast should be safe */
	Node* parent_node() const { return (Node*)_parent; }

	/** Called once per process cycle */
	virtual void process(samplecount nframes) = 0;
	
	/** Empty buffer contents completely (ie silence) */
	virtual void clear_buffers() = 0;
	
	virtual bool is_input()  const = 0;
	virtual bool is_output() const = 0;

	bool     is_sample()   const { return false; }
	size_t   num()         const { return _index; }
	size_t   poly()        const { return _poly; }
	DataType type()        const { return _type; }
	size_t   buffer_size() const { return _buffer_size; }

protected:
	Port(Node* const node, const string& name, size_t index, size_t poly, DataType type, size_t buffer_size);
	
	// Prevent copies (undefined)
	Port(const Port&);
	Port& operator=(const Port&);

	size_t    _index;
	size_t    _poly;
	DataType  _type;
	size_t    _buffer_size;
};


} // namespace Om

#endif // PORT_H