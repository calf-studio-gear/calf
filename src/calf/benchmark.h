/* Calf DSP Library
 * Reusable performance measurement classes.
 *
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
#ifndef __CALF_BENCHMARK_H
#define __CALF_BENCHMAR_H

#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include "primitives.h"
#include <algorithm>
#include <typeinfo>

namespace dsp {
#if 0
}; to keep editor happy
#endif


class median_stat
{
public:
    double *data;
    unsigned int pos, count;
    bool sorted;
    
    median_stat() {
        data = NULL;
        pos = 0;
        count = 0;
        sorted = false;
    }
    void start(int items) {
        if (data)
            delete []data;
        data = new double[items];
        pos = 0;
        count = items;
        sorted = false;
    }
    void add(double value)
    {
        assert(pos < count);
        data[pos++] = value;
    }
    void end()
    {
        std::sort(&data[0], &data[count]);
        sorted = true;
    }
    float get()
    {
        assert(sorted);
        return data[count >> 1];
    }
};

// USE_RDTSC is for testing on my own machine, a crappy 1.6GHz Pentium 4 - it gives less headaches than clock() based measurements
#define USE_RDTSC 0
#define CLOCK_SPEED (1.6 * 1000.0 * 1000.0 * 1000.0)

#if USE_RDTSC
inline uint64_t rdtsc()
{
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}
#endif

struct benchmark_globals
{
    static bool warned;
};

template<typename Target, class Stat>
class simple_benchmark: public benchmark_globals
{
public:
    Target target;
    Stat &stat;

    simple_benchmark(const Target &_target, Stat &_stat)
    : target(_target)
    , stat(_stat)
    {
    }
    
    void measure(int runs, int repeats)
    {
        int priority = getpriority(PRIO_PROCESS, getpid());
        stat.start(runs);
        if (setpriority(PRIO_PROCESS, getpid(), -20) < 0) {
            if (!warned) {
                fprintf(stderr, "Warning: could not set process priority, measurements can be worthless\n");
                warned = true;
            }
        }
        for (int i = 0; i < runs; i++) {
            target.prepare();
            // XXXKF measuring CPU time with clock() sucks, 
#if USE_RDTSC
            uint64_t start = rdtsc();
#else
            clock_t start = ::clock();
#endif
            for (int j = 0; j < repeats; j++) {
                target.run();
            }
#if USE_RDTSC
            uint64_t end = rdtsc();
            double elapsed = double(end - start) / (CLOCK_SPEED * repeats * target.scaler());
#else
            clock_t end = ::clock();
            double elapsed = double(end - start) / (double(CLOCKS_PER_SEC) * repeats * target.scaler());
#endif
            stat.add(elapsed);
            target.cleanup();
            // printf("elapsed = %f start = %d end = %d diff = %d\n", elapsed, start, end, end - start);
        }
        setpriority(PRIO_PROCESS, getpid(), priority);
        stat.end();
    }
    
    float get_stat()
    {
        return stat.get();
    }
};

template<class T>
void do_simple_benchmark(int runs = 5, int repeats = 50000)
{
    dsp::median_stat stat;
    dsp::simple_benchmark<T, dsp::median_stat> benchmark(T(), stat);
    
    benchmark.measure(runs, repeats);
    
    printf("%-30s: %f/sec, %f/CDsr, value = %f\n", typeid(T).name(), 1.0 / stat.get(), 1.0 / (44100 * stat.get()), benchmark.target.result);
}


#if 0
{ to keep editor happy
#endif
}

#endif
