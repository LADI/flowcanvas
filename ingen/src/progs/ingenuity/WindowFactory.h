/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#ifndef WINDOW_FACTORY_H
#define WINDOW_FACTORY_H

#include <map>
#include <gtkmm.h>
#include "util/CountedPtr.h"
#include "PatchView.h"
#include "PatchModel.h"
using Ingen::Client::PatchModel;

namespace Ingenuity {

class PatchWindow;
class NodeControlWindow;
class NodePropertiesWindow;
class PatchPropertiesWindow;
class LoadPatchWindow;
class RenameWindow;


/** Manager/Factory for all windows.
 *
 * This serves as a nice centralized spot for all window management issues,
 * as well as an enumeration of all the windows in Ingenuity (the goal being
 * to reduce that number as much as possible).
 */
class WindowFactory {
public:
	WindowFactory();
	~WindowFactory();

	PatchWindow*       patch_window(CountedPtr<PatchModel> patch);
	NodeControlWindow* control_window(CountedPtr<NodeModel> node);

	void present_patch(CountedPtr<PatchModel> patch,
	                   PatchWindow*           preferred = NULL,
	                   CountedPtr<PatchView>  patch     = CountedPtr<PatchView>());

	void present_controls(CountedPtr<NodeModel> node);

	void present_load_plugin(CountedPtr<PatchModel> patch, MetadataMap data = MetadataMap());
	void present_load_patch(CountedPtr<PatchModel> patch, MetadataMap data = MetadataMap());
	void present_new_subpatch(CountedPtr<PatchModel> patch, MetadataMap data = MetadataMap());
	void present_load_subpatch(CountedPtr<PatchModel> patch, MetadataMap data = MetadataMap());
	void present_rename(CountedPtr<ObjectModel> object);
	void present_properties(CountedPtr<NodeModel> node);
	
	bool remove_patch_window(PatchWindow* win, GdkEventAny* ignored = NULL);

private:
	typedef std::map<Path, PatchWindow*>       PatchWindowMap;
	typedef std::map<Path, NodeControlWindow*> ControlWindowMap;

	PatchWindow* new_patch_window(CountedPtr<PatchModel> patch, CountedPtr<PatchView> view);


	NodeControlWindow* new_control_window(CountedPtr<NodeModel> node);
	bool               remove_control_window(NodeControlWindow* win, GdkEventAny* ignored);

	PatchWindowMap   _patch_windows;
	ControlWindowMap _control_windows;

	LoadPluginWindow*      _load_plugin_win;
	LoadPatchWindow*       _load_patch_win;
	NewSubpatchWindow*     _new_subpatch_win;
	LoadSubpatchWindow*    _load_subpatch_win;
	NodePropertiesWindow*  _node_properties_win;
	PatchPropertiesWindow* _patch_properties_win;
	RenameWindow*          _rename_win;
};

}

#endif // WINDOW_FACTORY_H