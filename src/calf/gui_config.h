#ifndef CALF_GUI_CONFIG_H
#define CALF_GUI_CONFIG_H

#include <glib.h>
#include <string>
#include <vector>

namespace calf_utils {

struct config_exception: public std::exception
{
    std::string content;
    const char *content_ptr;
    config_exception(const std::string &text) : content(text) { content_ptr = content.c_str(); }
    virtual const char *what() const throw() { return content_ptr; }
    virtual ~config_exception() throw() { }
};

struct config_listener_iface
{
    virtual void on_config_change() = 0;
    virtual ~config_listener_iface() {}
};

struct config_notifier_iface
{
    virtual ~config_notifier_iface() {}
};

struct config_db_iface
{
    virtual bool has_dir(const char *key) = 0;
    virtual bool get_bool(const char *key, bool def_value) = 0;
    virtual int get_int(const char *key, int def_value) = 0;
    virtual std::string get_string(const char *key, const std::string &def_value) = 0;
    virtual void set_bool(const char *key, bool value) = 0;
    virtual void set_int(const char *key, int value) = 0;
    virtual void set_string(const char *key, const std::string &value) = 0;
    virtual void save() = 0;
    virtual config_notifier_iface *add_listener(config_listener_iface *listener) = 0;
    virtual ~config_db_iface() {}
};

class gkeyfile_config_db: public config_db_iface
{
protected:
    class notifier: public config_notifier_iface
    {
    protected:
        gkeyfile_config_db *parent;
        config_listener_iface *listener;
        notifier(gkeyfile_config_db *_parent, config_listener_iface *_listener);
        virtual ~notifier();
        friend class gkeyfile_config_db;
    };
protected:
    GKeyFile *keyfile;
    std::string filename;
    std::string section;
    std::vector<notifier *> notifiers;

    void handle_error(GError *error);
    void remove_notifier(notifier *n);
    friend class notifier;
public:
    gkeyfile_config_db(GKeyFile *kf, const char *filename, const char *section);
    virtual bool has_dir(const char *key);
    virtual bool get_bool(const char *key, bool def_value);
    virtual int get_int(const char *key, int def_value);
    virtual std::string get_string(const char *key, const std::string &def_value);
    virtual void set_bool(const char *key, bool value);
    virtual void set_int(const char *key, int value);
    virtual void set_string(const char *key, const std::string &value);
    virtual void save();
    virtual config_notifier_iface *add_listener(config_listener_iface *listener);
    virtual ~gkeyfile_config_db();
};

struct gui_config
{
    int rack_float, float_size;
    bool rack_ears;
    
    gui_config();
    ~gui_config();
    void load(config_db_iface *db);
    void save(config_db_iface *db);
};

};

#endif
