/* LV2 OSC Messages Extension
 * Copyright (C) 2007 Dave Robillard <dave@drobilla.net>
 * 
 * This header is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This header is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LV2_CONTEXTS_H
#define LV2_CONTEXTS_H

#define LV2_CONTEXTS_URI "http://lv2plug.in/ns/dev/context"

#define LV2_CONTEXT_MESSAGE "http://lv2plug.in/ns/dev/context#MessageContext"

#define LV2_CONTEXTS_SET_OUTPUT_WRITTEN(flags, index) \
	((uint8_t*)flags)[(index) / 8] |= 1 << ((index) % 8)

#define LV2_CONTEXTS_UNSET_OUTPUT_WRITTEN(flags, index) \
	((uint8_t*)flags)[(index) / 8] &= ~(1 << ((index) % 8))

#include "lv2.h"
#include <stdbool.h>


typedef struct {
	bool (*message_run)(LV2_Handle instance, uint32_t* outputs_written);
	void (*message_connect_port)(LV2_Handle instance, uint32_t port, void* data);
} LV2MessageContext;


#endif // LV2_CONTEXTS_H
