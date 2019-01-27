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
#include <calf/utils.h>
#include <calf/giface.h>
#include <calf/modules_filter.h>

using namespace dsp;
using namespace calf_plugins;

#define SET_IF_CONNECTED(name) if (params[AM::param_##name] != NULL) *params[AM::param_##name] = name;

/**********************************************************************
 * EQUALIZER N BAND by Markus Schmidt and Krzysztof Foltman
**********************************************************************/

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
    hp_freq_old = lp_freq_old = hp_q_old = 0;
    hs_freq_old = ls_freq_old = lp_q_old = 0;
    hs_level_old = ls_level_old = 0;
    hs_q_old = ls_q_old = 0;
    keep_gliding = 0;
    last_peak = 0;
    indiv_old = -1;
    analyzer_old = false;
    for (int i = 0; i < AM::PeakBands; i++)
    {
        p_freq_old[i] = 0;
        p_level_old[i] = 0;
        p_q_old[i] = 0;
    }
    for (int i = 0; i < graph_param_count; i++)
        old_params_for_graph[i] = -1;
    redraw_graph = true;
}

template<class BaseClass, bool has_lphp>
void equalizerNband_audio_module<BaseClass, has_lphp>::activate()
{
    is_active = true;
    // set all filters
    params_changed();
}

template<class BaseClass, bool has_lphp>
void equalizerNband_audio_module<BaseClass, has_lphp>::deactivate()
{
    is_active = false;
}

static inline void copy_lphp(biquad_d2 filters[3][2])
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
        float hpq = *params[AM::param_hp_q], lpq = *params[AM::param_lp_q];
        
        if(hpfreq != hp_freq_old || hpq != hp_q_old) {
            hpfreq = glide(hp_freq_old, hpfreq, keep_gliding);
            hp[0][0].set_hp_rbj(hpfreq, hpq, (float)srate, 1.0);
            copy_lphp(hp);
            hp_freq_old = hpfreq;
        }
        if(lpfreq != lp_freq_old || lpq != lp_q_old) {
            lpfreq = glide(lp_freq_old, lpfreq, keep_gliding);
            lp[0][0].set_lp_rbj(lpfreq, lpq, (float)srate, 1.0);
            copy_lphp(lp);
            lp_freq_old = lpfreq;
        }
    }
    
    // then shelves
    float hsfreq = *params[AM::param_hs_freq], hslevel = *params[AM::param_hs_level], hsq =*params[AM::param_hs_q];
    float lsfreq = *params[AM::param_ls_freq], lslevel = *params[AM::param_ls_level], lsq =*params[AM::param_ls_q];
    
    if(lsfreq != ls_freq_old || lslevel != ls_level_old || lsq != ls_q_old) {
        lsfreq = glide(ls_freq_old, lsfreq, keep_gliding);
        lsL.set_lowshelf_rbj(lsfreq, lsq, lslevel, (float)srate);
        lsR.copy_coeffs(lsL);
        ls_level_old = lslevel;
        ls_freq_old = lsfreq;
        ls_q_old = lsq;
    }
    if(hsfreq != hs_freq_old || hslevel != hs_level_old || hsq != hs_q_old) {
        hsfreq = glide(hs_freq_old, hsfreq, keep_gliding);
        hsL.set_highshelf_rbj(hsfreq, hsq, hslevel, (float)srate);
        hsR.copy_coeffs(hsL);
        hs_level_old = hslevel;
        hs_freq_old = hsfreq;
        hs_q_old = hsq;
    }
    for (int i = 0; i < AM::PeakBands; i++)
    {
        int offset = i * params_per_band;
        float freq = *params[AM::param_p1_freq + offset];
        float level = *params[AM::param_p1_level + offset];
        float q = *params[AM::param_p1_q + offset];
        if(freq != p_freq_old[i] || level != p_level_old[i] || q != p_q_old[i]) {
            freq = glide(p_freq_old[i], freq, keep_gliding);
            pL[i].set_peakeq_rbj(freq, q, level, (float)srate);
            pR[i].copy_coeffs(pL[i]);
            p_freq_old[i] = freq;
            p_level_old[i] = level;
            p_q_old[i] = q;
        }
    }
    if (*params[AM::param_individuals] != indiv_old) {
        indiv_old = *params[AM::param_individuals];
        redraw_graph = true;
    }
    
    // check if any important parameter for redrawing the graph changed
    for (int i = 0; i < graph_param_count; i++) {
        if (*params[AM::first_graph_param + i] != old_params_for_graph[i])
            redraw_graph = true;
        old_params_for_graph[i] = *params[AM::first_graph_param + i];
    }
    
    _analyzer.set_params(
        256, 1, 6, 0, 1,
        *params[AM::param_analyzer_mode] + (*params[AM::param_analyzer_mode] >= 3 ? 5 : 1),
        0, 0, 15, 2, 0, 0
    );
    
    if ((bool)*params[AM::param_analyzer_active] != analyzer_old) {
        redraw_graph = true;
        analyzer_old = (bool)*params[AM::param_analyzer_active];
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
                if (active == 1 || active == 2 || active == 4)
                    left = lp[0][0].process(left);
                if (active == 1 || active == 3 || active == 5)
                    right = lp[0][1].process(right);
                break;
            case MODE24DB:
                if (active == 1 || active == 2 || active == 4)
                    left = lp[1][0].process(lp[0][0].process(left));
                if (active == 1 || active == 3 || active == 5)
                    right = lp[1][1].process(lp[0][1].process(right));
                break;
            case MODE36DB:
                if (active == 1 || active == 2 || active == 4)
                    left = lp[2][0].process(lp[1][0].process(lp[0][0].process(left)));
                if (active == 1 || active == 3 || active == 5)
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
                if (active == 1 || active == 2 || active == 4)
                    left = hp[0][0].process(left);
                if (active == 1 || active == 3 || active == 5)
                    right = hp[0][1].process(right);
                break;
            case MODE24DB:
                if (active == 1 || active == 2 || active == 4)
                    left = hp[1][0].process(hp[0][0].process(left));
                if (active == 1 || active == 3 || active == 5)
                    right = hp[1][1].process(hp[0][1].process(right));
                break;
            case MODE36DB:
                if (active == 1 || active == 2 || active == 4)
                    left = hp[2][0].process(hp[1][0].process(hp[0][0].process(left)));
                if (active == 1 || active == 3 || active == 5)
                    right = hp[2][1].process(hp[1][1].process(hp[0][1].process(right)));
                break;
        }
        if (active > 3) undiff_ms(left, right);
    }
}

