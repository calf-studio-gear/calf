/* Calf DSP Library
 * Common plugin interface definitions (shared between LADSPA/LV2/DSSI/standalone).
 *
 * Copyright (C) 2007-2010 Krzysztof Foltman
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02111-1307, USA.
 */
#ifndef CALF_GIFACE_H
#define CALF_GIFACE_H

#include <config.h>
#include "primitives.h"
#include <complex>
#include <exception>
#include <string>
#include <vector>

namespace osctl {
    struct osc_client;
}

namespace calf_plugins {

enum {
    MAX_SAMPLE_RUN = 256
};

struct automation_range;

/// Values ORed together for flags field in parameter_properties
enum parameter_flags
{
  PF_TYPEMASK = 0x000F, ///< bit mask for type
  PF_FLOAT = 0x0000, ///< any float value
  PF_INT = 0x0001,   ///< integer value (still represented as float)
  PF_BOOL = 0x0002,  ///< bool value (usually >=0.5f is treated as TRUE, which is inconsistent with LV2 etc. which treats anything >0 as TRUE)
  PF_ENUM = 0x0003,  ///< enum value (min, min+1, ..., max, only guaranteed to work when min = 0)
  PF_ENUM_MULTI = 0x0004, ///< SET / multiple-choice

  PF_SCALEMASK = 0xF0, ///< bit mask for scale
  PF_SCALE_DEFAULT = 0x00, ///< no scale given
  PF_SCALE_LINEAR = 0x10, ///< linear scale
  PF_SCALE_LOG = 0x20, ///< log scale
  PF_SCALE_GAIN = 0x30, ///< gain = -96dB..0 or -inf dB
  PF_SCALE_PERC = 0x40, ///< percent
  PF_SCALE_QUAD = 0x50, ///< quadratic scale (decent for some gain/amplitude values)
  PF_SCALE_LOG_INF = 0x60, ///< log scale + +inf (FAKE_INFINITY)

  PF_CTLMASK =        0x000F00, ///< bit mask for control type
  PF_CTL_DEFAULT =    0x000000, ///< try to figure out automatically
  PF_CTL_KNOB =       0x000100, ///< knob
  PF_CTL_FADER =      0x000200, ///< fader (slider)
  PF_CTL_TOGGLE =     0x000300, ///< toggle button
  PF_CTL_COMBO =      0x000400, ///< combo box
  PF_CTL_RADIO =      0x000500, ///< radio button
  PF_CTL_BUTTON =     0x000600, ///< push button
  PF_CTL_METER  =     0x000700, ///< volume meter
  PF_CTL_LED    =     0x000800, ///< light emitting diode
  PF_CTL_LABEL  =     0x000900, ///< label

  PF_CTLOPTIONS     = 0x00F000, ///< bit mask for control (widget) options
  PF_CTLO_HORIZ     = 0x001000, ///< horizontal version of the control (unused)
  PF_CTLO_VERT      = 0x002000, ///< vertical version of the control (unused)
  PF_CTLO_LABEL     = 0x004000, ///< add a text display to the control (meters only)
  PF_CTLO_REVERSE   = 0x008000, ///< use VU_MONOCHROME_REVERSE mode (meters only)

  PF_PROP_MASK     =  0x3F0000, ///< bit mask for properties
  PF_PROP_NOBOUNDS =  0x010000, ///< no epp:hasStrictBounds
  PF_PROP_EXPENSIVE = 0x020000, ///< epp:expensive, may trigger expensive calculation
  PF_PROP_OUTPUT_GAIN=0x040000, ///< epp:outputGain + skip epp:hasStrictBounds
  PF_PROP_OPTIONAL  = 0x080000, ///< connection optional
  PF_PROP_GRAPH     = 0x100000, ///< add graph
  PF_PROP_OUTPUT    = 0x200000, ///< output port (flag, cannot be combined with others)

  PF_UNITMASK     = 0x0F000000,  ///< bit mask for units   \todo reduce to use only 5 bits
  PF_UNIT_DB      = 0x01000000,  ///< decibels
  PF_UNIT_DBFS      = 0x0E000000,  ///< decibels relative to Full Scale
  PF_UNIT_COEF    = 0x02000000,  ///< multiply-by factor
  PF_UNIT_HZ      = 0x03000000,  ///< Hertz
  PF_UNIT_SEC     = 0x04000000,  ///< second
  PF_UNIT_MSEC    = 0x05000000,  ///< millisecond
  PF_UNIT_CENTS   = 0x06000000,  ///< cents (1/100 of a semitone, 1/1200 of an octave)
  PF_UNIT_SEMITONES = 0x07000000,///< semitones
  PF_UNIT_BPM     = 0x08000000,  ///< beats per minute
  PF_UNIT_DEG     = 0x09000000,  ///< degrees
  PF_UNIT_NOTE    = 0x0A000000,  ///< MIDI note number
  PF_UNIT_RPM     = 0x0B000000,  ///< revolutions per minute
  PF_UNIT_SAMPLES = 0x0C000000,  ///< samples

