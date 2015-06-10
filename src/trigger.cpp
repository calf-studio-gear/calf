/* Calf DSP Library
 * Trigger implementation
 *
 * Copyright (C) 2013 Vladimir Sadovnikov
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
 * Boston, MA  02110-1301  USA
 */
#include <calf/giface.h>
#include <calf/modules_dev.h>
#include <calf/buffer.h>

#include <string.h>
#include <math.h>
#include <sndfile.h>

#if ENABLE_EXPERIMENTAL

#define SET_IF_CONNECTED(name, value) if (params[trigger_audio_module::name] != NULL) *params[trigger_audio_module::name] = value;

using namespace dsp;
using namespace calf_plugins;
using namespace std;

//-----------------------------------------------------------------------------
// Level detectors
template <class F>
    void level_detector(const mono_trigger_t *trg, float &level, float &angle)
    {
        size_t lkp_limit    = trg->lkp_size - 1;
        float dt            = 1.0 / trg->lkp_samples;
        float pr_sample     = 0.0;
        float pr_f          = 0.0;
        float t             = 0.0;

        float v_level       = 0.0;
        float v_angle       = 0.0;

        size_t l = (trg->lkp_size + trg->lkp_ptr - trg->lkp_samples) & lkp_limit; // add lkp_size for unsigned math
        while  (l != trg->lkp_ptr)
        {
            float sample    = trg->lkp_buffer[l];
            float f         = F::f(t);

            v_level        += (f - pr_f) * sample;
            v_angle        += f * (sample - pr_sample);

            l               = (l + 1) & lkp_limit;
            t              += dt;
            pr_sample       = sample;
            pr_f            = f;
        }

        level       = v_level;
        angle       = v_angle;
    }

typedef struct linear_function_t
{
    static float f(float t)
    {
        return t;
    }
} linear_function_t;

typedef struct sqrt_function_t
{
    static float f(float t)
    {
        return sqrtf(1.0 - (1.0 - t) * (1.0 - t));
    }
} sqrt_function_t;

typedef struct quad_function_t
{
    static float f(float t)
    {
        return (1.0 - sqrtf(1.0 - t*t));
    }
} quad_function_t;

void linear_detector(const mono_trigger_t *trg, float &level, float &attack)
{
    level_detector<linear_function_t>(trg, level, attack);
}

void sqrt_detector(const mono_trigger_t *trg, float &level, float &attack)
{
    level_detector<sqrt_function_t>(trg, level, attack);
}

void quad_detector(const mono_trigger_t *trg, float &level, float &attack)
{
    level_detector<quad_function_t>(trg, level, attack);
}

//-----------------------------------------------------------------------------
void trigger_sample_t::free()
{
    srate       = 0;
    channels    = 0;
    samples     = 0;
    filename    = "";

    if (frames != NULL)
    {
        delete[] frames;
        frames = NULL;
    }
}

bool trigger_sample_t::load(const char *file)
{
    SNDFILE *sf_obj;
    SF_INFO sf_info;

    free();

    printf("loading file: %s\n", file);

    if ((sf_obj = sf_open(file, SFM_READ, &sf_info)) == NULL)
        return false;

    printf("file parameters: frames=%d, channels=%d, sample_rate=%d\n",
            (int)sf_info.frames, (int)sf_info.channels, (int)sf_info.samplerate);

    float *data = new float[sf_info.frames * sf_info.channels];

    if (sf_readf_float(sf_obj, data, sf_info.frames) < sf_info.frames)
    {
        delete [] data;
        sf_close(sf_obj);
        return false;
    }

    sf_close(sf_obj);

    srate           = sf_info.samplerate;
    channels        = sf_info.channels;
    samples         = sf_info.frames;
    frames          = data;
    filename        = file;

    return true;
}

//-----------------------------------------------------------------------------
mono_trigger_t::mono_trigger_t()
{
    playbacks       = 0;
    state           = TRG_STATE_CLOSED;
    lkp_buffer      = NULL;
    lkp_ptr         = 0;
    lkp_size        = 0;
    lkp_samples     = 0;
    release         = 0;

    rel_samples     = 0;
    max_playbacks   = 0;
    track           = -1;
    min_offset      = 0;
    max_offset      = 0;
    mode            = 0;
    open_thresh     = 0;
    close_thresh    = 0;
    dynamics        = 0;
    dry             = 0;
    wet             = 0;
    in_gain         = 0;
    out_gain        = 0;

    mtr_in          = 0;
    mtr_out         = 0;
    fired           = false;

    detector        = linear_detector;
}

