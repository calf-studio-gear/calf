/* lv2_event.h - C header file for the LV2 events extension.
 * 
 * Copyright (C) 2006-2007 Lars Luthman <lars.luthman@gmail.com>
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

#ifndef LV2_EVENT_H
#define LV2_EVENT_H
 
#define LV2_EVENT_URI "http://lv2plug.in/ns/ext/event"
#define LV2_EVENT_AUDIO_STAMP 0

#include <stdint.h>

/** @file
 * This header defines the code portion of the LV2 events extension with URI
 * <http://lv2plug.in/ns/ext/event> ('lv2ev').
 *
 * This extension is a generic transport mechanism for time stamped events
 * of any type (e.g. MIDI, OSC, ramps, etc).  Each port can transport mixed
 * events of any type; the type of events and timestamps are defined by a URI
 * which is mapped to an integer by the host for performance reasons.
 *
 * This extension requires the host to support the LV2 URI Map extension.
 * Any host which supports this extension MUST guarantee that any call to
 * the LV2 URI Map uri_to_id function with the URI of this extension as the
 * 'map' argument returns a value within the range of uint16_t.
 */


/** The best Pulses Per Quarter Note for tempo-based uint32_t timestmaps.
 * Equal to 2^12 * 5 * 7 * 9 * 11 * 13 * 17, which is evenly divisble
 * by all integers from 1 through 18 inclusive, and powers of 2 up to 2^12.
 */
static const uint32_t LV2_EVENT_PPQN = 3136573440;



/** An LV2 event.
 *
 * LV2 events are generic time-stamped containers for any type of event.
 * The type field defines the format of a given event's contents.
 *
 * This struct defines the header of an LV2 event.  An LV2 event is a single
 * chunk of POD (plain old data), usually contained in a flat buffer
 * (see LV2_EventBuffer below).  Unless a required feature says otherwise,
 * hosts may assume a deep copy of an LV2 event can be created safely
 * using a simple:
 *
 * memcpy(ev_copy, ev, sizeof(LV2_Event) + ev->size);  (or equivalent)
 */
typedef struct {

	/** The frames portion of timestamp.  The units used here can optionally be
	 * set for a port (with the lv2ev:timeUnits property), otherwise this
	 * is audio frames, corresponding to the sample_count parameter of the
	 * LV2 run method (e.g. frame 0 is the first frame for that call to run).
	 */
	uint32_t frames;

	/** The sub-frames portion of timestamp.  The units used here can
	 * optionally be set for a port (with the lv2ev:timeUnits property),
	 * otherwise this is 1/(2^32) of an audio frame.
	 */
	uint32_t subframes;
	
	/** The type of this event, as a number which represents some URI
	 * defining an event type.  This value MUST be some value previously
	 * returned from a call to the uri_to_id function defined in the LV2
	 * URI map extension.
	 * The type 0 is a special reserved value, meaning the event contains a
	 * single pointer (of native machine size) to a dynamically allocated
	 * LV2_Object.  This allows events to carry large non-POD payloads
	 * (e.g. images, waveforms, large SYSEX dumps, etc.) without copying.
	 * See the LV2 Object extension for details.
	 * Plugins which do not support the LV2 Object extension MUST NOT store,
	 * copy, or pass through to an output any event with type 0.
	 * If the type is not 0, plugins may assume the event is POD and should
	 * gracefully ignore or pass through (with a simple copy) any events
	 * of a type the plugin does not recognize.
	 */
	uint16_t type;

	/** The size of the data portion of this event in bytes, which immediately
	 * follows.  The header size (12 bytes) is not included in this value.
	 */
	uint16_t size;

	/* size bytes of data follow here */

} LV2_Event;



/** A buffer of LV2 events.
 *
 * Like events (which this contains) an event buffer is a single chunk of POD:
 * the entire buffer (including contents) can be copied with a single memcpy.
 * The first contained event begins sizeof(LV2_EventBuffer) bytes after
 * the start of this struct.
 *
 * After this header, the buffer contains an event header (defined by struct
 * LV2_Event), followed by that event's contents (padded to 64 bits), followed by
 * another header, etc:
 *
 * |       |       |       |       |       |       |
 * | | | | | | | | | | | | | | | | | | | | | | | | |
 * |FRAMES |SUBFRMS|TYP|LEN|DATA..DATA..PAD|FRAMES | ...
 */
typedef struct {

	/** The number of events in this buffer.
	 * INPUTS: The host must set this field to the number of events
	 *     contained in the data buffer before calling run().
	 *     The plugin must not change this field.
	 * OUTPUTS: The plugin must set this field to the number of events it
	 *     has written to the buffer before returning from run().
	 *     Any initial value should be ignored by the plugin.
	 */
	uint32_t event_count;

	/** The size of the data buffer in bytes.
	 * This is set by the host and must not be changed by the plugin.
	 * The host is allowed to change this between run() calls.
	 */
	uint32_t capacity;

	/** The size of the initial portion of the data buffer containing data.
	 * INPUTS: The host must set this field to the number of bytes used
	 *     by all events it has written to the buffer (including headers)
	 *     before calling the plugin's run().
	 *     The plugin must not change this field.
	 * OUTPUTS: The plugin must set this field to the number of bytes
	 *     used by all events it has written to the buffer (including headers)
	 *     before returning from run().
	 *     Any initial value should be ignored by the plugin.
	 */
	uint32_t size;
	
	/** The type of the time stamps for events in this buffer.
	 * As a special exception, '0' always means audio frames and subframes
	 * (1/UINT32_MAX'th of a frame) in the sample rate passed to instantiate.
	 * INPUTS: The host must set this field to the numeric ID of some URI
	 *     which defines the meaning of the frames and subframes fields of
	 *     contained events (obtained by the LV2 URI Map uri_to_id function
	 *     with the URI of this extension as the 'map' argument).
	 *     The host must never pass a plugin a buffer which uses a stamp type
	 *     the plugin does not 'understand'.  The value of this field must
	 *     never change, except when connect_port is called on the input
	 *     port, at which time the host MUST have set the stamp_type field to
	 *     the value that will be used for all run calls (until a reconnect).
	 * OUTPUTS: The plugin may set this to any value that has been returned
	 *     from uri_to_id with the URI of this extension for a 'map' argument.
	 *     When connected to a buffer with connect_port, output ports MUST set
	 *     this field to the type of time stamp they will be writing.  On any
	 *     call to connect_port on an event input port, the plugin may change
	 *     this field on any output port, it is the responsibility of the host
	 *     to check if any of these values have changed.
	 */
	uint16_t stamp_type;

	/** For possible future use.  Hosts and plugins that do not know of any
	 * use for this value MUST always set it to 0. */
	uint16_t pad;

} LV2_Event_Buffer;


#endif // LV2_EVENT_H