  PF_SYNC_BPM     = 0x10000000, ///< sync a bpm setting with the host environment

  PF_DIGITMASK    = 0xE0000000,
  PF_DIGIT_0      = 0x20000000,
  PF_DIGIT_1      = 0x40000000,
  PF_DIGIT_2      = 0x60000000,
  PF_DIGIT_3      = 0x80000000,
  PF_DIGIT_ALL    = 0xA0000000,
};

/// A fake infinity value (because real infinity may break some hosts)
#define FAKE_INFINITY (65536.0 * 65536.0)
/// Check for infinity (with appropriate-ish tolerance)
#define IS_FAKE_INFINITY(value) (fabs(value-FAKE_INFINITY) < 1.0)

/// Information record about plugin's menu command
struct plugin_command_info
{
    const char *label;           ///< short command name / label
    const char *name;            ///< human-readable command name
    const char *description;     ///< description (for status line etc.)
};

/// Range, default value, flags and names for a parameter
struct parameter_properties
{
    /// default value
    float def_value;
    /// minimum value
    float min;
    /// maximum value
    float max;
    /// number of steps (for an integer value from 0 to 100 this will be 101; for 0/90/180/270/360 this will be 5), or 0 for continuous
    float step;
    /// logical OR of parameter_flags
    uint32_t flags;
    /// for PF_ENUM: array of text values (from min to max step 1), otherwise NULL
    const char **choices;
    /// parameter label (for use in LV2 label field etc.)
    const char *short_name;
    /// parameter human-readable name
    const char *name;
    /// convert from [0, 1] range to [min, max] (applying scaling)
    float from_01(double value01) const;
    /// convert from [min, max] to [0, 1] range (applying reverse scaling)
    double to_01(float value) const;
    /// stringify (in sensible way)
    std::string to_string(float value) const;
    /// convert string (from text entry) to value
    float string_to_value(const char* string) const;
    /// get required width (for reserving GUI space)
    int get_char_count() const;
    /// get increment step based on step value (if specified) and other factors
    float get_increment() const;
};

struct cairo_iface
{
    int size_x, size_y, pad_x, pad_y;
    virtual void set_source_rgba(float r, float g, float b, float a = 1.f) = 0;
    virtual void set_line_width(float width) = 0;
    virtual void set_dash(const double *dash, int length) = 0;
    virtual void draw_label(const char *label, float x, float y, int pos, float margin, float align) = 0;
    virtual ~cairo_iface() {}
};

struct progress_report_iface
{
    virtual void report_progress(float percentage, const std::string &message) = 0;
    virtual ~progress_report_iface() {}
};

/// possible bit masks for get_layers
enum layers_flags {
    LG_NONE            = 0x000000,
    LG_CACHE_GRID      = 0x000001,
    LG_REALTIME_GRID   = 0x000002,
    LG_CACHE_GRAPH     = 0x000004,
    LG_REALTIME_GRAPH  = 0x000008,
    LG_CACHE_DOT       = 0x000010,
    LG_REALTIME_DOT    = 0x000020,
    LG_CACHE_MOVING    = 0x000040,
    LG_REALTIME_MOVING = 0x000080
};

/// possible values for get_moving
enum moving_directions {
    LG_MOVING_LEFT     = 0x000000,
    LG_MOVING_RIGHT    = 0x000001,
    LG_MOVING_UP       = 0x000002,
    LG_MOVING_DOWN     = 0x000004
};

/// 'provides live line graph values' interface
struct line_graph_iface
{
    /// Obtain subindex'th graph of parameter 'index'
    /// @param index parameter/graph number (usually tied to particular plugin control port)
    /// @param subindex graph number (there may be multiple overlaid graphs for one parameter, eg. for monosynth 2x12dB filters)
    /// @param phase 0 if in cache phase or 1 if in realtime phase
    /// @param data buffer for normalized output values
    /// @param points number of points to fill
    /// @param context cairo context to adjust (for multicolour graphs etc.)
    /// @retval true graph data was returned; subindex+1 graph may or may not be available
    /// @retval false graph data was not returned; subindex+1 graph does not exist either
    virtual bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode = 0) const { return false; }

    /// Obtain subindex'th moving surface of parameter 'index'
    /// @param index parameter/dot number (usually tied to particular plugin control port)
    /// @param subindex mvoing line number (there may be multiple graphs for one parameter)
    /// @param direction 0 if horizontal or 1 if vertical movement
    /// @param data buffer for normalized output values
    /// @param x number of points direction to fill in x
    /// @param y number of points direction to fill in y
    virtual bool get_moving(int index, int subindex, int &direction, float *data, int x, int y, int &offset, uint32_t &color) const { return false; }