template<class BaseClass, bool has_lphp>
uint32_t equalizerNband_audio_module<BaseClass, has_lphp>::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[AM::param_bypass] > 0.5f, numsamples);
    if (keep_gliding)
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
    numsamples += offset;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 0, 0, 0};
            meters.process(values);
            _analyzer.process(0, 0);
            ++offset;
        }
    } else {
        // process
        uint32_t orig_numsamples = numsamples-offset;
        uint32_t orig_offset = offset;
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
            if (active == 1 || active == 2 || active == 4)
                procL = lsL.process(procL);
            if (active == 1 || active == 3 || active == 5)
                procR = lsR.process(procR);
            if (active > 3) undiff_ms(procL, procR);
            
            active = *params[AM::param_hs_active];
            if (active > 3) diff_ms(procL, procR);
            if (active == 1 || active == 2 || active == 4)
                procL = hsL.process(procL);
            if (active == 1 || active == 3 || active == 5)
                procR = hsR.process(procR);
            if (active > 3) undiff_ms(procL, procR);
            
            for (int i = 0; i < AM::PeakBands; i++)
            {
                int offset = i * params_per_band;
                int active = *params[AM::param_p1_active + offset];
                if (active > 3) diff_ms(procL, procR);
                if (active == 1 || active == 2 || active == 4)
                    procL = pL[i].process(procL);
                if (active == 1 || active == 3 || active == 5)
                    procR = pR[i].process(procR);
                if (active > 3) undiff_ms(procL, procR);
            }
            
            outL = procL * *params[AM::param_level_out];
            outR = procR * *params[AM::param_level_out];
            
            // analyzer
            _analyzer.process((inL + inR) / 2.f, (outL + outR) / 2.f);
        
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            float values[] = {inL, inR, outL, outR};
            meters.process(values);

            // next sample
            ++offset;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
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
    meters.fall(numsamples);
    return outputs_mask;
}

static inline float adjusted_lphp_gain(const float *const *params, int param_active, int param_mode, const biquad_d2 &filter, float freq, float srate)
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

