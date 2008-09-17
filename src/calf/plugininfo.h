/* Calf DSP Library
 * Plugin introspection interface
 *
 * Copyright (C) 2008 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __CALF_PLUGININFO_H
#define __CALF_PLUGININFO_H

#include <string>

namespace synth {

/// A sink to send information about an audio port
struct audio_port_info_iface
{
    /// Called if it's an input port
    virtual audio_port_info_iface& input() { return *this; }
    /// Called if it's an output port
    virtual audio_port_info_iface& output() { return *this; }
    virtual ~audio_port_info_iface() {}
};

/// A sink to send information about a control port (very incomplete, missing stuff: units, integer, boolean, toggled, notAutomatic, notGUI...)
struct control_port_info_iface
{
    /// Called if it's an input port
    virtual control_port_info_iface& input() { return *this; }
    /// Called if it's an output port
    virtual control_port_info_iface& output() { return *this; }
    /// Called to mark the port as using linear range [from, to]
    virtual control_port_info_iface& lin_range(double from, double to) { return *this; }
    /// Called to mark the port as using log range [from, to]
    virtual control_port_info_iface& log_range(double from, double to) { return *this; }
    virtual ~control_port_info_iface() {}
};

/// A sink to send information about a plugin
struct plugin_info_iface
{
    /// Set plugin names (ID, name and label)
    virtual void names(const std::string &id, const std::string &name, const std::string &label, const std::string &category) {}
    /// Add an audio port (returns a sink which accepts further description)
    virtual audio_port_info_iface &audio_port(const std::string &id, const std::string &name)=0;
    /// Add a control port (returns a sink which accepts further description)
    virtual control_port_info_iface &control_port(const std::string &id, const std::string &name, double def_value)=0;
    /// Called after plugin has reported all the information
    virtual void finalize() {}
    virtual ~plugin_info_iface() {}
};

/// A sink to send information about plugins
struct plugin_list_info_iface
{
    /// Add an empty plugin object and return the sink to be filled with information
    virtual plugin_info_iface &plugin() = 0;
    virtual ~plugin_list_info_iface() {}
};

};

#endif