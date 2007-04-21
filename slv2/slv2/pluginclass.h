/* SLV2
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 *  
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __SLV2_PLUGIN_CLASS_H__
#define __SLV2_PLUGIN_CLASS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <slv2/types.h>

/** \addtogroup data
 * @{
 */


/** Get the URI of this plugin class.
 *
 * Returned value is owned by \a plugin_class and must not be freed by caller.
 *
 * Time = O(1)
 */
const char* slv2_plugin_class_get_uri(SLV2PluginClass plugin_class);


/** Get the label of this plugin_class, ie "Oscillators".
 *
 * Returned value is owned by \a plugin_class and must not be freed by caller.
 *
 * Time = O(1)
 */
const char* slv2_plugin_class_get_label(SLV2PluginClass plugin_class);

#if 0
/** Get the path of this plugin_class, ie "Plugins/Generators/Oscillators".
 *
 * SLV2 automatically generates an ideal plugin_class heirarchy given the plugins
 * present.  The last fragment of the path is the plugin_class's label,
 * as returned by slv2_plugin_class_get_label.
 *
 * Returned value is owned by \a plugin_class and must not be freed by caller.
 *
 * Time = O(1)
 */
const char* slv2_plugin_class_get_path(SLV2PluginClass plugin_class);
#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SLV2_PLUGIN_CLASS_H__ */