template<class BaseClass, bool has_lphp>
bool equalizerNband_audio_module<BaseClass, has_lphp>::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (phase && *params[AM::param_analyzer_active]) {
        bool r = _analyzer.get_graph(subindex, phase, data, points, context, mode);
        if (*params[AM::param_analyzer_mode] == 2) {
            set_channel_color(context, subindex ? 0 : 1, 0.15);
        } else {
            context->set_source_rgba(0,0,0,0.1);
        }
        return r;
    } else if (phase && !*params[AM::param_analyzer_active]) {
        last_peak = 0;
        redraw_graph = false;
        return false;
    } else {
        int max = PeakBands + 2 + (has_lphp ? 2 : 0);
        
        if (!is_active
        || (subindex && !*params[AM::param_individuals])
        || (subindex > max && *params[AM::param_individuals])) {
            last_peak = 0;
            redraw_graph = false;
            return false;
        }
        
        // first graph is the overall frequency response graph
        if (!subindex)
            return ::get_graph(*this, subindex, data, points, 128 * *params[AM::param_zoom], 0);
        
        // get out if max band is reached
        if (last_peak >= max) {
            last_peak = 0;
            redraw_graph = false;
            return false;
        }
        
        // get the next filter to draw a curve for and leave out inactive
        // filters
        while (last_peak < PeakBands && !*params[AM::param_p1_active + last_peak * params_per_band])
            last_peak ++;
        if (last_peak == PeakBands && !*params[AM::param_ls_active])
            last_peak ++;
        if (last_peak == PeakBands + 1 && !*params[AM::param_hs_active])
            last_peak ++;
        if (has_lphp && last_peak == PeakBands + 2 && !*params[AM::param_hp_active])
            last_peak ++;
        if (has_lphp && last_peak == PeakBands + 3 && !*params[AM::param_lp_active])
            last_peak ++;
        
        // get out if max band is reached
        if (last_peak >= max) { // and !*params[param_analyzer_active]) {
            last_peak = 0;
            redraw_graph = false;
            return false;
        }
         //else if *params[param_analyzer_active]) {
            //bool goon = _analyzer.get_graph(subindex, phase, data, points, context, mode);
            //if (!goon)
                //last_peak = 0;
            //return goon;
        //}
            
        // draw the individual curve of the actual filter
        for (int i = 0; i < points; i++) {
            double freq = 20.0 * pow (20000.0 / 20.0, i * 1.0 / points);
            if (last_peak < PeakBands) {
                data[i] = pL[last_peak].freq_gain(freq, (float)srate);
            } else if (last_peak == PeakBands) {
                data[i] = lsL.freq_gain(freq, (float)srate);
            } else if (last_peak == PeakBands + 1) {
                data[i] = hsL.freq_gain(freq, (float)srate);
            } else if (last_peak == PeakBands + 2 && has_lphp) {
                data[i] = adjusted_lphp_gain(params, AM::param_hp_active, AM::param_hp_mode, hp[0][0], freq, (float)srate);
            } else if (last_peak == PeakBands + 3 && has_lphp) {
                data[i] = adjusted_lphp_gain(params, AM::param_lp_active, AM::param_lp_mode, lp[0][0], freq, (float)srate);
            }
            data[i] = dB_grid(data[i], 128 * *params[AM::param_zoom], 0);
        }
        
        last_peak ++;
        *mode = 4;
        context->set_source_rgba(0,0,0,0.075);
        return true;
    }
}
template<class BaseClass, bool has_lphp>
bool equalizerNband_audio_module<BaseClass, has_lphp>::get_layers(int index, int generation, unsigned int &layers) const
{
    redraw_graph = redraw_graph || !generation;
    layers = *params[AM::param_analyzer_active] ? LG_REALTIME_GRAPH : 0;
    layers |= (generation ? LG_NONE : LG_CACHE_GRID) | (redraw_graph ? LG_CACHE_GRAPH : LG_NONE);
    redraw_graph |= (bool)*params[AM::param_analyzer_active];
    return redraw_graph || !generation;
}

template<class BaseClass, bool has_lphp>
bool equalizerNband_audio_module<BaseClass, has_lphp>::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active || phase)
        return false;
    return get_freq_gridline(subindex, pos, vertical, legend, context, true, 128 * *params[AM::param_zoom], 0);
}

template<class BaseClass, bool has_lphp>
float equalizerNband_audio_module<BaseClass, has_lphp>::freq_gain(int index, double freq) const
{
    float ret = 1.f;
    if (has_lphp)
    {
        ret *= adjusted_lphp_gain(params, AM::param_hp_active, AM::param_hp_mode, hp[0][0], freq, (float)srate);
        ret *= adjusted_lphp_gain(params, AM::param_lp_active, AM::param_lp_mode, lp[0][0], freq, (float)srate);
    }
    ret *= (*params[AM::param_ls_active] > 0.f) ? lsL.freq_gain(freq, (float)srate) : 1;
    ret *= (*params[AM::param_hs_active] > 0.f) ? hsL.freq_gain(freq, (float)srate) : 1;
    for (int i = 0; i < PeakBands; i++)
        ret *= (*params[AM::param_p1_active + i * params_per_band] > 0.f) ? pL[i].freq_gain(freq, (float)srate) : 1;
    return ret;
}

template<class BaseClass, bool has_lphp>
inline std::string equalizerNband_audio_module<BaseClass, has_lphp>::get_crosshair_label(int x, int y, int sx, int sy, float q, int dB, int name, int note, int cents) const
{ 
    return frequency_crosshair_label(x, y, sx, sy, q, dB, name, note, cents, 128 * *params[AM::param_zoom], 0);
}

namespace calf_plugins {
template class equalizerNband_audio_module<equalizer5band_metadata, false>;
template class equalizerNband_audio_module<equalizer8band_metadata, true>;
template class equalizerNband_audio_module<equalizer12band_metadata, true>;
}

