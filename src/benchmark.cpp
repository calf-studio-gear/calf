/* Calf DSP Library
 * Benchmark for selected parts of the library.
 * Copyright (C) 2007 Krzysztof Foltman
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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <config.h>
#include <calf/giface.h>
#include <calf/modules.h>
#include <calf/modules_dev.h>
#include <calf/benchmark.h>

// #define TEST_OSC

using namespace std;
using namespace dsp;

#ifdef TEST_OSC
#include <calf/osctl.h>
using namespace osctl;
#endif

bool benchmark_globals::warned = false;

template<int BUF_SIZE>
struct empty_benchmark
{
    void prepare()
    {
    }
    void cleanup()
    {
    }
    double scaler() { return BUF_SIZE; }
};

struct filter_lp24dB_benchmark
{
    enum { BUF_SIZE = 256 };
    float buffer[BUF_SIZE];
    float result;
    dsp::biquad<float> biquad, biquad2;
    void prepare()
    {
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = i;
        biquad.set_lp_rbj(2000, 0.7, 44100);
        biquad2.set_lp_rbj(2000, 0.7, 44100);
        result = 0;
    }
    void cleanup() { result = buffer[BUF_SIZE - 1]; }
    double scaler() { return BUF_SIZE; }
};

struct filter_24dB_lp_twopass_d1: public filter_lp24dB_benchmark
{
    void run()
    {
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad.process_d1(buffer[i]);
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad2.process_d1(buffer[i]);
    }
};

struct filter_24dB_lp_onepass_d1: public filter_lp24dB_benchmark
{
    void run()
    {
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad2.process_d1(biquad.process_d2(buffer[i]));
    }
};

struct filter_12dB_lp_d1: public filter_lp24dB_benchmark
{
    void run()
    {
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad.process_d1(buffer[i]);
    }
};

struct filter_24dB_lp_twopass_d2: public filter_lp24dB_benchmark
{
    void run()
    {
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad.process_d1(buffer[i]);
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad2.process_d1(buffer[i]);
    }
};

struct filter_24dB_lp_onepass_d2: public filter_lp24dB_benchmark
{
    void run()
    {
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad2.process_d2(biquad.process_d2(buffer[i]));
    }
};

struct filter_24dB_lp_onepass_d2_lp: public filter_lp24dB_benchmark
{
    void run()
    {
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad2.process_d2_lp(biquad.process_d2_lp(buffer[i]));
    }
};

struct filter_12dB_lp_d2: public filter_lp24dB_benchmark
{
    void run()
    {
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad.process_d2(buffer[i]);
    }
};

#define ALIGN_TEST_RUN 1024

struct __attribute__((aligned(8))) alignment_test: public empty_benchmark<ALIGN_TEST_RUN>
{
    char __attribute__((aligned(8))) data[ALIGN_TEST_RUN * sizeof(double) + sizeof(double)];
    float result;

    virtual double *get_ptr()=0;
    virtual ~alignment_test() {}
    void prepare()
    {
        memset(data, 0, sizeof(data));
        result = 0;
    }
    void cleanup()
    {
        result = 0;
        double *p = get_ptr();
        for (int i=0; i < ALIGN_TEST_RUN; i++)
            result += p[i];
    }
};

struct aligned_double: public alignment_test
{    
    virtual double *get_ptr() { return (double *)data; }
    void run()
    {
        double *ptr = (double *)data;
        for (int i=0; i < ALIGN_TEST_RUN; i++)
            ptr[i] += 1 + i;
    }
};

struct __attribute__((aligned(8))) misaligned_double: public alignment_test
{
    virtual double *get_ptr() { return (double *)(data+4); }
    void run()
    {
        double *ptr = (double *)(data + 4);
        for (int i=0; i < ALIGN_TEST_RUN; i++)
            ptr[i] += 1 + i;
    }
};

const char *unit = NULL;

static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {"unit", 1, 0, 'u'},
    {0,0,0,0},
};

void biquad_test()
{
        do_simple_benchmark<filter_24dB_lp_twopass_d1>();
        do_simple_benchmark<filter_24dB_lp_onepass_d1>();
        do_simple_benchmark<filter_12dB_lp_d1>();
        do_simple_benchmark<filter_24dB_lp_twopass_d2>();
        do_simple_benchmark<filter_24dB_lp_onepass_d2>();
        do_simple_benchmark<filter_24dB_lp_onepass_d2_lp>();
        do_simple_benchmark<filter_12dB_lp_d2>();
}

void alignment_test()
{
        do_simple_benchmark<misaligned_double>();
        do_simple_benchmark<aligned_double>();
}

template<class Effect>
void get_default_effect_params(float params[Effect::param_count], uint32_t &sr);

template<>
void get_default_effect_params<synth::reverb_audio_module>(float params[3], uint32_t &sr)
{
    typedef synth::reverb_audio_module mod;
    params[mod::par_decay] = 4;
    params[mod::par_hfdamp] = 2000;
    params[mod::par_amount] = 2;
    sr = 4000;
}

template<>
void get_default_effect_params<synth::filter_audio_module>(float params[4], uint32_t &sr)
{
    typedef synth::filter_audio_module mod;
    params[mod::par_cutoff] = 500;
    params[mod::par_resonance] = 3;
    params[mod::par_mode] = 2;
    params[mod::par_inertia] = 16;
    sr = 4000;
}

template<>
void get_default_effect_params<synth::flanger_audio_module>(float params[5], uint32_t &sr)
{
    typedef synth::flanger_audio_module mod;
    params[mod::par_delay] = 1;
    params[mod::par_depth] = 2;
    params[mod::par_rate] = 1;
    params[mod::par_fb] = 0.9;
    params[mod::par_amount] = 0.9;
    sr = 44100;
}

template<class Effect, unsigned int bufsize = 256>
class effect_benchmark: public empty_benchmark<bufsize>
{
public:
    Effect effect;
    float inputs[Effect::in_count][bufsize];
    float outputs[Effect::out_count][bufsize];
    float result;
    float params[Effect::param_count];
    
    void prepare()
    {
        for (int b = 0; b < Effect::out_count; b++)
        {
            effect.outs[b] = outputs[b];
            dsp::zero(outputs[b], bufsize);
        }
        for (int b = 0; b < Effect::in_count; b++)
        {
            effect.ins[b] = inputs[b];
            for (unsigned int i = 0; i < bufsize; i++)
                inputs[b][i] = (float)(10 + i*0.3f*(b+1));
        }
        for (int i = 0; i < Effect::param_count; i++)
            effect.params[i] = &params[i];
        ::get_default_effect_params<Effect>(params, effect.srate);
        result = 0.f;
        effect.activate();
    }
    void run()
    {
        effect.params_changed();
        effect.process(0, bufsize, 3, 3);
    }
    void cleanup()
    {
        for (int b = 0; b < Effect::out_count; b++)
        {
            for (unsigned int i = 0; i < bufsize; i++)
                result += outputs[b][i];
        }
    }
};

void effect_test()
{
    dsp::do_simple_benchmark<effect_benchmark<synth::flanger_audio_module> >(5, 10000);
    dsp::do_simple_benchmark<effect_benchmark<synth::reverb_audio_module> >(5, 1000);
    dsp::do_simple_benchmark<effect_benchmark<synth::filter_audio_module> >(5, 10000);
}

void reverbir_calc()
{
    enum { LEN = 1048576 };
    static float data[2][LEN];
    
    for (int t = 1; t < 38; t++)
    {
        reverb<float> rvb;
        
        memset(data, 0, sizeof(data));
        data[0][0] = 1;
        data[1][0] = 0;
        
        rvb.set_cutoff(22000);
        rvb.setup(44100);
        rvb.reset();
        rvb.set_fb(t < 19 ? t * 0.05 : 0.905 + (t - 19) * 0.005);
        
        for (int i = 0; i < LEN; i++)
            rvb.process(data[0][i], data[1][i]);

        int i;
        for (i = LEN - 1; i > 0; i--)
        {
            // printf("[%d]=%f\n", i, data[0][i]);
            if (fabs(data[0][i]) < 0.001 && fabs(data[1][i]) < 0.001)
                continue;
            break;
        }
        if (i < LEN - 1)
            printf("%f\t%f\n", rvb.get_fb(), i / 44100.0);
    }
}

#ifdef TEST_OSC
void osctl_test()
{
    string sdata = string("\000\000\000\003123\000test\000\000\000\000\000\000\000\001\000\000\000\002", 24);
    osc_stream is(sdata);
    vector<osc_data> data;
    is.read("bsii", data);
    assert(is.pos == sdata.length());
    assert(data.size() == 4);
    assert(data[0].type == osc_blob);
    assert(data[1].type == osc_string);
    assert(data[2].type == osc_i32);
    assert(data[3].type == osc_i32);
    assert(data[0].strval == "123");
    assert(data[1].strval == "test");
    assert(data[2].i32val == 1);
    assert(data[3].i32val == 2);
    osc_stream os("");
    os.write(data);
    assert(os.buffer == sdata);
    osc_server srv;
    srv.bind("0.0.0.0", 4541);
    
    osc_client cli;
    cli.bind("0.0.0.0", 0);
    cli.set_addr("0.0.0.0", 4541);
    if (!cli.send("/blah", data))
        g_error("Could not send the OSC message");
    
    g_main_loop_run(g_main_loop_new(NULL, FALSE));
}
#endif

int main(int argc, char *argv[])
{
    while(1) {
        int option_index;
        int c = getopt_long(argc, argv, "u:hv", long_options, &option_index);
        if (c == -1)
            break;
        switch(c) {
            case 'h':
            case '?':
                printf("Benchmark suite Calf plugin pack\nSyntax: %s [--help] [--version] [--unit biquad|alignment|effects]\n", argv[0]);
                return 0;
            case 'v':
                printf("%s\n", PACKAGE_STRING);
                return 0;
            case 'u':
                unit = optarg;
                break;
        }
    }
    
#ifdef TEST_OSC
    if (!strcmp(unit, "osc"))
        osctl_test();
#endif
    
    if (!unit || !strcmp(unit, "biquad"))
        biquad_test();
    
    if (!unit || !strcmp(unit, "alignment"))
        alignment_test();

    if (!unit || !strcmp(unit, "effects"))
        effect_test();

    if (!strcmp(unit, "reverbir"))
        reverbir_calc();

    return 0;
}
