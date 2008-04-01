/* Calf DSP Library
 * Framework for synthesizer-like plugins. This is based
 * on my earlier work on Drawbar electric organ emulator.
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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __CALF_SYNTH_H
#define __CALF_SYNTH_H

#include <list>
#include <stack>
#include <bitset>
#include "primitives.h"
#include "audio_fx.h"

namespace synth {

/**
 * A kind of set with fast non-ordered iteration, used for storing lists of pressed keys.
 */
class keystack {
private:
    int dcount;
    uint8_t active[128];
    uint8_t states[128];
public:
    keystack() {
        memset(states, 0xFF, sizeof(states));
        dcount = 0;
    }
    void clear() {
        for (int i=0; i<dcount; i++)
            states[active[i]] = 0xFF;
        dcount = 0;
    }
    bool push(int key) {
        assert(key >= 0 && key <= 127);
        if (states[key] != 0xFF) {
            return true;
        }
        states[key] = dcount;
        active[dcount++] = key;
        return false;
    }
    bool pop(int key) {
        if (states[key] == 0xFF)
            return false;
        int pos = states[key];
        if (pos != dcount-1) {
            // reuse the popped item's stack position for stack top
            int last = active[dcount-1];
            active[pos] = last;
            // mark that position's new place on stack
            states[last] = pos;
        }
        states[key] = 0xFF;
        dcount--;
        return true;
    }    
    inline bool has(int key) {
        return states[key] != 0xFF;
    }
    inline int count() {
        return dcount;
    }
    inline bool empty() {
        return (dcount == 0);
    }
    inline int nth(int n) {
        return active[n];
    }
};

/**
 * Convert MIDI note number to normalized UINT phase (where 1<<32 is full cycle).
 * @param MIDI note number
 * @param cents detune in cents (1/100 of a semitone)
 * @param sr sample rate
 */
inline unsigned int midi_note_to_phase(int note, double cents, int sr) {
    double incphase = 440*pow(2.0, (note-69)/12.0 + cents/1200.0)/sr;
    if (incphase >= 1.0) incphase = fmod(incphase, 1.0);
    incphase *= 65536.0*65536.0;
    return (unsigned int)incphase;
}

// Base class for all voice objects
class voice {
public:
    int sample_rate;
    bool released, sostenuto;

    voice() : sample_rate(-1), released(false), sostenuto(false) {}

    /// reset voice to default state (used when a voice is to be reused)
    virtual void setup(int sr) { sample_rate = sr; }
    /// reset voice to default state (used when a voice is to be reused)
    virtual void reset()=0;
    /// a note was pressed
    virtual void note_on(int note, int vel)=0;
    /// a note was released
    virtual void note_off(int vel)=0;
    /// check if voice can be removed from active voice list
    virtual bool get_active()=0;
    /// render voice data to buffer
    virtual void render_to(float *buf[], int nsamples)=0;
    /// return the note used by this voice
    virtual int get_current_note()=0;
    /// empty virtual destructor
    virtual ~voice() {}
};

/// Base class for all kinds of polyphonic instruments, provides
/// somewhat reasonable voice management, pedal support - and 
/// little else. It's implemented as a base class with virtual
/// functions, so there's some performance loss, but it shouldn't
/// be horrible.
/// @todo it would make sense to support all notes off controller too
struct basic_synth {
protected:
    int sample_rate;
    bool hold, sostenuto;
    std::list<synth::voice *> active_voices;
    std::stack<synth::voice *> unused_voices;
    std::bitset<128> gate;

    void kill_note(int note, int vel, bool just_one);
public:
    virtual void setup(int sr) {
        sample_rate = sr;
        hold = false;
        sostenuto = false;
    }
    virtual synth::voice *give_voice();
    virtual synth::voice *alloc_voice()=0;
    virtual void render_to(float *output[], int nsamples);
    virtual void note_on(int note, int vel);
    virtual void first_note_on(int note, int vel) {}
    virtual void control_change(int ctl, int val);
    virtual void note_off(int note, int vel);
    virtual void on_pedal_release();
    virtual ~basic_synth();
};

// temporarily here, will be moved to separate file later
// this is a weird envelope and perhaps won't turn out to
// be a good idea in the long run, still, worth trying
class adsr
{
public:
    enum env_state { STOP, ATTACK, DECAY, SUSTAIN, RELEASE, LOCKDECAY };
    
    env_state state;
    // note: these are *rates*, not times
    double attack, decay, sustain, release, release_time;
    double value, thisrelease, thiss;
    
    adsr()
    {
        attack = decay = sustain = release = thisrelease = thiss = 0.f;
        reset();
    }
    inline void reset()
    {
        value = 0.f;
        thiss = 0.f;
        state = STOP;
    }
    inline void set(float a, float d, float s, float r, float er)
    {
        attack = 1.0 / (a * er);
        decay = (1 - s) / (d * er);
        sustain = s;
        release_time = r * er;
        release = s / release_time;
        // in release:
        // lock thiss setting (start of release for current note) and unlock thisrelease setting (current note's release rate)
        if (state != RELEASE)
            thiss = s;
        else
            thisrelease = thiss / release_time;
    }
    inline bool released()
    {
        return state == LOCKDECAY || state == RELEASE || state == STOP;
    }
    inline void note_on()
    {
        state = ATTACK;
        thiss = sustain;
    }
    inline void note_off()
    {
        if (state == STOP)
            return;
        thiss = std::max(sustain, value);
        thisrelease = thiss / release_time;
        // we're in attack or decay, and if decay is faster than release
        if (value > sustain && decay > thisrelease) {
            // use standard release time later (because we'll be switching at sustain point)
            thisrelease = release;
            state = LOCKDECAY;
        } else {
            // in attack/decay, but use fixed release time
            // in case value fell below sustain, assume it didn't (for the purpose of calculating release rate only)
            state = RELEASE;
        }
    }
    inline void advance()
    {
        switch(state)
        {
        case ATTACK:
            value += attack;
            if (value >= 1.0) {
                value = 1.0;
                state = DECAY;
            }
            break;
        case DECAY:
            value -= decay;
            if (value < sustain)
            {
                value = sustain;
                state = SUSTAIN;
            }
            break;
        case LOCKDECAY:
            value -= decay;
            if (value < sustain)
            {
                if (value < 0.f)
                    value = 0.f;
                state = RELEASE;
                thisrelease = release;
            }
            break;
        case SUSTAIN:
            value = sustain;
            if (value < 0.00001f) {
                value = 0;
                state = STOP;
            }
            break;
        case RELEASE:
            value -= thisrelease;
            if (value <= 0.f) {
                value = 0.f;
                state = STOP;
            }
            break;
        case STOP:
            value = 0.f;
            break;
        }
    }
};


}

#endif