/**********************************************************************
 * EQUALIZER 30 BAND
**********************************************************************/

equalizer30band_audio_module::equalizer30band_audio_module() :
    conv(OrfanidisEq::eqGainRangeDb),
    swL(10000), swR(10000)
{
    is_active = false;
    srate     = 0;

    //Construct equalizers
    using namespace OrfanidisEq;

    fg.set30Bands();

    Eq* ptr30L = new Eq(fg, butterworth);
    Eq* ptr30R = new Eq(fg, butterworth);
    eq_arrL.push_back(ptr30L);
    eq_arrR.push_back(ptr30R);

    ptr30L = new Eq(fg, chebyshev1);
    ptr30R = new Eq(fg, chebyshev1);
    eq_arrL.push_back(ptr30L);
    eq_arrR.push_back(ptr30R);

    ptr30L = new Eq(fg, chebyshev2);
    ptr30R = new Eq(fg, chebyshev2);
    eq_arrL.push_back(ptr30L);
    eq_arrR.push_back(ptr30R);

    ptr30L = new Eq(fg, elliptic);
    ptr30R = new Eq(fg, elliptic);
    eq_arrL.push_back(ptr30L);
    eq_arrR.push_back(ptr30R);

    flt_type = butterworth;
    flt_type_old = none;

    //Set switcher
    swL.set_previous(butterworth);
    swL.set(butterworth);

    swR.set_previous(butterworth);
    swR.set(butterworth);
}

equalizer30band_audio_module::~equalizer30band_audio_module()
{
    for (unsigned int i = 0; i < eq_arrL.size(); i++)
        delete eq_arrL[i];

    for (unsigned int i = 0; i < eq_arrR.size(); i++)
        delete eq_arrR[i];
}

void equalizer30band_audio_module::activate()
{
    is_active = true;
}

void equalizer30band_audio_module::deactivate()
{
    is_active = false;
}

void equalizer30band_audio_module::params_changed()
{
    using namespace OrfanidisEq;

    int psl=0, psr=0, pgl=0, pgr=0, pql=0, pqr=0;

    switch (int(*params[param_linked])) {
        case 0:
            psl = param_gain_scale11;
            pgl = param_gain10;
            pql = param_gainscale1;
            psr = param_gain_scale21;
            pgr = param_gain20;
            pqr = param_gainscale2;
            *params[param_l_active] = 0.5;
            *params[param_r_active] = 0.5;
            break;
        case 1:
            psl = param_gain_scale11;
            pgl = param_gain10;
            pql = param_gainscale1;
            psr = param_gain_scale11;
            pgr = param_gain10;
            pqr = param_gainscale1;
            *params[param_l_active] = 1;
            *params[param_r_active] = 0;
            break;
        case 2:
            psl = param_gain_scale21;
            pgl = param_gain20;
            pql = param_gainscale2;
            psr = param_gain_scale21;
            pgr = param_gain20;
            pqr = param_gainscale2;
            *params[param_l_active] = 0;
            *params[param_r_active] = 1;
            break;
    }

    //Change gain indicators
    *params[param_gain_scale10] = *params[pgl] * *params[pql];
    *params[param_gain_scale20] = *params[pgr] * *params[pqr];

    for(unsigned int i = 0; i < fg.getNumberOfBands(); i++) {
        *params[param_gain_scale11 + band_params*i] = (*params[param_gain11 + band_params*i])*
                *params[param_gainscale1];
        *params[param_gain_scale21 + band_params*i] = (*params[param_gain21 + band_params*i])*
                *params[param_gainscale2];
    }

    //Pass gains to eq's
    for (unsigned int i = 0; i < fg.getNumberOfBands(); i++) {
        eq_arrL[*params[param_filters]]->changeBandGainDb(i,*params[psl + band_params*i]);
        eq_arrR[*params[param_filters]]->changeBandGainDb(i,*params[psr + band_params*i]);
    }

    //Upadte filter type
    flt_type = (filter_type)int((*params[param_filters] + 1));
}

void equalizer30band_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;

    //Change sample rate for eq's
    for(unsigned int i = 0; i < eq_arrL.size(); i++)
    {
        eq_arrL[i]->setSampleRate(srate);
        eq_arrL[i]->setSampleRate(srate);
    }

    int meter[] = {param_level_in_vuL, param_level_in_vuR, param_level_out_vuL, param_level_out_vuR};
    int clip[] = {param_level_in_clipL, param_level_in_clipR, param_level_out_clipL, param_level_out_clipR};
    meters.init(params, meter, clip, 4, sr);
}

