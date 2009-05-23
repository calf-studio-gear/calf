/* Calf DSP Library
 * Modulation matrix boilerplate code.
 *
 * Copyright (C) 2009 Krzysztof Foltman
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
#ifndef __CALF_MODMATRIX_H
#define __CALF_MODMATRIX_H
 
#include "giface.h"

namespace dsp {

struct modulation_entry
{
    int src1, src2;
    int mapping;
    float amount;
    int dest;
};

};

namespace calf_plugins {

class mod_matrix: public table_edit_iface
{
protected:
    enum modulation_mode { mod_positive, mod_bipolar, mod_negative, mod_squared, mod_squared_bipolar, mod_antisquared, mod_antisquared_bipolar, mod_parabola, mod_type_count };
    /// Polynomials for different scaling modes (1, x, x^2)
    static const float scaling_coeffs[mod_type_count][3];
    /// Column descriptions for table widget
    table_column_info table_columns[6];
    
    dsp::modulation_entry *matrix;
    unsigned int matrix_rows;
    const char **mod_src_names, **mod_dest_names;

    mod_matrix(dsp::modulation_entry *_matrix, unsigned int _rows, const char **_src_names, const char **_dest_names);
public:
    virtual const table_column_info *get_table_columns(int param);
    virtual uint32_t get_table_rows(int param);
    virtual std::string get_cell(int param, int row, int column);
    virtual void set_cell(int param, int row, int column, const std::string &src, std::string &error);

    /// Process modulation matrix, calculate outputs from inputs
    inline void calculate_modmatrix(float *moddest, int moddest_count, float *modsrc)
    {
        for (int i = 0; i < moddest_count; i++)
            moddest[i] = 0;
        for (unsigned int i = 0; i < matrix_rows; i++)
        {
            dsp::modulation_entry &slot = matrix[i];
            if (slot.dest) {
                float value = modsrc[slot.src1] * modsrc[slot.src2];
                value = dsp::clip<float>(value, 0, 1);
                const float *c = scaling_coeffs[slot.mapping];
                value = c[0] + c[1] * value + c[2] * value * value;
                moddest[slot.dest] += value * slot.amount;
            }
        }
    }
};
    
};

#endif
