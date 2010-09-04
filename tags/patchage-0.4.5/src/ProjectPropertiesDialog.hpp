// -*- Mode: C++ ; indent-tabs-mode: t -*-
/* This file is part of Patchage.
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

#ifndef PATCHAGE_PROJECT_PROPERTIES_DIALOG_HPP
#define PATCHAGE_PROJECT_PROPERTIES_DIALOG_HPP

#include <boost/shared_ptr.hpp>

struct ProjectPropertiesDialogImpl;
class Project;

class ProjectPropertiesDialog {
public:
	ProjectPropertiesDialog(Glib::RefPtr<Gnome::Glade::Xml> xml);
	~ProjectPropertiesDialog();

	void run(boost::shared_ptr<Project> project);

private:
	ProjectPropertiesDialogImpl* _impl;
};

#endif // PATCHAGE_PROJECT_PROPERTIES_DIALOG_HPP