uint32_t equalizer30band_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t orig_numsamples = numsamples;
    uint32_t orig_offset = offset;
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    numsamples += offset;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 0, 0, 0};
            meters.process(values);
            ++offset;
        }
    } else {
        // process
        while(offset < numsamples) {

            double inL = ins[0][offset] * *params[param_level_in];
            double inR = ins[1][offset] * *params[param_level_in];

            double outL = inL;
            double outR = inR;

            unsigned int eq_index = swL.get_state() - 1;
            eq_arrL[eq_index]->SBSProcess(&inL, &outL);
            eq_arrR[eq_index]->SBSProcess(&inR, &outR);

            //If filter type switched
            if(flt_type_old != flt_type)
            {
                swL.set(flt_type);
                swR.set(flt_type);
                flt_type_old = flt_type;
            }

            outL *= swL.get_ramp();
            outR *= swR.get_ramp();

            outL = outL* conv.fastDb2Lin(*params[param_gain_scale10]);
            outR = outR* conv.fastDb2Lin(*params[param_gain_scale20]);

            outL = outL* *params[param_level_out];
            outR = outR* *params[param_level_out];

            outs[0][offset] = outL;
            outs[1][offset] = outR;

            // meters
            float values[] = {(float)inL, (float)inR, (float)outL, (float)outR};
            meters.process(values);

            // next sample
            ++offset;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
    }

    meters.fall(orig_numsamples);
    return outputs_mask;
}

/**********************************************************************
 * FILTERKLAVIER by Hans Baier 
**********************************************************************/

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
    redraw_graph = true;
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
    redraw_graph = true;
}

void filterclavier_audio_module::note_off(int channel, int note, int vel)
{
    if (note == last_note) {
        inertia_filter_module::inertia_resonance.set_inertia(param_props[par_max_resonance].min);
        inertia_filter_module::inertia_gain.set_inertia(min_gain);
        inertia_filter_module::calculate_filter();
        last_velocity = 0;
        redraw_graph = true;
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

/**********************************************************************
 * EMPHASIS by Damien Zammit 
**********************************************************************/

emphasis_audio_module::emphasis_audio_module()
{
    is_active = false;
    srate = 0;
    redraw_graph = true;
    mode = -1;
    type = -1;
}

void emphasis_audio_module::activate()
{
    is_active = true;
    // set all filters
    params_changed();
}

void emphasis_audio_module::deactivate()
{
    is_active = false;
}

void emphasis_audio_module::params_changed()
{
    if (mode != *params[param_mode] || type != *params[param_type] || bypass_ != *params[param_bypass])
        redraw_graph = true;
    mode    = *params[param_mode];
    type    = *params[param_type];
    bypass_ = *params[param_bypass];
    riaacurvL.set(srate, mode, type);
    riaacurvR.set(srate, mode, type);
}

uint32_t emphasis_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t orig_numsamples = numsamples;
    uint32_t orig_offset = offset;
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    if (!bypassed)
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
    numsamples += offset;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 0, 0, 0};
            meters.process(values);
            ++offset;
        }
    } else {
        // process
        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            
            float procL = inL;
            float procR = inR;
            
            procL = riaacurvL.process(procL);
            procR = riaacurvR.process(procR);

            outL = procL * *params[param_level_out];
            outR = procR * *params[param_level_out];
            
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            float values[] = {inL, inR, outL, outR};
            meters.process(values);
            
            // next sample
            ++offset;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
        // clean up
        riaacurvL.sanitize();
        riaacurvR.sanitize();
    }
    meters.fall(orig_numsamples);
    return outputs_mask;
}
bool emphasis_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (phase || subindex)
        return false;
    if (bypass_)
        context->set_source_rgba(0.15, 0.2, 0.0, 0.3);
    return ::get_graph(*this, subindex, data, points, 32, 0);
}
bool emphasis_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (phase)
        return false;
    return get_freq_gridline(subindex, pos, vertical, legend, context, true, 32, 0);
}

/**********************************************************************
 * CROSSOVER N BAND by Markus Schmidt
**********************************************************************/

template<class XoverBaseClass>
xover_audio_module<XoverBaseClass>::xover_audio_module()
{
    is_active = false;
    srate = 0;
    redraw_graph = true;
    buffer = NULL;
    crossover.init(AM::channels, AM::bands, 44100);
}
template<class XoverBaseClass>
xover_audio_module<XoverBaseClass>::~xover_audio_module()
{
    free(buffer);
}
template<class XoverBaseClass>
void xover_audio_module<XoverBaseClass>::activate()
{
    is_active = true;
    params_changed();
}

