/* Calf Analyzer FFT Library
 * Copyright (C) 2007-2013 Krzysztof Foltman, Markus Schmidt,
 * Christian Holschuh and others
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
 
#include <limits.h>
#include <memory.h>
#include <math.h>
#include <calf/giface.h>
#include <calf/analyzer.h>
#include <calf/modules_dev.h>
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#include <calf/utils.h>

using namespace dsp;
using namespace calf_plugins;

#define sinc(x) (x == 0) ? 1 : sin(M_PI * x)/(M_PI * x);
#define RGBAtoINT(r, g, b, a) ((uint32_t)(r * 255) << 24) + ((uint32_t)(g * 255) << 16) + ((uint32_t)(b * 255) << 8) + (uint32_t)(a * 255)

analyzer::analyzer() {
    _accuracy       = -1;
    _acc            = -1;
    _scale          = -1;
    _mode           = -1;
    _post           = -1;
    _hold           = -1;
    _smooth         = -1;
    _resolution     = -1.f;
    _offset         = -1.f;
    _freeze         = -1;
    _view           = -1;
    _windowing      = -1;
    _speed          = -1;
    fpos            = 0;
    _draw_upper     = 0;
    sanitize        = true;
    recreate_plan   = true;
    
    spline_buffer = (int*) calloc(200, sizeof(int));
    
    fft_buffer = (float*) calloc(max_fft_buffer_size, sizeof(float));
    
    fft_inL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_outL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_inR = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_outR = (float*) calloc(max_fft_cache_size, sizeof(float));
    
    fft_smoothL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_smoothR = (float*) calloc(max_fft_cache_size, sizeof(float));
    
    fft_deltaL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_deltaR = (float*) calloc(max_fft_cache_size, sizeof(float));
    
    fft_holdL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_holdR = (float*) calloc(max_fft_cache_size, sizeof(float));
    
    fft_freezeL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_freezeR = (float*) calloc(max_fft_cache_size, sizeof(float));
    
    analyzer_phase_drawn = 0;
}
analyzer::~analyzer()
{
    free(fft_freezeR);
    free(fft_freezeL);
    free(fft_holdR);
    free(fft_holdL);
    free(fft_deltaR);
    free(fft_deltaL);
    free(fft_smoothR);
    free(fft_smoothL);
    free(fft_outR);
    free(fft_outL);
    free(fft_inR);
    free(fft_inL);
    free(fft_buffer);
    free(spline_buffer);
}
void analyzer::set_sample_rate(uint32_t sr) {
    srate = sr;
}

void analyzer::set_params(float resolution, float offset, int accuracy, int hold, int smoothing, int mode, int scale, int post, int speed, int windowing, int view, int freeze)
{
    _speed     = speed;
    _windowing = windowing;
    _freeze    = freeze;
    _view      = view;
    
    if(accuracy != _acc) {
        _accuracy = 1 << (7 + (int)accuracy);
        _acc = accuracy;
        recreate_plan = true;
    }
    if(hold != _hold) {
        _hold = hold;
        sanitize = true;
    }
    if(smoothing != _smooth) {
        _smooth = smoothing;
        sanitize = true;
    }
    if (mode != _mode) {
        _mode = mode;
        sanitize = true;
        redraw_graph = true;
    }
    if(scale != _scale) {
        _scale = scale;
        sanitize = true;
    }
    if(post != _post) {
        _post = post;
        sanitize = true;
    }
    if(resolution != _resolution || offset != _offset) {
        _resolution = resolution;
        _offset = offset;
        redraw_graph = true;
    }
}
void analyzer::process(float L, float R) {
    fft_buffer[fpos] = L;
    fft_buffer[fpos + 1] = R;
    fpos += 2;
    fpos %= (max_fft_buffer_size - 2);
}

bool analyzer::do_fft(int subindex, int points) const
{
    if (recreate_plan) {
        lintrans = -1;
        recreate_plan = false;
        sanitize = true;
    }
    if (sanitize) {
        // null the overall buffer
        dsp::zero(fft_inL,     max_fft_cache_size);
        dsp::zero(fft_inR,     max_fft_cache_size);
        dsp::zero(fft_outL,    max_fft_cache_size);
        dsp::zero(fft_outR,    max_fft_cache_size);
        dsp::zero(fft_holdL,   max_fft_cache_size);
        dsp::zero(fft_holdR,   max_fft_cache_size);
        dsp::zero(fft_smoothL, max_fft_cache_size);
        dsp::zero(fft_smoothR, max_fft_cache_size);
        dsp::zero(fft_deltaL,  max_fft_cache_size);
        dsp::zero(fft_deltaR,  max_fft_cache_size);
        dsp::zero(spline_buffer, 200);
        analyzer_phase_drawn = 0;
        sanitize = false;
    }
    
    bool fftdone = false; // if fft was renewed, this one is set to true
    int __speed = 16 - (int)_speed;
    if(_mode == 5 && _smooth) {
        // there's no falling for difference mode, only smoothing
        _smooth = 2;
    }
    if(_mode > 5 && _mode < 11) {
        // there's no smoothing for spectralizer mode
        //_smooth = 0;
    }
    
    if(subindex == 0) {
        // #####################################################################
        // We are doing FFT here, so we first have to setup fft-buffers from
        // the main buffer and we use this cycle for filling other buffers
        // like smoothing, delta and hold
        // #####################################################################
        if(!((int)analyzer_phase_drawn % __speed)) {
            // seems we have to do a fft, so let's read the latest data from the
            // buffer to send it to fft afterwards
            // we want to remember old fft_out values for smoothing as well
            // and we fill the hold buffer in this (extra) cycle
            for(int i = 0; i < _accuracy; i++) {
                // go to the right position back in time according to accuracy
                // settings and cycling in the main buffer
                int _fpos = (fpos - _accuracy * 2 \
                    + (i * 2)) % max_fft_buffer_size;
                if(_fpos < 0)
                    _fpos = max_fft_buffer_size + _fpos;
                float L = fft_buffer[_fpos];
                float R = fft_buffer[_fpos + 1];
                float win = 0.54 - 0.46 * cos(2 * M_PI * i / _accuracy);
                L *= win;
                R *= win;
                
                // #######################################
                // Do some windowing functions on the
                // buffer
                // #######################################
                int _m = 2;
                float _f = 1.f;
                float _a, a0, a1, a2, a3;
                switch(_windowing) {
                    case 0:
                    default:
                        // Linear
                        _f = 1.f;
                        break;
                    case 1:
                        // Hamming
                        _f = 0.54 + 0.46 * cos(2 * M_PI * (i - 2 / points));
                        break;
                    case 2:
                        // von Hann
                        _f = 0.5 * (1 + cos(2 * M_PI * (i - 2 / points)));
                        break;
                    case 3:
                        // Blackman
                        _a = 0.16;
                        a0 = 1.f - _a / 2.f;
                        a1 = 0.5;
                        a2 = _a / 2.f;
                        _f = a0 + a1 * cos((2.f * M_PI * i) / points - 1) + \
                            a2 * cos((4.f * M_PI * i) / points - 1);
                        break;
                    case 4:
                        // Blackman-Harris
                        a0 = 0.35875;
                        a1 = 0.48829;
                        a2 = 0.14128;
                        a3 = 0.01168;
                        _f = a0 - a1 * cos((2.f * M_PI * i) / points - 1) + \
                            a2 * cos((4.f * M_PI * i) / points - 1) - \
                            a3 * cos((6.f * M_PI * i) / points - 1);
                        break;
                    case 5:
                        // Blackman-Nuttall
                        a0 = 0.3653819;
                        a1 = 0.4891775;
                        a2 = 0.1365995;
                        a3 = 0.0106411;
                        _f = a0 - a1 * cos((2.f * M_PI * i) / points - 1) + \
                            a2 * cos((4.f * M_PI * i) / points - 1) - \
                            a3 * cos((6.f * M_PI * i) / points - 1);
                        break;
                    case 6:
                        // Sine
                        _f = sin((M_PI * i) / (points - 1));
                        break;
                    case 7:
                        // Lanczos
                        _f = sinc((2.f * i) / (points - 1) - 1);
                        break;
                    case 8:
                        // Gauß
                        _a = 2.718281828459045;
                        _f = pow(_a, -0.5f * pow((i - (points - 1) / 2) / (0.4 * (points - 1) / 2.f), 2));
                        break;
                    case 9:
                        // Bartlett
                        _f = (2.f / (points - 1)) * (((points - 1) / 2.f) - \
                            fabs(i - ((points - 1) / 2.f)));
                        break;
                    case 10:
                        // Triangular
                        _f = (2.f / points) * ((2.f / points) - fabs(i - ((points - 1) / 2.f)));
                        break;
                    case 11:
                        // Bartlett-Hann
                        a0 = 0.62;
                        a1 = 0.48;
                        a2 = 0.38;
                        _f = a0 - a1 * fabs((i / (points - 1)) - 0.5) - \
                            a2 * cos((2 * M_PI * i) / (points - 1));
                        break;
                }
                L *= _f;
                if(_mode > _m)
                    R *= _f;

                // perhaps we need to compute two FFT's, so store left and right
                // channel in case we need only one FFT, the left channel is
                // used as 'standard'"
                float valL;
                float valR;
                
                switch(_mode) {
                    default:
                        // left channel (mode 1)
                        // or both channels (mode 3, 4, 5, 7, 9, 10)
                        valL = L;
                        valR = R;
                        break;
                    case 0:
                    case 6:
                        // average (mode 0)
                        valL = (L + R) / 2;
                        valR = (L + R) / 2;
                        break;
                    case 2:
                    case 8:
                        // right channel (mode 2)
                        valL = R;
                        valR = L;
                        break;
                }
                // store values in analyzer buffer
                fft_inL[i] = valL;
                fft_inR[i] = valR;
                
                // fill smoothing & falling buffer
                if(_smooth == 2) {
                    fft_smoothL[i] = fft_outL[i];
                    fft_smoothR[i] = fft_outR[i];
                }
                if(_smooth == 1) {
                    if(fft_smoothL[i] < fabs(fft_outL[i])) {
                        fft_smoothL[i] = fabs(fft_outL[i]);
                        fft_deltaL[i] = 1.f;
                    }
                    if(fft_smoothR[i] < fabs(fft_outR[i])) {
                        fft_smoothR[i] = fabs(fft_outR[i]);
                        fft_deltaR[i] = 1.f;
                    }
                }
                
                // fill hold buffer with last out values
                // before fft is recalced
                if(fabs(fft_outL[i]) > fft_holdL[i])
                    fft_holdL[i] = fabs(fft_outL[i]);
                if(fabs(fft_outR[i]) > fft_holdR[i])
                    fft_holdR[i] = fabs(fft_outR[i]);
            }
            
            // run fft
            // this takes our latest buffer and returns an array with
            // non-normalized
            fft.execute_r2r(_acc + 7, fft_inL, fft_outL, fft_temp, false);
            //run fft for for right channel too. it is needed for stereo image 
            //and stereo difference modes
            if(_mode >= 3) {
                fft.execute_r2r(_acc + 7, fft_inR, fft_outR, fft_temp, false);
            }
            // ...and set some values for later use
            analyzer_phase_drawn = 0;     
            fftdone = true;  
        }
        analyzer_phase_drawn ++;
    }
    return fftdone;
}

void analyzer::draw(int subindex, float *data, int points, bool fftdone) const
{
    double freq; // here the frequency of the actual drawn pixel gets stored
    int iter = 0; // this is the pixel we have been drawing the last box/bar/line
    int _iter = 1; // this is the next pixel we want to draw a box/bar/line
    int _last = -1; // used for mode 10 (parallel spectralizer) to prevent overwriting real values with INFINITY
    float posneg = 1;
    int __speed = 16 - (int)_speed;
    if (lintrans < 0 && srate) {
        // accuracy was changed so we have to recalc linear transition
        int _lintrans = (int)((float)points * log((20.f + 2.f * \
            (float)srate / (float)_accuracy) / 20.f) / log(1000.f));  
        lintrans = (int)(_lintrans + points % _lintrans / \
            floor(points / _lintrans)) / 2; // / 4 was added to see finer bars but breaks low end
    }
    for (int i = 0; i <= points; i++)
    {
        // #####################################################################
        // Real business starts here. We will cycle through all pixels in
        // x-direction of the line-graph and decide what to show
        // #####################################################################
        // cycle through the points to draw
        // we need to know the exact frequency at this pixel
        freq = 20.f * pow (1000.f, (float)i / points);
        
        // we need to know the last drawn value over the time
        float lastoutL = 0.f; 
        float lastoutR = 0.f;
        
        // let's see how many pixels we may want to skip until the drawing
        // function has to draw a bar/box/line
        if(_scale || _view == 2) {
            // we have linear view enabled or we want to see tit... erm curves
            if((i % lintrans == 0 && points - i > lintrans) || i == points - 1) {
                _iter = std::max(1, (int)floor(freq * \
                    (float)_accuracy / (float)srate));
            }    
        } else {
            // we have logarithmic view enabled
            _iter = std::max(1, (int)floor(freq * (float)_accuracy / (float)srate));
        }
        if(_iter > iter && srate) {
            // we are flipping one step further in drawing
            if(fftdone && i) {
                // ################################
                // Manipulate the fft_out values
                // according to the post processing
                // ################################
                int n = 0;
                float var1L = 0.f; // used later for denoising peaks
                float var1R = 0.f;
                float diff_fft;
                switch(_mode) {
                    default:
                        // all normal modes
                        posneg = 1;
                        // only if we don't see difference mode
                        switch(_post) {
                            case 0:
                                // Analyzer Normalized - nothing to do
                                break;
                            case 1:
                                // Analyzer Additive - cycle through skipped values and
                                // add them
                                // if fft was renewed, recalc the absolute values if
                                // frequencies are skipped
                                for(int j = iter + 1; j < _iter; j++) {
                                    fft_outL[_iter] += fabs(fft_outL[j]);
                                    fft_outR[_iter] += fabs(fft_outR[j]);
                                }
                                fft_outL[_iter] /= (_iter - iter);
                                fft_outR[_iter] /= (_iter - iter);
                                break;
                            case 2:
                                // Analyzer Additive - cycle through skipped values and
                                // add them
                                // if fft was renewed, recalc the absolute values if
                                // frequencies are skipped
                                for(int j = iter + 1; j < _iter; j++) {
                                    fft_outL[_iter] += fabs(fft_outL[j]);
                                    fft_outR[_iter] += fabs(fft_outR[j]);
                                }
                                break;
                            case 3:
                                // Analyzer Denoised Peaks - filter out unwanted noise
                                for(int k = 0; k < std::max(10 , std::min(400 ,\
                                    (int)(2.f*(float)((_iter - iter))))); k++) {
                                    //collect amplitudes in the environment of _iter to
                                    //be able to erase them from signal and leave just
                                    //the peaks
                                    if(_iter - k > 0) {
                                        var1L += fabs(fft_outL[_iter - k]);
                                        n++;
                                    }
                                    if(k != 0) var1L += fabs(fft_outL[_iter + k]);
                                    else if(i) var1L += fabs(lastoutL);
                                    else var1L += fabs(fft_outL[_iter]);
                                    if(_mode == 3 || _mode == 4) {
                                        if(_iter - k > 0) {
                                            var1R += fabs(fft_outR[_iter - k]);
                                            n++;
                                        }
                                        if(k != 0) var1R += fabs(fft_outR[_iter + k]);
                                        else if(i) var1R += fabs(lastoutR);
                                        else var1R += fabs(fft_outR[_iter]);
                                    }
                                    n++;
                                }
                                //do not forget fft_out[_iter] for the next time
                                lastoutL = fft_outL[_iter];
                                //pumping up actual signal an erase surrounding
                                // sounds
                                fft_outL[_iter] = 0.25f * std::max((float)(n * 0.6f * \
                                    fabs(fft_outL[_iter]) - var1L), 1e-20f);
                                if(_mode == 3 || _mode == 4) {
                                    // do the same with R channel if needed
                                    lastoutR = fft_outR[_iter];
                                    fft_outR[_iter] = 0.25f * std::max((float)(n * \
                                        0.6f * fabs(fft_outR[_iter]) - var1R), 1e-20f);
                                }
                                break;
                        }
                        break;
                    case 5:
                        // Stereo Difference - draw the difference between left
                        // and right channel if fft was renewed, recalc the
                        // absolute values in left and right if frequencies are
                        // skipped.
                        // this is additive mode - no other mode is available
                        //for(int j = iter + 1; j < _iter; j++) {
                        //    fft_outL[_iter] += fabs(fft_outL[j]);
                        //    fft_outR[_iter] += fabs(fft_outR[j]);
                        //}
                        //calculate difference between left an right channel                        
                        diff_fft = fabs(fft_outL[_iter]) - fabs(fft_outR[_iter]);
                        posneg = fabs(diff_fft) / diff_fft;
                        //fft_outL[_iter] = diff_fft / _accuracy;
                        break;
                }
            }
            iter = _iter;
            // #######################################
            // Calculate transitions for falling and
            // smooting and fill delta buffers if fft
            // was done above
            // #######################################
            if(subindex == 0) {
                float _fdelta = 0.91;
                float _ffactor = 2000.f;
                
                if(_mode > 5 && _mode < 11) {
                    _fdelta = .99f;
                    _ffactor = 50.f;
                }
                if(_smooth == 2) {
                    // smoothing
                    if(fftdone) {
                        // rebuild delta values after fft was done
                        if(_mode < 5 || _mode > 5) {
                            fft_deltaL[iter] = pow(fabs(fft_outL[iter]) / fabs(fft_smoothL[iter]), 1.f / __speed);
                        } else {
                            fft_deltaL[iter] = (posneg * fabs(fft_outL[iter]) - fft_smoothL[iter]) / __speed;
                        }
                    } else {
                        // change fft_smooth according to delta
                        if(_mode < 5 || _mode > 5) {
                            fft_smoothL[iter] *= fft_deltaL[iter];
                        } else {
                            fft_smoothL[iter] += fft_deltaL[iter];
                        }
                    }
                } else if(_smooth == 1) {
                    // falling
                    if(fftdone) {
                        // rebuild delta values after fft was done
                        //fft_deltaL[iter] = _fdelta;
                    }
                    // change fft_smooth according to delta
                    fft_smoothL[iter] *= fft_deltaL[iter];
                    
                    if(fft_deltaL[iter] > _fdelta) {
                        fft_deltaL[iter] *= 1.f - (16.f - __speed) / _ffactor;
                    }
                }
                
                if((_mode > 2 && _mode < 5) || (_mode > 8 && _mode < 11)) {
                    // we need right buffers, too for stereo image and
                    // stereo analyzer
                    if(_smooth == 2) {
                        // smoothing
                        if(fftdone) {
                            // rebuild delta values after fft was done
                            if(_mode < 5) {
                                fft_deltaR[iter] = pow(fabs(fft_outR[iter]) / fabs(fft_smoothR[iter]), 1.f / __speed);
                            } else {
                                fft_deltaR[iter] = (posneg * fabs(fft_outR[iter]) - fft_smoothR[iter]) / __speed;
                            }
                        } else {
                            // change fft_smooth according to delta
                            if(_mode < 5) {
                                fft_smoothR[iter] *= fft_deltaR[iter];
                            } else {
                                fft_smoothR[iter] += fft_deltaR[iter];
                            }
                        }
                    } else if(_smooth == 1) {
                        // falling
                        if(fftdone) {
                            // rebuild delta values after fft was done
                            //fft_deltaR[iter] = _fdelta;
                        }
                        // change fft_smooth according to delta
                        fft_smoothR[iter] *= fft_deltaR[iter];
                        if(fft_deltaR[iter] > _fdelta)
                            fft_deltaR[iter] *= 1.f - (16.f - __speed) / _ffactor;
                    }
                }
            }
            // #######################################
            // Choose the L and R value from the right
            // buffer according to view settings
            // #######################################
            float valL = 0.f;
            float valR = 0.f;
            if (_freeze) {
                // freeze enabled
                valL = fft_freezeL[iter];
                valR = fft_freezeR[iter];
            } else if ((subindex == 1 && _mode < 3) \
                || subindex > 1 \
                || (_mode > 5 && _hold)) {
                // we draw the hold buffer
                valL = fft_holdL[iter];
                valR = fft_holdR[iter];
            } else {
                // we draw normally (no freeze)
                switch(_smooth) {
                    case 0:
                        // off
                        valL = fft_outL[iter];
                        valR = fft_outR[iter];
                        break;
                    case 1:
                        // falling
                        valL = fft_smoothL[iter];
                        valR = fft_smoothR[iter];
                        break;
                    case 2:
                        // smoothing
                        valL = fft_smoothL[iter];
                        valR = fft_smoothR[iter];
                        break;
                }
                // fill freeze buffer
                fft_freezeL[iter] = valL;
                fft_freezeR[iter] = valR;
            }
            if(_view < 2) {
                // #####################################
                // send values back to painting function
                // according to mode setting but only if
                // we are drawing lines or boxes
                // #####################################
                float tmp;
                int pos1, pos2;
                switch(_mode) {
                    case 3:
                    case 9:
                        // stereo analyzer/spectralizer
                        if(subindex == 0 || subindex == 2) {
                            data[i] = dB_grid(fabs(valL) / _accuracy * 2.f + 1e-20, _resolution, _offset);
                        } else {
                            data[i] = dB_grid(fabs(valR) / _accuracy * 2.f + 1e-20, _resolution, _offset);
                        }
                        break;
                    case 4:
                        // we want to draw Stereo Image
                        if(subindex == 0 || subindex == 2) {
                            // Left channel signal
                            tmp = dB_grid(fabs(valL) / _accuracy * 2.f + 1e-20, _resolution, _offset);
                            //only signals above the middle are interesting
                            data[i] = tmp < 0 ? 0 : tmp;
                        } else if (subindex == 1 || subindex == 3) {
                            // Right channel signal
                            tmp = dB_grid(fabs(valR) / _accuracy * 2.f + 1e-20, _resolution, _offset);
                            //only signals above the middle are interesting. after cutting away
                            //the unneeded stuff, the curve is flipped vertical at the middle.
                            if(tmp < 0) tmp = 0;
                            data[i] = -1.f * tmp;
                        }
                        break;
                    case 5:
                        // We want to draw Stereo Difference
                        if(i) {
                            tmp = dB_grid(fabs((fabs(valL) - fabs(valR))) / _accuracy * 2.f +  1e-20, _resolution, 1.f / _offset);
                            //only show differences above a threshhold which results from the db_grid-calculation
                            if (tmp < 0) tmp=0;
                            //bring right signals below the middle
                            tmp *= fabs(valL) < fabs(valR) ? -1.f : 1.f;
                            data[i] = tmp;
                        }
                        else data[i] = 0.f;
                        break;
                    case 10:
                        // spectralizer parallel
                        pos1 = i / 2;
                        pos2 = points / 2 + pos1;
                        data[pos1] = dB_grid(fabs(valL) / _accuracy * 2.f + 1e-20, _resolution, _offset);
                        data[pos2] = dB_grid(fabs(valR) / _accuracy * 2.f + 1e-20, _resolution, _offset);
                        _last = pos1;
                        break;
                    default:
                        // normal analyzer behavior
                        data[i] = dB_grid(fabs(valL) / _accuracy * 2.f + 1e-20, _resolution, _offset);
                        break;
                }
            }
        }
        else if(_view == 2) {
            // we have to draw splines, so we draw every x-pixel according to
            // the pre-generated fft_splinesL and fft_splinesR buffers
            data[i] = INFINITY;
            
//            int _splinepos = -1;
//            *mode=0;
//            
//            for (int i = 0; i<=points; i++) {
//                if (subindex == 1 and i == spline_buffer[_splinepos]) _splinepos++;
//                
//                freq = 20.f * pow (1000.f, (float)i / points); //1000=20000/20
//                float a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2;
//                
//                if(((i % lintrans == 0 and points - i > lintrans) or i == points - 1 ) and subindex == 0) {

//                    _iter = std::max(1, (int)floor(freq * (float)_accuracy / (float)srate));
//                    //printf("_iter %3d\n",_iter);
//                }
//                if(_iter > iter and subindex == 0)
//                {
//                    _splinepos++;
//                    spline_buffer[_splinepos] = _iter;
//                    //printf("_splinepos: %3d - lintrans: %3d\n", _splinepos,lintrans);
//                    if(fftdone and i and subindex == 0) 
//                    {
//                        // if fft was renewed, recalc the absolute values if frequencies
//                        // are skipped
//                        for(int j = iter + 1; j < _iter; j++) {
//                            fft_out[_iter] += fabs(fft_out[j]);
//                        }
//                    }
//                }
//                
//                if(fftdone and subindex == 1 and _splinepos >= 0 and (_splinepos % 3 == 0 or _splinepos == 0)) 
//                {
//                    float mleft, mright, y0, y1, y2, y3, y4;
//                    
//                    //jetzt spline interpolation
//                    y0 = dB_grid(fft_out[spline_buffer[_splinepos]] / _accuracy * 2.f + 1e-20, pow(64, _level), 0.5f);
//                    y1 = dB_grid(fft_out[spline_buffer[_splinepos + 1]] / _accuracy * 2.f + 1e-20, pow(64, _level), 0.5f);
//                    y2 = dB_grid(fft_out[spline_buffer[_splinepos + 2]] / _accuracy * 2.f + 1e-20, pow(64, _level), 0.5f);
//                    y3 = dB_grid(fft_out[spline_buffer[_splinepos + 3]] / _accuracy * 2.f + 1e-20, pow(64, _level), 0.5f);
//                    y4 = dB_grid(fft_out[spline_buffer[_splinepos + 4]] / _accuracy * 2.f + 1e-20, pow(64, _level), 0.5f);
//                    mleft  = y1 - y0;
//                    mright = y4 - y3;
//                    printf("y-werte %3d, %3d, %3d, %3d, %3d\n",y0,y1,y2,y3,y4);
//                    a0 = (-3*y3+15*y2-44*y1+32*y0+mright+22*mleft)/34;
//                    b0 = -(-3*y3+15*y2-78*y1+66*y0+mright+56*mleft)/34;
//                    c0 = mleft;
//                    d0 = y0;
//                    a1 = -(-15*y3+41*y2-50*y1+24*y0+5*mright+8*mleft)/34;
//                    b1 = (-6*y3+30*y2-54*y1+30*y0+2*mright+10*mleft)/17;
//                    c1 = -(3*y3-15*y2-24*y1+36*y0-mright+12*mleft)/34;
//                    d1 = y1;
//                    a2 = (-25*y3+40*y2-21*y1+6*y0+14*mright+2*mleft)/17;
//                    b2 = -(-33*y3+63*y2-42*y1+12*y0+11*mright+4*mleft)/17;
//                    c2 = (9*y3+6*y2-21*y1+6*y0-3*mright+2*mleft)/17;
//                    d2 = y2;
//                }
//                iter = _iter;
//                
//                
//                if(i > spline_buffer[_splinepos] and i <= spline_buffer[_splinepos + 1] and _splinepos >= 0 and subindex == 1) 
//                {
//                data[i] = a0 * pow(i / lintrans - _splinepos, 3) + b0 * pow(i / lintrans - _splinepos, 2) + c0 * (i / lintrans - _splinepos) + d0;
//                printf("1.spline\n");
//                }
//                if(i > spline_buffer[_splinepos + 1] and i <= spline_buffer[_splinepos + 2] and _splinepos >= 0 and subindex == 1) 
//                {
//                printf("2.spline\n");
//                data[i] = a1 * pow(i / lintrans - _splinepos, 3) + b1 * pow(i / lintrans - _splinepos, 2) + c1 * (i / lintrans - _splinepos) + d1;
//                }
//                if(i > spline_buffer[_splinepos + 2] and i <= spline_buffer[_splinepos + 3] and _splinepos >= 0 and subindex == 1) 
//                {
//                printf("3.spline\n");
//                data[i] = a2 * pow(i / lintrans - _splinepos, 3) + b2 * pow(i / lintrans - _splinepos, 2) + c2 * (i / lintrans - _splinepos) + d2;
//                }
//                if(subindex==1) printf("data[i] %3d _splinepos %2d\n", data[i], _splinepos);
//                if (subindex == 0) 
//                {
//                    data[i] = INFINITY;
//                } else data[i] = dB_grid(i/200, pow(64, _level), 0.5f);
//            
//            }
            
        }
        else {
            if (_mode != 10)
                data[i] = INFINITY;
            else if (_last != i / 2) {
                data[i / 2] = INFINITY;
                data[points / 2 + i / 2] = INFINITY;
            }
            else {
                data[i] = INFINITY;
            }
        }
    }
}

bool analyzer::get_graph(int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (!phase)
        return false;
    
    if ((subindex == 1 && !_hold && _mode < 3) \
     || (subindex > 1  && _mode < 3) \
     || (subindex == 2 && !_hold && (_mode == 3 || _mode == 4)) \
     || (subindex == 4 && (_mode == 3 || _mode == 4)) \
     || (subindex == 1 && _mode == 5) \
     || _mode > 5 \
    ) {
        // stop drawing when all curves have been drawn according to the mode
        // and hold settings
        return false;
    }
    bool fftdone = false;
    if (!subindex)
        fftdone = do_fft(subindex, points);
    draw(subindex, data, points, fftdone);
    
    // #############################
    // choose a drawing mode between
    // boxes, lines and bars
    // #############################
    
    // modes:
    // 0: left
    // 1: right
    // 2: average
    // 3: stereo
    // 4: image
    // 5: difference
    
    // views:
    // 0: bars
    // 1: lines
    // 2: cubic splines
    
    // *modes to set:
    // 0: lines
    // 1: blob
    // 2: bars
    // 3: boxes (little things on the values position
    // 4: centered bars (0dB is centered in y direction)
    
    if (_mode > 3 && _mode < 6) {
        // centered viewing modes like stereo image and stereo difference
        if(!_view) {
            // boxes
            if(subindex > 1) {
                // boxes (hold)
                *mode = 3;
            } else {
                // bars (signal)
                *mode = 4;
            }
        } else {
            // lines
            *mode = 0;
        }
    } else if(!_view) {
        // bars
        if((subindex == 0 && _mode < 3) || (subindex <= 1 && _mode == 3)) {
            // draw bars
            *mode = 2;
        } else {
            // draw boxes
            *mode = 3;
        }
    } else {
        // draw lines
        *mode = 0;
    }
    // ###################################
    // colorize the curves/boxes according
    // to the chosen display settings
    // ###################################
    
    // change alpha (or color) for hold lines or stereo modes
    if((subindex == 1 && _mode < 3) || (subindex > 1 && _mode == 4)) {
        // subtle hold line on left, right, average or stereo image
        context->set_source_rgba(0.15, 0.2, 0.0, 0.2);
    }
    if(subindex == 0 && _mode == 3) {
        // left channel in stereo analyzer
        context->set_source_rgba(0.25, 0.10, 0.0, 0.33);
    }
    if(subindex == 1 && _mode == 3) {
        // right channel in stereo analyzer
        context->set_source_rgba(0.05, 0.25, 0.0, 0.33);
    }
    if(subindex == 2 && _mode == 3) {
        // left hold in stereo analyzer
        context->set_source_rgba(0.45, 0.30, 0.2, 0.2);
    }
    if(subindex == 3 && _mode == 3) {
        // right hold in stereo analyzer
        context->set_source_rgba(0.25, 0.45, 0.2, 0.2);
    }
    
    context->set_line_width(0.75);
    return true;
}

bool analyzer::get_moving(int subindex, int &direction, float *data, int x, int y, int &offset, uint32_t &color) const
{
    if ((subindex && _mode != 9) || subindex > 1)
        return false;
    bool fftdone = false;
    if (!subindex)
        fftdone = do_fft(subindex, x);
    draw(subindex, data, x, fftdone);
    direction = LG_MOVING_UP;
    offset = 0;
    if (_mode == 9 && subindex) {
        color = RGBAtoINT(0.35, 0.1, 0, 0.4);
    } else if (_mode == 9) {
        color = RGBAtoINT(0.15, 0.35, 0, 0.4);
    }
    return true;
}

bool analyzer::get_gridline(int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (phase)
        return false;
    redraw_graph = false;
    float gain;
    int sub = subindex + (_draw_upper % 2) - 1;
    static const double dash[] = {2.0};
    switch (_mode) {
        case 0:
        case 1:
        case 2:
        case 3:
        default:
            return get_freq_gridline(subindex, pos, vertical, legend, context, true, _resolution, _offset);
        case 4:
            // stereo image
            if(subindex < 28)
                return get_freq_gridline(subindex, pos, vertical, legend, context, true);
            else {
                subindex -= 28;
            }
            gain = _draw_upper > 0 ? 1.f / (1 << (subindex - _draw_upper))
                                   : 1.f / (1 << subindex);
            pos = dB_grid(gain, _resolution, _offset);
            if (_draw_upper > 0)
                pos *= -1;
            
            context->set_dash(dash, 1);
            if ((!(subindex & 1) && !_draw_upper)
              || ((sub & 1) && _draw_upper > 0)) {
                // add a label and make the lines straight
                std::stringstream ss;
                ss << (subindex - std::max(0, _draw_upper)) * -6 << " dB";
                legend = ss.str();
                context->set_dash(dash, 0);
            }
        
            if (pos < 0 && !_draw_upper) {
                // start drawing the lower end
                _draw_upper = subindex;
                pos = -2;
            }
            if (_draw_upper < 0) {
                // end the drawing of the grid
                _draw_upper = 0;
                return false;
            }
            if (pos > 0 && _draw_upper > 0) {
                // center line
                _draw_upper = -1;
                pos = 0;
                context->set_dash(dash, 0);
            }
            else if (subindex)
                context->set_source_rgba(0, 0, 0, 0.1);
            vertical = false;
            return true;
        case 5:
            // stereo difference
            if(subindex < 28)
                return get_freq_gridline(subindex, pos, vertical, legend, context, true);
            else
                subindex -= 28;
                
            gain = _draw_upper > 0 ? 1.0 / (1 << (subindex - _draw_upper))
                                    : (1 << subindex);
            pos = dB_grid(gain, _resolution, 0);
            
            context->set_dash(dash, 1);
            if ((!(subindex & 1) && !_draw_upper)
              || ((subindex & 1) && _draw_upper)) {
                std::stringstream ss;
                ss << (subindex - std::max(0, _draw_upper)) * 6 - 72 << " dB";
                legend = ss.str();
                context->set_dash(dash, 0);
            }
            
            if (pos > 1 && !_draw_upper && (subindex & 1)) {
                _draw_upper = subindex;
            }
            if (pos < -1 && _draw_upper) {
                _draw_upper = 0;
                return false;
            }
            
            if (subindex)
                context->set_source_rgba(0, 0, 0, 0.1);
            vertical = false;
            return true;
        case 6:
        case 7:
        case 8:
        case 9:
            if (subindex < 28) {
                vertical = true;
                if (subindex == 9) legend = "100 Hz";
                if (subindex == 18) legend = "1 kHz";
                if (subindex == 27) legend = "10 kHz";
    
                float freq = subindex_to_freq(subindex);
                pos = log(freq / 20.0) / log(1000);
    
                if (!legend.empty()) {
                    context->set_source_rgba(0, 0, 0, 0.33);
                } else {
                    context->set_source_rgba(0, 0, 0, 0.2);
                }
                return true;
            }
            return false;
        case 10:
            //if (!subindex) {
                //cairo_t *ctx = context->context;
                //cairo_set_source_rgb(ctx, 0.35, 0.4, 0.2);
                //cairo_select_font_face(ctx, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
                //cairo_set_font_size(ctx, 20);
                //cairo_move_to (ctx, context->size_x / 2 - 20, 40);
                //cairo_show_text (ctx, "L");
                //cairo_move_to (ctx, context->size_x / 2 + 12, 40);
                //cairo_show_text (ctx, "R");
            //}
            if (subindex < 56) {
                vertical = true;
                if (subindex == 9 || subindex == 36) legend = "100 Hz";
                if (subindex == 18 || subindex == 45) legend = "1 kHz";
                if (subindex == 27 || subindex == 54) legend = "10 kHz";
    
                float freq = subindex_to_freq(subindex - (subindex > 27 ? 27 : 0));
                pos = log(freq / 20.0) / log(1000) / 2 + (subindex > 27 ? 0.5 : 0);
                if (!legend.empty() && subindex != 28) {
                    context->set_source_rgba(0, 0, 0, 0.33);
                } else if (subindex != 28) {
                    context->set_source_rgba(0, 0, 0, 0.2);
                }
                return true;
            }
            return false;
    }
    return false;
}
    
bool analyzer::get_layers(int generation, unsigned int &layers) const
{
    if (_mode > 5 && _mode < 11)
        layers = LG_REALTIME_MOVING;
    else
        layers = LG_REALTIME_GRAPH;
    layers |= ((!generation || redraw_graph) ? LG_CACHE_GRID : 0);
    return true;
}
