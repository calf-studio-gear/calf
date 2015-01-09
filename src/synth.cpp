/* Calf DSP Library
 * Generic polyphonic synthesizer framework.
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
#include <calf/synth.h>

using namespace dsp;
using namespace std;

void basic_synth::init_voices(int count)
{
    allocated_voices.init(count);
    active_voices.init(count);
    unused_voices.init(count);
    for (int i = 0; i < count; i++)
    {
        dsp::voice *v = alloc_voice();
        allocated_voices.add(v);
        unused_voices.add(v);
    }
}

void basic_synth::kill_note(int note, int vel, bool just_one)
{
    for_all_voices(it) {
        // preserve sostenuto notes
        if ((*it)->get_current_note() == note && !(sostenuto && (*it)->sostenuto)) {
            (*it)->note_off(vel);
            if (just_one)
                return;
        }
    }
}

dsp::voice *basic_synth::give_voice()
{
    if (active_voices.size() >= polyphony_limit)
        steal_voice();
    if (unused_voices.empty())
        return NULL;
    else {
        dsp::voice *v = unused_voices.pop();
        v->reset();
        return v;
    }   
}

void basic_synth::steal_voice()
{
    dsp::voice *found = NULL;
    float priority = 10000;
    //int idx = 0;
    for_all_voices(i)
    {
        //printf("Voice %d priority %f at %p\n", idx++, (*i)->get_priority(), *i);
        if ((*i)->get_priority() < priority)
        {
            priority = (*i)->get_priority();
            found = *i;
        }
    }
    //printf("Found: %p\n\n", *found);
    if (!found)
        return;
    
    found->steal();
}

void basic_synth::trim_voices()
{
    // count stealable voices
    unsigned int count = 0;
    for_all_voices(i)
    {
        if ((*i)->get_priority() < 10000)
            count++;
    }
    // printf("Count=%d limit=%d\n", count, polyphony_limit);
    // steal any voices above polyphony limit
    if (count > polyphony_limit) {
        for (unsigned int i = 0; i < count - polyphony_limit; i++)
            steal_voice();
    }
}

void basic_synth::note_on(int note, int vel)
{
    if (!vel) {
        note_off(note, 0);
        return;
    }
    bool perc = check_percussion();
    dsp::voice *v = give_voice();
    if (!v)
        return;
    v->setup(sample_rate);
    v->released = false;
    v->sostenuto = false;
    gate.set(note);
    v->note_on(note, vel);
    active_voices.add(v);
    if (perc) {
        percussion_note_on(note, vel);
    }
}

void basic_synth::note_off(int note, int vel)
{
    gate.reset(note);
    if (!hold)
        kill_note(note, vel, false);
}

void basic_synth::on_pedal_release()
{
    for_all_voices(i)
    {
        int note = (*i)->get_current_note();
        if (note < 0 || note > 127)
            continue;
        bool still_held = gate[note];
        // sostenuto pedal released
        if ((*i)->sostenuto && !sostenuto)
        {
            // mark note as non-sostenuto
            (*i)->sostenuto = false;
            // if key still pressed or hold pedal used, hold the note (as non-sostenuto so it can be released later by releasing the key or pedal)
            // if key has been released and hold pedal is not depressed, release the note
            if (!still_held && !hold)
                (*i)->note_off(127);
        }
        else if (!hold && !still_held && !(*i)->released)
        {
            (*i)->released = true;
            (*i)->note_off(127);
        }
    }
}

void basic_synth::control_change(int ctl, int val)
{
    if (ctl == 64) { // HOLD controller
        bool prev = hold;
        hold = (val >= 64);
        if (!hold && prev && !sostenuto) {
            on_pedal_release();
        }
    }
    if (ctl == 66) { // SOSTENUTO controller
        bool prev = sostenuto;
        sostenuto = (val >= 64);
        if (sostenuto && !prev) {
            // SOSTENUTO was pressed - move all notes onto sustain stack
            for_all_voices(i) {
                (*i)->sostenuto = true;
            }
        }
        if (!sostenuto && prev) {
            // SOSTENUTO was released - release all keys which were previously held
            on_pedal_release();
        }
    }
    if (ctl == 123 || ctl == 120) { // all notes off, all sounds off
        if (ctl == 120) { // for "all sounds off", automatically release hold and sostenuto pedal
            control_change(66, 0);
            control_change(64, 0);
        }
        for_all_voices(i)
        {
            if (ctl == 123)
                (*i)->note_off(127);
            else
                (*i)->steal();
        }
    }
    if (ctl == 121) { 
        control_change(1, 0);
        control_change(7, 100);
        control_change(10, 64);
        control_change(11, 127);
        // release hold..hold2
        for (int i = 64; i <= 69; i++)
            control_change(i, 0);
    }
}

void basic_synth::render_to(float (*output)[2], int nsamples)
{
    // render voices, eliminate ones that aren't sounding anymore
    for (dsp::voice **i = active_voices.begin(); i != active_voices.end(); ) {
        dsp::voice *v = *i;
        v->render_to(output, nsamples);
        if (!v->get_active()) {
            i = active_voices.erase(i);
            unused_voices.add(v);
            continue;
        }
        i++;
    }
} 

basic_synth::~basic_synth()
{
    for (voice_array::iterator i = allocated_voices.begin(); i != allocated_voices.end(); ++i)
        delete *i;
}