template<class XoverBaseClass>
void xover_audio_module<XoverBaseClass>::deactivate()
{
    is_active = false;
}
template<class XoverBaseClass>
void xover_audio_module<XoverBaseClass>::set_sample_rate(uint32_t sr)
{
    srate = sr;
    // set srate of crossover
    crossover.set_sample_rate(srate);
    // rebuild buffer
    buffer_size = (int)(srate / 10 * AM::channels * AM::bands + AM::channels * AM::bands); // buffer size attack rate multiplied by channels and bands
    buffer = (float*) calloc(buffer_size, sizeof(float));
    pos = 0;
    int amount = AM::bands * AM::channels + AM::channels;
    STACKALLOC(int, meter,amount);
    STACKALLOC(int, clip, amount);
    for(int b = 0; b < AM::bands; b++) {
        for (int c = 0; c < AM::channels; c++) {
            meter[b * AM::channels + c] = AM::param_meter_01 + b * params_per_band + c;
            clip[b * AM::channels + c] = -1;
        }
    }
    for (int c = 0; c < AM::channels; c++) {
        meter[c + AM::bands * AM::channels] = AM::param_meter_0 + c;
        clip[c + AM::bands * AM::channels] = -1;
    }
    meters.init(params, meter, clip, amount, srate);
}
template<class XoverBaseClass>
void xover_audio_module<XoverBaseClass>::params_changed()
{
    int mode = *params[AM::param_mode];
    crossover.set_mode(mode);
    for (int i = 0; i < AM::bands - 1; i++) {
        crossover.set_filter(i,  *params[AM::param_freq0 + i]);
    }
    for (int i = 0; i < AM::bands; i++) {
        int offset = i * params_per_band;
        crossover.set_level(i, *params[AM::param_level1 + offset]);
        crossover.set_active(i, *params[AM::param_active1 + offset] > 0.5);
    }
    redraw_graph = true;
}

template<class XoverBaseClass>
uint32_t xover_audio_module<XoverBaseClass>::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    unsigned int targ = numsamples + offset;
    float xval;
    float values[AM::bands * AM::channels + AM::channels];
    while(offset < targ) {
        // cycle through samples
        
        // level
        for (int c = 0; c < AM::channels; c++) {
            in[c] = ins[c][offset] * *params[AM::param_level];
        }
        crossover.process(in);
        
        for (int b = 0; b < AM::bands; b++) {
            int nbuf = 0;
            int off = b * params_per_band;
            // calc position in delay buffer
            if (*params[AM::param_delay1 + off]) {
                nbuf = srate * (fabs(*params[AM::param_delay1 + off]) / 1000.f) * AM::bands * AM::channels;
                nbuf -= nbuf % (AM::bands * AM::channels);
            }
            for (int c = 0; c < AM::channels; c++) {
                // define a pointer between 0 and channels * bands
                int ptr = b * AM::channels + c;
                
                // get output from crossover module if active
                xval = *params[AM::param_active1 + off] > 0.5 ? crossover.get_value(c, b) : 0.f;
                
                // fill delay buffer
                buffer[pos + ptr] = xval;
                
                // get value from delay buffer if neccessary
                if (*params[AM::param_delay1 + off])
                    xval = buffer[(pos - (int)nbuf + ptr + buffer_size) % buffer_size];
                
                // set value with phase to output
                outs[ptr][offset] = *params[AM::param_phase1 + off] > 0.5 ? xval * -1 : xval;
                
                // band meters
                values[b * AM::channels + c] = outs[ptr][offset];
            }
        }
        // in meters
        for (int c = 0; c < AM::channels; c++) {
            values[c + AM::bands * AM::channels] = ins[c][offset];
        }
        meters.process(values);
        // next sample
        ++offset;
        // delay buffer pos forward
        pos = (pos + AM::channels * AM::bands) % buffer_size;
        
    } // cycle trough samples
    meters.fall(numsamples);
    return outputs_mask;
}

template<class XoverBaseClass>
bool xover_audio_module<XoverBaseClass>::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    return crossover.get_graph(subindex, phase, data, points, context, mode);
}
template<class XoverBaseClass>
bool xover_audio_module<XoverBaseClass>::get_layers(int index, int generation, unsigned int &layers) const
{
    return crossover.get_layers(index, generation, layers);
}

namespace calf_plugins {
template class xover_audio_module<xover2_metadata>;
template class xover_audio_module<xover3_metadata>;
template class xover_audio_module<xover4_metadata>;
}


/**********************************************************************
 * Vocoder by Markus Schmidt and Christian Holschuh
**********************************************************************/

vocoder_audio_module::vocoder_audio_module()
{
    is_active = false;
    srate     = 0;
    attack    = 0;
    release   = 0;
    fcoeff    = 0;
    bands     = 0;
    bands_old = -1;
    order     = 0;
    order_old = -1;
    lower_old = upper_old = tilt_old = 0;
    fcoeff    = log10(20.f);
    log2_     = log(2);
    memset(env_mods, 0, 32 * 2 * sizeof(double));
}

void vocoder_audio_module::activate()
{
    is_active = true;
}

void vocoder_audio_module::deactivate()
{
    is_active = false;
}

