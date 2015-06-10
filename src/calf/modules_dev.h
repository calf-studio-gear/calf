/* Calf DSP Library
 * Prototype audio modules
 *
 * Copyright (C) 2008 Thor Harald Johansen <thj@thj.no>
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
#ifndef __CALF_MODULES_DEV_H
#define __CALF_MODULES_DEV_H

#include <calf/metadata.h>

#if ENABLE_EXPERIMENTAL
#include <fluidsynth.h>
#endif

namespace calf_plugins {

#if ENABLE_EXPERIMENTAL
    
/// Tiny wrapper for fluidsynth
class fluidsynth_audio_module: public audio_module<fluidsynth_metadata>
{
protected:
    /// Current sample rate
    uint32_t srate;
    /// FluidSynth Settings object
    fluid_settings_t *settings;
    /// FluidSynth Synth object
    fluid_synth_t *synth;
    /// Soundfont filename
    std::string soundfont;
    /// Soundfont filename (as received from Fluidsynth)
    std::string soundfont_name;
    /// TAB-separated preset list (preset+128*bank TAB preset name LF)
    std::string soundfont_preset_list;
    /// FluidSynth assigned SoundFont ID
    int sfid;
    /// Map of preset+128*bank to preset name
    std::map<uint32_t, std::string> sf_preset_names;
    /// Last selected preset+128*bank in each channel
    uint32_t last_selected_presets[16];
    /// Serial number of status data
    volatile int status_serial;
    /// Preset number to set on next process() call
    volatile int set_presets[16];
    volatile bool soundfont_loaded;

    /// Update last_selected_preset based on synth object state
    void update_preset_num(int channel);
    /// Send a bank/program change sequence for a specific channel/preset combo
    void select_preset_in_channel(int ch, int new_preset);
    /// Create a fluidsynth object and load the current soundfont
    fluid_synth_t *create_synth(int &new_sfid);
public:
    /// Constructor to initialize handles to NULL
    fluidsynth_audio_module();

    void post_instantiate(uint32_t sr);
    void set_sample_rate(uint32_t sr) { srate = sr; }
    /// Handle MIDI Note On message (by sending it to fluidsynth)
    void note_on(int channel, int note, int vel);
    /// Handle MIDI Note Off message (by sending it to fluidsynth)
    void note_off(int channel, int note, int vel);
    /// Handle pitch bend message.
    inline void pitch_bend(int channel, int value)
    {
        fluid_synth_pitch_bend(synth, 0, value + 0x2000);
    }
    /// Handle control change messages.
    void control_change(int channel, int controller, int value);
    /// Handle program change messages.
    void program_change(int channel, int program);

    /// Update variables from control ports.
    void params_changed() {
    }
    void activate();
    void deactivate();
    /// No CV inputs for now
    bool is_cv(int param_no) { return false; }
    /// Practically all the stuff here is noisy... for now
    bool is_noisy(int param_no) { return true; }
    /// Main processing function
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    /// DSSI-style configure function for handling string port data
    char *configure(const char *key, const char *value);
    void send_configures(send_configure_iface *sci);
    int send_status_updates(send_updates_iface *sui, int last_serial);
    uint32_t message_run(const void *valid_inputs, void *output_ports) { 
        // silence a default printf (which is kind of a warning about unhandled message_run)
        return 0;
    }
    ~fluidsynth_audio_module();
};

#define TRIGGER_MAX_PLAYBACK            16
#define TRIGGER_MIN_LOOKUP_MS           0.1
#define TRIGGER_MAX_LOOKUP_MS           20

typedef struct trigger_sample_t
{
    uint32_t    srate;          // Sample rate
    size_t      channels;       // Channels per file
    size_t      samples;        // Number of samples per track
    float      *frames;         // Frame data
    std::string filename;       // File name

    inline trigger_sample_t()
    {
        srate       = 0;
        channels    = 0;
        samples     = 0;
        frames      = NULL;
        filename    = "";
    }

    ~trigger_sample_t()
    {
        free();
    }

    void free();
    bool load(const char *file);
} trigger_sample_t;

typedef struct trigger_playback_t
{
    size_t      offset;         // Playback pointer
    size_t      limit;          // Maximum playback pointer
    float       gain;           // Loudness

    inline trigger_playback_t()
    {
        offset = 0;
        limit  = 0;
        gain   = 0;
    }

    inline void start(size_t off, size_t lim, float amp)
    {
        offset = off;
        limit  = lim;
        gain   = amp;
    }
} trigger_playback_t;

struct mono_trigger_t;

typedef void (* level_detector_t)(const mono_trigger_t *t, float &level, float &attack);

#define TRG_STATE_CLOSED        0
#define TRG_STATE_MEASURE       1
#define TRG_STATE_OPEN          2


typedef struct mono_trigger_t
{
    // Internal state
    size_t              playbacks;  // Number of currently active playbacks
    size_t              state;      // State of trigger
    trigger_playback_t  playback[TRIGGER_MAX_PLAYBACK]; // Playback tasks
    float              *lkp_buffer; // Lookup buffer
    size_t              lkp_ptr;    // Lookup buffer pointer
    size_t              lkp_size;   // Lookup buffer size, guaranteed to be power of 2
    size_t              lkp_samples;// Lookup samples
    ssize_t             release;    // Number of samples before trigger releases

    // Configurable parameters
    size_t              rel_samples;    // Initial value of release in samples
    size_t              max_playbacks;  // Maximum value of playbacks
    ssize_t             track;          // ID of track to use
    size_t              min_offset;     // Min offset samples
    size_t              max_offset;     // Max offset samples
    size_t              mode;           // Mode
    float               open_thresh;    // Open threshold
    float               close_thresh;   // Close threshold
    float               dynamics;       // Dynamics
    float               dry;            // Dry amount
    float               wet;            // Wet amount
    float               in_gain;        // Input gain
    float               out_gain;       // Output gain

    // Meter outputs
    float               mtr_in;         // Input meter
    float               mtr_out;        // Output meter
    bool                fired;          // Flag that trigger has fired

    level_detector_t    detector;       // Level detector

    mono_trigger_t();
    ~mono_trigger_t();

    inline void mute() { track = -1; max_offset = 0; }
    void        free();
    void        process(const float *in, float *out, const trigger_sample_t *t_sample, size_t count);
    void        change_sample_rate(uint32_t srate);
} mono_trigger_t;

/// Trigger implementation
class trigger_audio_module: public audio_module<trigger_metadata>
{
protected:
    // Serial number of status data
    int status_serial;

    // Current sample rate
    uint32_t srate;

    // Trigger data
    mono_trigger_t      channels[2];
    trigger_sample_t    sample;
    uint32_t            flash[2];

public:
    /// Constructor to initialize handles to NULL
    trigger_audio_module();
    virtual ~trigger_audio_module();

    void set_sample_rate(uint32_t sr);

    /// Update variables from control ports.
    void params_changed();

    /// Main processing function
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask);

    /// DSSI-style configure function for handling string port data
    char *configure(const char *key, const char *value);
    void send_configures(send_configure_iface *sci);
    int send_status_updates(send_updates_iface *sui, int last_serial);
};

#endif
    
};

#endif
