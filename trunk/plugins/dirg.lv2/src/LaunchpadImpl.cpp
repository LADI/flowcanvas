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

#include "LaunchpadImpl.hpp"

#include <poll.h>
#include <stdint.h>
#include <stdlib.h>

#include <iostream>

#include <boost/foreach.hpp>

#define VENDOR_ID      0x1235
#define PRODUCT_ID     0x000e
#define POLL_TIMEOUT 10

using std::cerr;
using std::cout;
using std::endl;
using sigc::signal;

static bool
hasError(libusb_transfer* transfer)
{
	switch (transfer->status) {
	case LIBUSB_TRANSFER_ERROR:
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_NO_DEVICE:
		return true;
	default:
		return false;
	}
}

void
LaunchpadImpl::callback(libusb_transfer* transfer)
{
	LaunchpadImpl* pad    = reinterpret_cast<LaunchpadImpl*>(transfer->user_data);
	bool           isRead = (pad->readTransfer_ == transfer);

	if (transfer->status == LIBUSB_TRANSFER_CANCELLED) {
		//ok?
	} else if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
		if (isRead) {
			pad->handleReadTransfer(transfer);
		} else {
			pad->handleWriteTransfer(transfer);
		}
		libusb_submit_transfer(transfer);
	} else if (hasError(transfer)) {
		cerr << "got error " << transfer->status << endl;
		pad->shallDisconnect_ = true;
		return;
	} else if (transfer->status == LIBUSB_TRANSFER_TIMED_OUT) {
		libusb_submit_transfer(transfer);
		// FIXME: retry?
	} else {
		cerr << "unkown error " << transfer->status << " occured in readcallback"
		     << endl;
	}
}

void
LaunchpadImpl::handleMidi(const MidiEvent& data)
{
	if (data.at(0) == LIVE) {
		const int pos = data.at(1) - 104;
		const ButtonID id(MARGIN_H, 0, pos);
		signal_(id, data.at(2) == 127);
	} else if (data.at(1) / 8 % 2 == 1) {
		const int pos = data.at(1) / 16;
		const ButtonID id(MARGIN_V, pos, 0);
		signal_(id, data.at(2) == 127);
	} else {
		const int row    = data.at(1) / 16;
		const int column = data.at(1) % 16;
		const ButtonID id(GRID, row, column);
		signal_(id, data.at(2) == 127);
	}
}

void
LaunchpadImpl::handleWriteTransfer(libusb_transfer* transfer)
{
	boost::mutex::scoped_lock lock(writeMutex_);
	if (!writeData_.empty()) {
		unsigned int pos = 0;
		while (pos < writeData_.size() && pos < 8) {
			uint8_t status       = writeData_.front();
			bool    statusChange = status != lastMidiStatus_;

			if (statusChange && (pos > 5)) {
				break;
			} else if (!statusChange && (pos > 6)) {
				break;
			}

			if (statusChange) {
				transfer->buffer[pos] = status;
				++pos;
				lastMidiStatus_ = status;
			}
			writeData_.pop();

			for (int data = 0; data < 2; ++data, ++pos) {
				transfer->buffer[pos] = writeData_.front();
				writeData_.pop();
			}
		}
		for (int i = pos; i < 8; i++) {
			transfer->buffer[i] = 145; //fill rest of message with junk
		}
		lastMidiStatus_ = 0;
	} else {
		for (int i = 0; i < 8; i++) {
			transfer->buffer[i] = 145; //fill rest of message with junk
		}
	}
}

void
LaunchpadImpl::handleReadTransfer(libusb_transfer* transfer)
{
	for (int i = 0; i < transfer->actual_length; i++) {
		if (transfer->buffer[i] == LIVE) {
			isMatrixMidiData_ = false;
		} else if (transfer->buffer[i] == MATRIX) {
			isMatrixMidiData_ = true;
		} else {
			if (isMatrixMidiData_) {
				readData_.push_back(MATRIX);
			} else {
				readData_.push_back(LIVE);
			}
			readData_.push_back(transfer->buffer[i]);
			++i;
			readData_.push_back(transfer->buffer[i]);
			handleMidi(readData_);
			readData_.clear();
		}
	}
}

LaunchpadImpl::LaunchpadImpl()
	: descriptors_(0)
	, nDescriptors_(0)
	, maxPacketSize_(8)
	, lastMidiPos_(-1)
	, lastMidiStatus_(0)
	, isMatrixMidiData_(false)
	, isConnected_(false)
	, shallDisconnect_(false)
	, quit_(false)
{
	reconnectWait_ = 1;
	thread_        = boost::thread(boost::bind(&LaunchpadImpl::run, this));
}

LaunchpadImpl::~LaunchpadImpl()
{
	shallDisconnect_ = true;
	quit_            = true;
	thread_.join();
}

