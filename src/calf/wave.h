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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
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

}