void vocoder_audio_module::params_changed()
{
    attack  = exp(log(0.01)/( *params[param_attack]  * srate * 0.001));
    release = exp(log(0.01)/( *params[param_release] * srate * 0.001));
    
    int b = *params[param_bands];
    bands = (b + 2) * 4 + (b > 1 ? (b - 2) * 4 : 0);
    order = std::min(8.f, *params[param_order]);
    bool draw = false;
    for (int i = 0; i < 32; i++) {
        if (q_old[i] != *params[param_q0 + i * band_params]) {
            draw = true;
            q_old[i] = *params[param_q0 + i * band_params];
        }
    }
    if (draw || bands != bands_old || *params[param_order] != order_old
    || *params[param_hiq] != hiq_old || *params[param_lower] != lower_old
    || *params[param_upper] != upper_old || *params[param_tilt] != tilt_old) {
        float q = pow(10, (fmodf(std::min(8.999f, *params[param_order]), 1.f) * (7.f / pow(1.3, order))) / 20);
        q += *params[param_hiq];
        bands_old = bands;
        order_old = *params[param_order];
        hiq_old = *params[param_hiq];
        lower_old = *params[param_lower];
        upper_old = *params[param_upper];
        tilt_old = *params[param_tilt];
        float to = *params[param_tilt] < 0 ? *params[param_lower] : *params[param_upper];
        float from = *params[param_tilt] < 0 ? *params[param_upper] : *params[param_lower];
        float tilt = fabs(*params[param_tilt]); 
        float freq = from;
        for (int i = 0; i < bands; i++) {
            int _i = *params[param_tilt] < 0 ? bands - i - 1 : i;
            float _freq = log10(freq);
            float _q = q * *params[param_q0 + _i * band_params];
            //detector[0][0][i].set_bp_rbj(pow(10, fcoeff + (0.5f + (float)i) * 3.f / (float)bands), _q, (double)srate);
            float step = (log10(to) - _freq) / (bands - i) * (1 + tilt);
            float f = pow(10, _freq + (0.5 * step));
            bandfreq[_i] = f;
            detector[0][0][_i].set_bp_rbj(f, _q, (double)srate);
            for (int j = 0; j < order; j++) {
                if (j)
                    detector[0][j][_i].copy_coeffs(detector[0][0][_i]);
                detector[1][j][_i].copy_coeffs(detector[0][0][_i]);
                modulator[0][j][_i].copy_coeffs(detector[0][0][_i]);
                modulator[1][j][_i].copy_coeffs(detector[0][0][_i]);
            }
            freq = pow(10, _freq + step);
        }
        redraw_graph = true;
    }
    _analyzer.set_params(256, 1, 6, 0, 1, 0, 0, 0, 15, 2, 0, 0);
    redraw_graph = true;
}

int vocoder_audio_module::get_solo() const {
    for (int i = 0; i < bands; i++)
        if (*params[param_solo0 + i * band_params])
            return 1;
    return 0;
}

