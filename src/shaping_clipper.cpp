/* Calf DSP plugin pack
 * Distortion-shaping clipper
 *
 * Copyright (C) 2014-2021 Jason Jang
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
#include <calf/shaping_clipper.h>
#include <algorithm>
#include <complex>
#include <cmath>

shaping_clipper::shaping_clipper(int sample_rate, int fft_size, float clip_level) {
    this->sample_rate = sample_rate;
    this->size = fft_size;
    this->clip_level = clip_level;
    this->iterations = 6;
    this->adaptive_distortion_strength = 1.0;
    this->overlap = fft_size / 4;
    this->pffft = pffft_new_setup(fft_size, PFFFT_REAL);

    // The psy masking calculation is O(n^2),
    // so skip it for frequencies not covered by base sampling rantes (i.e. 44k)
    if (sample_rate <= 50000) {
        this->num_psy_bins = fft_size / 2;
    } else if (sample_rate <= 100000) {
        this->num_psy_bins = fft_size / 4;
    } else {
        this->num_psy_bins = fft_size / 8;
    }

    this->window.resize(fft_size);
    this->inv_window.resize(fft_size);
    generate_hann_window();

    this->in_frame.resize(fft_size);
    this->out_dist_frame.resize(fft_size);
    this->margin_curve.resize(fft_size / 2 + 1);
    // normally I use __builtin_ctz for this but the intrinsic is different for
    // different compilers.
    // 2 spread_table entries per octave.
    int spread_table_rows = log2(this->num_psy_bins) * 2;
    this->spread_table.resize(spread_table_rows * this->num_psy_bins);
    this->spread_table_range.resize(spread_table_rows);
    this->spread_table_index.resize(this->num_psy_bins);

    // default curve
    int points[][2] = { {0,14}, {125,14}, {250,16}, {500,18}, {1000,20}, {2000,20}, {4000,20}, {8000,15}, {16000,5}, {20000,-10} };
    int num_points = 10;
    set_margin_curve(points, num_points);
    generate_spread_table();
}

shaping_clipper::~shaping_clipper() {
    pffft_destroy_setup(this->pffft);
}

int shaping_clipper::get_feed_size() {
    return this->overlap;
}

void shaping_clipper::set_clip_level(float clip_level) {
    this->clip_level = clip_level;
}

void shaping_clipper::set_iterations(int iterations) {
    this->iterations = iterations;
}

void shaping_clipper::set_adaptive_distortion_strength(float strength) {
    this->adaptive_distortion_strength = strength;
}

void shaping_clipper::feed(const float* in_samples, float* out_samples, bool diff_only, float* total_margin_shift) {
    // shift in/out buffers
    for (int i = 0; i < this->size - this->overlap; i++) {
        this->in_frame[i] = this->in_frame[i + this->overlap];
        this->out_dist_frame[i] = this->out_dist_frame[i + this->overlap];
    }
    for (int i = 0; i < this->overlap; i++) {
        this->in_frame[i + this->size - this->overlap] = in_samples[i];
        this->out_dist_frame[i + this->size - this->overlap] = 0;
    }

    float peak;
    float* windowed_frame = (float*)alloca(sizeof(float) * this->size);
    float* clipping_delta = (float*)alloca(sizeof(float) * this->size);
    float* spectrum_buf = (float*)alloca(sizeof(float) * this->size);
    float* mask_curve = (float*)alloca(sizeof(float) * this->size / 2 + 1);

    apply_window(this->in_frame.data(), windowed_frame);
    pffft_transform_ordered(this->pffft, windowed_frame, spectrum_buf, NULL, PFFFT_FORWARD);
    calculate_mask_curve(spectrum_buf, mask_curve);

    // It would be easier to calculate the peak from the unwindowed input.
    // This is just for consistency with the clipped peak calculateion
    // because the inv_window zeros out samples on the edge of the window.
    float orig_peak = 0;
    for (int i = 0; i < this->size; i++) {
        orig_peak = std::max<float>(orig_peak, std::abs(windowed_frame[i] * inv_window[i]));
    }
    orig_peak /= this->clip_level;
    peak = orig_peak;

    // clear clipping_delta
    for (int i = 0; i < this->size; i++) {
        clipping_delta[i] = 0;
    }

    if (total_margin_shift) {
        *total_margin_shift = 1.0;
    }

    // repeat clipping-filtering process a few times to control both the peaks and the spectrum
    for (int i = 0; i < this->iterations; i++) {
        // The last 1/3 of rounds have boosted delta to help reach the peak target faster
        float delta_boost = 1.0;
        if (i >= this->iterations - this->iterations / 3) {
	  // boosting the delta when largs peaks are still present is dangerous
	  if (peak < 2.0) {
	    delta_boost = 2.0;
	  }
	}
        clip_to_window(windowed_frame, clipping_delta, delta_boost);

        pffft_transform_ordered(this->pffft, clipping_delta, spectrum_buf, NULL, PFFFT_FORWARD);

        limit_clip_spectrum(spectrum_buf, mask_curve);

        pffft_transform_ordered(this->pffft, spectrum_buf, clipping_delta, NULL, PFFFT_BACKWARD);
        // see pffft.h
        for (int i = 0; i < this->size; i++) {
            clipping_delta[i] /= this->size;
        }

        peak = 0;
        for (int i = 0; i < this->size; i++)
            peak = std::max<float>(peak, std::abs((windowed_frame[i] + clipping_delta[i]) * inv_window[i]));
        peak /= this->clip_level;

        // Automatically adjust mask_curve as necessary to reach peak target
        float mask_curve_shift = 1.122; // 1.122 is 1dB
        if (orig_peak > 1.0 && peak > 1.0) {
            float diff_achieved = orig_peak - peak;
            if (i + 1 < this->iterations - this->iterations / 3 && diff_achieved > 0) {
                float diff_needed = orig_peak - 1.0;
                float diff_ratio = diff_needed / diff_achieved;
                // If a good amount of peak reduction was already achieved,
                // don't shift the mask_curve by the full peak value
                // On the other hand, if only a little peak reduction was achieved,
                // don't shift the mask_curve by the enormous diff_ratio.
                diff_ratio = std::min<float>(diff_ratio, peak);
                mask_curve_shift = std::max<float>(mask_curve_shift, diff_ratio);
            } else {
                // If the peak got higher than the input or we are in the last 1/3 rounds,
                // go back to the heavy-handed peak heuristic.
                mask_curve_shift = std::max<float>(mask_curve_shift, peak);
            }
        }

        mask_curve_shift = 1.0 + (mask_curve_shift - 1.0) * this->adaptive_distortion_strength;

        if (total_margin_shift && peak > 1.01 && i < this->iterations - 1) {
            *total_margin_shift *= mask_curve_shift;
        }

        // Be less strict in the next iteration.
        // This helps with peak control.
        for (int i = 0; i < this->size / 2 + 1; i++) {
            mask_curve[i] *= mask_curve_shift;
        }
    }

    // do overlap & add
    apply_window(clipping_delta, this->out_dist_frame.data(), true);

    for (int i = 0; i < this->overlap; i++) {
        out_samples[i] = this->out_dist_frame[i] / 1.5;
        // 4 times overlap with squared hanning window results in 1.5 time increase in amplitude
        if (!diff_only) {
            out_samples[i] += this->in_frame[i];
        }
    }
}

void shaping_clipper::generate_hann_window() {
    double pi = acos(-1);
    for (int i = 0; i < this->size; i++) {
        float value = 0.5 * (1 - cos(2 * pi * i / this->size));
        this->window[i] = value;
        // 1/window to calculate unwindowed peak.
        this->inv_window[i] = value > 0.1 ? 1.0 / value : 0;
    }
}

void shaping_clipper::generate_spread_table() {
    // Calculate tent-shape function in log-log scale.

    // As an optimization, only consider bins close to "bin"
    // This reduces the number of multiplications needed in calculate_mask_curve
    // The masking contribution at faraway bins is negligeable

    // Another optimization to save memory and speed up the calculation of the
    // spread table is to calculate and store only 2 spread functions per
    // octave, and reuse the same spread function for multiple bins.
    int table_index = 0;
    int bin = 0;
    int increment = 1;
    while (bin < this->num_psy_bins) {
        float sum = 0;
        int base_idx = table_index * this->num_psy_bins;
        int start_bin = bin * 3 / 4;
        int end_bin = std::min(this->num_psy_bins, ((bin + 1) * 4 + 2) / 3);

        for (int j = start_bin; j < end_bin; j++) {
            // add 0.5 so i=0 doesn't get log(0)
            float rel_idx_log = std::abs(log((j + 0.5) / (bin + 0.5)));
            float value;
            if (j >= bin) {
                // mask up
                value = exp(-rel_idx_log * 40);
            } else {
                // mask down
                value = exp(-rel_idx_log * 80);
            }
            // the spreading function is centred in the row
            sum += value;
            this->spread_table[base_idx + this->num_psy_bins / 2 + j - bin] = value;
        }
        // now normalize it
        for (int j = start_bin; j < end_bin; j++) {
            this->spread_table[base_idx + this->num_psy_bins / 2 + j - bin] /= sum;
        }

        this->spread_table_range[table_index] = std::make_pair(start_bin - bin, end_bin - bin);

        int next_bin;
        if (bin <= 1) {
            next_bin = bin + 1;
        } else {
            if ((bin & (bin - 1)) == 0) {
                // power of 2
                increment = bin / 2;
            }
            next_bin = bin + increment;
        }

        // set bins between "bin" and "next_bin" to use this table_index
        for (int i = bin; i < next_bin; i++) {
            this->spread_table_index[i] = table_index;
        }

        bin = next_bin;
        table_index++;
    }
}

void shaping_clipper::set_margin_curve(int points[][2], int num_points) {
    this->margin_curve[0] = points[0][1];

    int j = 0;
    for (int i = 0; i < num_points - 1; i++) {
        while (j < this->size / 2 + 1 && j * this->sample_rate / this->size < points[i + 1][0]) {
            // linearly interpolate between points
            int binHz = j * this->sample_rate / this->size;
            margin_curve[j] = points[i][1] + (binHz - points[i][0]) * (points[i + 1][1] - points[i][1]) / (points[i + 1][0] - points[i][0]);
            j++;
        }
    }
    // handle bins after the last point
    while (j < this->size / 2 + 1) {
        margin_curve[j] = points[num_points - 1][1];
        j++;
    }

    // convert margin curve to linear amplitude scale
    for (j = 0; j < this->size / 2 + 1; j++) {
        margin_curve[j] = pow(10, margin_curve[j] / 20);
    }
}

void shaping_clipper::apply_window(const float* in_frame, float* out_frame, const bool add_to_out_frame) {
    const float* window = this->window.data();
    for (int i = 0; i < this->size; i++) {
        if (add_to_out_frame) {
            out_frame[i] += in_frame[i] * window[i];
        } else {
            out_frame[i] = in_frame[i] * window[i];
        }
    }
}

void shaping_clipper::clip_to_window(const float* windowed_frame, float* clipping_delta, float delta_boost) {
    const float* window = this->window.data();
    for (int i = 0; i < this->size; i++) {
        float limit = this->clip_level * window[i];
        float effective_value = windowed_frame[i] + clipping_delta[i];
        if (effective_value > limit) {
            clipping_delta[i] += (limit - effective_value) * delta_boost;
        } else if (effective_value < -limit) {
            clipping_delta[i] += (-limit - effective_value) * delta_boost;
        }
    }
}

void shaping_clipper::calculate_mask_curve(const float* spectrum, float* mask_curve) {
    for (int i = 0; i < this->size / 2 + 1; i++) {
        mask_curve[i] = 0;
    }
    for (int i = 0; i < this->num_psy_bins; i++) {
        float magnitude;
        if (i == 0) {
            magnitude = std::abs(spectrum[0]);
        } else if (i == this->size / 2) {
            magnitude = std::abs(spectrum[1]);
        } else {
            // although the negative frequencies are omitted because they are redundant,
            // the magnitude of the positive frequencies are not doubled.
            // Multiply the magnitude by 2 to simulate adding up the + and - frequencies.
            magnitude = abs(std::complex<float>(spectrum[2 * i], spectrum[2 * i + 1])) * 2;
        }
        int table_idx = this->spread_table_index[i];
        std::pair<int, int> range = this->spread_table_range[table_idx];
        int base_idx = table_idx * this->num_psy_bins;
        int start_bin = std::max(0, i + range.first);
        int end_bin = std::min(this->num_psy_bins, i + range.second);
        for (int j = start_bin; j < end_bin; j++) {
            mask_curve[j] += this->spread_table[base_idx + this->num_psy_bins / 2 + j - i] * magnitude;
        }
    }

    // for ultrasonic frequencies, skip the O(n^2) spread calculation and just copy the magnitude
    for (int i = this->num_psy_bins; i < this->size / 2 + 1; i++) {
        float magnitude;
        if (i == this->size / 2) {
            magnitude = std::abs(spectrum[1]);
        } else {
            // although the negative frequencies are omitted because they are redundant,
            // the magnitude of the positive frequencies are not doubled.
            // Multiply the magnitude by 2 to simulate adding up the + and - frequencies.
            magnitude = abs(std::complex<float>(spectrum[2 * i], spectrum[2 * i + 1])) * 2;
        }
        mask_curve[i] = magnitude;
    }

    for (int i = 0; i < this->size / 2 + 1; i++) {
        mask_curve[i] = mask_curve[i] / this->margin_curve[i];
    }
}

void shaping_clipper::limit_clip_spectrum(float* clip_spectrum, const float* mask_curve) {
    // bin 0
    float relative_distortion_level = std::abs(clip_spectrum[0]) / mask_curve[0];
    if (relative_distortion_level > 1.0) {
        clip_spectrum[0] /= relative_distortion_level;
    }
    // bin 1..N/2-1
    for (int i = 1; i < this->size / 2; i++) {
        float real = clip_spectrum[i * 2];
        float imag = clip_spectrum[i * 2 + 1];
        // although the negative frequencies are omitted because they are redundant,
        // the magnitude of the positive frequencies are not doubled.
        // Multiply the magnitude by 2 to simulate adding up the + and - frequencies.
        relative_distortion_level = abs(std::complex<float>(real, imag)) * 2 / mask_curve[i];
        if (relative_distortion_level > 1.0) {
            clip_spectrum[i * 2] /= relative_distortion_level;
            clip_spectrum[i * 2 + 1] /= relative_distortion_level;
        }
    }
    // bin N/2
    relative_distortion_level = std::abs(clip_spectrum[1]) / mask_curve[this->size / 2];
    if (relative_distortion_level > 1.0) {
        clip_spectrum[1] /= relative_distortion_level;
    }
}
