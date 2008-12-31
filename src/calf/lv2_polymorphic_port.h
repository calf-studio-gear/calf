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

#ifndef LV2_POLYMORPHIC_PORT_H
#define LV2_POLYMORPHIC_PORT_H

#define LV2_POLYMORPHIC_PORT_URI "http://lv2plug.in/ns/dev/polymorphic-port"


/** @file
 * This header defines the LV2 Polymorphic Port extension with the URI
 * <http://lv2plug.in/ns/dev/polymorphic-port>.
 *
 * This extension defines a buffer format for ports that can take on
 * various types dynamically at runtime.
 */
	

/** The data field of the LV2_Feature for this extension.
 *
 * To support this feature the host must pass an LV2_Feature struct to the
 * instantiate method with URI "http://lv2plug.in/ns/dev/polymorphic-port"
 * and data pointed to an instance of this struct.
 */
typedef struct {
	
	/** Set the type of a polymorphic port.
	 * If the plugin specifies constraints on port types, the host MUST NOT
	 * call the run method until all port types have been set to a valid
	 * configuration.  Whenever the type for a port is changed, the host
	 * MUST call connect_port before the next call to the run method.
	 * The return value of this function SHOULD be ignored by hosts at this
	 * time (future revisions of this extension may specify return values).
	 * Plugins which do not know of any future revision or extension that
	 * dictates otherwise MUST return 0 from this function.
	 * @param port Index of the port to connect (same as LV2 connect_port)
	 * @param type Mapped URI for the type of data being connected
	 * @param type_data Type specific data defined by type URI (may be NULL)
	 * @return Unused at this time
	 */
	uint32_t (*set_type)(LV2_Handle instance,
	                     uint32_t   port,
	                     uint32_t   type,
	                     void*      type_data);
	
} LV2_Polymorphic_Feature;


#endif // LV2_POLYMORPHIC_PORT_H

