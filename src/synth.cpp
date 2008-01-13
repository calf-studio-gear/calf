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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <config.h>
#include <assert.h>
#include <memory.h>
#if USE_JACK
#include <jack/jack.h>
#endif
#include <calf/giface.h>
#include <calf/synth.h>

using namespace dsp;
using namespace synth;

void basic_synth::kill_note(int note, int vel, bool just_one)
{
    for (list<synth::voice *>::iterator it = active_voices.begin(); it != active_voices.end(); it++) {
        if ((*it)->get_current_note() == note) {
            (*it)->note_off(vel);
            if (just_one)
                return;
        }
    }
}

synth::voice *basic_synth::give_voice()
{
    if (unused_voices.empty())
        return alloc_voice();
    else {
        synth::voice *v = unused_voices.top();
        unused_voices.pop();
        v->reset();
        return v;
    }   
}

void basic_synth::note_on(int note, int vel)
{
    if (!vel) {
        note_off(note, 0);
        return;
    }
    bool perc = keystack_hold.empty() && keystack.empty();
    synth::voice *v = alloc_voice();
    v->setup(sample_rate);
    if (hold)
        keystack_hold.push(note);
    else
        keystack.push(note);
    gate.set(note);
    v->note_on(note, vel);
    active_voices.push_back(v);
    if (perc) {
        first_note_on(note, vel);
    }
}

void basic_synth::note_off(int note, int vel) {
    gate.reset(note);
    if (keystack.pop(note)) {
        kill_note(note, vel, keystack_hold.has(note));
    }        
}
    
void basic_synth::control_change(int ctl, int val)
{
    if (ctl == 64) { // HOLD controller
        bool prev = hold;
        hold = (val >= 64);
        if (!hold && prev && !sostenuto) {
            // HOLD was released - release all keys which were previously held
            for (int i=0; i<keystack_hold.count(); i++) {
                int note = keystack_hold.nth(i);
                if (!gate.test(note)) {
                    kill_note(note, 0, false);
                    keystack_hold.pop(note);
                    i--;
                }
            }
            for (int i=0; i<keystack_hold.count(); i++)
                keystack.push(keystack_hold.nth(i));
            keystack_hold.clear();
        }
    }
    if (ctl == 66) { // SOSTENUTO controller
        bool prev = sostenuto;
        sostenuto = (val >= 64);
        if (sostenuto && !prev) {
            // SOSTENUTO was pressed - move all notes onto sustain stack
            for (int i=0; i<keystack.count(); i++) {
                keystack_hold.push(keystack.nth(i));
            }
            keystack.clear();
        }
        if (!sostenuto && prev) {
            // SOSTENUTO was released - release all keys which were previously held
            for (int i=0; i<keystack_hold.count(); i++) {
                kill_note(keystack_hold.nth(i), 0, false);
            }
            keystack_hold.clear();
        }
    }
}

void basic_synth::render_to(float *output[], int nsamples)
{
    // render voices, eliminate ones that aren't sounding anymore
    for (list<synth::voice *>::iterator i = active_voices.begin(); i != active_voices.end();) {
        synth::voice *v = *i;
        v->render_to(output, nsamples);
        if (!v->get_active()) {
            i = active_voices.erase(i);
            unused_voices.push(v);
            continue;
        }
        i++;
    }
} 

basic_synth::~basic_synth()
{
    while(!unused_voices.empty()) {
        delete unused_voices.top();
        unused_voices.pop();
    }
    for (list<voice *>::iterator i = active_voices.begin(); i != active_voices.end(); i++)
        delete *i;
}