void
LaunchpadImpl::connect()
{
	#define USB_OUT (2 | LIBUSB_ENDPOINT_OUT)
	#define USB_IN  (1 | LIBUSB_ENDPOINT_IN)
	if (!isConnected_ && (difftime(time(NULL), reconnectWait_) > 1)) {
		if (libusb_init(NULL) != 0) {
			cerr << "Could not load libusb." << endl;
		}
		libusb_set_debug(0, 3);
		handle_ = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
		if (handle_ == NULL) {
			libusb_exit(0);
			return;
		}
		if (libusb_claim_interface(handle_, 0) != 0) {
			libusb_close(handle_);
			libusb_exit(0);
			return;
		}
		libusb_reset_device(handle_);
		readTransfer_  = libusb_alloc_transfer(0);
		writeTransfer_ = libusb_alloc_transfer(0);

		libusb_fill_interrupt_transfer(
			readTransfer_, handle_, USB_IN,
			new uint8_t[maxPacketSize_], maxPacketSize_,
			callback, this, 200);

		uint8_t* writeData = new uint8_t[maxPacketSize_];
		libusb_fill_interrupt_transfer(
			writeTransfer_, handle_, USB_OUT,
			writeData, maxPacketSize_,
			callback, this, 200);

		pollarray_ = libusb_get_pollfds(0);
		for (nDescriptors_ = 0; pollarray_[nDescriptors_] != 0; nDescriptors_++) {}
		descriptors_ = new pollfd[nDescriptors_];

		libusb_submit_transfer(readTransfer_);
		libusb_submit_transfer(writeTransfer_);

		cout << "successfully connected to the launchpad" << endl;
		isConnected_ = true;
		fillCache();
	}
}


void
LaunchpadImpl::fillCache()
{
	boost::mutex::scoped_lock lock(writeMutex_);
	for (int i = 0; i < 128; i++) {
		Key key(128, i);
		cache_[key] = 0;

		key         = Key(144, i);
		cache_[key] = 0;

		key         = Key(146, i);
		cache_[key] = 0;

		key         = Key(176, i);
		cache_[key] = 0;
	}
}

bool
shouldCancel(libusb_transfer* transfer)
{
	switch (transfer->status) {
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_COMPLETED:
	case LIBUSB_TRANSFER_NO_DEVICE:
		return false;
	default:
		return true;
	}
}

void
LaunchpadImpl::disconnect()
{
	if (isConnected_) {
		timeval usbTimeout;
		usbTimeout.tv_sec  = 0;
		usbTimeout.tv_usec = 20000;

		if (shouldCancel(readTransfer_)) {
			libusb_cancel_transfer(readTransfer_);
		}
		if (shouldCancel(writeTransfer_)) {
			libusb_cancel_transfer(writeTransfer_);
		}

		delete[] descriptors_;
		descriptors_ = 0;
		free(pollarray_);
		pollarray_ = 0;
		libusb_free_transfer(readTransfer_);
		libusb_free_transfer(writeTransfer_);
		libusb_release_interface(handle_, 0);
		libusb_close(handle_);
		libusb_exit(0);

		reconnectWait_   = time(NULL);
		isConnected_     = false;
		shallDisconnect_ = false;

		cout << "disconnected from launchpad" << endl;
	}
}

void
LaunchpadImpl::run()
{
	timeval usbTimeout;
	usbTimeout.tv_sec  = 0;
	usbTimeout.tv_usec = 200000;

	while (!quit_) {
		if (!isConnected_) {
			connect();
			if (!isConnected_) {
				usleep(10000);
			}
		} else {
			poll(descriptors_, nDescriptors_, POLL_TIMEOUT);
			libusb_handle_events(0);
		}
		if (shallDisconnect_ && isConnected_) {
			disconnect();
		}
	}
}

void
LaunchpadImpl::writeMidi(uint8_t b1, uint8_t b2, uint8_t b3)
{
	boost::mutex::scoped_lock lock(writeMutex_);

	Key key(b1, b2);
	if (cache_[key] != b3) {
		writeData_.push(b1);
		writeData_.push(b2);
		writeData_.push(b3);
		cache_[key] = b3;
	}
}

void
LaunchpadImpl::setButton(ButtonID id, uint8_t velocity)
{
	const int row = id.row;
	const int col = id.col;
	switch (id.group) {
	case GRID:
		if (row >= 0 && row <= 7 && col >= 0 && col <= 7) {
			writeMidi(144, row * 16 + col, velocity);
		}
		break;
	case MARGIN_H:
		if (col >= 0 && col <= 7) {
			writeMidi(176, 104 + col, velocity);
		}
		break;
	case MARGIN_V:
		if (row >= 0 && row <= 7) {
			writeMidi(144, row * 16 + 8, velocity);
		}
		break;
	}
}
