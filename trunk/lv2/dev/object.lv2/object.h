/* lv2_object.h - C header file for the LV2 Object extension.
 *
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
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

#ifndef LV2_OBJECT_H
#define LV2_OBJECT_H

#define LV2_OBJECT_URI       "http://lv2plug.in/ns/dev/object"
#define LV2_BLOB_SUPPORT_URI "http://lv2plug.in/ns/dev/object#blobSupport"

#define LV2_OBJECT_REFERENCE_TYPE 0

#include <stdint.h>

/** @file
 * This header defines the code portion of the LV2 Object extension with URI
 * <http://lv2plug.in/ns/dev/object>.
 */

/** An LV2 Object.
 *
 * An "Object" is a generic chunk of memory with a given type and size.
 * The type field defines how to interpret an object.
 *
 * All objects are by definition Plain Old Data (POD) and may be safely
 * copied (e.g. with memcpy) using the size field, except objects with type 0.
 * An object with type 0 is a reference, and may only be used via the functions
 * provided in LV2_Blob_Support (e.g. it MUST NOT be manually copied).
 */
typedef struct _LV2_Object {

	/** The type of this object.  This number represents a URI, mapped to an
	 * integer using the extension <http://lv2plug.in/ns/ext/uri-map>
	 * with "http://lv2plug.in/ns/dev/object" as the 'map' argument.
	 * Type 0 is a special case which indicates this object
	 * is a reference and MUST NOT be copied manually.
	 */
	uint32_t type;

	/** The size of this object, not including this header, in bytes. */
	uint32_t size;

	/* size bytes of data follow here */
	uint8_t body[];

} LV2_Object;

/** Reference, an LV2_Object with type 0 */
typedef LV2_Object LV2_Reference;


/* Everything below here is related to blobs, which are dynamically allocated
 * objects that are not necessarily POD.  This functionality is optional,
 * hosts may support objects without implementing blob support.
 * Blob support is an LV2 Feature.
 */


typedef void* LV2_Blob_Data;

/** Dynamically Allocated LV2 Blob.
 *
 * This is a blob of data of any type, dynamically allocated in memory.
 * Unlike an LV2_Object, a blob is not necessarily POD.  Plugins may only
 * refer to blobs via a Reference (an LV2_Object with type 0), there is no
 * way for a plugin to directly create, copy, or destroy a Blob.
 */
typedef struct _LV2_Blob {

	/** Pointer to opaque data.
	 *
	 * Plugins MUST NOT interpret this data in any way.  Hosts may store
	 * whatever information they need to associate with references here.
	 */
	LV2_Blob_Data data;

	/** Get blob's type as a URI mapped to an integer.
	 *
	 * The return value may be any type URI, mapped to an integer with the
	 * URI Map extension.  If this type is an LV2_Object type, get returns
	 * a pointer to the LV2_Object header (e.g. a blob with type obj:Int32
	 * does NOT return a pointer to a int32_t).
	 */
	uint32_t (*type)(struct _LV2_Blob* blob);

	/** Get blob's body.
	 *
	 * Returns a pointer to the start of the blob data.  The format of this
	 * data is defined by the return value of the type method.  It MUST NOT
	 * be used in any way by code which does not understand that type.
	 */
	void* (*get)(struct _LV2_Blob* blob);

} LV2_Blob;


typedef void* LV2_Blob_Support_Data;

typedef void (*LV2_Blob_Destroy)(LV2_Blob* blob);

/** The data field of the LV2_Feature for the LV2 Object extension.
 *
 * A host which supports this extension must pass an LV2_Feature struct to the
 * plugin's instantiate method with 'URI' "http://lv2plug.in/ns/dev/object" and
 * 'data' pointing to an instance of this struct.  All fields of this struct,
 * MUST be set to non-NULL values by the host (except possibly data).
 */
typedef struct {

	/** Pointer to opaque data.
	 *
	 * The plugin MUST pass this to any call to functions in this struct.
	 * Otherwise, it must not be interpreted in any way.
	 */
	LV2_Blob_Support_Data data;

	/** The size of a reference, in bytes.
	 *
	 * This value is provided by the host so plugins can allocate large
	 * enough chunks of memory to store references.
	 */
	size_t reference_size;

	/** Initialize a reference to point to a newly allocated Blob.
	 *
	 * @param data Must be the data member of this struct.
	 * @param reference Pointer to an area of memory at least as large as
	 *     the reference_size field of this struct.  On return, this will
	 *     be the unique reference to the new blob which is owned by the
	 *     caller.  Caller MUST NOT pass a valid reference.
	 * @param destroy Function to destroy a blob of this type.  This function
	 *     MUST clean up any resources contained in the blob, but MUST NOT
	 *     attempt to free the memory pointed to by its LV2_Blob* parameter.
	 * @param type Type of blob to allocate.
	 * @param size Size of blob to allocate in bytes.
	 */
	void (*lv2_blob_new)(LV2_Blob_Support_Data data,
	                     LV2_Reference*        reference,
	                     LV2_Blob_Destroy      destroy_func,
	                     uint32_t              type,
	                     uint32_t              size);

	/** Return a pointer to the Blob referred to by @a ref.
	 *
	 * The returned value MUST NOT be used in any way other than by calling
	 * methods defined in LV2_Blob (e.g. it MUST NOT be copied or destroyed).
	 */
	LV2_Blob* (*lv2_reference_get)(LV2_Blob_Support_Data data,
	                               LV2_Reference*        ref);

	/** Copy a reference.
	 * This copies a reference but not the blob it refers to,
	 * i.e. after this call @a dst and @a src refer to the same LV2_Blob.
	 */
	void (*lv2_reference_copy)(LV2_Blob_Support_Data data,
	                           LV2_Reference*        dst,
	                           LV2_Reference*        src);

	/** Reset (release) a reference.
	 * After this call, @a ref is invalid.  Use of this function is only
	 * necessary if a plugin makes a copy of a reference it does not later
	 * send to an output (which transfers ownership to the host).
	 */
	void (*lv2_reference_reset)(LV2_Blob_Support_Data data,
	                            LV2_Reference*        ref);

} LV2_Blob_Support;


#endif // LV2_OBJECT_H

