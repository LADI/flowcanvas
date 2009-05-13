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

#ifndef LADSPANODE_H
#define LADSPANODE_H

#include <string>
#include <ladspa.h>
#include <boost/optional.hpp>
#include "types.hpp"
#include "NodeBase.hpp"
#include "PluginImpl.hpp"

namespace Ingen {


/** An instance of a LADSPA plugin.
 *
 * \ingroup engine
 */
class LADSPANode : public NodeBase
{
public:
	LADSPANode(PluginImpl*              plugin,
	           const std::string&       name,
	           bool                     polyphonic,
	           PatchImpl*               parent,
	           const LADSPA_Descriptor* descriptor,
	           SampleRate               srate,
	           size_t                   buffer_size);

	~LADSPANode();

	bool instantiate();

	bool prepare_poly(uint32_t poly);
	bool apply_poly(Raul::Maid& maid, uint32_t poly);

	void activate();
	void deactivate();

	void process(ProcessContext& context);

	void set_port_buffer(uint32_t voice, uint32_t port_num, Buffer* buf);

protected:
	void get_port_limits(unsigned long            port_index,
	                     boost::optional<Sample>& default_value,
	                     boost::optional<Sample>& lower_bound,
	                     boost::optional<Sample>& upper_bound);

	const LADSPA_Descriptor* _descriptor;
	Raul::Array<LADSPA_Handle>* _instances;
	Raul::Array<LADSPA_Handle>* _prepared_instances;
};


} // namespace Ingen

#endif // LADSPANODE_H
