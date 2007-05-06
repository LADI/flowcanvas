/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#include <raul/Process.h>
#include "engine.h"
#include "Engine.h"
#include "QueuedEngineInterface.h"
#include "tuning.h"
#include "util.h"

namespace Ingen {

/*
void
catch_int(int)
{
	signal(SIGINT, catch_int);
	signal(SIGTERM, catch_int);

	std::cout << "[Main] Ingen interrupted." << std::endl;
	engine->quit();
}
*/

Engine*
new_engine()
{
	set_denormal_flags();
	return new Engine();
}


bool
launch_osc_engine(int port)
{
	char port_str[6];
	snprintf(port_str, 6, "%u", port);
	const string cmd = string("ingen -e --engine-port=").append(port_str);

	if (Raul::Process::launch(cmd)) {
		return true;
		//return SharedPtr<EngineInterface>(new OSCEngineSender(
		//			string("osc.udp://localhost:").append(port_str)));
	} else {
		cerr << "Failed to launch engine process." << endl;
		//return SharedPtr<EngineInterface>();
		return false;
	}
}

/*
void
run(int port)
{
	signal(SIGINT, catch_int);
	signal(SIGTERM, catch_int);

	set_denormal_flags();

	Engine* engine = new_engine();

	engine->start_jack_driver();
	engine->start_osc_driver(port);

	engine->activate();

	engine->main();

	delete engine;
}
*/

} // namespace Ingen