    /// Obtain subindex'th dot of parameter 'index'
    /// @param index parameter/dot number (usually tied to particular plugin control port)
    /// @param subindex dot number (there may be multiple dots graphs for one parameter)
    /// @param phase 0 if in cache phase or 1 if in realtime phase
    virtual bool get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const { return false; }

    /// Obtain subindex'th dot of parameter 'index'
    /// @param index parameter/dot number (usually tied to particular plugin control port)
    /// @param subindex dot number (there may be multiple dots graphs for one parameter)
    /// @param phase 0 if in cache phase or 1 if in realtime phase
    virtual bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const { return false; }

    /// Return which layers need to be redrawn in the next GTK drawing cycle
    /// @param index Parameter/graph identifier (usually tied to particular plugin control port)
    /// @param generation The overall amount of drawing cycles since the last full refresh of all surfaces
    /// @param layers Bitmask defining the layers to be redrawn (see layers_flags above)
    /// @retval true there's at least one layer to be redrawn; false nothing to draw in this cycle
    virtual bool get_layers(int index, int generation, unsigned int &layers) const { return false; }

    /// Return a label for the crosshairs they are enabled
    /// @param x Position of the mouse pointer in x direction
    /// @param y Position of the mouse pointer in y direction
    /// @param sx Horizontal size of the widget in pixels
    /// @param sy Vertical size of the widget in pixels
    virtual std::string get_crosshair_label( int x, int y, int sx, int sy, float q, int dB, int name, int note, int cents) const { std::string s = ""; return s; }

    /// Standard destructor to make compiler happy
    virtual ~line_graph_iface() {}
};

/// 'provides live line graph values' interface
struct phase_graph_iface
{
    virtual bool get_phase_graph(int index, float ** _buffer, int *_length, int * _mode, bool * _use_fade, float * _fade, int * _accuracy, bool * _display) const { return false; };
    virtual ~phase_graph_iface() {}
};

enum table_column_type
{
    TCT_UNKNOWN, ///< guard invalid type
    TCT_FLOAT, ///< float value (encoded as C locale string)
    TCT_ENUM, ///< enum value (see: 'values' array in table_column_info) - encoded as string base 10 representation of integer
    TCT_STRING, ///< string value (encoded as C-escaped string)
    TCT_OBJECT, ///< external object, encoded as string
    TCT_LABEL, ///< string value (encoded as C-escaped string)
};

/// parameters of
struct table_column_info
{
    const char *name; ///< column label
    table_column_type type; ///< column data type
    float min; ///< minimum value (for float)
    float max; ///< maximum value (for float and enum)
    float def_value; ///< default value (for float and enum)
    const char **values; ///< NULL unless a TCT_ENUM, where it represents a NULL-terminated list of choices
};

/// 'has string parameters containing tabular data' interface
struct table_metadata_iface
{
    /// retrieve the table layout for specific parameter
    virtual const table_column_info *get_table_columns() const = 0;

    /// return the fixed number of rows, or 0 if the number of rows is variable
    virtual uint32_t get_table_rows() const = 0;

    virtual ~table_metadata_iface() {}
};

/// 'may receive configure variables' interface
struct send_configure_iface
{
    /// Called to set configure variable
    /// @param key variable name
    /// @param value variable content
    virtual void send_configure(const char *key, const char *value) = 0;

    virtual ~send_configure_iface() {}
};

/// 'may receive new status values' interface
struct send_updates_iface
{
    /// Called to set configure variable
    /// @param key variable name
    /// @param value variable content
    virtual void send_status(const char *key, const char *value) = 0;

    virtual ~send_updates_iface() {}
};

struct plugin_command_info;

/// General information about the plugin - @todo XXXKF lacks the "new" id-label-name triple
struct ladspa_plugin_info
{
    /// LADSPA ID
    uint32_t unique_id;
    /// plugin short name (camel case)
    const char *label;
    /// plugin human-readable name
    const char *name;
    /// maker (author)
    const char *maker;
    /// copyright notice
    const char *copyright;
    /// plugin type for LRDF/LV2
    const char *plugin_type;
};