uint32_t vocoder_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t orig_numsamples = numsamples;
    uint32_t orig_offset = offset;
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    int solo = get_solo();
    numsamples += offset;
    float led[32] = {0};
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 0, 0, 0, 0, 0};
            meters.process(values);
            ++offset;
        }
    } else {
        // process
        while(offset < numsamples) {
            // cycle through samples
            double outL = 0;
            double outR = 0;
            double pL   = 0;
            double pR   = 0;
            
            // carrier with level
            double cL = ins[0][offset] * *params[param_carrier_in];
            double cR = ins[1][offset] * *params[param_carrier_in];
            
            // modulator with level
            double mL = ins[2][offset] * *params[param_mod_in];
            double mR = ins[3][offset] * *params[param_mod_in];
            
            // noise generator
            double nL = (float)rand() / (float)RAND_MAX;
            double nR = (float)rand() / (float)RAND_MAX;
            
            for (int i = 0; i < bands; i++) {
                double mL_ = mL;
                double mR_ = mR;
                double cL_ = cL + nL * *params[param_noise0 + i * band_params];
                double cR_ = cR + nR * *params[param_noise0 + i * band_params];
                
                if ((solo && *params[param_solo0 + i * band_params]) || !solo) {
                    for (int j = 0; j < order; j++) {
                        // filter modulator
                        if (*params[param_link] > 0.5) {
                            mL_ = detector[0][j][i].process(std::max(mL_, mR_));
                            mR_ = mL_;
                        } else {
                            mL_ = detector[0][j][i].process(mL_);
                            mR_ = detector[1][j][i].process(mR_);
                        }
                        // filter carrier with noise
                        cL_ = modulator[0][j][i].process(cL_);
                        cR_ = modulator[1][j][i].process(cR_);
                    }
                    // level by envelope with levelling
                    cL_ *= env_mods[0][i] * ((float)order / 2 + 4) * 4;
                    cR_ *= env_mods[1][i] * ((float)order / 2 + 4) * 4;
                    
                    // add band volume setting
                    cL_ *= *params[param_volume0 + i * band_params];
                    cR_ *= *params[param_volume0 + i * band_params];
                    
                    // add filtered modulator
                    cL_ += mL_ * *params[param_mod0 + i * band_params];
                    cR_ += mR_ * *params[param_mod0 + i * band_params];
                    
                    // Balance
                    cL_ *= (*params[param_pan0 + i * band_params] > 0
                         ? -*params[param_pan0 + i * band_params] + 1 : 1);
                    cR_ *= (*params[param_pan0 + i * band_params] < 0
                          ? *params[param_pan0 + i * band_params] + 1 : 1);
                    
                    // add to outputs with proc level
                    pL += cL_ * *params[param_proc];
                    pR += cR_ * *params[param_proc];
                }
                // LED
                if (*params[param_detectors] > 0.5)
                    if (env_mods[0][i] + env_mods[1][i] > led[i])
                        led[i] = env_mods[0][i] + env_mods[1][i];
                    
                // advance envelopes
                env_mods[0][i] = _sanitize((fabs(mL_) > env_mods[0][i] ? attack : release) * (env_mods[0][i] - fabs(mL_)) + fabs(mL_));
                env_mods[1][i] = _sanitize((fabs(mR_) > env_mods[1][i] ? attack : release) * (env_mods[1][i] - fabs(mR_)) + fabs(mR_));
            }
            
            outL = pL;
            outR = pR;
            
            // dry carrier
            outL += cL * *params[param_carrier];
            outR += cR * *params[param_carrier];
            
            // dry modulator
            outL += mL * *params[param_mod];
            outR += mR * *params[param_mod];
            
            // analyzer
            switch ((int)*params[param_analyzer]) {
                case 0:
                default:
                    break;
                case 1:
                    _analyzer.process((float)cL, (float)cR);
                    break;
                case 2:
                    _analyzer.process((float)mL, (float)mR);
                    break;
                case 3:
                    _analyzer.process((float)pL, (float)pR);
                    break;
                case 4:
                    _analyzer.process((float)outL, (float)outR);
                    break;
            }
            
            // out level
            outL *= *params[param_out];
            outR *= *params[param_out];
            
            // send to outputs
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            // meters
            float values[] = {(float)cL, (float)cR, (float)mL, (float)mR, (float)outL, (float)outR};
            meters.process(values);
            
            // next sample
            ++offset;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
        // clean up
        for (int i = 0; i < bands; i++) {
            for (int j = 0; j < order; j++) {
                detector[0][j][i].sanitize();
                detector[1][j][i].sanitize();
                modulator[0][j][i].sanitize();
                modulator[1][j][i].sanitize();
            }
        }
    }
    
    // LED
    for (int i = 0; i < 32; i++) {
        float val = 0;
        if (*params[param_detectors] > 0.5)
            val = std::max(0.0, 1 + log((led[i] / 2) * order) / log2_ / 10);
        *params[param_level0 + i * band_params] = val;
    }
    meters.fall(orig_numsamples);
    return outputs_mask;
}
bool vocoder_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (phase && *params[param_analyzer]) {
        if (subindex) {
            return false;
        }
        bool r = _analyzer.get_graph(subindex, phase, data, points, context, mode);
        context->set_source_rgba(0,0,0,0.25);
        return r;
    } else if (phase) {
        return false;
    } else {
        // quit
        if (subindex >= bands) {
            redraw_graph = false;
            return false;
        }
        int solo = get_solo();
        if (solo && !*params[param_solo0 + subindex * band_params])
            context->set_source_rgba(0,0,0,0.15);
        context->set_line_width(0.99);
        int drawn = 0;
        for (int i = 0; i < points; i++) {
            double freq = 20.0 * pow (20000.0 / 20.0, i * 1.0 / points);
            float level = 1;
            for (int j = 0; j < order; j++)
                level *= detector[0][0][subindex].freq_gain(freq, srate);
            level *= *params[param_volume0 + subindex * band_params];
            data[i] = dB_grid(level, 256, 0.4);
            if (!drawn && freq > bandfreq[subindex]) {
                drawn = 1;
                char str[32];
                sprintf(str, "%d", subindex + 1);
                draw_cairo_label(context, str, i, context->size_y * (1 - (data[i] + 1) / 2.f), 0, 0, 0.5);
            }
        }
    }
    return true;
}
bool vocoder_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    redraw_graph = redraw_graph || !generation;
    layers = *params[param_analyzer] ? LG_REALTIME_GRAPH : 0;
    layers |= (generation ? LG_NONE : LG_CACHE_GRID) | (redraw_graph ? LG_CACHE_GRAPH : LG_NONE);
    redraw_graph |= (bool)*params[param_analyzer];
    return redraw_graph || !generation;
}