mono_trigger_t::~mono_trigger_t()
{
    free();
}

void mono_trigger_t::free()
{
    if (lkp_buffer != NULL)
    {
        delete [] lkp_buffer;
        lkp_buffer      = NULL;
        lkp_size        = 0;
        lkp_ptr         = 0;
    }
}

void mono_trigger_t::change_sample_rate(uint32_t srate)
{
    free();

    size_t max_buf_size = round(TRIGGER_MAX_LOOKUP_MS * srate * 0.001);
    size_t buf_size = 1;
    while (buf_size < max_buf_size)
        buf_size <<= 1;

    lkp_buffer      = new float[buf_size];
    lkp_size        = buf_size;
    lkp_ptr         = 0;
}

void mono_trigger_t::process(const float *in, float *out, const trigger_sample_t *t_sample, size_t count)
{
    size_t  lkp_limit   = lkp_size - 1;

    mtr_in              = 0.0;
    mtr_out             = 0.0;
    fired               = false;

    for (size_t i = 0; i < count; i++)
    {
        // Get sample
        float sample        = in[i] * in_gain;
        if (mtr_in < sample)
            mtr_in = sample;

        // Store sample to lookup buffer
        lkp_buffer[lkp_ptr] = (sample < 0) ? -sample : sample;
        lkp_ptr             = (lkp_ptr + 1) & lkp_limit;

        // Perform lookup
        float level = 0, angle = 0;
        detector(this, level, angle);

        // Check trigger state
        if (state == TRG_STATE_CLOSED) // Trigger is closed
        {
            if ((level >= open_thresh) && (angle > 0)) // Signal is growing, open trigger
            {
                // Mark trigger open
                state       = TRG_STATE_MEASURE;
                fired       = true;
                release     = rel_samples;
            }
        }
        else if (state == TRG_STATE_MEASURE)
        {
            fired       = true;

            if ((level >= open_thresh) && (angle <= 0)) // Signal is in peak
            {
                state = TRG_STATE_OPEN;

                // Append Playback
                if (playbacks >= max_playbacks)
                {
                    // Cleanup oldest playbacks
                    size_t remove = playbacks - max_playbacks + 1;
                    for (size_t r = remove; r < playbacks; r++)
                        playback[r - remove] = playback[r];
                    playbacks = max_playbacks;
                }
                else
                    playbacks++;

                // Initialize new playback
                trigger_playback_t *pb = &playback[playbacks - 1];
                pb->start(
                    min_offset, max_offset,
                    (mode == 0) ?
                        1.0 * (1.0 - dynamics) + level * dynamics :
                        level + (1.0 - level) * (1.0 - dynamics)
                );
            }
        }
        else // Trigger is open, check for release
        {
            if ((level <= close_thresh) && (angle <= 0)) // Signal is falling, close trigger
            {
                if ((release--) <= 0)
                    state = TRG_STATE_CLOSED;
            }
            else // Reset release sample counter
                release = rel_samples;
        }

        // Render samples
        float render = 0.0;
        for (size_t p = 0; p < playbacks; )
        {
            trigger_playback_t *pb = &playback[p];

            // Check that index is not out of bound
            if ((pb->offset >= t_sample->samples) || (pb->offset >= pb->limit))
            {
                // Remove finished playback
                for (size_t r = p+1; r < playbacks; r++)
                    playback[r-1] = playback[r];

                // Decrement number of playbacks
                playbacks--;
            }
            else
            {
                // Get sample & increment index
                if ((track < (ssize_t)t_sample->channels) && (track >= 0))
                    render += t_sample->frames[pb->offset * t_sample->channels + track] * pb->gain;
                pb->offset++;

                // Increment playback calculation index
                p++;
            }
        }

        // This is real output
        out[i] = (sample * dry + render * wet) * out_gain;

        if (mtr_out < out[i])
            mtr_out = out[i];
    }
}