/// An interface returning metadata about a plugin
struct plugin_metadata_iface
{
    enum { simulate_stereo_input = true, has_live_updates = true };
    /// @return plugin long name
    virtual const char *get_name() const = 0;
    /// @return plugin LV2 label
    virtual const char *get_id() const = 0;
    /// @return plugin human-readable label
    virtual const char *get_label() const = 0;
    /// @return total number of parameters
    virtual int get_param_count() const = 0;
    /// Return custom XML
    virtual char *get_gui_xml(const char *prefix) const = 0;
    /// @return number of audio inputs
    virtual int get_input_count() const =0;
    /// @return number of audio outputs
    virtual int get_output_count() const =0;
    /// @return number of optional inputs
    virtual int get_inputs_optional() const =0;
    /// @return number of optional outputs
    virtual int get_outputs_optional() const =0;
    /// @return true if plugin can work in hard-realtime conditions
    virtual bool is_rt_capable() const =0;
    /// @return true if plugin has MIDI input
    virtual bool get_midi() const =0;
    /// @return true if plugin has MIDI input
    virtual bool requires_midi() const =0;
    /// @return true if plugin requires instance access
    virtual bool requires_instance_access() const =0;
    /// @return port offset of first control (parameter) port (= number of audio inputs + number of audio outputs in all existing plugins as for 1 Aug 2008)
    virtual int get_param_port_offset() const  = 0;
    /// @return NULL-terminated list of menu commands
    virtual plugin_command_info *get_commands() const { return NULL; }
    /// @return description structure for given parameter
    virtual const parameter_properties *get_param_props(int param_no) const = 0;
    /// @return retrieve names of audio ports (@note control ports are named in parameter_properties, not here)
    virtual const char **get_port_names() const = 0;
    /// @return description structure for the plugin
    virtual const ladspa_plugin_info &get_plugin_info() const = 0;
    /// is a given parameter a control voltage?
    virtual bool is_cv(int param_no) const = 0;
    /// is the given parameter non-interpolated?
    virtual bool is_noisy(int param_no) const = 0;
    /// obtain array of names of configure variables (or NULL is none needed)
    virtual void get_configure_vars(std::vector<std::string> &names) const { names.clear(); }
    /// @return table_metadata_iface if any
    virtual const table_metadata_iface *get_table_metadata_iface(const char *key) const { return NULL; }
    /// @return whether to auto-connect right input with left input if unconnected
    virtual bool get_simulate_stereo_input() const = 0;
    /// @return whether live UI events are generated
    virtual bool sends_live_updates() const = 0;

    /// Do-nothing destructor to silence compiler warning
    virtual ~plugin_metadata_iface() {}
};

/// Interface for host-GUI-plugin interaction (should be really split in two, but ... meh)
struct plugin_ctl_iface
{
    /// @return value of given parameter
    virtual float get_param_value(int param_no) = 0;
    /// Set value of given parameter
    virtual void set_param_value(int param_no, float value) = 0;
    /// Load preset with given number
    virtual bool activate_preset(int bank, int program) = 0;
    /// @return volume level for port'th port (if supported by the implementation, currently only jack_host<Module> implements that by measuring signal level on plugin ports)
    virtual float get_level(unsigned int port)=0;
    /// Execute menu command with given number
    virtual void execute(int cmd_no)=0;
    /// Set a configure variable on a plugin
    virtual char *configure(const char *key, const char *value) = 0;
    /// Send all configure variables set within a plugin to given destination (which may be limited to only those that plugin understands)
    virtual void send_configures(send_configure_iface *)=0;
    /// Restore all state (parameters and configure vars) to default values - implemented in giface.cpp
    virtual void clear_preset();
    /// Call a named function in a plugin - this will most likely be redesigned soon - and never used
    /// @retval false call has failed, result contains an error message
    virtual bool blobcall(const char *command, const std::string &request, std::string &result) { result = "Call not supported"; return false; }
    /// Update status variables changed since last_serial
    /// @return new last_serial
    virtual int send_status_updates(send_updates_iface *sui, int last_serial) = 0;
    /// Return metadata object
    virtual const plugin_metadata_iface *get_metadata_iface() const = 0;
    /// @return line_graph_iface if any
    virtual const line_graph_iface *get_line_graph_iface() const = 0;
    /// @return phase_graph_iface if any
    virtual const phase_graph_iface *get_phase_graph_iface() const = 0;
    /// @return serial number of last automation write (JACK host only)
    virtual int get_write_serial(int param_no) { return 0; }

    /// Add or update parameter automation routing
    virtual void add_automation(uint32_t source, const automation_range &dest) {}
    /// Remove parameter automation routing
    virtual void delete_automation(uint32_t source, int param_no) {}
    /// Retrieve automation list for a given parameter
    /// @param param_no parameter to retrieve automation list for, or -1 for all
    virtual void get_automation(int param_no, std::multimap<uint32_t, automation_range> &dests) {}
    /// Return the source identifier for the most recently seen control change that could be used for automation
    virtual uint32_t get_last_automation_source() { return 0xFFFFFFFF; }
    virtual void send_automation_configures(send_configure_iface *) {}
    /// Do-nothing destructor to silence compiler warning
    virtual ~plugin_ctl_iface() {}
};

struct plugin_list_info_iface;

/// A class to retrieve and query the list of Calf plugins
class plugin_registry
{
public:
    typedef std::vector<const plugin_metadata_iface *> plugin_vector;
private:
    plugin_vector plugins;
    plugin_registry();
public:
    /// Get the singleton object.
    static plugin_registry &instance();

