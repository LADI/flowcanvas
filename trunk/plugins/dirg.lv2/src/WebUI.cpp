/* Dirg
 * Copyright (C) 2010 David Robillard <http://drobilla.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdio.h>
#include <math.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <iostream>
#include <string>

#include <boost/thread.hpp>

#include <libsoup/soup.h>

#include "dirg_internal.hpp"
#include "UI.hpp"

using std::cout;
using std::cerr;
using std::endl;

struct WebUI : public UI {
	WebUI(const PadState& state, const std::string& dir);
	~WebUI();

	void activate();
	void deactivate();

	void set_colour(ButtonID button, float hue, float value);

private:
	inline void send_update(SoupMessage* msg);

	inline void message_callback(SoupServer* server, SoupMessage* msg,
	                             const char* path_str, GHashTable* query,
	                             SoupClientContext* client);

	static void _message_callback(SoupServer* s, SoupMessage* m, const char* p,
	                              GHashTable* q, SoupClientContext* c, void* ptr) {
		return reinterpret_cast<WebUI*>(ptr)->message_callback(s, m, p, q, c);
	}

	static const unsigned GRID_W = 8;
	static const unsigned GRID_H = 8;

	boost::thread     thread;
	SoupServer*       server;
	SoupMessage*      update_msg;
	const PadState&   state;
	const std::string dir;
};

// TODO: Make this a loadable module with this function as entry point
SPtr<UI>
dirg_new_web_ui(const PadState& state, const char* dir)
{
	return SPtr<UI>(new WebUI(state, dir));
}

static std::string
colour_to_string(const ButtonState& but)
{
	if (but.value == 0.0f)
		return "\"#000\"";

	const float g = 2.0f * (1.0 - but.hue);
	const float r = 2.0f * but.hue;

	const int r_level = std::min(static_cast<int>(lrintf(r * 3)), 3);
	const int g_level = std::min(static_cast<int>(lrintf(g * 3)), 3);
	assert(g_level >= 0);
	assert(g_level <= 3);
	assert(r_level >= 0);
	assert(r_level <= 3);

	static const char* const digits = "0123456789ABCDEF";

	std::string ret("\"#000\"");
	ret[2] = digits[r_level * 5];
	ret[3] = digits[g_level * 5];

	return ret;
}

void
WebUI::send_update(SoupMessage* msg)
{
	std::string response = "[";
	for (unsigned y = 0; y < GRID_H; ++y) {
		response.append("[");
		for (unsigned x = 0; x < GRID_W; ++x) {
			const ButtonID id(GRID, y, x);
			response.append(colour_to_string(state.get(id)));
			if (x < GRID_W - 1)
				response.append(",");
		}
		response.append("]");
		if (y < GRID_H - 1)
			response += ",";
	}
	response.append("]");

	soup_message_set_response(msg, "text/plain", SOUP_MEMORY_COPY,
	                          response.c_str(), response.length());

	soup_server_unpause_message(server, msg);
}

void
WebUI::message_callback(SoupServer* server, SoupMessage* msg, const char* path_str,
                        GHashTable* query, SoupClientContext* client)
{
	if (msg->method == SOUP_METHOD_GET) {
		// Return state immediately
		if (!strcmp(path_str, "/update")) {
			soup_message_set_status(msg, SOUP_STATUS_OK);
			send_update(msg);
			return;
		}

		// Respond to message when next update occurs (long poll)
		if (!strcmp(path_str, "/event")) {
			if (update_msg) {
				soup_message_set_status(update_msg, SOUP_STATUS_NO_CONTENT);
				soup_server_unpause_message(server, update_msg);
			}

			soup_message_set_status(msg, SOUP_STATUS_OK);
			soup_server_pause_message(server, msg);
			update_msg = msg;
			return;
		}

		// Serve file
		if (!strcmp(path_str, "/"))
			path_str = "index.html";
		const std::string file_path(dir + path_str);
		int fd = open(file_path.c_str(), O_RDONLY);
		if (!fd) {
			fprintf(stderr, "Failed to open file %s\n", file_path.c_str());
			soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
			return;
		}

		struct stat info;
		if (fstat(fd, &info)) {
			fprintf(stderr, "Failed to stat file %s\n", file_path.c_str());
			soup_message_set_status(msg, SOUP_STATUS_FORBIDDEN);
			return;
		}

		void* file = mmap(NULL, info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
		if (!file || file == MAP_FAILED) {
			fprintf(stderr, "Failed to mmap file %s\n", file_path.c_str());
			soup_message_set_status(msg, SOUP_STATUS_FORBIDDEN);
			return;
		}

		// Infer MIME type from file extension (ick)
		const std::string ext = file_path.substr(file_path.find_last_of("."));
		const char* mime_type = "text/plain";
		if (ext == ".html")
			mime_type = "text/html";
		else if (ext == ".css")
			mime_type = "text/css";
		else if (ext == ".js")
			mime_type = "application/javascript";
		else if (ext == ".ico")
			mime_type = "image/vnd.microsoft.icon";

		printf("Serving %s as %s\n", file_path.c_str(), mime_type);
		soup_message_set_response(msg, mime_type, SOUP_MEMORY_COPY,
		                          (const char*)file, info.st_size);

		soup_message_set_status(msg, SOUP_STATUS_OK);

		munmap(file, info.st_size);
		close(fd);
	} else if (msg->method == SOUP_METHOD_POST) {
		if (strcmp(path_str, "/update")) {
			soup_message_set_status(msg, SOUP_STATUS_FORBIDDEN);
			return;
		}

		int group, row, col;
		if (sscanf(msg->request_body->data,
		           "{\"selector\":\"click\",\"group\":%d,\"x\":%d,\"y\":%d}",
		           &group, &col, &row) == 3) {
			button_pressed.emit(ButtonID(ButtonGroup(group), row, col));
		}

		soup_message_set_response(msg, "application/json",
		                          SOUP_MEMORY_COPY, "\"ok\"", 4);

		soup_message_set_status(msg, SOUP_STATUS_OK);
	}
}

WebUI::WebUI(const PadState& s, const std::string& d)
	: server(NULL)
	, update_msg(NULL)
	, state(s)
	, dir((d[d.length() - 1] == '/' ? d : (d + "/")) + "www/")
{
	g_type_init();
}

WebUI::~WebUI()
{
	if (server)
		deactivate();
}

void
WebUI::activate()
{
	assert(!server);
	
	server = soup_server_new(SOUP_SERVER_PORT, 12345, NULL);
	soup_server_add_handler(server, NULL, _message_callback, this, NULL);

	cout << "Started HTTP server on port " << soup_server_get_port(server) << endl;

	thread = boost::thread(boost::bind(soup_server_run, server));
}

void
WebUI::deactivate()
{
	if (server) {
		soup_server_quit(server);
		thread.join();
		server = NULL;
	}
}
	
void
WebUI::set_colour(ButtonID button, float hue, float value)
{
	if (update_msg) {
		send_update(update_msg);
		update_msg = NULL;
	}
}
