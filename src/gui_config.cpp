#include <calf/gui_config.h>
#include <assert.h>
#include <stdio.h>

using namespace std;
using namespace calf_utils;

gui_config::gui_config()
{
    rack_float = 0;
    float_size = 1;
    rack_ears = true;
}

gui_config::~gui_config()
{
}

void gui_config::load(config_db_iface *db)
{
    rack_float = db->get_int("rack-float", gui_config().rack_float);
    float_size = db->get_int("float-size", gui_config().float_size);
    rack_ears = db->get_bool("show-rack-ears", gui_config().rack_ears);
}

void gui_config::save(config_db_iface *db)
{
    db->set_int("rack-float", rack_float);
    db->set_int("float-size", float_size);
    db->set_bool("show-rack-ears", rack_ears);
    db->save();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

gkeyfile_config_db::notifier::notifier(gkeyfile_config_db *_parent, config_listener_iface *_listener)
{
    parent = _parent;
    listener = _listener;
}

gkeyfile_config_db::notifier::~notifier()
{
    parent->remove_notifier(this);
 }


////////////////////////////////////////////////////////////////////////////////////////////////////

gkeyfile_config_db::gkeyfile_config_db(GKeyFile *kf, const char *_filename, const char *_section)
{
    keyfile = kf;
    filename = _filename;
    section = _section;
}

void gkeyfile_config_db::handle_error(GError *error)
{
    if (error)
    {
        string msg = error->message;
        g_error_free(error);
        throw config_exception(msg.c_str());
    }
}

void gkeyfile_config_db::remove_notifier(notifier *n)
{
    for (size_t i = 0; i < notifiers.size(); i++)
    {
        if (notifiers[i] == n)
        {
            notifiers.erase(notifiers.begin() + i);
            return;
        }
    }
    assert(0);
}

bool gkeyfile_config_db::has_dir(const char *key)
{
    return g_key_file_has_group(keyfile, key);
}

static bool check_not_found_and_delete(GError *error)
{
    if (error && error->domain == G_KEY_FILE_ERROR)
    {
        switch(error->code)
        {
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
            case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
                g_error_free(error);
                return true;
            default:
                return false;
        }
    }
    return false;
}

bool gkeyfile_config_db::get_bool(const char *key, bool def_value)
{
    GError *err = NULL;
    bool value = (bool)g_key_file_get_boolean(keyfile, section.c_str(), key, &err);
    if (err)
    {
        if (check_not_found_and_delete(err))
            return def_value;
        handle_error(err);
    }
    return value;
}

int gkeyfile_config_db::get_int(const char *key, int def_value)
{
    GError *err = NULL;
    int value = g_key_file_get_integer(keyfile, section.c_str(), key, &err);
    if (err)
    {
        if (check_not_found_and_delete(err))
            return def_value;
        handle_error(err);
    }
    return value;
}

std::string gkeyfile_config_db::get_string(const char *key, const std::string &def_value)
{
    GError *err = NULL;
    gchar *value = g_key_file_get_string(keyfile, section.c_str(), key, &err);
    if (err)
    {
        if (check_not_found_and_delete(err))
            return def_value;
        handle_error(err);
    }
    return value;
}

void gkeyfile_config_db::set_bool(const char *key, bool value)
{
    g_key_file_set_boolean(keyfile, section.c_str(), key, (gboolean)value);
}

void gkeyfile_config_db::set_int(const char *key, int value)
{
    g_key_file_set_integer(keyfile, section.c_str(), key, value);
}

void gkeyfile_config_db::set_string(const char *key, const std::string &value)
{
    g_key_file_set_string(keyfile, section.c_str(), key, value.c_str());
}

void gkeyfile_config_db::save()
{
    GError *err = NULL;
    gsize length = 0;
    gchar *data = g_key_file_to_data(keyfile, &length, &err);
    if (err)
        handle_error(err);

    if (!g_file_set_contents(filename.c_str(), data, length, &err))
    {
        g_free(data);
        handle_error(err);
    }
    g_free(data);
    
    for (size_t i = 0; i < notifiers.size(); i++)
        notifiers[i]->listener->on_config_change();
}

config_notifier_iface *gkeyfile_config_db::add_listener(config_listener_iface *listener)
{
    notifier *n = new notifier(this, listener);
    notifiers.push_back(n);
    return n;
}

gkeyfile_config_db::~gkeyfile_config_db()
{
}

