/* Calf DSP Library
 * Various utility functions.
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
 
#include <assert.h>
#include <config.h>
#include <calf/osctl.h>
#include <calf/utils.h>

using namespace calf_utils;
using namespace std;
using namespace osctl;

string calf_utils::encodeMap(const dictionary &data)
{
    osc_stream str;
    str.write((int32_t)data.size());
    for(dictionary::const_iterator i = data.begin(); i != data.end(); i++)
    {
        str.write(osc_data(i->first));
        str.write(osc_data(i->second));
    }
    return str.buffer;
}

void calf_utils::decodeMap(dictionary &data, const string &src)
{
    osc_stream str(src);
    osc_data tmp, tmp2;
    int32_t count = 0;
    str.read(osc_i32, tmp);
    count = tmp.i32val;
    data.clear();
    for (int i = 0; i < count; i++)
    {
        str.read(osc_string, tmp);
        str.read(osc_string, tmp2);
        data[tmp.strval] = tmp2.strval;
    }
}
