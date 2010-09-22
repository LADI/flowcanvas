/* lv2_persist.h - C header file for the LV2 Persist extension.
 * Copyright (C) 2010 Leonard Ritter <paniq@paniq.org>
 *
 * This header is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This header is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this header; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307 USA
 */

/** @file
 * C header for the LV2 Persist extension <http://lv2plug.in/ns/ext/persist>.
 */

#ifndef LV2_PERSIST_H
#define LV2_PERSIST_H

#ifdef __cplusplus
extern "C" {
#endif

#define LV2_PERSIST_URI "http://lv2plug.in/ns/ext/persist"

/** Type of a persistent piece of data. */
enum DataType {
	LV2_PERSIST_BLOB,    ///< http://lv2plug.in/ns/ext/persist#Blob
	LV2_PERSIST_STRING,  ///< http://lv2plug.in/ns/ext/persist#String
	LV2_PERSIST_FILENAME ///< http://lv2plug.in/ns/ext/persist#FileName
};

/** Causes the host to store a value under a given key.
 *
 * This callback is passed by the host to LV2_Persist.save().
 * @param callback_data Must be the callback_data argument passed to save().
 * @param key A private string or URI under which the data is to be stored.
 * @param type The type of @a value.
 * @param value Pointer to the value to be stored.
 * @param size The size of the data at @a value in bytes.  If @a size is 0
 *
 * If @a type is LV2_PERSIST BLOB, @a value points to an opaque region of
 * memory @a size bytes long.
 *
 * If @a type is LV2_PERSIST_STRING or LV2_PERSIST_FILENAME, @a value points
 * to a null-terminated string and @a size is ignored.
 *
 * If @a type is LV2_PERSIST_FILENAME, the host may store the file in any
 * way (e.g. linking, deep copying) and plugin MUST NOT expect the same
 * filename to be restored (though the same file contents are guaranteed
 * to be restored.  This allows hosts to avoid copying very large files
 * by linking local projects while retaining the ability to export/archive
 * projects by deep copying.
 *
 * A 'size' of 0 is valid. The host may choose to store nothing.
 */
typedef void (*LV2_Persist_Store_Function)(
	void*       callback_data,
	const char* key,
	DataType    type,
	const void* value,
	size_t      size);

/** Causes the host to retrieve a value under a given key.
 *
 * This callback is passed by the host to LV2_Persist.restore().
 * @param callback_data' Must be the callback_data argument passed to restore().
 * @param key A private string or URI under which a value has been stored.
 * @param type (Output) Set to the type of the restored value.
 * @param size (Output) If non-NULL, set to the size of the restored value.
 * @return A pointer to the restored value, or NULL if no value has been
 *         stored under that key.
 *
 * The returned value MUST remain valid until restore() returns.  The plugin
 * MUST NOT attempt to access a returned pointer outside of the restore()
 * context (it must make a copy in order to do so).
 */
typedef const void* (*LV2_Persist_Retrieve_Function)(
	void*       callback_data,
	const char* key,
	DataType*   type,
	size_t*     size);

/** When the plugin's extension_data is called with argument LV2_PERSIST_URI,
 * the plugin is expected to return an LV2_Persist structure, which remains
 * valid indefinitely.
 *
 * The host can use the exposed function pointers to save and restore
 * the state of a plugin to a map of string keys to binary blobs at any
 * time.
 *
 * The usual application would be to save the plugins state when the
 * project document is to be saved, and to restore the state when
 * a project document has been loaded. Other applications are possible.
 *
 * Blob maps are meant to be only compatible between instances of the
 * same plugin. However, should a future extension require persistent
 * data to follow an URI key naming scheme, this restriction no longer
 * applies.
 */
typedef struct _LV2_Persist {
	/** Causes the plugin to save state data which it wants to preserve
	 * across plugin lifetime using a store callback provided by
	 * the host.
	 *
	 * @param instance The instance handle of the plugin.
	 * @param store The host-provided store callback.
	 * @param callback_data	An opaque pointer to host data, e.g. the map or
	 *        file where the values are to be stored.  If @a store is called,
	 *        this MUST be passed as its callback_data parameter.
	 *
	 * The map on which save() operates must always be empty before
	 * the first call to store(). The plugin is expected to store all
	 * blobs of interest.
	 *
	 * The @a callback_data pointer and @a store function MUST NOT be used
	 * beyond the scope of save().
	 */
	void (*save)(LV2_Handle                 instance,
	             LV2_Persist_Store_Function store,
	             void*                      callback_data);

	/** Causes the plugin to restore state data using a retrieve callback
	 * provided by the host.
	 *
	 * @param instance The instance handle of the plugin.
	 * @param retrieve The host-provided restore callback.
	 * @param callback_data	An opaque pointer to host data, e.g. the map or
	 *        file from which the values are to be restored.  If @a retrieve is
	 *        called, this MUST be passed as its callback_data parameter.
	 *
	 * The map on which restore() operates must contain values stored
	 * by an instance of a plugin with the same URI as this instance, or be
	 * empty.
	 *
	 * The plugin MUST gracefully fall back to a default value
	 * when a blob can not be retrieved. This allows the host to reset
	 * the plugin state with an empty map.
	 *
	 * The @a callback_data pointer and @a store function MUST NOT be used
	 * beyond the scope of restore().
	 */
	void (*restore)(LV2_Handle                    instance,
	                LV2_Persist_Retrieve_Function retrieve,
	                void*                         callback_data);

} LV2_Persist;

typedef void* LV2_Persist_FileSupport_Data;

/** Feature structure passed by host to instantiate with feature URI
 * <http://lv2plug.in/ns/ext/persist#FileSupport>.
 */
typedef struct {

	LV2_Persist_FileSupport_Data data;
	
	/** Return the full path that should be used for a file owned by this
	 * plugin called @a name.
	 *
	 * @param data MUST be the @a data member of this struct.
	 * @param name The name of the file.
	 * @return A newly allocated path which the plugin may use to create a new
	 *         file.  The plugin is responsible for freeing the returned string.
	 */
	char* new_file_path(LV2_Persist_FileSupport_Data data,
	                    const char*                  name);
	
} LV2_Persist_FileSupport;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV2_PERSIST_H */
