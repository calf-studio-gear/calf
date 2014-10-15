/* Calf Analyzer FFT Library
 * Copyright (C) 2007-2013 Krzysztof Foltman, Markus Schmidt,
 * Christian Holschuh and others
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
 
#ifndef CALF_ANALYZER_H
#define CALF_ANALYZER_H

#include <assert.h>
#include <limits.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "giface.h"
#include "metadata.h"
#include "loudness.h"
#include <math.h>
#include "plugin_tools.h"
#include "fft.h"

#define MATH_E 2.718281828

namespace calf_plugins {

class analyzer: public frequency_response_line_graph
{
private:
    mutable int _accuracy;
    mutable int _acc;
    mutable int _scale;
    mutable int _post;
    mutable int _hold;
    mutable int _smooth;
    mutable int _speed;
    mutable int _windowing;
    mutable int _view;
    mutable int _freeze;
    mutable int _mode;
    mutable float _resolution;
    mutable float _offset;
    mutable bool _falling;
    mutable int _draw_upper;

    
public:
    uint32_t srate;
    analyzer();
    void process(float L, float R);
    void set_sample_rate(uint32_t sr);
    bool set_mode(int mode);
    void invalidate();
    void set_params(float resolution, float offset, int accuracy, int hold, int smoothing, int mode, int scale, int post, int speed, int windowing, int view, int freeze);
    ~analyzer();
    bool do_fft(int subindex, int points) const;
    void draw(int subindex, float *data, int points, bool fftdone) const;
    bool get_graph(int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_moving(int subindex, int &direction, float *data, int x, int y, int &offset, uint32_t &color) const;
    bool get_gridline(int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    bool get_layers(int generation, unsigned int &layers) const;
protected:
    int fft_buffer_size;
    float *fft_buffer;
    int *spline_buffer;
    int fpos;
    mutable bool sanitize, recreate_plan;
    static const int MAX_FFT_ORDER = 15;
    dsp::fft<float, MAX_FFT_ORDER> fft;
    mutable dsp::fft<float, MAX_FFT_ORDER>::complex fft_temp[1 << MAX_FFT_ORDER];
    static const int max_fft_cache_size = 32768;
    static const int max_fft_buffer_size = max_fft_cache_size * 2;
    float *fft_inL, *fft_outL;
    float *fft_inR, *fft_outR;
    float *fft_smoothL, *fft_smoothR;
    float *fft_deltaL, *fft_deltaR;
    float *fft_holdL, *fft_holdR;
    float *fft_freezeL, *fft_freezeR;
    mutable int lintrans;
    mutable int analyzer_phase_drawn;
};

};
#endif
