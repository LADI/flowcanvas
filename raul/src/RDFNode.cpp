/* This file is part of Raul.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Raul is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Raul is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <string>
#include <raul/RDFWorld.h>
#include <raul/RDFNode.h>

using namespace std;

namespace Raul {
namespace RDF {


Node::Node(World& world, Type type, const std::string& s)
{
	if (type == RESOURCE)
		_node = librdf_new_node_from_uri_string(world.world(), (const unsigned char*)s.c_str());
	else if (type == LITERAL)
		_node = librdf_new_node_from_literal(world.world(), (const unsigned char*)s.c_str(), NULL, 0);
	else if (type == BLANK)
		_node = librdf_new_node_from_blank_identifier(world.world(), (const unsigned char*)s.c_str());
	else
		_node = NULL;

	assert(this->type() == type);
}
	

Node::Node(World& world)
	: _node(librdf_new_node(world.world()))
{
}


Node::Node(librdf_node* node)
	: _node(librdf_new_node_from_node(node))
{
	assert(node);
}


Node::Node(const Node& other)
	: _node(other._node ? librdf_new_node_from_node(other._node): NULL)
{
}


Node::~Node()
{
	if (_node)
		librdf_free_node(_node);
}


string
Node::to_string() const
{
	const Type type = this->type();
	if (type == RESOURCE) {
		return string((const char*)librdf_uri_as_string(librdf_node_get_uri(_node)));
	} else if (type == LITERAL) {
		return string((const char*)librdf_node_get_literal_value(_node));
	} else if (type == BLANK) {
		return string((const char*)librdf_node_get_blank_identifier(_node));
	} else {
		return "";
	}
}


string
Node::to_quoted_uri_string() const
{
	assert(type() == RESOURCE);
	string str = "<";
	str.append((const char*)librdf_uri_as_string(librdf_node_get_uri(_node)));
	str.append(">");
	return str;
}


bool
Node::is_int()
{
	if (_node && librdf_node_get_type(_node) == LIBRDF_NODE_TYPE_LITERAL) {
		librdf_uri* datatype_uri = librdf_node_get_literal_value_datatype_uri(_node);
		if (datatype_uri && !strcmp((const char*)librdf_uri_as_string(datatype_uri),
					"http://www.w3.org/2001/XMLSchema#integer"))
			return true;
	}
	return false;
}


int
Node::to_int()
{
	assert(is_int());
	return strtol((const char*)librdf_node_get_literal_value(_node), NULL, 10);
}


bool
Node::is_float()
{
	if (_node && librdf_node_get_type(_node) == LIBRDF_NODE_TYPE_LITERAL) {
		librdf_uri* datatype_uri = librdf_node_get_literal_value_datatype_uri(_node);
		if (datatype_uri && !strcmp((const char*)librdf_uri_as_string(datatype_uri),
					"http://www.w3.org/2001/XMLSchema#decimal"))
			return true;
	}
	return false;
}


float
Node::to_float()
{
	assert(is_float());
	return strtod((const char*)librdf_node_get_literal_value(_node), NULL);
}


} // namespace RDF
} // namespace Raul
