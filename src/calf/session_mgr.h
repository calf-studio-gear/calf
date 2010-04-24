/* Calf DSP Library Utility Application - calfjackhost
 * Internal session manager API
 *
 * Copyright (C) 2010 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef CALF_SESSION_MGR_H
#define CALF_SESSION_MGR_H

#include <string>

namespace calf_plugins {

struct session_load_iface
{
    virtual bool get_next_item(std::string &key, std::string &value) = 0;
    virtual ~session_load_iface() {}
};
    
struct session_save_iface
{
    virtual void write_next_item(const std::string &key, const std::string &value) = 0;
    virtual ~session_save_iface() {}
};
    
struct session_client_iface
{
    virtual void load(session_load_iface *) = 0;
    virtual void save(session_save_iface *) = 0;
    virtual ~session_client_iface() {}
};
    
struct session_manager_iface
{
    virtual bool is_being_restored() = 0;
    virtual void set_jack_client_name(const std::string &name) = 0;
    virtual void connect(const std::string &name) = 0;
    virtual void poll() = 0;
    virtual void disconnect() = 0;
    virtual ~session_manager_iface() {}
};

#if USE_LASH
extern session_manager_iface *create_lash_session_mgr(session_client_iface *client, int &argc, char **&argv);
#endif

};

#endif
