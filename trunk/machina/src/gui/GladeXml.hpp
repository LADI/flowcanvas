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

#include <fstream>
#include <iostream>
#include <string>

#include <gtkmm.h>
#include <libglademm/xml.h>

#include "machina-config.h"

class GladeXml
{
public:
	static Glib::RefPtr<Gnome::Glade::Xml> create() {
		Glib::RefPtr<Gnome::Glade::Xml> xml;

		// Check for the .glade file in current directory
		std::string glade_filename = "./machina.glade";
		std::ifstream fs(glade_filename.c_str());
		if (fs.fail()) { // didn't find it, check PKGDATADIR
			fs.clear();
			glade_filename = MACHINA_DATA_DIR;
			glade_filename += "/machina.glade";

			fs.open(glade_filename.c_str());
			if (fs.fail()) {
				std::cerr << "Unable to find machina.glade in current directory or "
					<< MACHINA_DATA_DIR << "." << std::endl;
				exit(EXIT_FAILURE);
			}
			fs.close();
		}

		try {
			xml = Gnome::Glade::Xml::create(glade_filename);
		} catch(const Gnome::Glade::XmlError& ex) {
			std::cerr << ex.what() << std::endl;
			throw ex;
		}

		return xml;
	}
};

