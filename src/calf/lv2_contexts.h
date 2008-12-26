/* LV2 OSC Messages Extension
 * Copyright (C) 2007-2008 Dave Robillard <dave@drobilla.net>
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

#include <stdint.h>

#define LV2_CONTEXTS_URI "http://lv2plug.in/ns/dev/contexts"

#define LV2_CONTEXT_MESSAGE "http://lv2plug.in/ns/dev/contexts#MessageContext"

static inline void
lv2_contexts_set_port_valid(uint32_t* flags, uint32_t index) {
	((uint8_t*)flags)[(index) / 8] |= 1 << ((index) % 8);
}

static inline void
lv2_contexts_unset_port_valid(uint32_t* flags, uint32_t index) {
	((uint8_t*)flags)[(index) / 8] &= ~(1 << ((index) % 8));
}

static inline int
lv2_contexts_port_is_valid(uint32_t flags, uint32_t index) {
	return ((flags & (1 << index)) != 0);
}

#include "lv2.h"
#include <stdbool.h>


typedef struct {
	/** The message run function.  This is called once to process a set of
	 * inputs and produce a set of outputs.  Before calling the host MUST
	 * set valid_inputs such that the bit corresponding to each input port
	 * is 1 iff data is present.  The plugin MUST only inspect bits
	 * corresponding to ports in the message thread.  Before returning the
	 * plugin MUST set valid_outputs such that the bit corresponding to
	 * each output port of the message context is 1 iff data has been written
	 * to that port in the duration of this function invocation.
	 * The plugin must return 1 if outputs have been written, 0 otherwise.
	 */
	uint32_t (*message_run)(LV2_Handle instance,
	                        uint32_t*  valid_inputs,
	                        uint32_t*  valid_outputs);

	/** The message thread function alalogous to the LV2 connect_port
	 * function.  This function must only be used to connect ports that
	 * belong to the message context. */ 
	void (*message_connect_port)(LV2_Handle instance,
	                             uint32_t   port,
	                             void*      data);

} LV2MessageContext;


#endif // LV2_CONTEXTS_H