    /// Get all plugin metadata objects
    const plugin_vector &get_all() { return plugins; }
    /// Get single plugin metadata object by URI
    const plugin_metadata_iface *get_by_uri(const char *URI);
    /// Get single plugin metadata object by URI
    const plugin_metadata_iface *get_by_id(const char *id, bool case_sensitive = false);
};

/// Load and strdup a text file with GUI definition
extern char *load_gui_xml(const std::string &plugin_id);

/// Interface to audio processing plugins (the real things, not only metadata)
struct audio_module_iface
{
    /// Handle MIDI Note On
    virtual void note_on(int channel, int note, int velocity) = 0;
    /// Handle MIDI Note Off
    virtual void note_off(int channel, int note, int velocity) = 0;
    /// Handle MIDI Program Change
    virtual void program_change(int channel, int program) = 0;
    /// Handle MIDI Control Change
    virtual void control_change(int channel, int controller, int value) = 0;
    /// Handle MIDI Pitch Bend
    /// @param value pitch bend value (-8192 to 8191, defined as in MIDI ie. 8191 = 200 ct by default)
    virtual void pitch_bend(int channel, int value) = 0;
    /// Handle MIDI Channel Pressure
    /// @param value channel pressure (0 to 127)
    virtual void channel_pressure(int channel, int value) = 0;
    /// Called when params are changed (before processing)
    virtual void params_changed() = 0;
    /// LADSPA-esque activate function, except it is called after ports are connected, not before
    virtual void activate() = 0;
    /// LADSPA-esque deactivate function
    virtual void deactivate() = 0;
    /// Set sample rate for the plugin
    virtual void set_sample_rate(uint32_t sr) = 0;
    /// Execute menu command with given number
    virtual void execute(int cmd_no) = 0;
    /// DSSI configure call, value = NULL = reset to default
    virtual char *configure(const char *key, const char *value) = 0;
    /// Send all understood configure vars (none by default)
    virtual void send_configures(send_configure_iface *sci) = 0;
    /// Send all supported status vars (none by default)
    virtual int send_status_updates(send_updates_iface *sui, int last_serial) = 0;
    /// Reset parameter values for epp:trigger type parameters (ones activated by oneshot push button instead of check box)
    virtual void params_reset() = 0;
    /// Called after instantiating (after all the feature pointers are set - including interfaces like progress_report_iface)
    virtual void post_instantiate(uint32_t sample_rate) = 0;
    /// Return the arrays of port buffer pointers
    virtual void get_port_arrays(float **&ins_ptrs, float **&outs_ptrs, float **&params_ptrs) = 0;
    /// Return metadata object
    virtual const plugin_metadata_iface *get_metadata_iface() const = 0;
    /// Set the progress report interface to communicate progress to
    virtual void set_progress_report_iface(progress_report_iface *iface) = 0;
    /// Clear a part of output buffers that have 0s at mask; subdivide the buffer so that no runs > MAX_SAMPLE_RUN are fed to process function
    virtual uint32_t process_slice(uint32_t offset, uint32_t end) = 0;
    /// The audio processing loop; assumes numsamples <= MAX_SAMPLE_RUN, for larger buffers, call process_slice
    virtual uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) = 0;
    /// Message port processing function
    virtual uint32_t message_run(const void *valid_ports, void *output_ports) = 0;
    /// @return line_graph_iface if any
    virtual const line_graph_iface *get_line_graph_iface() const = 0;
     /// @return phase_graph_iface if any
    virtual const phase_graph_iface *get_phase_graph_iface() const = 0;
    virtual ~audio_module_iface() {}
};

/// Empty implementations for plugin functions.
template<class Metadata>
class audio_module: public Metadata, public audio_module_iface
{
public:
    typedef Metadata metadata_type;
    using Metadata::in_count;
    using Metadata::out_count;
    using Metadata::param_count;
    float *ins[(Metadata::in_count != 0)  ? Metadata::in_count : 1];
    float *outs[(Metadata::out_count != 0) ? Metadata::out_count : 1];
    float *params[Metadata::param_count];
    bool questionable_data_reported_in;
    bool questionable_data_reported_out;

    progress_report_iface *progress_report;

    audio_module() {
        progress_report = NULL;
        memset(ins, 0, sizeof(ins));
        memset(outs, 0, sizeof(outs));
        memset(params, 0, sizeof(params));
        questionable_data_reported_in = false;
        questionable_data_reported_out = false;
    }

