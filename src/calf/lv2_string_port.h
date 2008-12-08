/* lv2_string_port.h - C header file for LV2 string port extension.
 * Draft version 3.
 *
 * Copyright (C) 2008 Krzysztof Foltman <wdev@foltman.com>
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

#ifndef LV2_STRING_PORT_H
#define LV2_STRING_PORT_H

#include <stdint.h>

/** \file
Input ports string value lifetime:
If the port does not have a context specified (it runs in the default,
realtime audio processing context), the values in the structure and
the actual string data MUST remain unchanged for the time a run() function
of a plugin is executed. However, if the port belongs to a different
context, the same data MUST remain unchanged only for the time a run() or
message_process() function of a given context is executed.

Output ports string value lifetime:
The plugin may only change the string or length in a run() function (if
the port belongs to default context) or in context-defined counterparts
(if the port belongs to another context). Because of that, using default
context output string ports is contraindicated for longer strings. 

UI issues:
When using port_event / write_port (and possible other communication
mechanisms), the format parameter should contain the numeric value
of URI LV2_STRING_PORT_URI (mapped with http://lv2plug.in/ns/extensions/ui
specified as map URI).

It's probably possible to use ports belonging to message context 
<http://lv2plug.in/ns/dev/contexts#MessageContext> for transfer. However,
contexts mechanism does not offer any way to notify the message recipient
about which ports have been changed. To remedy that, this extension defines
a flag LV2_STRING_DATA_CHANGED_FLAG that carries that information inside
a port value structure.

Storage:
The value of string port are assumed to be "persistent": if a host
saves and restores a state of a plugin (e.g. control port values), the
values of input string ports should also be assumed to belong to that
state. This also applies to message context: if a session is being restored,
the host MUST resend the last value that was sent to the port before session
has been saved. In other words, string port values "stick" to message ports.
*/

/** URI for the string port transfer mechanism feature */
#define LV2_STRING_PORT_URI "http://lv2plug.in/ns/dev/string-port#StringTransfer"

/** Flag: port data has been updated; for input ports, this flag is set by
the host. For output ports, this flag is set by the plugin. */
#define LV2_STRING_DATA_CHANGED_FLAG 1

/** structure for string port data */
struct _LV2_String_Data
{
    /** buffer for UTF-8 encoded zero-terminated string value; host-allocated */
    char *data;
    /** length in bytes (not characters), not including zero byte */ 
    size_t len;
    /** output ports: storage space in bytes; must be >= RDF-specified requirements */ 
    size_t storage;
    /** flags defined above */
    uint32_t flags;
    /** undefined yet, used for padding to 8 bytes */
    uint32_t pad;
};

typedef struct _LV2_String_Data LV2_String_Data;

#endif
