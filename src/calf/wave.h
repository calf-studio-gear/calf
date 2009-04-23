/* Calf DSP Library
 * Primitive interface to NAS WAV file operations.
 * Not really useful for now, I've used it for testing previously.
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
 * Boston, MA 02111-1307, USA.
 */

#include <audio/wave.h>

namespace dsp {

template<class T>
bool load_wave(dsp::dynamic_buffer<T> &dest, const char *file_name) {
    WaveInfo *file = WaveOpenFileForReading(file_name);
    typedef dsp::sample_traits<T> st;
    if (file->channels == st::channels && file->bitsPerSample == st::bps) {
        dest.resize(file->numSamples);
        WaveReadFile((char *)dest.data(), dest.size()*sizeof(T), file);
        WaveCloseFile(file);
        return true;
    }
    WaveCloseFile(file);
    fprintf(stderr, "Sorry, need a %d channels and %d bps, got %d channels and %d bps\n", st::channels, st::bps, file->channels, file->bitsPerSample);
    return false;
}

template<class T>
class wave_player {
    T *data;
    bool is_active;
    unsigned int size;
    wpos pos, rate;
public:
    void set_wave(T *_data, int _size) { 
        data = _data;
        size = _size;
        is_active = false;
    }
    void get_wave(T *&_data, int &_size) {
        _data = data;
        _size = size;
    }
    void set_pos(wpos _pos) {
        pos = _pos;
        is_active = true;
    }
    wpos get_pos() {
        return pos;
    }
    void set_rate(wpos _rate) {
        rate = _rate;
    }
    template<class U>U get() {
        U result;
        if (!is_active) {
            dsp::zero(result);
            return result;
        }
        unsigned int ipos = pos.ipart();
        if (ipos>=size) {
            dsp::zero(result);
            is_active = false;
            return result;
        }
        result = pos.lerp_ptr_lookup_int<T, 12, int >(data);
        pos += rate;
        return result;
    }
    bool get_active() {
        return is_active;
    }
};

}
