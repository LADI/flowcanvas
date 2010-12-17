/* This file is part of redlandmm.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
 *
 * redlandmm is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * redlandmm is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef REDLANDMM_NODE_HPP
#define REDLANDMM_NODE_HPP

#include <cassert>
#include <cstring>
#include <locale>
#include <stdexcept>
#include <string>

#include <glibmm/ustring.h>
#include <redland.h>

#include "redlandmm/World.hpp"
#include "redlandmm/Wrapper.hpp"

#define REDLANDMM_XSD "http://www.w3.org/2001/XMLSchema#"

namespace Redland {

class World;

/** An RDF Node (resource, literal, etc)
 */
class Node : public Wrapper<librdf_node> {
public:
	enum Type {
		UNKNOWN  = LIBRDF_NODE_TYPE_UNKNOWN,
		RESOURCE = LIBRDF_NODE_TYPE_RESOURCE,
		LITERAL  = LIBRDF_NODE_TYPE_LITERAL,
		BLANK    = LIBRDF_NODE_TYPE_BLANK
	};

	Node() : _world(NULL) {}

	Node(World& world, Type t, const std::string& s);
	Node(World& world);
	Node(World& world, librdf_node* node);
	Node(const Node& other);
	~Node();

	Type type() const { return ((_c_obj) ? (Type)librdf_node_get_type(_c_obj) : UNKNOWN); }

	World* world() const { return _world; }

	librdf_node* get_node() const { return _c_obj; }
	librdf_uri*  get_uri()  const { return librdf_node_get_uri(_c_obj); }

	bool is_valid() const { return type() != UNKNOWN; }

	inline operator const char*() const { return to_c_string(); }

	const Node& operator=(const Node& other) {
		if (_c_obj)
			librdf_free_node(_c_obj);
		_world = other._world;
		_c_obj = (other._c_obj) ? librdf_new_node_from_node(other._c_obj) : NULL;
		return *this;
	}

	inline bool operator==(const Node& other) const {
		return librdf_node_equals(_c_obj, other._c_obj);
	}

	const char* to_c_string() const;
	std::string to_string() const;

	Glib::ustring to_turtle_token() const;

	inline bool is_literal_type(const char* type_uri) const;

	inline bool is_resource() const { return _c_obj && librdf_node_is_resource(_c_obj); }
	inline bool is_blank()    const { return _c_obj && librdf_node_is_blank(_c_obj); }
	inline bool is_int()      const { return is_literal_type(REDLANDMM_XSD "integer"); }
	inline bool is_float()    const { return is_literal_type(REDLANDMM_XSD "decimal"); }
	inline bool is_bool()     const { return is_literal_type(REDLANDMM_XSD "boolean"); }

	int   to_int()   const;
	float to_float() const;
	bool  to_bool()  const;

	static Node blank_id(World& world, const std::string base="b") {
		const uint64_t num = world.blank_id();
		std::ostringstream ss;
		ss << base << num;
		return Node(world, Node::BLANK, ss.str());
	}

private:
	World* _world;
};


class Resource : public Node {
public:
	Resource(World& world, const std::string& s) : Node(world, Node::RESOURCE, s) {}
};


class Literal : public Node {
public:
	Literal(World& world, const std::string& s) : Node(world, Node::LITERAL, s) {}
};


inline
Node::Node(World& world, Type type, const std::string& s)
	: _world(&world)
{
	Glib::Mutex::Lock lock(world.mutex(), Glib::TRY_LOCK);

	switch (type) {
	case RESOURCE:
		_c_obj = librdf_new_node_from_uri_string(
			world.world(), (const unsigned char*)world.expand_uri(s).c_str());
		break;
	case LITERAL:
		_c_obj = librdf_new_node_from_literal(
			world.world(), (const unsigned char*)s.c_str(), NULL, 0);
		break;
	case BLANK:
		_c_obj = librdf_new_node_from_blank_identifier(
			world.world(), (const unsigned char*)s.c_str());
		break;
	default:
		_c_obj = NULL;
	}

	assert(this->type() == type);
}

inline
Node::Node(World& world)
	: _world(&world)
{
	Glib::Mutex::Lock lock(world.mutex(), Glib::TRY_LOCK);
	_c_obj = librdf_new_node(world.world());
}

inline
Node::Node(World& world, librdf_node* node)
	: _world(&world)
{
	Glib::Mutex::Lock lock(world.mutex(), Glib::TRY_LOCK);
	_c_obj = node ? librdf_new_node_from_node(node) : NULL;
}

inline
Node::Node(const Node& other)
	: Wrapper<librdf_node>()
	, _world(other.world())
{
	if (_world) {
		Glib::Mutex::Lock lock(_world->mutex(), Glib::TRY_LOCK);
		_c_obj = (other._c_obj ? librdf_new_node_from_node(other._c_obj) : NULL);
	}

	assert(to_string() == other.to_string());
}

inline
Node::~Node()
{
	if (_world) {
		Glib::Mutex::Lock lock(_world->mutex(), Glib::TRY_LOCK);
		if (_c_obj)
			librdf_free_node(_c_obj);
	}
}


inline std::string
Node::to_string() const
{
	return std::string(to_c_string());
}


inline const char*
Node::to_c_string() const
{
	const Type type = this->type();
	if (type == RESOURCE) {
		assert(librdf_node_get_uri(_c_obj));
		return (const char*)librdf_uri_as_string(librdf_node_get_uri(_c_obj));
	} else if (type == LITERAL) {
		return (const char*)librdf_node_get_literal_value(_c_obj);
	} else if (type == BLANK) {
		return (const char*)librdf_node_get_blank_identifier(_c_obj);
	} else {
		return "";
	}
}


inline Glib::ustring
Node::to_turtle_token() const
{
	assert(type() == RESOURCE);
	assert(librdf_node_get_uri(_c_obj));
	Glib::ustring str = "<";
	str.append((const char*)librdf_uri_as_string(librdf_node_get_uri(_c_obj)));
	str.append(">");
	return str;
}

inline bool
Node::is_literal_type(const char* type_uri) const
{
	if (_c_obj && librdf_node_get_type(_c_obj) == LIBRDF_NODE_TYPE_LITERAL) {
		librdf_uri* datatype_uri = librdf_node_get_literal_value_datatype_uri(_c_obj);
		if (datatype_uri && !strcmp((const char*)librdf_uri_as_string(datatype_uri), type_uri))
			return true;
	}
	return false;
}
	
inline int
Node::to_int() const
{
	assert(is_int());
	std::locale c_locale("C");
	std::stringstream ss((const char*)librdf_node_get_literal_value(_c_obj));
	ss.imbue(c_locale);
	int i = 0;
	ss >> i;
	return i;
}

inline float
Node::to_float() const
{
	assert(is_float());
	std::locale c_locale("C");
	std::stringstream ss((const char*)librdf_node_get_literal_value(_c_obj));
	ss.imbue(c_locale);
	float f = 0.0f;
	ss >> f;
	return f;
}

inline bool
Node::to_bool() const
{
	assert(is_bool());
	return !strcmp((const char*)librdf_node_get_literal_value(_c_obj), "true");
}

} // namespace Redland

#endif // REDLANDMM_NODE_HPP
