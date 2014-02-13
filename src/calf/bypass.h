#ifndef CALF_BYPASS_H
#define CALF_BYPASS_H

#include "inertia.h"

namespace dsp {

class bypass
{
    inertia<linear_ramp> ramp;
    float first_value, next_value;
    
public:
    bypass(int _ramp_len = 1024)
    : ramp(linear_ramp(_ramp_len))
    {
    }
    
    /// Pass the new state of the bypass button, and return the ramp-aware
    /// bypass state
    bool update(bool new_state, uint32_t nsamples)
    {
        ramp.set_inertia(new_state ? 1.f : 0.f);
        first_value = ramp.get_last();
        ramp.step_many(nsamples);
        next_value = ramp.get_last();
        return first_value >= 1 && next_value >= 1;
    }
    
    /// Apply ramp to prevent clicking
    void crossfade(float *inputs[], float *outputs[], uint32_t nbuffers, uint32_t offset, uint32_t nsamples)
    {
        if (!nsamples || (first_value + next_value) == 0)
            return;
        float step = (next_value - first_value) / nsamples;
        for (uint32_t b = 0; b < nbuffers; ++b)
        {
            float *out = outputs[b] + offset, *in = inputs[b] + offset;
            if (first_value >= 1 && next_value >= 1)
                memcpy(out, in, nsamples * sizeof(float));
            else
            {
                for (uint32_t i = 0; i < nsamples; ++i)
                {
                    float bypass_amt = first_value + i * step;
                    out[i] += (in[i] - out[i]) * bypass_amt;
                }
            }
        }
    }
};

}

#endif