    /// Handle MIDI Note On
    void note_on(int channel, int note, int velocity) {}
    /// Handle MIDI Note Off
    void note_off(int channel, int note, int velocity) {}
    /// Handle MIDI Program Change
    void program_change(int channel, int program) {}
    /// Handle MIDI Control Change
    void control_change(int channel, int controller, int value) {}
    /// Handle MIDI Pitch Bend
    /// @param value pitch bend value (-8192 to 8191, defined as in MIDI ie. 8191 = 200 ct by default)
    void pitch_bend(int channel, int value) {}
    /// Handle MIDI Channel Pressure
    /// @param value channel pressure (0 to 127)
    void channel_pressure(int channel, int value) {}
    /// Called when params are changed (before processing)
    void params_changed() {}
    /// LADSPA-esque activate function, except it is called after ports are connected, not before
    void activate() {}
    /// LADSPA-esque deactivate function
    void deactivate() {}
    /// Set sample rate for the plugin
    void set_sample_rate(uint32_t sr) { }
    /// Execute menu command with given number
    void execute(int cmd_no) {}
    /// DSSI configure call
    virtual char *configure(const char *key, const char *value) { return NULL; }
    /// Send all understood configure vars (none by default)
    void send_configures(send_configure_iface *sci) {}
    /// Send all supported status vars (none by default)
    int send_status_updates(send_updates_iface *sui, int last_serial) { return last_serial; }
    /// Reset parameter values for epp:trigger type parameters (ones activated by oneshot push button instead of check box)
    void params_reset() {}
    /// Called after instantiating (after all the feature pointers are set - including interfaces like progress_report_iface)
    void post_instantiate(uint32_t) {}
    /// Handle 'message context' port message
    /// @arg output_ports pointer to bit array of output port "changed" flags, note that 0 = first audio input, not first parameter (use input_count + output_count)
    uint32_t message_run(const void *valid_ports, void *output_ports) {
        fprintf(stderr, "ERROR: message run not implemented\n");
        return 0;
    }
    /// Return the array of input port pointers
    virtual void get_port_arrays(float **&ins_ptrs, float **&outs_ptrs, float **&params_ptrs)
    {
        ins_ptrs = ins;
        outs_ptrs = outs;
        params_ptrs = params;
    }
    /// Return metadata object
    virtual const plugin_metadata_iface *get_metadata_iface() const { return this; }
    /// Set the progress report interface to communicate progress to
    virtual void set_progress_report_iface(progress_report_iface *iface) { progress_report = iface; }

    /// utility function: zero port values if mask is 0
    inline void zero_by_mask(uint32_t mask, uint32_t offset, uint32_t nsamples)
    {
        for (int i=0; i<Metadata::out_count; i++) {
            if ((mask & (1 << i)) == 0) {
                dsp::zero(outs[i] + offset, nsamples);
            }
        }
    }
    /// utility function: call process, and if it returned zeros in output masks, zero out the relevant output port buffers
    uint32_t process_slice(uint32_t offset, uint32_t end)
    {
        bool had_errors = false;
        for (int i=0; i<Metadata::in_count; ++i) {
            float *indata = ins[i];
            if (indata) {
                float errval = 0;
                for (uint32_t j = offset; j < end; j++)
                {
                    if (!std::isfinite(indata[j]) || fabs(indata[j]) > 4294967296.0) {
                        errval = indata[j];
                        had_errors = true;
                    }
                }
                if (had_errors && !questionable_data_reported_in) {
                    fprintf(stderr, "Warning: Plugin %s got questionable value %f on its input %d\n", Metadata::get_name(), errval, i);
                    questionable_data_reported_in = true;
                }
            }
        }
        uint32_t total_out_mask = 0;
        while(offset < end)
        {
            uint32_t newend = std::min(offset + MAX_SAMPLE_RUN, end);
            uint32_t out_mask = !had_errors ? process(offset, newend - offset, -1, -1) : 0;
            total_out_mask |= out_mask;
            zero_by_mask(out_mask, offset, newend - offset);
            offset = newend;
        }
        for (int i=0; i<Metadata::out_count; ++i) {
            if (total_out_mask & (1 << i))
            {
                had_errors = false;
                float *outdata = outs[i];
                float errval = 0;
                for (uint32_t j = offset; j < end; j++)
                {
                    if (!std::isfinite(outdata[j]) || fabs(outdata[j]) > 4294967296.0) {
                        errval = outdata[j];
                        had_errors = true;
                    }
                }
                if (had_errors && !questionable_data_reported_out) {
                    fprintf(stderr, "Warning: Plugin %s generated questionable value %f on its output %d - this is most likely a bug in the plugin!\n", Metadata::get_name(), errval, i);
                    questionable_data_reported_out = true;
                }
                if (had_errors)
                    dsp::zero(outs[i] + offset, end - offset);
            }
        }
        return total_out_mask;
    }
    /// @return line_graph_iface if any
    virtual const line_graph_iface *get_line_graph_iface() const { return dynamic_cast<const line_graph_iface *>(this); }
    /// @return phase_graph_iface if any
    virtual const phase_graph_iface *get_phase_graph_iface() const { return dynamic_cast<const phase_graph_iface *>(this); }
};