//-----------------------------------------------------------------------------
trigger_audio_module::trigger_audio_module()
{
    status_serial   = 1;
    srate           = 0;
}

void trigger_audio_module::set_sample_rate(uint32_t sr)
{
    channels[0].change_sample_rate(sr);
    channels[1].change_sample_rate(sr);

    srate = sr;
}

void trigger_audio_module::params_changed()
{
    // Set-up knobs
    for (size_t i=0; i<2; i++)
    {
        mono_trigger_t *t   = &channels[i];

        t->lkp_samples      = *params[par_lookup] * srate * 0.001 ;
        t->rel_samples      = *params[par_release] * srate * 0.001;
        t->max_playbacks    = *params[par_sample_playbacks];
        t->min_offset       = *params[par_sample_head] * srate * 0.001;
        t->mode             = (*params[par_dyn_mode] > 0.5) ? 1 : 0;
        t->open_thresh      = *params[par_open_threshold];
        t->close_thresh     = *params[par_close_threshold];
        t->dynamics         = *params[par_dyn_amount];
        t->dry              = *params[par_dry];
        t->wet              = *params[par_wet];
        t->in_gain          = *params[par_in_gain];
        t->out_gain         = *params[par_out_gain];

        switch (size_t(*params[par_dyn_function]))
        {
            case 0: t->detector = linear_detector; break;
            case 1: t->detector = sqrt_detector; break;
            case 2: t->detector = quad_detector; break;
        }
    }

    // Set-up files
    if ((sample.frames != NULL) && (sample.channels > 0))
    {
        if (sample.channels > *params[par_sample_track_l])
        {
            channels[0].track       = *params[par_sample_track_l];
            channels[0].max_offset  = *params[par_sample_tail] * sample.samples;
        }
        else
            channels[0].mute();

        if (sample.channels > *params[par_sample_track_r])
        {
            channels[1].track       = *params[par_sample_track_r];
            channels[1].max_offset  = *params[par_sample_tail] * sample.samples;
        }
        else
            channels[1].mute();
    }
    else
    {
        channels[0].mute();
        channels[1].mute();
    }
}

uint32_t trigger_audio_module::process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    float mtr_input[2], mtr_output[2];
    mtr_input[0]    = 0;
    mtr_input[1]    = 0;
    mtr_output[0]   = 0;
    mtr_output[1]   = 0;

    // Check bypass
    if (*params[par_bypass] > 0.5f)
    {
        flash[0]    = 0;
        flash[1]    = 0;

        // everything bypassed
        nsamples += offset;
        while (offset < nsamples)
        {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
    }
    else
    {
        // Call trigger processing
        for (size_t i=0; i<2; i++)
        {
            flash[i] -= std::min(flash[i], nsamples);

            mono_trigger_t *t = &channels[i];

            t->process(&ins[i][offset], &outs[i][offset], &sample, nsamples);

            // Update meters & flashing
            mtr_input[i]    = t->mtr_in;
            mtr_output[i]   = t->mtr_out;

            if ((t->fired) || (t->state != TRG_STATE_CLOSED))
                flash[i] = srate >> 3;
        }
    }

    SET_IF_CONNECTED(mtr_input_l, mtr_input[0]);
    SET_IF_CONNECTED(mtr_input_r, mtr_input[1]);
    SET_IF_CONNECTED(mtr_output_l, mtr_output[0]);
    SET_IF_CONNECTED(mtr_output_r, mtr_output[1]);
    SET_IF_CONNECTED(flash_l, flash[0]);
    SET_IF_CONNECTED(flash_r, flash[1]);

    return outputs_mask;
}

trigger_audio_module::~trigger_audio_module()
{
}

void trigger_audio_module::send_configures(send_configure_iface *sci)
{
    sci->send_configure("file", sample.filename.c_str());
}

int trigger_audio_module::send_status_updates(send_updates_iface *sui, int last_serial)
{
    if (status_serial != last_serial)
    {
        sui->send_status("file", sample.filename.c_str());
    }
    return status_serial;
}

char *trigger_audio_module::configure(const char *key, const char *value)
{
    if (strcmp(key, "file") == 0)
    {
        channels[0].mute();
        channels[1].mute();

        sample.load(value);

        params_changed();
    }

    return NULL;
}

#endif
