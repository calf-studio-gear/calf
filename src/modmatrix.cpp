/* Calf DSP Library
 * Modulation matrix boilerplate code.
 *
 * Copyright (C) 2001-2007 Krzysztof Foltman
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
#include <assert.h>
#include <sstream>
#include <calf/modmatrix.h>

using namespace std;
using namespace dsp;
using namespace calf_plugins;

mod_matrix::mod_matrix(modulation_entry *_matrix, unsigned int _rows, const char **_src_names, const char **_dest_names)
: matrix(_matrix)
, matrix_rows(_rows)
, mod_src_names(_src_names)
, mod_dest_names(_dest_names)
{
    table_column_info tci[5] = {
        { "Source", TCT_ENUM, 0, 0, 0, mod_src_names },
        { "Modulator", TCT_ENUM, 0, 0, 0, mod_src_names },
        { "Amount", TCT_FLOAT, 0, 1, 1, NULL},
        { "Destination", TCT_ENUM, 0, 0, 0, mod_dest_names  },
        { NULL }
    };
    assert(sizeof(table_columns) == sizeof(tci));
    memcpy(table_columns, tci, sizeof(table_columns));
}

const table_column_info *mod_matrix::get_table_columns(int param)
{
    return table_columns;
}

uint32_t mod_matrix::get_table_rows(int param)
{
    return matrix_rows;
}

std::string mod_matrix::get_cell(int param, int row, int column)
{
    assert(row >= 0 && row < (int)matrix_rows);
    modulation_entry &slot = matrix[row];
    switch(column) {
        case 0: // source 1
            return mod_src_names[slot.src1];
        case 1: // source 2
            return mod_src_names[slot.src2];
        case 2: // amount
            return calf_utils::f2s(slot.amount);
        case 3: // destination
            return mod_dest_names[slot.dest];
        default: 
            assert(0);
            return "";
    }
}
    
void mod_matrix::set_cell(int param, int row, int column, const std::string &src, std::string &error)
{
    assert(row >= 0 && row < (int)matrix_rows);
    modulation_entry &slot = matrix[row];
    switch(column) {
        case 0:
        case 1:
        {
            for (int i = 0; mod_src_names[i]; i++)
            {
                if (src == mod_src_names[i])
                {
                    if (column == 0)
                        slot.src1 = i;
                    else
                        slot.src2 = i;
                    error.clear();
                    return;
                }
            }
            error = "Invalid source name";
            return;
        }
        case 2:
        {
            stringstream ss(src);
            ss >> slot.amount;
            error.clear();
            return;
        }
        case 3:
        {
            for (int i = 0; mod_dest_names[i]; i++)
            {
                if (src == mod_dest_names[i])
                {
                    slot.dest = i;
                    error.clear();
                    return;
                }
            }
            error = "Invalid destination name";
            return;
        }
        
    }
}

