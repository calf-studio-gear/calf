/* Calf DSP Library
 * Audio module (plugin) metadata - header file
 *
 * Copyright (C) 2007-2008 Krzysztof Foltman
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

#ifndef __CALF_METADATA_H
#define __CALF_METADATA_H

#include "giface.h"

#define MONO_VU_METER_PARAMS param_meter_in, param_meter_out, param_clip_in, param_clip_out
#define STEREO_VU_METER_PARAMS param_meter_inL, param_meter_inR, param_meter_outL, param_meter_outR, param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR
#define PERIODICAL_PARAMS param_timing, param_bpm, param_ms, param_hz, param_bpm_host

namespace calf_plugins {

struct flanger_metadata: public plugin_metadata<flanger_metadata>
{
public:
    enum { par_delay, par_depth, par_rate, par_fb, par_stereo, par_reset, par_amount, par_dryamount,
        param_on, param_level_in, param_level_out,
        STEREO_VU_METER_PARAMS, param_lfo,
        param_count };
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    PLUGIN_NAME_ID_LABEL("flanger", "flanger", "Flanger")
};

struct phaser_metadata: public plugin_metadata<phaser_metadata>
{
    enum { par_freq, par_depth, par_rate, par_fb, par_stages, par_stereo, par_reset, par_amount, par_dryamount,
        param_on, param_level_in, param_level_out,
        STEREO_VU_METER_PARAMS, param_lfo,
        param_count };
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    PLUGIN_NAME_ID_LABEL("phaser", "phaser", "Phaser")
};

struct filter_metadata: public plugin_metadata<filter_metadata>
{
    enum { par_cutoff, par_resonance, par_mode, par_inertia,
        param_bypass, param_level_in, param_level_out,
        STEREO_VU_METER_PARAMS,
        param_count };
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, rt_capable = true, require_midi = false, support_midi = false, require_instance_access = false };
    PLUGIN_NAME_ID_LABEL("filter", "filter", "Filter")
    /// do not export mode and inertia as CVs, as those are settings and not parameters
    bool is_cv(int param_no) const { return param_no != par_mode && param_no != par_inertia; }
};

/// Filterclavier - metadata
struct filterclavier_metadata: public plugin_metadata<filterclavier_metadata>
{
    enum { par_transpose, par_detune, par_max_resonance, par_mode, par_inertia,
        param_bypass, param_level_in, param_level_out,
        STEREO_VU_METER_PARAMS,
        param_count };
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, rt_capable = true, require_midi = true, support_midi = true, require_instance_access = false };
    PLUGIN_NAME_ID_LABEL("filterclavier", "filterclavier", "Filterclavier")
    /// do not export mode and inertia as CVs, as those are settings and not parameters
    bool is_cv(int param_no) const { return param_no != par_mode && param_no != par_inertia; }
};

/// Envelope Filter - metadata
struct envelopefilter_metadata: public plugin_metadata<envelopefilter_metadata>
{
    enum { in_count = 4, out_count = 2, ins_optional = 2, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS,
           param_mix, param_q, param_mode,
           param_attack, param_release,
           param_upper, param_lower, param_gain,
           param_sidechain, param_response,
           param_count };
    PLUGIN_NAME_ID_LABEL("envelopefilter", "envelopefilter", "Envelope Filter")
};

struct reverb_metadata: public plugin_metadata<reverb_metadata>
{
    enum { param_meter_inL, param_meter_inR, param_clip_outL, 
           par_decay, par_hfdamp, par_roomsize, par_diffusion, par_amount, par_dry, par_predelay, par_basscut, par_treblecut, par_on,
           param_level_in, param_level_out,
           param_meter_outL, param_meter_outR, param_clip_inL, param_clip_inR, param_clip_outR,
           param_count };
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    PLUGIN_NAME_ID_LABEL("reverb", "reverb", "Reverb")
};

struct vintage_delay_metadata: public plugin_metadata<vintage_delay_metadata>
{
    enum {  param_on, param_level_in, param_level_out,
            STEREO_VU_METER_PARAMS,
            par_divide, par_time_l, par_time_r, par_feedback,
            par_amount, par_mixmode, par_medium, par_dryamount,
            par_width, par_frag, par_pbeats, par_pfrag,
            PERIODICAL_PARAMS,
            param_count };
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, rt_capable = true, support_midi = false, require_midi = false, require_instance_access = false };
    PLUGIN_NAME_ID_LABEL("vintagedelay", "vintagedelay", "Vintage Delay")
};

struct comp_delay_metadata: public plugin_metadata<comp_delay_metadata>
{
    enum { par_distance_mm, par_distance_cm, par_distance_m, par_dry, par_wet, param_temp,
        param_bypass, param_level_in, param_level_out,
        STEREO_VU_METER_PARAMS, param_count };
    enum { in_count = 2, out_count = 2, ins_optional = 1, outs_optional = 1, rt_capable = true, support_midi = false, require_midi = false, require_instance_access = false };
    PLUGIN_NAME_ID_LABEL("compdelay", "compdelay", "Compensation Delay Line")
};

struct haas_enhancer_metadata: public plugin_metadata<haas_enhancer_metadata>
{
    enum {
        param_bypass, param_level_in, param_level_out,
        par_s_gain,
        STEREO_VU_METER_PARAMS,
        param_meter_sideL, param_meter_sideR,
        par_m_source, par_m_phase,
        par_s_delay0, par_s_balance0, par_s_gain0, par_s_phase0,
        par_s_delay1, par_s_balance1, par_s_gain1, par_s_phase1,
        param_count
    };
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, rt_capable = true, support_midi = false, require_midi = false, require_instance_access = false };
    PLUGIN_NAME_ID_LABEL("haasenhancer", "haasenhancer", "Haas Stereo Enhancer")
};

struct reverse_delay_metadata: public plugin_metadata<reverse_delay_metadata>
{
    enum { par_bpm, par_bpm_host, par_divide, par_time_l, par_time_r, par_feedback, par_amount, par_width, par_sync, par_sync_led_l, par_sync_led_r, par_reset, par_window,
        param_bypass, param_level_in, param_level_out,
        STEREO_VU_METER_PARAMS,
        param_count };
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, rt_capable = true, support_midi = false, require_midi = false, require_instance_access = false };
    PLUGIN_NAME_ID_LABEL("reversedelay", "reversedelay", "Reverse Delay")
};

struct rotary_speaker_metadata: public plugin_metadata<rotary_speaker_metadata>
{
public:
    enum { par_speed, par_spacing, par_shift, par_moddepth, par_treblespeed, par_bassspeed, par_micdistance, par_reflection, par_am_depth, par_test, par_meter_l, par_meter_h,
        param_bypass, param_level_in, param_level_out,
        STEREO_VU_METER_PARAMS,
        param_count };
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = true, require_midi = false, rt_capable = true, require_instance_access = false };
    PLUGIN_NAME_ID_LABEL("rotary_speaker", "rotaryspeaker", "Rotary Speaker")
};

/// A multitap stereo chorus thing - metadata
struct multichorus_metadata: public plugin_metadata<multichorus_metadata>
{
public:
    enum { par_delay, par_depth, par_rate, par_stereo, par_voices, par_vphase, par_amount, par_dryamount, par_freq, par_freq2, par_q, par_overlap,
        param_on, param_level_in, param_level_out,
        STEREO_VU_METER_PARAMS, param_lfo,
        param_count };
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, rt_capable = true, support_midi = false, require_midi = false, require_instance_access = false };
    PLUGIN_NAME_ID_LABEL("multichorus", "multichorus", "Multi Chorus")
};

enum CalfEqMode {
    MODE12DB,
    MODE24DB,
    MODE36DB
};

/// Monosynth - metadata
struct monosynth_metadata: public plugin_metadata<monosynth_metadata>
{
    enum { wave_saw, wave_sqr, wave_pulse, wave_sine, wave_triangle, wave_varistep, wave_skewsaw, wave_skewsqr, wave_test1, wave_test2, wave_test3, wave_test4, wave_test5, wave_test6, wave_test7, wave_test8, wave_count };
    enum { flt_lp12, flt_lp24, flt_2lp12, flt_hp12, flt_lpbr, flt_hpbr, flt_bp6, flt_2bp6 };
    enum { par_wave1, par_wave2, par_pw1, par_pw2, par_detune, par_osc2xpose, par_oscmode, par_oscmix, par_filtertype, par_cutoff, par_resonance, par_cutoffsep, par_env1tocutoff, par_env1tores, par_env1toamp,
        par_env1attack, par_env1decay, par_env1sustain, par_env1fade, par_env1release,
        par_keyfollow, par_legato, par_portamento, par_vel2filter, par_vel2amp, par_master, par_pwhlrange,
        par_lforate, par_lfodelay, par_lfofilter, par_lfopitch, par_lfopw, par_mwhl_lfo, par_scaledetune,
        par_env2tocutoff, par_env2tores, par_env2toamp,
        par_env2attack, par_env2decay, par_env2sustain, par_env2fade, par_env2release,
        par_stretch1, par_window1,
        par_lfo1trig, par_lfo2trig,
        par_lfo2rate, par_lfo2delay,
        par_o2unison, par_o2unisonfrq,
        par_osc1xpose,
        par_midi,
        param_count };
    enum { in_count = 0, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = true, require_midi = true, rt_capable = true, require_instance_access = false };
    enum { step_size = 64, step_shift = 6 };
    enum { mod_matrix_slots = 10 };
    enum {
        modsrc_none,
        modsrc_velocity,
        modsrc_pressure,
        modsrc_modwheel,
        modsrc_env1,
        modsrc_env2,
        modsrc_lfo1,
        modsrc_lfo2,
        modsrc_count,
    };
    enum {
        moddest_none,
        moddest_attenuation,
        moddest_oscmix,
        moddest_cutoff,
        moddest_resonance,
        moddest_o1detune,
        moddest_o2detune,
        moddest_o1pw,
        moddest_o2pw,
        moddest_o1stretch,
        moddest_o2unisonamp,
        moddest_o2unisondetune,
        moddest_count,
    };
    PLUGIN_NAME_ID_LABEL("monosynth", "monosynth", "Monosynth")

    mod_matrix_metadata mm_metadata;

    monosynth_metadata();
    /// Lookup of table edit interface
    virtual const table_metadata_iface *get_table_metadata_iface(const char *key) const { if (!strcmp(key, "mod_matrix")) return &mm_metadata; else return NULL; }
    void get_configure_vars(std::vector<std::string> &names) const;
};

/// Thor's compressor - metadata
/// Added some meters and stripped the weighting part
struct compressor_metadata: public plugin_metadata<compressor_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, MONO_VU_METER_PARAMS,
           param_threshold, param_ratio, param_attack, param_release, param_makeup, param_knee, param_detection, param_stereo_link, param_compression, param_mix,
           param_count };
    PLUGIN_NAME_ID_LABEL("compressor", "compressor", "Compressor")
};

/// Damien's compressor - metadata
/// Added some meters and stripped the weighting part
struct monocompressor_metadata: public plugin_metadata<monocompressor_metadata>
{
    enum { in_count = 1, out_count = 1, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, MONO_VU_METER_PARAMS,
           param_threshold, param_ratio, param_attack, param_release, param_makeup, param_knee, param_compression, param_mix,
           param_count };
    PLUGIN_NAME_ID_LABEL("monocompressor", "monocompressor", "Mono Compressor")
};

/// Markus's sidechain compressor - metadata
struct sidechaincompressor_metadata: public plugin_metadata<sidechaincompressor_metadata>
{
    enum { in_count = 4, out_count = 2, ins_optional = 2, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, MONO_VU_METER_PARAMS,
           param_threshold, param_ratio, param_attack, param_release, param_makeup, param_knee, param_detection, param_stereo_link, param_compression,
           param_sc_mode, param_f1_freq, param_f2_freq, param_f1_level, param_f2_level,
           param_sc_listen, param_f1_active, param_f2_active, param_sc_route, param_sc_level, param_mix, param_count };
    PLUGIN_NAME_ID_LABEL("sidechaincompressor", "sidechaincompressor", "Sidechain Compressor")
};

/// Markus's multibandcompressor - metadata
struct multibandcompressor_metadata: public plugin_metadata<multibandcompressor_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS,
           param_freq0, param_freq1, param_freq2,
           param_mode,
           param_threshold0, param_ratio0, param_attack0, param_release0, param_makeup0, param_knee0,
           param_detection0, param_compression0, param_output0, param_bypass0, param_solo0,
           param_threshold1, param_ratio1, param_attack1, param_release1, param_makeup1, param_knee1,
           param_detection1, param_compression1, param_output1, param_bypass1, param_solo1,
           param_threshold2, param_ratio2, param_attack2, param_release2, param_makeup2, param_knee2,
           param_detection2, param_compression2, param_output2, param_bypass2, param_solo2,
           param_threshold3, param_ratio3, param_attack3, param_release3, param_makeup3, param_knee3,
           param_detection3, param_compression3, param_output3, param_bypass3, param_solo3,
           param_notebook,
           param_count };
    PLUGIN_NAME_ID_LABEL("multibandcompressor", "multibandcompressor", "Multiband Compressor")
};

/// Markus's deesser - metadata
struct deesser_metadata: public plugin_metadata<deesser_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_detected, param_compression, param_detected_led, param_clip_out,
           param_detection, param_mode,
           param_threshold, param_ratio, param_laxity, param_makeup,
           param_f1_freq, param_f2_freq, param_f1_level, param_f2_level, param_f2_q,
           param_sc_listen, param_count };
    PLUGIN_NAME_ID_LABEL("deesser", "deesser", "Deesser")
};

/// Damiens' Gate - metadata
/// Added some meters and stripped the weighting part
struct gate_metadata: public plugin_metadata<gate_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, MONO_VU_METER_PARAMS,
           param_range, param_threshold, param_ratio, param_attack, param_release, param_makeup, param_knee, param_detection, param_stereo_link, param_gating,
           param_count };
    PLUGIN_NAME_ID_LABEL("gate", "gate", "Gate")
};

/// Markus's sidechain gate - metadata
struct sidechaingate_metadata: public plugin_metadata<sidechaingate_metadata>
{
    enum { in_count = 4, out_count = 2, ins_optional = 2, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, MONO_VU_METER_PARAMS,
           param_range, param_threshold, param_ratio, param_attack, param_release, param_makeup, param_knee, param_detection, param_stereo_link, param_gating,
           param_sc_mode, param_f1_freq, param_f2_freq, param_f1_level, param_f2_level,
           param_sc_listen, param_f1_active, param_f2_active, param_sc_route, param_sc_level, param_count };
    PLUGIN_NAME_ID_LABEL("sidechaingate", "sidechaingate", "Sidechain Gate")
};

/// Markus's multiband gate - metadata
struct multibandgate_metadata: public plugin_metadata<multibandgate_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS,
           param_freq0, param_freq1, param_freq2,
           param_mode,
           param_range0, param_threshold0, param_ratio0, param_attack0, param_release0, param_makeup0, param_knee0,
           param_detection0, param_gating0, param_output0, param_bypass0, param_solo0,
           param_range1, param_threshold1, param_ratio1, param_attack1, param_release1, param_makeup1, param_knee1,
           param_detection1, param_gating1, param_output1, param_bypass1, param_solo1,
           param_range2, param_threshold2, param_ratio2, param_attack2, param_release2, param_makeup2, param_knee2,
           param_detection2, param_gating2, param_output2, param_bypass2, param_solo2,
           param_range3, param_threshold3, param_ratio3, param_attack3, param_release3, param_makeup3, param_knee3,
           param_detection3, param_gating3, param_output3, param_bypass3, param_solo3,
           param_notebook,
           param_count };
    PLUGIN_NAME_ID_LABEL("multibandgate", "multibandgate", "Multiband Gate")
};

/// Markus's limiter - metadata
struct limiter_metadata: public plugin_metadata<limiter_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS,
           param_limit, param_attack, param_release,
           param_att,
           param_asc, param_asc_led, param_asc_coeff,
           param_oversampling,
           param_auto_level,
           param_count };
    PLUGIN_NAME_ID_LABEL("limiter", "limiter", "Limiter")
};

/// Markus's and Chrischis multibandlimiter - metadata
struct multibandlimiter_metadata: public plugin_metadata<multibandlimiter_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS,
           param_freq0, param_freq1, param_freq2,
           param_mode,
           param_limit, param_attack, param_release, param_minrel,
           param_att0, param_att1, param_att2, param_att3,
           param_weight0, param_weight1, param_weight2, param_weight3,
           param_release0, param_release1, param_release2, param_release3,
           param_solo0, param_solo1, param_solo2, param_solo3,
           param_effrelease0, param_effrelease1, param_effrelease2, param_effrelease3,
           param_asc, param_asc_led, param_asc_coeff,
           param_oversampling,
           param_auto_level,
           param_count };
    PLUGIN_NAME_ID_LABEL("multibandlimiter", "multibandlimiter", "Multiband Limiter")
};

/// Markus's and Chrischis sidechainlimiter - metadata
struct sidechainlimiter_metadata: public plugin_metadata<sidechainlimiter_metadata>
{
    enum { in_count = 4, out_count = 2, ins_optional = 2, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS,
           param_meter_scL, param_meter_scR,
           param_freq0, param_freq1, param_freq2,
           param_mode,
           param_limit, param_attack, param_release, param_minrel,
           param_att0, param_att1, param_att2, param_att3, param_att_sc,
           param_weight0, param_weight1, param_weight2, param_weight3, param_weight_sc,
           param_release0, param_release1, param_release2, param_release3, param_release_sc,
           param_solo0, param_solo1, param_solo2, param_solo3, param_solo_sc,
           param_effrelease0, param_effrelease1, param_effrelease2, param_effrelease3, param_effrelease_sc,
           param_asc, param_asc_led, param_asc_coeff,
           param_oversampling, param_level_sc,
           param_auto_level,
           param_count };
    PLUGIN_NAME_ID_LABEL("sidechainlimiter", "sidechainlimiter", "Sidechain Limiter")
};

/// Damien's RIAA and CD Emphasis - metadata
struct emphasis_metadata: public plugin_metadata<emphasis_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out, STEREO_VU_METER_PARAMS, param_mode, param_type,
           param_count };
    PLUGIN_NAME_ID_LABEL("emphasis", "emphasis", "Emphasis")
};

/// Markus's 5-band EQ - metadata
struct equalizer5band_metadata: public plugin_metadata<equalizer5band_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS,
           param_ls_active, param_ls_level, param_ls_freq, param_ls_q,
           param_hs_active, param_hs_level, param_hs_freq, param_hs_q,
           param_p1_active, param_p1_level, param_p1_freq, param_p1_q,
           param_p2_active, param_p2_level, param_p2_freq, param_p2_q,
           param_p3_active, param_p3_level, param_p3_freq, param_p3_q,
           param_individuals, param_zoom, param_analyzer_active, param_analyzer_mode,
           param_count };
    // dummy parameter numbers, shouldn't be used EVER, they're only there to avoid pushing LP/HP filters to a separate class
    // and potentially making inlining and optimization harder for the compiler
    enum { param_lp_active = 0xDEADBEEF, param_hp_active, param_hp_mode, param_lp_mode, param_hp_freq, param_lp_freq, param_hp_q, param_lp_q };
    enum { PeakBands = 3, first_graph_param = param_ls_active, last_graph_param = param_p3_q };
    PLUGIN_NAME_ID_LABEL("equalizer5band", "eq5", "Equalizer 5 Band")
};

/// Markus's 8-band EQ - metadata
struct equalizer8band_metadata: public plugin_metadata<equalizer8band_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS,
           param_hp_active, param_hp_freq, param_hp_mode, param_hp_q,
           param_lp_active, param_lp_freq, param_lp_mode, param_lp_q,
           param_ls_active, param_ls_level, param_ls_freq, param_ls_q,
           param_hs_active, param_hs_level, param_hs_freq, param_hs_q,
           param_p1_active, param_p1_level, param_p1_freq, param_p1_q,
           param_p2_active, param_p2_level, param_p2_freq, param_p2_q,
           param_p3_active, param_p3_level, param_p3_freq, param_p3_q,
           param_p4_active, param_p4_level, param_p4_freq, param_p4_q,
           param_individuals, param_zoom, param_analyzer_active, param_analyzer_mode,
           param_count };
    enum { PeakBands = 4, first_graph_param = param_hp_active, last_graph_param = param_p4_q };
    PLUGIN_NAME_ID_LABEL("equalizer8band", "eq8", "Equalizer 8 Band")
};
/// Markus's 12-band EQ - metadata
struct equalizer12band_metadata: public plugin_metadata<equalizer12band_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS,
           param_hp_active, param_hp_freq, param_hp_mode, param_hp_q,
           param_lp_active, param_lp_freq, param_lp_mode, param_lp_q,
           param_ls_active, param_ls_level, param_ls_freq, param_ls_q,
           param_hs_active, param_hs_level, param_hs_freq, param_hs_q,
           param_p1_active, param_p1_level, param_p1_freq, param_p1_q,
           param_p2_active, param_p2_level, param_p2_freq, param_p2_q,
           param_p3_active, param_p3_level, param_p3_freq, param_p3_q,
           param_p4_active, param_p4_level, param_p4_freq, param_p4_q,
           param_p5_active, param_p5_level, param_p5_freq, param_p5_q,
           param_p6_active, param_p6_level, param_p6_freq, param_p6_q,
           param_p7_active, param_p7_level, param_p7_freq, param_p7_q,
           param_p8_active, param_p8_level, param_p8_freq, param_p8_q,
           param_individuals, param_zoom, param_analyzer_active, param_analyzer_mode,
           param_count };
    enum { PeakBands = 8, first_graph_param = param_hp_active, last_graph_param = param_p8_q };
    PLUGIN_NAME_ID_LABEL("equalizer12band", "eq12", "Equalizer 12 Band")
};
/// 30-band EQ - metadata
struct equalizer30band_metadata: public plugin_metadata<equalizer30band_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum {
            param_level_in, param_level_in_vuL, param_level_in_vuR, param_level_in_clipL, param_level_in_clipR,
            param_bypass, param_filters,
            param_gainscale1, param_gainscale2,
            param_level_out, param_level_out_vuL, param_level_out_vuR, param_level_out_clipL, param_level_out_clipR,

            //1 - 30 Bands
            param_gain10,  param_gain_scale10,
            param_gain11,  param_gain_scale11,
            param_gain12,  param_gain_scale12,
            param_gain13,  param_gain_scale13,
            param_gain14,  param_gain_scale14,
            param_gain15,  param_gain_scale15,
            param_gain16,  param_gain_scale16,
            param_gain17,  param_gain_scale17,
            param_gain18,  param_gain_scale18,
            param_gain19,  param_gain_scale19,
            param_gain110, param_gain_scale110,
            param_gain111, param_gain_scale111,
            param_gain112, param_gain_scale112,
            param_gain113, param_gain_scale113,
            param_gain114, param_gain_scale114,
            param_gain115, param_gain_scale115,
            param_gain116, param_gain_scale116,
            param_gain117, param_gain_scale117,
            param_gain118, param_gain_scale118,
            param_gain119, param_gain_scale119,
            param_gain120, param_gain_scale120,
            param_gain121, param_gain_scale121,
            param_gain122, param_gain_scale122,
            param_gain123, param_gain_scale123,
            param_gain124, param_gain_scale124,
            param_gain125, param_gain_scale125,
            param_gain126, param_gain_scale126,
            param_gain127, param_gain_scale127,
            param_gain128, param_gain_scale128,
            param_gain129, param_gain_scale129,
            param_gain130, param_gain_scale130,

            //2 - 30 Bands
            param_gain20,  param_gain_scale20,
            param_gain21,  param_gain_scale21,
            param_gain22,  param_gain_scale22,
            param_gain23,  param_gain_scale23,
            param_gain24,  param_gain_scale24,
            param_gain25,  param_gain_scale25,
            param_gain26,  param_gain_scale26,
            param_gain27,  param_gain_scale27,
            param_gain28,  param_gain_scale28,
            param_gain29,  param_gain_scale29,
            param_gain210, param_gain_scale210,
            param_gain211, param_gain_scale211,
            param_gain212, param_gain_scale212,
            param_gain213, param_gain_scale213,
            param_gain214, param_gain_scale214,
            param_gain215, param_gain_scale215,
            param_gain216, param_gain_scale216,
            param_gain217, param_gain_scale217,
            param_gain218, param_gain_scale218,
            param_gain219, param_gain_scale219,
            param_gain220, param_gain_scale220,
            param_gain221, param_gain_scale221,
            param_gain222, param_gain_scale222,
            param_gain223, param_gain_scale223,
            param_gain224, param_gain_scale224,
            param_gain225, param_gain_scale225,
            param_gain226, param_gain_scale226,
            param_gain227, param_gain_scale227,
            param_gain228, param_gain_scale228,
            param_gain229, param_gain_scale229,
            param_gain230, param_gain_scale230,
            
            param_l_active, param_r_active, param_linked,

           param_count };
    enum { band_params = 2 };
    enum { gain_step = 6 };

    PLUGIN_NAME_ID_LABEL("equalizer30band", "eq30", "Equalizer 30 Band")
};
/// Markus's X-Overs
struct xover2_metadata: public plugin_metadata<xover2_metadata>
{
    enum { in_count = 2, out_count = 4, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_level, param_meter_0, param_meter_1, param_mode,
           param_freq0,
           param_level1, param_active1, param_phase1, param_delay1, param_meter_01, param_meter_11,
           param_level2, param_active2, param_phase2, param_delay2, param_meter_02, param_meter_12,
           param_count };
    enum { channels = 2, bands = 2 };
    PLUGIN_NAME_ID_LABEL("xover2band", "xover2", "X-Over 2 Band")
};
struct xover3_metadata: public plugin_metadata<xover3_metadata>
{
    enum { in_count = 2, out_count = 6, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_level, param_meter_0, param_meter_1, param_mode,
           param_freq0, param_freq1,
           param_level1, param_active1, param_phase1, param_delay1, param_meter_01, param_meter_11,
           param_level2, param_active2, param_phase2, param_delay2, param_meter_02, param_meter_12,
           param_level3, param_active3, param_phase3, param_delay3, param_meter_03, param_meter_13,
           param_count };
    enum { channels = 2, bands = 3 };
    PLUGIN_NAME_ID_LABEL("xover3band", "xover3", "X-Over 3 Band")
};
struct xover4_metadata: public plugin_metadata<xover4_metadata>
{
    enum { in_count = 2, out_count = 8, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_level, param_meter_0, param_meter_1, param_mode,
           param_freq0, param_freq1, param_freq2,
           param_level1, param_active1, param_phase1, param_delay1, param_meter_01, param_meter_11,
           param_level2, param_active2, param_phase2, param_delay2, param_meter_02, param_meter_12,
           param_level3, param_active3, param_phase3, param_delay3, param_meter_03, param_meter_13,
           param_level4, param_active4, param_phase4, param_delay4, param_meter_04, param_meter_14,
           param_count };
    enum { channels = 2, bands = 4 };
    PLUGIN_NAME_ID_LABEL("xover4band", "xover4", "X-Over 4 Band")
};

/// Markus and Chrischis Vocoder - metadata
struct vocoder_metadata: public plugin_metadata<vocoder_metadata>
{
    enum { in_count = 4, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_link, param_detectors,
           param_carrier_in, param_carrier_inL, param_carrier_inR, param_carrier_clip_inL, param_carrier_clip_inR,
           param_mod_in, param_mod_inL, param_mod_inR, param_mod_clip_inL, param_mod_clip_inR,
           param_out, param_outL, param_outR, param_clip_outL, param_clip_outR,
           param_carrier, param_mod, param_proc, param_order, param_bands, param_hiq,
           param_attack, param_release, param_analyzer,
           param_volume0, param_pan0, param_noise0, param_mod0, param_solo0, param_level0, param_q0,
           param_volume1, param_pan1, param_noise1, param_mod1, param_solo1, param_level1, param_q1,
           param_volume2, param_pan2, param_noise2, param_mod2, param_solo2, param_level2, param_q2,
           param_volume3, param_pan3, param_noise3, param_mod3, param_solo3, param_level3, param_q3,
           param_volume4, param_pan4, param_noise4, param_mod4, param_solo4, param_level4, param_q4,
           param_volume5, param_pan5, param_noise5, param_mod5, param_solo5, param_level5, param_q5,
           param_volume6, param_pan6, param_noise6, param_mod6, param_solo6, param_level6, param_q6,
           param_volume7, param_pan7, param_noise7, param_mod7, param_solo7, param_level7, param_q7,
           param_volume8, param_pan8, param_noise8, param_mod8, param_solo8, param_level8, param_q8,
           param_volume9, param_pan9, param_noise9, param_mod9, param_solo9, param_level9, param_q9,
           param_volume10, param_pan10, param_noise10, param_mod10, param_solo10, param_level10, param_q10,
           param_volume11, param_pan11, param_noise11, param_mod11, param_solo11, param_level11, param_q11,
           param_volume12, param_pan12, param_noise12, param_mod12, param_solo12, param_level12, param_q12,
           param_volume13, param_pan13, param_noise13, param_mod13, param_solo13, param_level13, param_q13,
           param_volume14, param_pan14, param_noise14, param_mod14, param_solo14, param_level14, param_q14,
           param_volume15, param_pan15, param_noise15, param_mod15, param_solo15, param_level15, param_q15,
           param_volume16, param_pan16, param_noise16, param_mod16, param_solo16, param_level16, param_q16,
           param_volume17, param_pan17, param_noise17, param_mod17, param_solo17, param_level17, param_q17,
           param_volume18, param_pan18, param_noise18, param_mod18, param_solo18, param_level18, param_q18,
           param_volume19, param_pan19, param_noise19, param_mod19, param_solo19, param_level19, param_q19,
           param_volume20, param_pan20, param_noise20, param_mod20, param_solo20, param_level20, param_q20,
           param_volume21, param_pan21, param_noise21, param_mod21, param_solo21, param_level21, param_q21,
           param_volume22, param_pan22, param_noise22, param_mod22, param_solo22, param_level22, param_q22,
           param_volume23, param_pan23, param_noise23, param_mod23, param_solo23, param_level23, param_q23,
           param_volume24, param_pan24, param_noise24, param_mod24, param_solo24, param_level24, param_q24,
           param_volume25, param_pan25, param_noise25, param_mod25, param_solo25, param_level25, param_q25,
           param_volume26, param_pan26, param_noise26, param_mod26, param_solo26, param_level26, param_q26,
           param_volume27, param_pan27, param_noise27, param_mod27, param_solo27, param_level27, param_q27,
           param_volume28, param_pan28, param_noise28, param_mod28, param_solo28, param_level28, param_q28,
           param_volume29, param_pan29, param_noise29, param_mod29, param_solo29, param_level29, param_q29,
           param_volume30, param_pan30, param_noise30, param_mod30, param_solo30, param_level30, param_q30,
           param_volume31, param_pan31, param_noise31, param_mod31, param_solo31, param_level31, param_q31,
           param_lower, param_upper, param_tilt,
           param_count };
    enum { band_params = 7 };
    PLUGIN_NAME_ID_LABEL("vocoder", "vocoder", "Vocoder")
};

/// Markus's Pulsator - metadata
struct pulsator_metadata: public plugin_metadata<pulsator_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out, STEREO_VU_METER_PARAMS,
           param_mode, param_amount, param_offset_l, param_offset_r,
           param_mono, param_reset, param_pwidth,
           PERIODICAL_PARAMS,
           param_count };
    PLUGIN_NAME_ID_LABEL("pulsator", "pulsator", "Pulsator")
};

/// Markus's Ring Modulator - metadata
struct ringmodulator_metadata: public plugin_metadata<ringmodulator_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out, STEREO_VU_METER_PARAMS,
           param_mod_mode, param_mod_freq, param_mod_amount, param_mod_phase, param_mod_detune, param_mod_listen,
           
           param_lfo1_mode, param_lfo1_freq, param_lfo1_reset,
           param_lfo1_mod_freq_lo, param_lfo1_mod_freq_hi, param_lfo1_mod_freq_active,
           param_lfo1_mod_detune_lo, param_lfo1_mod_detune_hi, param_lfo1_mod_detune_active,
           param_lfo1_activity,
           
           param_lfo2_mode, param_lfo2_freq, param_lfo2_reset, 
           param_lfo2_lfo1_freq_lo, param_lfo2_lfo1_freq_hi, param_lfo2_lfo1_freq_active,
           param_lfo2_mod_amount_lo, param_lfo2_mod_amount_hi, param_lfo2_mod_amount_active,
           param_lfo2_activity,
           
           param_count };
    PLUGIN_NAME_ID_LABEL("ringmodulator", "ringmodulator", "Ring Modulator")
};

/// Markus's Saturator - metadata
struct saturator_metadata: public plugin_metadata<saturator_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 1, outs_optional = 1, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS,
           param_mix, param_drive, param_blend,
           param_lp_pre_freq, param_hp_pre_freq, param_lp_post_freq, param_hp_post_freq,
           param_p_freq, param_p_level, param_p_q, param_pre, param_post, param_count };
    PLUGIN_NAME_ID_LABEL("saturator", "saturator", "Saturator")
};
/// Markus's Exciter - metadata
struct exciter_metadata: public plugin_metadata<exciter_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 1, outs_optional = 1, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out, param_amount, MONO_VU_METER_PARAMS, param_drive, param_blend, param_meter_drive,
           param_freq, param_listen, param_ceil_active, param_ceil, param_count };
    PLUGIN_NAME_ID_LABEL("exciter", "exciter", "Exciter")
};
/// Markus's Bass Enhancer - metadata
struct bassenhancer_metadata: public plugin_metadata<bassenhancer_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 1, outs_optional = 1, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out, param_amount, MONO_VU_METER_PARAMS, param_drive, param_blend, param_meter_drive,
           param_freq, param_listen, param_floor_active, param_floor, param_count };
    PLUGIN_NAME_ID_LABEL("bassenhancer", "bassenhancer", "Bass Enhancer")
};
/// Markus's and Chrischi's Crusher Module - metadata
struct crusher_metadata: public plugin_metadata<crusher_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 1, outs_optional = 1, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS,
           param_bits, param_morph, param_mode, param_dc, param_aa,
           param_samples, param_lfo, param_lforange, param_lforate,
           param_count };
    PLUGIN_NAME_ID_LABEL("crusher", "crusher", "Crusher")
};
/// Markus's Stereo Module - metadata
struct stereo_metadata: public plugin_metadata<stereo_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 1, outs_optional = 1, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS, param_balance_in, param_balance_out, param_softclip,
           param_mute_l, param_mute_r, param_phase_l, param_phase_r,
           param_mode, param_slev, param_sbal, param_mlev, param_mpan,
           param_stereo_base, param_delay,
           param_meter_phase, param_sc_level, param_stereo_phase,
           param_count };
    PLUGIN_NAME_ID_LABEL("stereo", "stereo", "Stereo Tools")
};
/// Markus's Mono Module - metadata
struct mono_metadata: public plugin_metadata<mono_metadata>
{
    enum { in_count = 1, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           param_meter_in, param_meter_outL, param_meter_outR, param_clip_in,param_clip_outL, param_clip_outR,
           param_balance_out, param_softclip,
           param_mute_l, param_mute_r, param_phase_l, param_phase_r,
           param_delay, param_stereo_base, param_stereo_phase, param_sc_level,
           param_count };
    PLUGIN_NAME_ID_LABEL("mono", "mono", "Mono Input")
};
/// Markus's  multibandenhancer - metadata
struct multibandenhancer_metadata: public plugin_metadata<multibandenhancer_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS,
           param_freq0, param_freq1, param_freq2,
           param_mode,
           param_base0, param_base1, param_base2, param_base3,
           param_drive0, param_drive1, param_drive2, param_drive3,
           param_blend0, param_blend1, param_blend2, param_blend3,
           param_solo0, param_solo1, param_solo2, param_solo3,
           param_count };
    PLUGIN_NAME_ID_LABEL("multibandenhancer", "multibandenhancer", "Multiband Enhancer")
};
/// Markus's  multispread - metadata
struct multispread_metadata: public plugin_metadata<multispread_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 1, outs_optional = 0, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS,
           param_mono, param_filters,
           param_amount0, param_amount1, param_amount2, param_amount3,
           param_intensity,
           param_count };
    PLUGIN_NAME_ID_LABEL("multispread", "multispread", "Multi Spread")
};
/// Markus's and Chrischi's Analyzer
struct analyzer_metadata: public plugin_metadata<analyzer_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 1, outs_optional = 1, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = true };
    enum { param_meter_L, param_meter_R, param_clip_L, param_clip_R,
           param_analyzer_level, param_analyzer_mode,
           param_analyzer_scale, param_analyzer_post,
           param_analyzer_view, param_analyzer_smoothing,
           param_analyzer_windowing,
           param_analyzer_accuracy, param_analyzer_speed,
           param_analyzer_display, param_analyzer_hold, param_analyzer_freeze,
           param_gonio_mode, param_gonio_use_fade, param_gonio_fade, param_gonio_accuracy, param_gonio_display,
           param_count };
    PLUGIN_NAME_ID_LABEL("analyzer", "analyzer", "Analyzer")
};
/// Markus's and Chrischi's Transient Designer
struct transientdesigner_metadata: public plugin_metadata<transientdesigner_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 1, outs_optional = 1, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS, param_mix,
           param_attack_time, param_attack_boost,
           param_sustain_threshold, param_release_time, param_release_boost,
           param_display, param_display_threshold, param_lookahead,
           param_view, param_hipass, param_lopass, param_hp_mode, param_lp_mode, param_listen,
           param_count };
    PLUGIN_NAME_ID_LABEL("transientdesigner", "transientdesigner", "Transient Designer")
};
/// Markus's Vinyl Simulator
struct vinyl_metadata: public plugin_metadata<vinyl_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 1, outs_optional = 1, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = true };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS,
           param_drone, param_speed, param_aging, param_freq,
           param_gain0, param_pitch0, param_active0,
           param_gain1, param_pitch1, param_active1,
           param_gain2, param_pitch2, param_active2,
           param_gain3, param_pitch3, param_active3,
           param_gain4, param_pitch4, param_active4,
           param_gain5, param_pitch5, param_active5,
           param_gain6, param_pitch6, param_active6,
           param_count };
    PLUGIN_NAME_ID_LABEL("vinyl", "vinyl", "Vinyl")
};
/// Markus's Tape Simulator
struct tapesimulator_metadata: public plugin_metadata<tapesimulator_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 1, outs_optional = 1, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { param_bypass, param_level_in, param_level_out,
           STEREO_VU_METER_PARAMS, param_mix, param_lp,
           param_speed, param_noise, param_mechanical, param_magnetical, param_post,
           param_count };
    PLUGIN_NAME_ID_LABEL("tapesimulator", "tapesimulator", "Tape Simulator")
};
/// Organ - enums for parameter IDs etc. (this mess is caused by organ split between plugin and generic class - which was
/// a bad design decision and should be sorted out some day) XXXKF @todo
struct organ_enums
{
    enum {
        par_drawbar1, par_drawbar2, par_drawbar3, par_drawbar4, par_drawbar5, par_drawbar6, par_drawbar7, par_drawbar8, par_drawbar9,
        par_frequency1, par_frequency2, par_frequency3, par_frequency4, par_frequency5, par_frequency6, par_frequency7, par_frequency8, par_frequency9,
        par_waveform1, par_waveform2, par_waveform3, par_waveform4, par_waveform5, par_waveform6, par_waveform7, par_waveform8, par_waveform9,
        par_detune1, par_detune2, par_detune3, par_detune4, par_detune5, par_detune6, par_detune7, par_detune8, par_detune9,
        par_phase1, par_phase2, par_phase3, par_phase4, par_phase5, par_phase6, par_phase7, par_phase8, par_phase9,
        par_pan1, par_pan2, par_pan3, par_pan4, par_pan5, par_pan6, par_pan7, par_pan8, par_pan9,
        par_routing1, par_routing2, par_routing3, par_routing4, par_routing5, par_routing6, par_routing7, par_routing8, par_routing9,
        par_foldover,
        par_percdecay, par_perclevel, par_percwave, par_percharm, par_percvel2amp,
        par_percfmdecay, par_percfmdepth, par_percfmwave, par_percfmharm, par_percvel2fm,
        par_perctrigger, par_percstereo,
        par_filterchain,
        par_filter1type,
        par_master,
        par_f1cutoff, par_f1res, par_f1env1, par_f1env2, par_f1env3, par_f1keyf,
        par_f2cutoff, par_f2res, par_f2env1, par_f2env2, par_f2env3, par_f2keyf,
        par_eg1attack, par_eg1decay, par_eg1sustain, par_eg1release, par_eg1velscl, par_eg1ampctl,
        par_eg2attack, par_eg2decay, par_eg2sustain, par_eg2release, par_eg2velscl, par_eg2ampctl,
        par_eg3attack, par_eg3decay, par_eg3sustain, par_eg3release, par_eg3velscl, par_eg3ampctl,
        par_lforate, par_lfoamt, par_lfowet, par_lfophase, par_lfomode, par_lfotype,
        par_transpose, par_detune,
        par_polyphony,
        par_quadenv,
        par_pwhlrange,
        par_bassfreq,
        par_bassgain,
        par_treblefreq,
        par_treblegain,
        par_midi,
        param_count
    };
    enum organ_waveform {
        wave_sine,
        wave_sinepl1, wave_sinepl2, wave_sinepl3,
        wave_ssaw, wave_ssqr, wave_spls, wave_saw, wave_sqr, wave_pulse, wave_sinepl05, wave_sqr05, wave_halfsin, wave_clvg, wave_bell, wave_bell2,
        wave_w1, wave_w2, wave_w3, wave_w4, wave_w5, wave_w6, wave_w7, wave_w8, wave_w9,
        wave_dsaw, wave_dsqr, wave_dpls,
        wave_count_small,
        wave_strings = wave_count_small,
        wave_strings2,
        wave_sinepad,
        wave_bellpad,
        wave_space,
        wave_choir,
        wave_choir2,
        wave_choir3,
        wave_count,
        wave_count_big = wave_count - wave_count_small
    };
    enum {
        ampctl_none,
        ampctl_direct,
        ampctl_f1,
        ampctl_f2,
        ampctl_all,
        ampctl_count
    };
    enum {
        lfotype_allpass = 0,
        lfotype_cv1,
        lfotype_cv2,
        lfotype_cv3,
        lfotype_cvfull,
        lfotype_count
    };
    enum {
        lfomode_off = 0,
        lfomode_direct,
        lfomode_filter1,
        lfomode_filter2,
        lfomode_voice,
        lfomode_global,
        lfomode_count
    };
    enum {
        perctrig_first = 0,
        perctrig_each,
        perctrig_eachplus,
        perctrig_polyphonic,
        perctrig_count
    };
};

/// Organ - metadata
struct organ_metadata: public organ_enums, public plugin_metadata<organ_metadata>
{
    enum { in_count = 0, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = true, require_midi = true, rt_capable = true, require_instance_access = false };
    PLUGIN_NAME_ID_LABEL("organ", "organ", "Organ")

public:
    plugin_command_info *get_commands() const;
    void get_configure_vars(std::vector<std::string> &names) const;
};

/// FluidSynth - metadata
struct fluidsynth_metadata: public plugin_metadata<fluidsynth_metadata>
{
    enum { par_master, par_interpolation, par_reverb, par_chorus, param_count };
    enum { in_count = 0, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = true, require_midi = true, rt_capable = false, require_instance_access = true };
    PLUGIN_NAME_ID_LABEL("fluidsynth", "fluidsynth", "Fluidsynth")

public:
    void get_configure_vars(std::vector<std::string> &names) const;
};

/// Wavetable - metadata
struct wavetable_metadata: public plugin_metadata<wavetable_metadata>
{
    enum {
        wt_fmshiny,
        wt_fmshiny2,
        wt_rezo,
        wt_metal,
        wt_bell,
        wt_blah,
        wt_pluck,
        wt_stretch,
        wt_stretch2,
        wt_hardsync,
        wt_hardsync2,
        wt_softsync,
        wt_bell2,
        wt_bell3,
        wt_tine,
        wt_tine2,
        wt_clav,
        wt_clav2,
        wt_gtr,
        wt_gtr2,
        wt_gtr3,
        wt_gtr4,
        wt_gtr5,
        wt_reed,
        wt_reed2,
        wt_silver,
        wt_brass,
        wt_multi,
        wt_multi2,
        wt_count
    };
    enum {
        modsrc_none,
        modsrc_velocity,
        modsrc_pressure,
        modsrc_modwheel,
        modsrc_env1,
        modsrc_env2,
        modsrc_env3,
        modsrc_lfo1,
        modsrc_lfo2,
        modsrc_keyfollow,
        modsrc_count,
    };
    enum {
        moddest_none,
        moddest_attenuation,
        moddest_oscmix,
        moddest_cutoff,
        moddest_resonance,
        moddest_o1shift,
        moddest_o2shift,
        moddest_o1detune,
        moddest_o2detune,
        moddest_pitch,
        moddest_count,
    };
    enum {
        par_o1wave, par_o1offset, par_o1transpose, par_o1detune, par_o1level,
        par_o2wave, par_o2offset, par_o2transpose, par_o2detune, par_o2level,
        par_eg1attack, par_eg1decay, par_eg1sustain, par_eg1fade, par_eg1release, par_eg1velscl,
        par_eg2attack, par_eg2decay, par_eg2sustain, par_eg2fade, par_eg2release, par_eg2velscl,
        par_eg3attack, par_eg3decay, par_eg3sustain, par_eg3fade, par_eg3release, par_eg3velscl,
        par_pwhlrange,
        par_eg1toamp,
        par_lfo1rate,
        par_lfo2rate,
        par_midi,
        param_count };
    enum { in_count = 0, out_count = 2, ins_optional = 0, outs_optional = 0, support_midi = true, require_midi = true, rt_capable = true, require_instance_access = false };
    enum { mod_matrix_slots = 10 };
    enum { step_size = 64 };
    PLUGIN_NAME_ID_LABEL("wavetable", "wavetable", "Wavetable")
    mod_matrix_metadata mm_metadata;

    wavetable_metadata();
    /// Lookup of table edit interface
    virtual const table_metadata_iface *get_table_metadata_iface(const char *key) const { if (!strcmp(key, "mod_matrix")) return &mm_metadata; else return NULL; }
    void get_configure_vars(std::vector<std::string> &names) const;
};

struct pitch_metadata: public plugin_metadata<pitch_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 1, outs_optional = 1, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { 
        par_pd_threshold,
        par_pd_subdivide,
        par_tune, par_note, par_cents, par_clarity, par_freq,
        param_count
    };
    PLUGIN_NAME_ID_LABEL("pitch", "pitch", "Pitch Tools")
};

struct widgets_metadata: public plugin_metadata<widgets_metadata>
{
    enum { in_count = 2, out_count = 2, ins_optional = 1, outs_optional = 1, support_midi = false, require_midi = false, rt_capable = true, require_instance_access = false };
    enum { 
        param_meter1,
        param_meter2,
        param_meter3,
        param_meter4,
        param_value1,
        param_value2,
        param_count
    };
    PLUGIN_NAME_ID_LABEL("widgets", "widgets", "Widget Test")
};

};

#endif
