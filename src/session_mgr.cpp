/* Calf DSP Library Utility Application - calfjackhost
 * Session manager API implementation for LASH 0.5.4 and 0.6.0
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
#include "config.h"
#include <calf/session_mgr.h>

#if USE_LASH

#include <calf/utils.h>
#include <gtk/gtk.h>
#include <lash/lash.h>
#include <glib.h>
#include <string.h>

using namespace std;
using namespace calf_plugins;


class lash_session_manager_base: public session_manager_iface
{
protected:
    session_client_iface *client;
    int lash_source_id;
    lash_client_t *lash_client;
    bool restoring_session;
    static gboolean update_lash(void *self) { ((session_manager_iface *)self)->poll(); return TRUE; }
public:
    lash_session_manager_base(session_client_iface *_client);

    void connect(const std::string &name)
    {
        if (lash_client)
        {
            lash_source_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 250, update_lash, (session_manager_iface *)this, NULL); // 4 LASH reads per second... should be enough?
        }
    }
    void disconnect();
};

lash_session_manager_base::lash_session_manager_base(session_client_iface *_client)
{
    client = _client;
    lash_source_id = 0;
    lash_client = NULL;
    restoring_session = false;
}

void lash_session_manager_base::disconnect()
{
    if (lash_source_id)
    {
        g_source_remove(lash_source_id);
        lash_source_id = 0;
    }
}

# if !USE_LASH_0_6

class old_lash_session_manager: public lash_session_manager_base
{
    lash_args_t *lash_args;

public:
    old_lash_session_manager(session_client_iface *_client, int &argc, char **&argv);
    void send_lash(LASH_Event_Type type, const std::string &data);
    virtual void connect(const std::string &name);
    virtual void disconnect();
    virtual bool is_being_restored() { return restoring_session; }
    virtual void set_jack_client_name(const std::string &name);
    virtual void poll();

};

old_lash_session_manager::old_lash_session_manager(session_client_iface *_client, int &argc, char **&argv)
: lash_session_manager_base(_client)
{
    lash_args = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (!strncmp(argv[i], "--lash-project=", 14)) {
            restoring_session = true;
            break;
        }
    }
    lash_args = lash_extract_args(&argc, &argv);
    lash_client = lash_init(lash_args, PACKAGE_NAME, LASH_Config_Data_Set, LASH_PROTOCOL(2, 0));
    if (!lash_client) {
        g_warning("Failed to create a LASH connection");
    }
}

void old_lash_session_manager::send_lash(LASH_Event_Type type, const std::string &data)
{
    lash_send_event(lash_client, lash_event_new_with_all(type, data.c_str()));
}

void old_lash_session_manager::connect(const std::string &name)
{
    if (lash_client)
    {
        send_lash(LASH_Client_Name, name.c_str());
    }
    lash_session_manager_base::connect(name);
}

void old_lash_session_manager::disconnect()
{
    lash_session_manager_base::disconnect();
    if (lash_args)
        lash_args_destroy(lash_args);
}

void old_lash_session_manager::set_jack_client_name(const std::string &name)
{
    if (lash_client)
        lash_jack_client_name(lash_client, name.c_str());
}

void old_lash_session_manager::poll()
{
    do {
        lash_event_t *event = lash_get_event(lash_client);
        if (!event)
            break;
        
        // printf("type = %d\n", lash_event_get_type(event));
        
        switch(lash_event_get_type(event)) {        
            case LASH_Save_Data_Set:
            {
                struct writer: public session_save_iface
                {
                    lash_client_t *client;
                    
                    void write_next_item(const std::string &key, const std::string &value)
                    {
                        lash_config_t *cfg = lash_config_new_with_key(key.c_str());
                        lash_config_set_value(cfg, value.c_str(), value.length());
                        lash_send_config(client, cfg);
                    }
                };
                
                writer w;
                w.client = lash_client;
                client->save(&w);
                send_lash(LASH_Save_Data_Set, "");
                break;
            }
            
            case LASH_Restore_Data_Set:
            {
                struct reader: public session_load_iface
                {
                    lash_client_t *client;
                    
                    virtual bool get_next_item(std::string &key, std::string &value) {
                        lash_config_t *cfg = lash_get_config(client);
                        
                        if(cfg) {
                            key = lash_config_get_key(cfg);
                            // printf("key = %s\n", lash_config_get_key(cfg));
                            value = string((const char *)lash_config_get_value(cfg), lash_config_get_value_size(cfg));
                            lash_config_destroy(cfg);
                            return true;
                        }
                        return false;
                    }        
                };
                
                reader r;
                r.client = lash_client;
                client->load(&r);
                send_lash(LASH_Restore_Data_Set, "");
                break;
            }
                
            case LASH_Quit:
                gtk_main_quit();
                break;
            
            default:
                g_warning("Unhandled LASH event %d (%s)", lash_event_get_type(event), lash_event_get_string(event));
                break;
        }
    } while(1);
}

session_manager_iface *calf_plugins::create_lash_session_mgr(session_client_iface *client, int &argc, char **&argv)
{
    return new old_lash_session_manager(client, argc, argv);
}

# else

class new_lash_session_manager: public lash_session_manager_base
{
    lash_args_t *lash_args;

    static bool save_data_set_cb(lash_config_handle_t *handle, void *user_data);
    static bool load_data_set_cb(lash_config_handle_t *handle, void *user_data);
    static bool quit_cb(void *user_data);

public:
    new_lash_session_manager(session_client_iface *_client)
    
    virtual void set_jack_client_name(const std::string &) {}
};

new_lash_session_manager::new_lash_session_manager(session_client_iface *_client)
: lash_session_manager_base(_client)
{
    lash_client = lash_client_open(PACKAGE_NAME, LASH_Config_Data_Set, argc, argv);
    if (!lash_client) {
        g_warning("Failed to create a LASH connection");
        return;
    }
    restoring_session = lash_client_is_being_restored(lash_client);
    lash_set_save_data_set_callback(lash_client, save_data_set_cb, this);
    lash_set_load_data_set_callback(lash_client, load_data_set_cb, this);
    lash_set_quit_callback(lash_client, quit_cb, NULL);    
}

void new_lash_session_manager::poll()
{
    lash_dispatch(lash_client);
}

bool new_lash_session_manager::save_data_set_cb(lash_config_handle_t *handle, void *user_data)
{
    struct writer: public session_save_iface
    {
        lash_config_handle_t handle;
        
        virtual bool write_next_item(const std::string &key, const std::string &value) {
            lash_config_write_raw(handle, key.c_str(), (const void *)value.data(), value.length(), LASH_TYPE_RAW);
            return true;
        }        
    };
    
    writer w;
    w.handle = handle;
    client->save(&w);
    return true;
}

bool new_lash_session_manager::load_data_set_cb(lash_config_handle_t *handle, void *user_data)
{
    struct reader: public session_load_iface
    {
        lash_config_handle_t handle;
        
        virtual bool get_next_item(std::string &key, std::string &value) {
            const char *key_cstr;
            int type;
            void *data;
            size = lash_config_read(handle, &key_cstr, &data, &type);
            if (size == -1 || type != LASH_TYPE_RAW)
                return false;
            key = key_cstr;
            value = string((const char *)data, size);
            return true;
        }        
    };
    
    reader r;
    r.handle = handle;
    client->load(&r);
    return true;
}

bool new_lash_session_manager::quit_cb(void *user_data)
{
    gtk_main_quit();
    return true;
}

session_manager_iface *calf_plugins::create_lash_session_mgr(session_client_iface *_client, int &, char **&)
{
    return new new_lash_session_manager(client);
}

# endif

#endif