#if USE_EXEC_GUI || USE_DSSI

enum line_graph_item
{
    LGI_END = 0,
    LGI_GRAPH,
    LGI_SUBGRAPH,
    LGI_LEGEND,
    LGI_DOT,
    LGI_END_ITEM,
    LGI_SET_RGBA,
    LGI_SET_WIDTH,
};

/// A class to send status updates via OSC
struct dssi_feedback_sender
{
    /// OSC client object used to send updates
    osctl::osc_client *client;
    /// Is client shared with something else?
    bool is_client_shared;
    /// Quit flag (used to terminate the thread)
    bool quit;
    /// Indices of graphs to send
    std::vector<int> indices;
    /// Source for the graph data (interface to marshal)
    const calf_plugins::line_graph_iface *graph;

    /// Source for the graph data (interface to marshal)
    const calf_plugins::phase_graph_iface *phase;

    /// Create using a new client
    dssi_feedback_sender(const char *URI, const line_graph_iface *_graph);
    dssi_feedback_sender(osctl::osc_client *_client, const line_graph_iface *_graph);
    void add_graphs(const calf_plugins::parameter_properties *props, int num_params);
    void update();
    ~dssi_feedback_sender();
};
#endif

/// Metadata base class template, to provide default versions of interface functions
template<class Metadata>
class plugin_metadata: public plugin_metadata_iface
{
public:
    static const char *port_names[];
    static parameter_properties param_props[];
    static ladspa_plugin_info plugin_info;
    typedef plugin_metadata<Metadata> metadata_class;

    // These below are stock implementations based on enums and static members in Metadata classes
    // they may be overridden to provide more interesting functionality

    const char *get_name() const { return Metadata::impl_get_name(); }
    const char *get_id() const { return Metadata::impl_get_id(); }
    const char *get_label() const { return Metadata::impl_get_label(); }
    int get_input_count() const { return Metadata::in_count; }
    int get_output_count() const { return Metadata::out_count; }
    int get_inputs_optional() const { return Metadata::ins_optional; }
    int get_outputs_optional() const { return Metadata::outs_optional; }
    int get_param_count() const { return Metadata::param_count; }
    bool get_midi() const { return Metadata::support_midi; }
    bool requires_midi() const { return Metadata::require_midi; }
    bool requires_instance_access() const { return Metadata::require_instance_access; }
    bool is_rt_capable() const { return Metadata::rt_capable; }
    int get_param_port_offset()  const { return Metadata::in_count + Metadata::out_count; }
    char *get_gui_xml(const char *prefix) const { char xmlf[64]; sprintf(xmlf, "%s/%s", prefix, get_id()); char *data_ptr = calf_plugins::load_gui_xml(xmlf); return data_ptr; }
    plugin_command_info *get_commands() const { return NULL; }
    const parameter_properties *get_param_props(int param_no) const { return &param_props[param_no]; }
    const char **get_port_names() const { return port_names; }
    bool is_cv(int param_no) const { return true; }
    bool is_noisy(int param_no) const { return false; }
    const ladspa_plugin_info &get_plugin_info() const { return plugin_info; }
    bool get_simulate_stereo_input() const { return Metadata::simulate_stereo_input; }
    bool sends_live_updates() const { return Metadata::has_live_updates; }
};

#define CALF_PORT_NAMES(name) template<> const char *calf_plugins::plugin_metadata<name##_metadata>::port_names[]
#define CALF_PORT_PROPS(name) template<> parameter_properties calf_plugins::plugin_metadata<name##_metadata>::param_props[name##_metadata::param_count + 1]
#define CALF_PLUGIN_INFO(name) template<> calf_plugins::ladspa_plugin_info calf_plugins::plugin_metadata<name##_metadata>::plugin_info
#define PLUGIN_NAME_ID_LABEL(name, id, label) \
    static const char *impl_get_name() { return name; } \
    static const char *impl_get_id() { return id; } \
    static const char *impl_get_label() { return label; } \

extern const char *calf_copyright_info;

bool get_freq_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context, bool use_frequencies = true, float res = 256, float ofs = 0.4);

/// convert amplitude value to normalized grid-ish value
static inline float dB_grid(float amp, float res = 256, float ofs = 0.4)
{
    return log(amp) * (1.0 / log(res)) + ofs;
}

template<class Fx>
static bool get_graph(Fx &fx, int subindex, float *data, int points, float res = 256, float ofs = 0.4)
{
    for (int i = 0; i < points; i++)
    {
        //typedef std::complex<double> cfloat;
        double freq = 20.0 * pow (20000.0 / 20.0, i * 1.0 / points);
        data[i] = dB_grid(fx.freq_gain(subindex, freq), res, ofs);
    }
    return true;
}

