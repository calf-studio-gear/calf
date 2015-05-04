#ifndef CALF_MODULES_PITCH_H
#define CALF_MODULES_PITCH_H

#include <assert.h>
#include <limits.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "analyzer.h"
#include "giface.h"
#include "metadata.h"
#include "fft.h"
#include <math.h>
#include "bypass.h"

#if ENABLE_EXPERIMENTAL

namespace calf_plugins {

class pitch_audio_module: public audio_module<pitch_metadata>, public line_graph_iface
{
protected:
    typedef dsp::fft<float, 12> pfft;
    enum { BufferSize = 4096 };
    uint32_t srate;
    pfft transform;
    float inputbuf[BufferSize];
    pfft::complex waveform[2 * BufferSize], spectrum[2 * BufferSize], autocorr[2 * BufferSize];
    float magarr[BufferSize / 2];
    float sumsquares[BufferSize + 1], sumsquares_last;
    uint32_t write_ptr;
    
    void recompute();
public:
    typedef pitch_audio_module AM;

    pitch_audio_module();
    ~pitch_audio_module();
    void params_changed();
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);

    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_layers(int index, int generation, unsigned int &layers) const { layers = LG_REALTIME_GRAPH; return true; }
};

};
#endif

#endif
