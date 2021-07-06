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
#include <vector>
#include "pffft.h"

class shaping_clipper
{
public:

    /**
     *  sample_rate is only used to generate the margin_curve.
     *  fftSize should be a multiple of 4.
     *  clip_level means symmetric clipping from -clip_level to +clip_level
     */
    shaping_clipper(int sample_rate, int fft_size, float clip_level = 16384);
    ~shaping_clipper();

    /**
     *  Put fftSize/4 samples in in_smaples and get fftSize/4 samples out in out_samples.
     *  The output in out_samples corresponds to the input 3 calls ago.
     *  total_margin_shift is an optional parameter that can be used to get the total margin adjustment in the frame
     */
    void feed(const float* in_smaples, float* out_samples, bool diff_only = false, float* total_margin_shift = NULL);

    /**
     *  Returns fftSize/4
     */
    int get_feed_size();

    /**
     *  Set the clipping level in sample value
     */
    void set_clip_level(float clip_level);

    /**
     *  Set the number of clipping iterations.
     *  Setting iterations to 0 effectively works as a bypass.
     */
    void set_iterations(int iterations);

    /**
     *  Set the adaptive distortion strength.
     *  The adaptive distortion strength affects how easily the clipper gives up
     *  distortion control to reach the clip level target.
     */
    void set_adaptive_distortion_strength(float strength);

    /**
     *  Sets the margin_curve from a list of (Hz, dB) tuples.
     *  The curve is linearly interpolated between the points
     *  The first point must be at 0Hz.
     *  margin_curve represents the minimum ratio between
     *  the clean input and the clipping distortion at each frequency
     */
    void set_margin_curve(int points[][2], int num_points);

private:
    int size;
    int overlap;
    int num_psy_bins;
    PFFFT_Setup* pffft;
    float sample_rate;
    float clip_level;
    float iterations;
    float adaptive_distortion_strength;

    /**
     *  in_frame: unmodified input audio
     *  out_dist_frame: clipping distortion multiplied by 1.5. The 1.5 factor is due to overlap and add.
     *  margin_curve: see generateMarginCurve
     *  window: the Hann window
     *  inv_window: inverse of the Hann window used to calculate the unwindowed peak
     *  spread_table: see generate_spread_table
     */
    std::vector<float> in_frame, out_dist_frame, margin_curve, window, inv_window, spread_table;

    /**
     *  spreadtableIndex: for each bin, which entry in the spread table to use
     *                    (eacn entry is num_psy_bins values)
     *  spread_table_range: for each entry in the spread table, what is the range of bins (ie. -2 to +4) that are non-zero
     */
    std::vector<int> spread_table_index;
    std::vector<std::pair<int, int>> spread_table_range;

    /**
     *  Generate the Hann window and inverse window.
     */
    void generate_hann_window();

    /**
    *  Generate the spreading functions used by calculate_mask_curve
    *  The spread_table contains entries of size/2 values each.
    *  To save memory, only 2 entries are stored per octave, and each entry is shared by a range of bins.
    *  The spread scale proportionally with frequency.
    *  The spread_table entries are normalized to add up to 1.
    #  Eacn entry is centred, meaning the num_psy_bins/2'th value is the peak of the tent-shaped function.
    */
    void generate_spread_table();

    /**
     *  Applies the window to the in_frame and store the result in the out_frame
     *  If add_to_out_frame is true, the results is added to the out_frame instead
     */
    void apply_window(const float* in_frame, float* out_frame, const bool add_to_out_frame = false);

    /**
     *  Clips the windowed_frame to the window scaled by the clip_level.
     *  The clipping distortion is multiplied by delta_boost
     *  The existing values in clipping_delta is applied to the windowed_frame
     *  to get the effective sample values, taking previous clipping iterations
     *  into account.
     *  Should only be used with windowed input
     */
    void clip_to_window(const float* windowed_frame, float* clipping_delta, float delta_boost = 1.0);

    /**
     *  Calculates the original signal level considering psychoacoustic masking.
     *  mask_curve is in linear scale.
     */
    void calculate_mask_curve(const float* spectrum, float* mask_curve);

    /**
     *  Limit the magnitude of each bin to the mask_curve
     */
    void limit_clip_spectrum(float* clip_spectrum, const float* mask_curve);

};
