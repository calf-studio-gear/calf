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
