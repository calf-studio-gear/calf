/* lv2_data_access.h - C header file for the LV2 Data Access extension.
 * Copyright (C) 2008 Dave Robillard <dave@drobilla.net>
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

#ifndef LV2_DATA_ACCESS_H
#define LV2_DATA_ACCESS_H

#define LV2_DATA_ACCESS_URI "http://lv2plug.in/ns/ext/data-access"


/** @file
 * This header defines the LV2 Extension Data extension with the URI
 * <http://lv2plug.in/ns/ext/data-access>.
 *
 * This extension defines a method for (e.g.) plugin UIs to have (possibly
 * marshalled) access to the extension_data function on a plugin instance.
 */
	

/** The data field of the LV2_Feature for this extension.
 *
 * To support this feature the host must pass an LV2_Feature struct to the
 * instantiate method with URI "http://lv2plug.in/ns/ext/data-access"
 * and data pointed to an instance of this struct.
 */
typedef struct {
	
	/** A pointer to a method the UI can call to get data (of a type specified
	 * by some other extension) from the plugin.
	 *
	 * This call never is never guaranteed to return anything, UIs should
	 * degrade gracefully if direct access to the plugin data is not possible
	 * (in which case this function will return NULL).
	 *
	 * This is for access to large data that can only possibly work if the UI
	 * and plugin are running in the same process.  For all other things, use
	 * the normal LV2 UI communication system.
	 */
	const void* (*data_access)(const char* uri);

} LV2_Extension_Data_Feature;


#endif // LV2_DATA_ACCESS_H

