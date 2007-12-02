#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <config.h>
#include <calf/giface.h>
#include <calf/modules.h>
#include <calf/benchmark.h>

using namespace std;
using namespace dsp;

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
                printf("Benchmark suite Calf plugin pack\nSyntax: %s [--help] [--version] [--unit biquad|alignment]\n", argv[0]);
                return 0;
            case 'v':
                printf("%s\n", PACKAGE_STRING);
                return 0;
            case 'u':
                unit = optarg;
                break;
        }
    }
    
    if (!unit || !strcmp(unit, "biquad")) {
        do_simple_benchmark<filter_24dB_lp_twopass_d1>();
        do_simple_benchmark<filter_24dB_lp_onepass_d1>();
        do_simple_benchmark<filter_12dB_lp_d1>();
        do_simple_benchmark<filter_24dB_lp_twopass_d2>();
        do_simple_benchmark<filter_24dB_lp_onepass_d2>();
        do_simple_benchmark<filter_12dB_lp_d2>();
    }
    
    if (!unit || !strcmp(unit, "alignment")) {
        do_simple_benchmark<misaligned_double>();
        do_simple_benchmark<aligned_double>();
    }

    return 0;
}
