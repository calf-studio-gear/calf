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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
 
#define BENCHMARK_PLUGINS

#ifdef BENCHMARK_PLUGINS
#include <calf/giface.h>
#include <calf/modules.h>
#include <calf/modules_comp.h>
#include <calf/modules_dev.h>
#include <calf/modules_eq.h>
#include <calf/modules_mod.h>
#else
#include <config.h>
#endif

#include <calf/audio_fx.h>
#include <calf/fft.h>
#include <calf/loudness.h>
#include <calf/benchmark.h>
#include <getopt.h>

// #define TEST_OSC

using namespace std;
using namespace dsp;

#ifdef TEST_OSC
#include <calf/osctl.h>
#include <calf/osctlnet.h>
#include <calf/osctl_glib.h>
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

template<class filter_class>
struct filter_lp24dB_benchmark
{
    enum { BUF_SIZE = 256 };
    float buffer[BUF_SIZE];
    float result;
    filter_class biquad, biquad2;
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

struct filter_24dB_lp_twopass_d1: public filter_lp24dB_benchmark<biquad_d1<> >
{
    void run()
    {
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad.process(buffer[i]);
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad2.process(buffer[i]);
    }
};

struct filter_24dB_lp_onepass_d1: public filter_lp24dB_benchmark<biquad_d1<> >
{
    void run()
    {
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad2.process(biquad.process(buffer[i]));
    }
};

struct filter_12dB_lp_d1: public filter_lp24dB_benchmark<biquad_d1<> >
{
    void run()
    {
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad.process(buffer[i]);
    }
};

struct filter_24dB_lp_twopass_d2: public filter_lp24dB_benchmark<biquad_d2<> >
{
    void run()
    {
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad.process(buffer[i]);
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad2.process(buffer[i]);
    }
};

struct filter_24dB_lp_onepass_d2: public filter_lp24dB_benchmark<biquad_d2<> >
{
    void run()
    {
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad2.process(biquad.process(buffer[i]));
    }
};

struct filter_24dB_lp_onepass_d2_lp: public filter_lp24dB_benchmark<biquad_d2<> >
{
    void run()
    {
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad2.process_lp(biquad.process_lp(buffer[i]));
    }
};

struct filter_12dB_lp_d2: public filter_lp24dB_benchmark<biquad_d2<> >
{
    void run()
    {
        for (int i = 0; i < BUF_SIZE; i++)
            buffer[i] = biquad.process(buffer[i]);
    }
};

template<int N>
struct fft_test_class
{
    typedef fft<float, N> fft_class;
    fft_class ffter;
    float result;
    complex<float> data[1 << N], output[1 << N];
    void prepare() {
        for (int i = 0; i < (1 << N); i++)
            data[i] = sin(i);
        result = 0;
    }
    void cleanup()
    {
    }
    void run()
    {
        ffter.calculate(data, output, false);
    }
    double scaler() { return 1 << N; }
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

void fft_test()
{
        do_simple_benchmark<fft_test_class<17> >(5, 10);
}

void alignment_test()
{
        do_simple_benchmark<misaligned_double>();
        do_simple_benchmark<aligned_double>();
}

#ifdef BENCHMARK_PLUGINS
template<class Effect>
void get_default_effect_params(float params[Effect::param_count], uint32_t &sr);

template<>
void get_default_effect_params<calf_plugins::reverb_audio_module>(float params[3], uint32_t &sr)
{
    typedef calf_plugins::reverb_audio_module mod;
    params[mod::par_decay] = 4;
    params[mod::par_hfdamp] = 2000;
    params[mod::par_amount] = 2;
    sr = 4000;
}

template<>
void get_default_effect_params<calf_plugins::filter_audio_module>(float params[4], uint32_t &sr)
{
    typedef calf_plugins::filter_audio_module mod;
    params[mod::par_cutoff] = 500;
    params[mod::par_resonance] = 3;
    params[mod::par_mode] = 2;
    params[mod::par_inertia] = 16;
    sr = 4000;
}

template<>
void get_default_effect_params<calf_plugins::flanger_audio_module>(float params[5], uint32_t &sr)
{
    typedef calf_plugins::flanger_audio_module mod;
    params[mod::par_delay] = 1;
    params[mod::par_depth] = 2;
    params[mod::par_rate] = 1;
    params[mod::par_fb] = 0.9;
    params[mod::par_amount] = 0.9;
    sr = 44100;
}

template<>
void get_default_effect_params<calf_plugins::compressor_audio_module>(float params[], uint32_t &sr)
{
    typedef calf_plugins::compressor_audio_module mod;
    params[mod::param_threshold] = 10;
    params[mod::param_ratio] = 2;
    params[mod::param_attack] = 0.1;
    params[mod::param_release] = 100;
    params[mod::param_makeup] = 1;
    params[mod::param_stereo_link] = 1;
    params[mod::param_detection] = 0;
    params[mod::param_knee] = 40000;
    params[mod::param_bypass] = 0;
    sr = 44100;
}

template<>
void get_default_effect_params<calf_plugins::multichorus_audio_module>(float params[], uint32_t &sr)
{
    typedef calf_plugins::multichorus_audio_module mod;
    params[mod::par_delay] = 10;
    params[mod::par_depth] = 10;
    params[mod::par_rate] = 1;
    params[mod::par_voices] = 6;
    params[mod::par_stereo] = 180;
    params[mod::par_vphase] = 20;
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
    dsp::do_simple_benchmark<effect_benchmark<calf_plugins::flanger_audio_module> >(5, 10000);
    dsp::do_simple_benchmark<effect_benchmark<calf_plugins::reverb_audio_module> >(5, 1000);
    dsp::do_simple_benchmark<effect_benchmark<calf_plugins::filter_audio_module> >(5, 10000);
    dsp::do_simple_benchmark<effect_benchmark<calf_plugins::compressor_audio_module> >(5, 10000);
    dsp::do_simple_benchmark<effect_benchmark<calf_plugins::multichorus_audio_module> >(5, 10000);
}

#else
void effect_test()
{
    printf("Test temporarily removed due to refactoring\n");
}
#endif
void reverbir_calc()
{
    enum { LEN = 1048576 };
    static float data[2][LEN];
    
    for (int t = 1; t < 38; t++)
    {
        dsp::reverb rvb;
        
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

void eq_calc()
{
    biquad_coeffs<float> bqc;
    bqc.set_lowshelf_rbj(2000, 2.0, 4.0, 10000);
    for (int i = 0; i <= 5000; i += 100)
    {
        printf("%d %f\n", i, bqc.freq_gain(i * 1.0, 10000));
    }
}

void aweighting_calc()
{
    aweighter aw;
    float fs = 44100;
    aw.set(fs);
    for (int i = 10; i < 20000; i += 10)
    {
        printf("%d %f\n", i, 20*log10(aw.freq_gain(i * 1.0, fs)));
    }
}

#ifdef TEST_OSC

struct my_sink: public osc_message_sink<osc_strstream>
{
    GMainLoop *loop;
    osc_message_dump<osc_strstream, ostream> dump;
    my_sink() : dump(cout) {}
    virtual void receive_osc_message(std::string address, std::string type_tag, osc_strstream &buffer)
    {
        dump.receive_osc_message(address, type_tag, buffer);
        assert(address == "/blah");
        assert(type_tag == "bsii");

        string_buffer blob;
        string str;
        uint32_t val1, val2;
        buffer >> blob >> str >> val1 >> val2;
        assert(blob.data == "123");
        assert(str == "test");
        assert(val1 == 1);
        assert(val2 == 2);
        g_main_loop_quit(loop);
    }
    
};

void osctl_test()
{
    string sdata = string("\000\000\000\003123\000test\000\000\000\000\000\000\000\001\000\000\000\002", 24);
    string_buffer sb(sdata);
    osc_strstream is(sb);
    osc_inline_typed_strstream os;
    
    string_buffer blob;
    string str;
    uint32_t val1, val2;
    is >> blob >> str >> val1 >> val2;
    assert(blob.data == "123");
    assert(str == "test");
    assert(val1 == 1);
    assert(val2 == 2);
    
    os << blob << str << val1 << val2;
    assert(os.buf_data.data == sdata);
    assert(os.buf_types.data == "bsii");
    
    GMainLoop *main_loop = g_main_loop_new(NULL, FALSE);
    my_sink sink;
    sink.loop = main_loop;
    osc_server srv;
    srv.sink = &sink;
    srv.bind("0.0.0.0", 4541);
    
    osc_client cli;
    cli.bind("0.0.0.0", 0);
    cli.set_addr("0.0.0.0", 4541);
    if (!cli.send("/blah", os))
    {
        g_error("Could not send the OSC message");
    }
    
    g_main_loop_run(main_loop);
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
    if (unit && !strcmp(unit, "osc"))
        osctl_test();
#endif
    
    if (!unit || !strcmp(unit, "biquad"))
        biquad_test();
    
    if (!unit || !strcmp(unit, "alignment"))
        alignment_test();

    if (!unit || !strcmp(unit, "effects"))
        effect_test();

    if (unit && !strcmp(unit, "reverbir"))
        reverbir_calc();

    if (unit && !strcmp(unit, "eq"))
        eq_calc();

    if (unit && !strcmp(unit, "aweighting"))
        aweighting_calc();

    if (!unit || !strcmp(unit, "fft"))
        fft_test();
    
    return 0;
}
