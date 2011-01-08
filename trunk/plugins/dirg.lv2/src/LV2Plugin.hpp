/* LV2Plugin - C++ wrapper class for LV2 plugins
 * Copyright (C) 2006-2007 Lars Luthman <lars.luthman@gmail.com>
 * Copyright (C) 2009-2010 David Robillard <d@drobilla.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA
 */

#ifndef LV2PLUGIN_HPP
#define LV2PLUGIN_HPP

#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include "lv2.h"
#include "lv2_types.hpp"

namespace LV2 {

typedef std::vector<LV2_Descriptor> Descriptors;

/** @internal
 * Returns a list of all registered plugins (do not use directly).
 */
Descriptors& get_lv2_descriptors();

/** A template base class for LV2 plugins.
 * This class has default implementations for all functions, so you only
 * have to implement the functions that you need (for example run()). All
 * subclasses must have a constructor whose signature matches the one in the
 * example code below, otherwise it will not work with the template class
 * LV2::Register. The host will use these parameter to pass the sample rate,
 * the path to the bundle directory and the list of features passed from
 * the host when it creates a new instance of the plugin.
 *
 * This is a template so that simulated dynamic binding can be used for the
 * callbacks.  This allows extensions to be implemented separately, yet
 * combined into a single plugin with no runtime overhead (i.e. no
 * virtual function calls).
 * <code>
#include <LV2Plugin.hpp>

class MyLV2 : public LV2::Plugin<MyLV2> {
public:
    MyLV2(double r, const char* b, const LV2::Feature* const* f)
        : LV2::Plugin<MyLV2>(2)
    {}
    void run(uint32_t sample_count) {
        memcpy(p(1), p(0), sample_count * sizeof(float));
    }
};

static unsigned _ = MyLV2::register_class<MyLV2>("http://example.org/my-lv2");
</code>
 *
 * If the above code is compiled and linked with @c -llv2_plugin into a shared
 * module, that shared module will be a valid LV2 plugin binary containing a
 * single pass-through audio plugin with URI <http://example.org/my-lv2>.
 */
template<class Derived,
         class X1 = End, class X2 = End, class X3 = End, class X4 = End,
         class X5 = End, class X6 = End, class X7 = End, class X8 = End>
class Plugin
	: public MixinTree<Derived, X1, X2, X3, X4, X5, X6, X7, X8>
{
public:
	/** Initialises the port vector with the correct number of ports,
	 * and checks if all the required features are provided.
	 */
	explicit Plugin(uint32_t ports)
		: m_ports(ports, 0)
	{
		m_features    = s_features;
		m_bundle_path = s_bundle_path;
		s_features    = 0;
		s_bundle_path = 0;
		if (m_features) {
			FeatureHandlers hmap;
			Derived::map_feature_handlers(hmap);
			for (const Feature* const* iter = m_features; *iter != 0; ++iter) {
				FeatureHandlers::iterator miter;
				miter = hmap.find((*iter)->URI);
				if (miter != hmap.end()) {
					miter->second(static_cast<Derived*>(this), (*iter)->data);
				}
			}
		}
	}

	/** Connect a port to a buffer.
	 * Usually there is no need to override this, simply use
	 * <code>this->p<Type>(index)</code> to access the buffer connected to the
	 * port with the given index, casted to <code>Type</code>.
	 *
	 * In realtime safe plugins this function may not block, allocate memory,
	 * or otherwise take a long time to return.
	 */
	void connect_port(uint32_t port, void* data_location) {
		m_ports[port] = data_location;
	}

	/** Activate the plugin instance.
	 * Override this function if you need to do anything on activation.
	 * This function should reset all plugin state to defaults.
	 * This is always called before the host starts using the run() function.
	 */
	void activate() {}

	/** This is the process callback which should fill all output port buffers.
	 * You most likely want to override it.
	 *
	 * In realtime safe plugins this function may not block, allocate memory,
	 * or otherwise take a long time to return.
	 */
	void run(uint32_t sample_count) {}

	/** Deactivate the plugin instance.
	 * Override this function if you need to do anything on deactivation.
	 * This function should release all resources held by the plugin instance.
	 * The host calls this when it does not plan to make any more calls to
	 * run().  After a call to deactivate(), the host will not call run() until
	 * activate() has been called.
	 */
	void deactivate() {}

	/** Register a plugin class in the library.
	 * This must be called when the shared library is loaded by the host.
	 * The best way to do this is to initialise a global variable with the
	 * return value of this function, e.g.:
	 * <code>unsigned _ = MyLV2::register_class("http://example.org/my-lv2");</code>
	 */
	static unsigned register_class(const char* uri) {
		LV2_Descriptor desc;
		std::memset(&desc, 0, sizeof(LV2_Descriptor));
		desc.URI            = uri;
		desc.instantiate    = &Derived::_instantiate;
		desc.connect_port   = &Derived::_connect_port;
		desc.activate       = &Derived::_activate;
		desc.run            = &Derived::_run;
		desc.deactivate     = &Derived::_deactivate;
		desc.cleanup        = &Derived::_cleanup;
		desc.extension_data = &Derived::extension_data;
		get_lv2_descriptors().push_back(desc);
		return get_lv2_descriptors().size() - 1;
	}

protected:
	/** Use this function to access and cast port buffers, e.g.:
	 * <code>LV2_MIDI* midibuffer = p<LV2_MIDI>(midiport_index);</code>
	 */
	template<typename T>
	T*& p(uint32_t port) {
		return reinterpret_cast<T*&>(m_ports[port]);
	}

	template<typename T>
	const T*& p(uint32_t port) const {
		return reinterpret_cast<const T*&>(m_ports[port]);
	}

	/** Return the filesystem path to the bundle that contains this plugin. */
	const char* bundle_path() const {
		return m_bundle_path;
	}

private:
	/* Static wrapper functions used as fields of LV2_Descriptor */
	static void _connect_port(LV2_Handle instance, uint32_t port, void* buf) {
		reinterpret_cast<Derived*>(instance)->connect_port(port, buf);
	}
	static void _activate(LV2_Handle instance) {
		reinterpret_cast<Derived*>(instance)->activate();
	}
	static void _run(LV2_Handle instance, uint32_t sample_count) {
		reinterpret_cast<Derived*>(instance)->run(sample_count);
	}
	static void _deactivate(LV2_Handle instance) {
		reinterpret_cast<Derived*>(instance)->deactivate();
	}
	static LV2_Handle _instantiate(const LV2_Descriptor*     descriptor,
	                               double                    sample_rate,
	                               const char*               bundle_path,
	                               const LV2_Feature* const* features)
	{
		// copy some data to static variables so the subclasses don't have to
		// bother with it
		s_features    = features;
		s_bundle_path = bundle_path;

		Derived* t = new Derived(sample_rate, bundle_path, features);
		if (t->check_ok()) {
			return reinterpret_cast<LV2_Handle>(t);
		}
		delete t;
		return 0;
	}
	static void _cleanup(LV2_Handle instance) {
		delete reinterpret_cast<Derived*>(instance);
	}

	LV2::Feature const* const* m_features;
	char const*                m_bundle_path;

	static LV2::Feature const* const* s_features;
	static char const*                s_bundle_path;

	/** @internal
	 * Vector of pointers to all port buffers. You don't need to
	 * access it directly, use the p() function instead.
	 */
	std::vector<void*> m_ports;
};


template<class Derived,
         class X1, class X2, class X3, class X4,
         class X5, class X6, class X7, class X8>
LV2::Feature const* const*
Plugin<Derived, X1, X2, X3, X4, X5, X6, X7, X8>::s_features = 0;

template<class Derived,
         class X1, class X2, class X3, class X4,
         class X5, class X6, class X7, class X8>
char const*
Plugin<Derived, X1, X2, X3, X4, X5, X6, X7, X8>::s_bundle_path = 0;

}

#endif // LV2PLUGIN_HPP
