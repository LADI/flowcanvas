/* This file is part of Patchage.
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 *
 * Patchage is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Patchage is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PATCHAGE_DBUS_HPP
#define PATCHAGE_DBUS_HPP

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

class Patchage;

class DBus {
public:
	DBus(Patchage* app);

	bool call(
		bool response_expected,
		const char* service,
		const char* object,
		const char* iface,
		const char* method,
		DBusMessage** reply_ptr,
		int in_type,
		va_list ap);

	bool call(
		bool response_expected,
		const char* service,
		const char* object,
		const char* iface,
		const char* method,
		DBusMessage** reply_ptr,
		int in_type,
		...);

	DBusConnection* connection() { return _connection; }
	DBusError&      error()      { return _error; }

private:
	Patchage*       _app;
	DBusConnection* _connection;
	DBusError       _error;
};

#endif // PATCHAGE_DBUS_HPP
