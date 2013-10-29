/* Calf DSP plugin pack
 * Equalization related plugins
 *
 * Copyright (C) 2001-2010 Krzysztof Foltman, Markus Schmidt, Thor Harald Johansen and others
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
#include <limits.h>
#include <memory.h>
#include <calf/giface.h>
#include <calf/modules_filter.h>

using namespace dsp;
using namespace calf_plugins;

/// Equalizer 12 Band by Markus Schmidt
///
/// This module is based on Krzysztof's filters. It provides a couple
/// of different chained filters.
///////////////////////////////////////////////////////////////////////////////////////////////

inline void diff_ms(float &left, float &right) {
    float tmp = (left + right) / 2;
    right = left - right;
    left = tmp;
}
inline void undiff_ms(float &left, float &right) {
    float tmp = left + right / 2;
    right = left - right / 2;
    left = tmp;
}

template<class BaseClass, bool has_lphp>
equalizerNband_audio_module<BaseClass, has_lphp>::equalizerNband_audio_module()
{
    is_active = false;
    srate = 0;
    last_generation = 0;
    hp_freq_old = lp_freq_old = 0;
    hs_freq_old = ls_freq_old = 0;
    hs_level_old = ls_level_old = 0;
    keep_gliding = 0;
    for (int i = 0; i < AM::PeakBands; i++)
    {
        p_freq_old[i] = 0;
        p_level_old[i] = 0;
        p_q_old[i] = 0;
    }
}

template<class BaseClass, bool has_lphp>
void equalizerNband_audio_module<BaseClass, has_lphp>::activate()
{
    is_active = true;
    // set all filters
    params_changed();
    meters.reset();
}

template<class BaseClass, bool has_lphp>
void equalizerNband_audio_module<BaseClass, has_lphp>::deactivate()
{
    is_active = false;
}

static inline void copy_lphp(biquad_d2<float> filters[3][2])
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 2; j++)
            if (i || j)
                filters[i][j].copy_coeffs(filters[0][0]);
}

static inline double glide(double value, double target, int &keep_gliding)
{
    if (target == value)
        return value;
    keep_gliding = 1;
    if (target > value)
        return std::min(target, (value + 0.1) * 1.003);
    else
        return std::max(target, (value / 1.003) - 0.1);
}

template<class BaseClass, bool has_lphp>
void equalizerNband_audio_module<BaseClass, has_lphp>::params_changed()
{
    keep_gliding = 0;
    // set the params of all filters
    
    // lp/hp first (if available)
    if (has_lphp)
    {
        hp_mode = (CalfEqMode)(int)*params[AM::param_hp_mode];
        lp_mode = (CalfEqMode)(int)*params[AM::param_lp_mode];

        float hpfreq = *params[AM::param_hp_freq], lpfreq = *params[AM::param_lp_freq];
        
        if(hpfreq != hp_freq_old) {
            hpfreq = glide(hp_freq_old, hpfreq, keep_gliding);
            hp[0][0].set_hp_rbj(hpfreq, 0.707, (float)srate, 1.0);
            copy_lphp(hp);
            hp_freq_old = hpfreq;
        }
        if(lpfreq != lp_freq_old) {
            lpfreq = glide(lp_freq_old, lpfreq, keep_gliding);
            lp[0][0].set_lp_rbj(lpfreq, 0.707, (float)srate, 1.0);
            copy_lphp(lp);
            lp_freq_old = lpfreq;
        }
    }
    
    // then shelves
    float hsfreq = *params[AM::param_hs_freq], hslevel = *params[AM::param_hs_level];
    float lsfreq = *params[AM::param_ls_freq], lslevel = *params[AM::param_ls_level];
    
    if(lsfreq != ls_freq_old or lslevel != ls_level_old) {
        lsfreq = glide(ls_freq_old, lsfreq, keep_gliding);
        lsL.set_lowshelf_rbj(lsfreq, 0.707, lslevel, (float)srate);
        lsR.copy_coeffs(lsL);
        ls_level_old = lslevel;
        ls_freq_old = lsfreq;
    }
    if(hsfreq != hs_freq_old or hslevel != hs_level_old) {
        hsfreq = glide(hs_freq_old, hsfreq, keep_gliding);
        hsL.set_highshelf_rbj(hsfreq, 0.707, hslevel, (float)srate);
        hsR.copy_coeffs(hsL);
        hs_level_old = hslevel;
        hs_freq_old = hsfreq;
    }
    for (int i = 0; i < AM::PeakBands; i++)
    {
        int offset = i * params_per_band;
        float freq = *params[AM::param_p1_freq + offset];
        float level = *params[AM::param_p1_level + offset];
        float q = *params[AM::param_p1_q + offset];
        if(freq != p_freq_old[i] or level != p_level_old[i] or q != p_q_old[i]) {
            freq = glide(p_freq_old[i], freq, keep_gliding);
            pL[i].set_peakeq_rbj(freq, q, level, (float)srate);
            pR[i].copy_coeffs(pL[i]);
            p_freq_old[i] = freq;
            p_level_old[i] = level;
            p_q_old[i] = q;
        }
    }
}

template<class BaseClass, bool has_lphp>
inline void equalizerNband_audio_module<BaseClass, has_lphp>::process_hplp(float &left, float &right)
{
    if (!has_lphp)
        return;
    int active = *params[AM::param_lp_active];
    if (active > 0.f)
    {
        if (active > 3) diff_ms(left, right);
        switch(lp_mode)
        {
            case MODE12DB:
                if (active == 1 or active == 2 or active == 4)
                    left = lp[0][0].process(left);
                if (active == 1 or active == 3 or active == 5)
                    right = lp[0][1].process(right);
                break;
            case MODE24DB:
                if (active == 1 or active == 2 or active == 4)
                    left = lp[1][0].process(lp[0][0].process(left));
                if (active == 1 or active == 3 or active == 5)
                    right = lp[1][1].process(lp[0][1].process(right));
                break;
            case MODE36DB:
                if (active == 1 or active == 2 or active == 4)
                    left = lp[2][0].process(lp[1][0].process(lp[0][0].process(left)));
                if (active == 1 or active == 3 or active == 5)
                    right = lp[2][1].process(lp[1][1].process(lp[0][1].process(right)));
                break;
        }
        if (active > 3) undiff_ms(left, right);
    }
    active = *params[AM::param_hp_active];
    if (active > 0.f)
    {
        if (active > 3) diff_ms(left, right);
        switch(hp_mode)
        {
            case MODE12DB:
                if (active == 1 or active == 2 or active == 4)
                    left = hp[0][0].process(left);
                if (active == 1 or active == 3 or active == 5)
                    right = hp[0][1].process(right);
                break;
            case MODE24DB:
                if (active == 1 or active == 2 or active == 4)
                    left = hp[1][0].process(hp[0][0].process(left));
                if (active == 1 or active == 3 or active == 5)
                    right = hp[1][1].process(hp[0][1].process(right));
                break;
            case MODE36DB:
                if (active == 1 or active == 2 or active == 4)
                    left = hp[2][0].process(hp[1][0].process(hp[0][0].process(left)));
                if (active == 1 or active == 3 or active == 5)
                    right = hp[2][1].process(hp[1][1].process(hp[0][1].process(right)));
                break;
        }
        if (active > 3) undiff_ms(left, right);
    }
}

template<class BaseClass, bool has_lphp>
uint32_t equalizerNband_audio_module<BaseClass, has_lphp>::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[AM::param_bypass] > 0.f;
    if (keep_gliding && !bypass)
    {
        // ensure that if params have changed, the params_changed method is
        // called every 8 samples to interpolate filter parameters
        while(numsamples > 8 && keep_gliding)
        {
            params_changed();
            outputs_mask |= process(offset, 8, inputs_mask, outputs_mask);
            offset += 8;
            numsamples -= 8;
        }
        if (keep_gliding)
            params_changed();
    }
    uint32_t orig_offset = offset;
    uint32_t orig_numsamples = numsamples;
    numsamples += offset;
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // displays, too
        meters.bypassed(params, orig_numsamples);
    } else {
        // process
        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[AM::param_level_in];
            inL *= *params[AM::param_level_in];
            
            float procL = inL;
            float procR = inR;
            
            // all filters in chain
            process_hplp(procL, procR);
            
            int active = *params[AM::param_ls_active];
            if (active > 3) diff_ms(procL, procR);
            if (active == 1 or active == 2 or active == 4)
                procL = lsL.process(procL);
            if (active == 1 or active == 3 or active == 5)
                procR = lsR.process(procR);
            if (active > 3) undiff_ms(procL, procR);
            
            active = *params[AM::param_hs_active];
            if (active > 3) diff_ms(procL, procR);
            if (active == 1 or active == 2 or active == 4)
                procL = hsL.process(procL);
            if (active == 1 or active == 3 or active == 5)
                procR = hsR.process(procR);
            if (active > 3) undiff_ms(procL, procR);
            
            for (int i = 0; i < AM::PeakBands; i++)
            {
                int offset = i * params_per_band;
                int active = *params[AM::param_p1_active + offset];
                if (active > 3) diff_ms(procL, procR);
                if (active == 1 or active == 2 or active == 4)
                    procL = pL[i].process(procL);
                if (active == 1 or active == 3 or active == 5)
                    procR = pR[i].process(procR);
                if (active > 3) undiff_ms(procL, procR);
            }
            
            outL = procL * *params[AM::param_level_out];
            outR = procR * *params[AM::param_level_out];
            
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
                        
            // next sample
            ++offset;
        } // cycle trough samples
        meters.process(params, ins, outs, orig_offset, orig_numsamples);
        // clean up
        for(int i = 0; i < 3; ++i) {
            hp[i][0].sanitize();
            hp[i][1].sanitize();
            lp[i][0].sanitize();
            lp[i][1].sanitize();
        }
        lsL.sanitize();
        hsR.sanitize();
        for(int i = 0; i < AM::PeakBands; ++i) {
            pL[i].sanitize();
            pR[i].sanitize();
        }
    }
    // whatever has to be returned x)
    return outputs_mask;
}

template<class BaseClass, bool has_lphp>
bool equalizerNband_audio_module<BaseClass, has_lphp>::get_graph(int index, int subindex, float *data, int points, cairo_iface *context, int *mode) const
{
    if (!is_active)
        return false;
    if (index == AM::param_p1_freq && !subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points, 32, 0);
    }
    return false;
}

template<class BaseClass, bool has_lphp>
bool equalizerNband_audio_module<BaseClass, has_lphp>::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active) {
        return false;
    } else {
        return get_freq_gridline(subindex, pos, vertical, legend, context, true, 32, 0);
    }
}

template<class BaseClass, bool has_lphp>
int equalizerNband_audio_module<BaseClass, has_lphp>::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    if (!is_active) {
        return false;
    } else {
        bool changed = false;
        for (int i = 0; i < graph_param_count && !changed; i++)
        {
            if (*params[AM::first_graph_param + i] != old_params_for_graph[i])
                changed = true;
        }
        if (changed)
        {
            for (int i = 0; i < graph_param_count; i++)
                old_params_for_graph[i] = *params[AM::first_graph_param + i];
            
            last_generation++;
            subindex_graph = 0;
            subindex_dot = INT_MAX;
            subindex_gridline = INT_MAX;
        }
        else {
            subindex_graph = 0;
            subindex_dot = subindex_gridline = generation ? INT_MAX : 0;
        }
        if (generation == last_calculated_generation)
            subindex_graph = INT_MAX;
        return last_generation;
    }
    return false;
}

static inline float adjusted_lphp_gain(const float *const *params, int param_active, int param_mode, const biquad_d2<float> &filter, float freq, float srate)
{
    if(*params[param_active] > 0.f) {
        float gain = filter.freq_gain(freq, srate);
        switch((int)*params[param_mode]) {
            case MODE12DB:
                return gain;
            case MODE24DB:
                return gain * gain;
            case MODE36DB:
                return gain * gain * gain;
        }
    }
    return 1;
}

template<class BaseClass, bool use_hplp>
float equalizerNband_audio_module<BaseClass, use_hplp>::freq_gain(int index, double freq, uint32_t sr) const
{
    float ret = 1.f;
    if (use_hplp)
    {
        ret *= adjusted_lphp_gain(params, AM::param_hp_active, AM::param_hp_mode, hp[0][0], freq, (float)sr);
        ret *= adjusted_lphp_gain(params, AM::param_lp_active, AM::param_lp_mode, lp[0][0], freq, (float)sr);
    }
    ret *= (*params[AM::param_ls_active] > 0.f) ? lsL.freq_gain(freq, sr) : 1;
    ret *= (*params[AM::param_hs_active] > 0.f) ? hsL.freq_gain(freq, sr) : 1;
    for (int i = 0; i < PeakBands; i++)
        ret *= (*params[AM::param_p1_active + i * params_per_band] > 0.f) ? pL[i].freq_gain(freq, (float)sr) : 1;
    return ret;
}

template class equalizerNband_audio_module<equalizer5band_metadata, false>;
template class equalizerNband_audio_module<equalizer8band_metadata, true>;
template class equalizerNband_audio_module<equalizer12band_metadata, true>;


///////////////////////////////////////////////////////////////////////////////////////////////

phonoeq_audio_module::phonoeq_audio_module()
{
    is_active = false;
    srate = 0;
    last_generation = 0;
    for (int i = 0; i < 1; i++)
    {
        p_freq_old[i] = 0;
        p_level_old[i] = 0;
        p_q_old[i] = 0;
    }
}

void phonoeq_audio_module::activate()
{
    is_active = true;
    // set all filters
    params_changed();
    meters.reset();
}

void phonoeq_audio_module::deactivate()
{
    is_active = false;
}

void phonoeq_audio_module::params_changed()
{
    int mode = *params[param_mode];
    int type = *params[param_type];
    riaacurvL.set(srate, mode, type);        
    riaacurvR.set(srate, mode, type);        
}

uint32_t phonoeq_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[AM::param_bypass] > 0.f;
    if (!bypass)
    {
        // ensure that if params have changed, the params_changed method is
        // called every 8 samples to interpolate filter parameters
        while(numsamples > 8)
        {
            params_changed();
            outputs_mask |= process(offset, 8, inputs_mask, outputs_mask);
            offset += 8;
            numsamples -= 8;
        }
    }
    uint32_t orig_offset = offset;
    uint32_t orig_numsamples = numsamples;
    numsamples += offset;
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // displays, too
        meters.bypassed(params, orig_numsamples);
    } else {
        // process
        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[AM::param_level_in];
            inL *= *params[AM::param_level_in];
            
            float procL = inL;
            float procR = inR;
            
            procL = riaacurvL.process(procL);
            procR = riaacurvR.process(procR);

            outL = procL * *params[AM::param_level_out];
            outR = procR * *params[AM::param_level_out];
            
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
                        
            // next sample
            ++offset;
        } // cycle trough samples
        meters.process(params, ins, outs, orig_offset, orig_numsamples);
        // clean up
        riaacurvL.sanitize();
        riaacurvR.sanitize();
    }
    // whatever has to be returned x)
    return outputs_mask;
}

bool phonoeq_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context, int *mode) const
{
    if (!is_active)
        return false;
    if (!subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points, 32, 0);
    }
    return false;
}

bool phonoeq_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active) {
        return false;
    } else {
        return get_freq_gridline(subindex, pos, vertical, legend, context, true, 32, 0);
    }
}

int phonoeq_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    if (!is_active) {
        return false;
    } else {
        bool changed = true;
        if (changed)
        {
            last_generation++;
            subindex_graph = 0;
            subindex_dot = INT_MAX;
            subindex_gridline = INT_MAX;
        }
        else {
            subindex_graph = 0;
            subindex_dot = subindex_gridline = generation ? INT_MAX : 0;
        }
        if (generation == last_calculated_generation)
            subindex_graph = INT_MAX;
        return last_generation;
    }
    return false;
}

float phonoeq_audio_module::freq_gain(int index, double freq, uint32_t sr) const
{
    float ret;
    ret = riaacurvL.r1.freq_gain(freq, sr);
    return ret;
}



///////////////////////////////////////////////////////////////////////////////////////////////

bool filter_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context, int *mode) const
{
    if (!is_active)
        return false;
    if (index == par_cutoff && !subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}

int filter_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    if (fabs(inertia_cutoff.get_last() - old_cutoff) + 100 * fabs(inertia_resonance.get_last() - old_resonance) + fabs(*params[par_mode] - old_mode) > 0.1f)
    {
        old_cutoff = inertia_cutoff.get_last();
        old_resonance = inertia_resonance.get_last();
        old_mode = *params[par_mode];
        last_generation++;
        subindex_graph = 0;
        subindex_dot = INT_MAX;
        subindex_gridline = INT_MAX;
    }
    else {
        subindex_graph = 0;
        subindex_dot = subindex_gridline = generation ? INT_MAX : 0;
    }
    if (generation == last_calculated_generation)
        subindex_graph = INT_MAX;
    return last_generation;
}


///////////////////////////////////////////////////////////////////////////////////////////////

filterclavier_audio_module::filterclavier_audio_module() 
: filter_module_with_inertia<biquad_filter_module, filterclavier_metadata>(ins, outs, params)
, min_gain(1.0)
, max_gain(32.0)
, last_note(-1)
, last_velocity(-1)
{
}
    
void filterclavier_audio_module::params_changed()
{ 
    inertia_filter_module::inertia_cutoff.set_inertia(
        note_to_hz(last_note + *params[par_transpose], *params[par_detune]));
    
    float min_resonance = param_props[par_max_resonance].min;
     inertia_filter_module::inertia_resonance.set_inertia( 
             (float(last_velocity) / 127.0)
             // 0.001: see below
             * (*params[par_max_resonance] - min_resonance + 0.001)
             + min_resonance);
         
    adjust_gain_according_to_filter_mode(last_velocity);
    
    inertia_filter_module::calculate_filter(); 
}

void filterclavier_audio_module::activate()
{
    inertia_filter_module::activate();
}

void filterclavier_audio_module::set_sample_rate(uint32_t sr)
{
    inertia_filter_module::set_sample_rate(sr);
}

void filterclavier_audio_module::deactivate()
{
    inertia_filter_module::deactivate();
}


void filterclavier_audio_module::note_on(int channel, int note, int vel)
{
    last_note     = note;
    last_velocity = vel;
    inertia_filter_module::inertia_cutoff.set_inertia(
            note_to_hz(note + *params[par_transpose], *params[par_detune]));

    float min_resonance = param_props[par_max_resonance].min;
    inertia_filter_module::inertia_resonance.set_inertia( 
            (float(vel) / 127.0) 
            // 0.001: if the difference is equal to zero (which happens
            // when the max_resonance knom is at minimum position
            // then the filter gain doesnt seem to snap to zero on most note offs
            * (*params[par_max_resonance] - min_resonance + 0.001) 
            + min_resonance);
    
    adjust_gain_according_to_filter_mode(vel);
    
    inertia_filter_module::calculate_filter();
}

void filterclavier_audio_module::note_off(int channel, int note, int vel)
{
    if (note == last_note) {
        inertia_filter_module::inertia_resonance.set_inertia(param_props[par_max_resonance].min);
        inertia_filter_module::inertia_gain.set_inertia(min_gain);
        inertia_filter_module::calculate_filter();
        last_velocity = 0;
    }
}

void filterclavier_audio_module::adjust_gain_according_to_filter_mode(int velocity)
{
    int   mode = dsp::fastf2i_drm(*params[par_mode]);
    
    // for bandpasses: boost gain for velocities > 0
    if ( (mode_6db_bp <= mode) && (mode <= mode_18db_bp) ) {
        // gain for velocity 0:   1.0
        // gain for velocity 127: 32.0
        float mode_max_gain = max_gain;
        // max_gain is right for mode_6db_bp
        if (mode == mode_12db_bp)
            mode_max_gain /= 6.0;
        if (mode == mode_18db_bp)
            mode_max_gain /= 10.5;
        
        inertia_filter_module::inertia_gain.set_now(
                (float(velocity) / 127.0) * (mode_max_gain - min_gain) + min_gain);
    } else {
        inertia_filter_module::inertia_gain.set_now(min_gain);
    }
}

bool filterclavier_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context, int *mode) const
{
    if (!is_active || index != par_mode) {
        return false;
    }
    if (!subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}
