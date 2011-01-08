/* Launchpad controller class.
 * Copyright (C) 2010 David Robillard <http://drobilla.net>
 *
 * Based on liblaunchpad,
 * Copyright (C) 2010 Christian Loehnert <krampenschiesser@freenet.de>
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

#ifndef LAUNCHPADIMPL_HPP_
#define LAUNCHPADIMPL_HPP_

#include <poll.h>

#include <utility>
#include <vector>
#include <queue>

#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>
#include <boost/utility.hpp>

#include <libusb.h>

#include <sigc++/sigc++.h>

#include "dirg_internal.hpp"

typedef std::vector<uint8_t> MidiEvent;

/** Actual launchpad implementation.
 * There can only be one! DA dada DA DA (Highlander theme)
 */
class LaunchpadImpl : public boost::noncopyable
{
public:
	LaunchpadImpl();
	~LaunchpadImpl();

	bool isConnected() const { return isConnected_; }

	sigc::signal<void, ButtonID, bool>& getSignal() { return signal_; }

	void setButton(ButtonID id, uint8_t velocity);

private:
	void connect();
	void disconnect();

	void run();

	void writeMidi(uint8_t b1, uint8_t b2, uint8_t b3);

	void handleReadTransfer(libusb_transfer* transfer);
	void handleWriteTransfer(libusb_transfer* transfer);
	void handleMidi(const MidiEvent& data);

	void fillCache();

	sigc::signal<void, ButtonID, bool> signal_;

	typedef std::pair<uint8_t, uint8_t> Key;

	boost::unordered_map<Key, uint8_t> cache_;

	libusb_device_handle* handle_;
	libusb_transfer*      readTransfer_;
	libusb_transfer*      writeTransfer_;
	pollfd*               descriptors_;
	const libusb_pollfd** pollarray_;
	unsigned              nDescriptors_;
	int                   maxPacketSize_;

	time_t reconnectWait_;

	boost::thread thread_;

	std::vector<uint8_t> readData_;
	std::queue<uint8_t>  writeData_;
	boost::mutex         writeMutex_;
	int                  lastMidiPos_;
	uint8_t              lastMidiStatus_;

	bool isMatrixMidiData_;

	volatile bool isConnected_;
	volatile bool shallDisconnect_;
	volatile bool quit_;

private:
	static void callback(libusb_transfer* transfer);

	static const int LIVE   = 176;
	static const int MATRIX = 144;
};

#endif /* LAUNCHPADIMPL_HPP_ */
