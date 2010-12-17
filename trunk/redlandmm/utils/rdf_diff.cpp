/* This file is part of redlandmm.
 * Copyright (C) 2009 David Robillard <http://drobilla.net>
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

#include <stdio.h>
#include <string>
#include "redlandmm/Model.hpp"
#include "redlandmm/World.hpp"
#include "redlandmm/Delta.hpp"

using namespace std;
using namespace Redland;

int
main(int argc, char** argv)
{
	if (argc != 3) {
		printf("Usage: %s FROM TO\n", argv[0]);
		printf("Compare two RDF documents\n\n");
		printf("FROM and TO are considered file paths if they begin with / or .\n");
		printf("otherwise they are considered URIs\n");
		return 1;
	}

#define ARG_URI(i) (argv[i][0] == '/' || argv[i][0] == '.') ? string("file:") + argv[i] : argv[i]

	const string from = ARG_URI(1);
	const string to   = ARG_URI(2);

	Redland::World world;

	Model a(world, from, from);
	Model b(world, to, to);

	Delta delta(a, b);

	Model output(world);
	delta.serialise(output, "turtle", "");
	output.serialise_to_file_handle(stdout);

	return 0;
}

