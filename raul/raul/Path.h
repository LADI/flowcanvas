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

#ifndef RAUL_PATH_H
#define RAUL_PATH_H

#include <cctype>
#include <string>
#include <cassert>
using std::string;

	
/** Simple wrapper around standard string with useful path-specific methods.
 *
 * This enforces that a Path is a valid OSC path (though it is used for
 * GraphObject paths, which aren't directly OSC paths but a portion of one).
 *
 * A path is divided by slashes (/).  The first character MUST be a slash, and
 * the last character MUST NOT be a slash (except in the special case of the 
 * root path "/", which is the only valid single-character path).
 *
 * Valid characters are the 95 printable ASCII characters (32-126), excluding:
 * space # * , ? [ ] { }
 */
class Path : public std::basic_string<char> {
public:

	/** Construct a Path from an std::string.
	 *
	 * It is a fatal error to construct a Path from an invalid string,
	 * use is_valid first to check.
	 */
	Path(const std::basic_string<char>& path)
	: std::basic_string<char>(path)
	{
		assert(is_valid(path));
	}
	

	/** Construct a Path from a C string.
	 *
	 * It is a fatal error to construct a Path from an invalid string,
	 * use is_valid first to check.
	 */
	Path(const char* cpath)
	: std::basic_string<char>(cpath)
	{
		assert(is_valid(cpath));
	}
	

	static bool is_valid(const std::basic_string<char>& path)
	{
		if (path.length() == 0)
			return false;

		// Must start with a /
		if (path.at(0) != '/')
			return false;
		
		// Must not end with a slash unless "/"
		if (path.length() > 1 && path.at(path.length()-1) == '/')
			return false;
	
		assert(path.find_last_of("/") != string::npos);
		
		// Double slash not allowed
		if (path.find("//") != string::npos)
			return false;

		// All characters must be printable ASCII
		for (size_t i=0; i < path.length(); ++i)
			if (path.at(i) < 32 || path.at(i) > 126)
				return false;

		// Disallowed characters
		if (       path.find(" ") != string::npos 
				|| path.find("#") != string::npos
				|| path.find("*") != string::npos
				|| path.find(",") != string::npos
				|| path.find("?") != string::npos
				|| path.find("[") != string::npos
				|| path.find("]") != string::npos
				|| path.find("{") != string::npos
				|| path.find("}") != string::npos)
			return false;

		return true;
	}
	
	static bool is_valid_name(const std::basic_string<char>& path)
	{
		return is_valid(string("/").append(path));
	}


	/** Convert a string to a valid full path.
	 *
	 * This will make a best effort at turning @a str into a complete, valid
	 * Path, and will always return one.
	 */
	static string pathify(const std::basic_string<char>& str)
	{
		string path = str;

		if (path.length() == 0)
			return "/"; // this might not be wise

		// Must start with a /
		if (path.at(0) != '/')
			path = string("/").append(path);
		
		// Must not end with a slash unless "/"
		if (path.length() > 1 && path.at(path.length()-1) == '/')
			path = path.substr(0, path.length()-1); // chop trailing slash
	
		assert(path.find_last_of("/") != string::npos);
		
		replace_invalid_chars(path, false);

		assert(is_valid(path));
		
		return path;
	}
	
	/** Convert a string to a valid name (or "method" - tokens between slashes)
	 *
	 * This will strip all slashes, etc, and always return a valid name/method.
	 */
	static string nameify(const std::basic_string<char>& str)
	{
		string name = str;

		if (name.length() == 0)
			return "."; // this might not be wise

		replace_invalid_chars(name, true);

		assert(is_valid(string("/") + name));

		return name;
	}


	/** Replace any invalid characters in @a str with a suitable replacement.
	 *
	 * Makes a pretty name - underscores are a valid character, but this chops
	 * both spaces and underscores, uppercasing the next letter, to create
	 * uniform CamelCase names that look nice
	 */
	static void replace_invalid_chars(string& str, bool replace_slash = false)
	{
		for (size_t i=0; i < str.length(); ++i) {
			if (str[i] == ' ' || str[i] == '_') {
				str[i+1] = std::toupper(str[i+1]); // capitalize next char
				str = str.substr(0, i) + str.substr(i+1); // chop space/underscore
			} else if (str[i] ==  '[' || str[i] ==  '{') {
				str[i] = '(';
			} else if (str[i] ==  ']' || str[i] ==  '}') {
				str[i] = ')';
			} else if (str[i] < 32 || str.at(i) > 126
					|| str[i] ==  '#' 
					|| str[i] ==  '*' 
					|| str[i] ==  ',' 
					|| str[i] ==  '?' 
					|| (replace_slash && str[i] ==  '/')) {
				str[i] = '.';
			}
		}

		// Chop brackets
		while (true) {

			const string::size_type open  = str.find("(");
			const string::size_type close = str.find(")");

			if (open != string::npos) {
				if (close != string::npos)
					str.erase(open, (close - open) + 1);
			} else {
				break;
			}

		}
	}

	
	/** Return the name of this object (everything after the last '/').
	 * This is the "method name" for OSC paths.
	 */
	inline std::basic_string<char> name() const
	{
		if ((*this) == "/")
			return "";
		else
			return substr(find_last_of("/")+1);
	}
	
	
	/** Return the parent's path.
	 *
	 * Calling this on the path "/" will return "/".
	 * This is the (deepest) "container path" for OSC paths.
	 */
	inline Path parent() const
	{
		std::basic_string<char> parent = substr(0, find_last_of("/"));
		return (parent == "") ? "/" : parent;
	}
	
	/** Parent path with a "/" appended.
	 *
	 * This exists to avoid needing to be careful about the special case of "/".
	 * To create a child of a path, use parent.base() + child_name.
	 * Returned value is always a valid path, with the single exception that
	 * the last character is "/".
	 */
	inline string base() const
	{
		if ((*this) == "/")
			return *this;
		else
			return (*this) + "/";
	}

	inline bool is_child_of(const Path& parent) const
	{
		return (length() > parent.length() && substr(0, parent.length()) == parent);
	}
	
	inline bool is_parent_of(const Path& child) const
	{
		return child.is_child_of(*this);
	}
};
	

#endif // RAUL_PATH_H