/// convert normalized grid-ish value back to amplitude value
static inline float dB_grid_inv(float pos, float res = 256, float ofs = 0.4)
{
    return pow(res, pos - ofs);
}

/// Line graph interface implementation for frequency response graphs
class frequency_response_line_graph: public line_graph_iface
{
public:
    mutable bool redraw_graph;
    virtual bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    virtual bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    virtual bool get_layers(int index, int generation, unsigned int &layers) const;
    virtual float freq_gain(int index, double freq) const { return 0; };
    virtual std::string get_crosshair_label( int x, int y, int sx, int sy, float q, int dB, int name, int note, int cents) const;
};
std::string frequency_crosshair_label(int x, int y, int sx, int sy, float q, int dB, int name, int note, int cents, double res = 256, double ofs = 0.4);


/// set drawing color based on channel index (0 or 1)
void set_channel_color(cairo_iface *context, int channel, float alpha = 0.6);
void set_channel_dash(cairo_iface *context, int channel);
void draw_cairo_label(cairo_iface *context, const char *label, float x, float y, int pos, float margin, float align);

struct preset_access_iface
{
    virtual void store_preset() = 0;
    virtual void activate_preset(int preset, bool builtin) = 0;
    virtual ~preset_access_iface() {}
};

/// Implementation of table_metadata_iface providing metadata for mod matrices
class mod_matrix_metadata: public table_metadata_iface
{
public:
    /// Mapping modes
    enum mapping_mode {
        map_positive, ///< 0..100%
        map_bipolar, ///< -100%..100%
        map_negative, ///< -100%..0%
        map_squared, ///< x^2
        map_squared_bipolar, ///< x^2 scaled to -100%..100%
        map_antisquared, ///< 1-(1-x)^2 scaled to 0..100%
        map_antisquared_bipolar, ///< 1-(1-x)^2 scaled to -100..100%
        map_parabola, ///< inverted parabola (peaks at 0.5, then decreases to 0)
        map_type_count
    };
    const char **mod_src_names, **mod_dest_names;

    mod_matrix_metadata(unsigned int _rows, const char **_src_names, const char **_dest_names);
    virtual const table_column_info *get_table_columns() const;
    virtual uint32_t get_table_rows() const;
    /// Return a list of configure variables used by the modulation matrix
    void get_configure_vars(std::vector<std::string> &names) const;

protected:
    /// Column descriptions for table widget
    table_column_info table_columns[6];

    unsigned int matrix_rows;
};

/// Check if a given key is either prefix + rows or prefix + i2s(row) + "," + i2s(column)
/// @arg key key to parse
/// @arg prefix table prefix (e.g. "modmatrix:")
/// @arg is_rows[out] set to true if key == prefix + "rows"
/// @arg row[out] if key is of a form: prefix + row + "," + i2s(column), returns row, otherwise returns -1
/// @arg column[out] if key is of a form: prefix + row + "," + i2s(column), returns row, otherwise returns -1
/// @retval true if this is one of the recognized string forms
extern bool parse_table_key(const char *key, const char *prefix, bool &is_rows, int &row, int &column);

#if USE_EXEC_GUI
class table_via_configure
{
protected:
    typedef std::pair<int, int> coord;
    std::vector<table_column_info> columns;
    std::map<coord, std::string> values;
    int rows;
public:
    table_via_configure();
    void configure(const char *key, const char *value);
    virtual ~table_via_configure();
};
#endif

struct automation_range
{
    float min_value;
    float max_value;
    int param_no;

    automation_range(float l, float u, int param)
    : min_value(l)
    , max_value(u)
    , param_no(param)
    {}
    void send_configure(const plugin_metadata_iface *metadata, uint32_t from_controller, send_configure_iface *sci);
    static automation_range *new_from_configure(const plugin_metadata_iface *metadata, const char *key, const char *value, uint32_t &from_controller);
};

struct automation_map: public std::multimap<uint32_t, automation_range>
{
};

inline float subindex_to_freq(int subindex)
{
  float freq = 100;
  if (subindex < 9)
    freq = 10 * (subindex + 1);
  else if (subindex < 18)
    freq = 100 * (subindex - 9 + 1);
  else if (subindex < 27)
    freq = 1000 * (subindex - 18 + 1);
  else
    freq = 10000 * (subindex - 27 + 1);

  return freq;
}

};

#define FORWARD_DECLARE_METADATA(plugin) \
    template<> const char *calf_plugins::plugin_metadata<calf_plugins::plugin##_metadata>::port_names[]; \
    template<> calf_plugins::parameter_properties calf_plugins::plugin_metadata<calf_plugins::plugin##_metadata>::param_props[]; \
    template<> calf_plugins::ladspa_plugin_info calf_plugins::plugin_metadata<calf_plugins::plugin##_metadata>::plugin_info;


#endif
