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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
#include <config.h>
#include <calf/osctl.h>
#include <calf/utils.h>
#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <sstream>
#ifdef _MSC_VER
#include <windows.h>
#endif


using namespace std;
using namespace osctl;

namespace calf_utils {

string encode_map(const dictionary &data)
{
    osctl::string_buffer sb;
    osc_stream<osctl::string_buffer> str(sb);
    str << (uint32_t)data.size();
    for(dictionary::const_iterator i = data.begin(); i != data.end(); ++i)
    {
        str << i->first << i->second;
    }
    return sb.data;
}

void decode_map(dictionary &data, const string &src)
{
    osctl::string_buffer sb(src);
    osc_stream<osctl::string_buffer> str(sb);
    uint32_t count = 0;
    str >> count;
    string tmp, tmp2;
    data.clear();
    for (uint32_t i = 0; i < count; i++)
    {
        str >> tmp;
        str >> tmp2;
        data[tmp] = tmp2;
    }
}

std::string xml_escape(const std::string &src)
{
    string dest;
    for (size_t i = 0; i < src.length(); i++) {
        // XXXKF take care of string encoding
        if (src[i] < 0 || src[i] == '"' || src[i] == '<' || src[i] == '>' || src[i] == '&')
            dest += "&"+i2s((uint8_t)src[i])+";";
        else
            dest += src[i];
    }
    return dest;
}

std::string to_xml_attr(const std::string &key, const std::string &value)
{
    return " " + key + "=\"" + xml_escape(value) + "\"";
}

std::string load_file(const std::string &src)
{
    std::string str;
    FILE *f = fopen(src.c_str(), "rb");
    if (!f)
        throw file_exception(src);
    while(!feof(f))
    {
        char buffer[1024];
        int len = fread(buffer, 1, sizeof(buffer), f);
        if (len < 0)
        {
            fclose(f);
            throw file_exception(src);
        }
        str += string(buffer, len);
    }
    fclose(f);
    return str;
}

std::string i2s(int value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    
    return std::string(buf);
}

std::string f2s(double value)
{
    stringstream ss;
    ss << value;
    return ss.str();
}

std::string ff2s(double value)
{
    string s = f2s(value);
    if (s.find('.') == string::npos)
        s += ".0";
    return s;
}

std::string indent(const std::string &src, const std::string &indent)
{
    std::string dest;
    size_t pos = 0;
    do {
        size_t epos = src.find("\n", pos);
        if (epos == string::npos)
            break;
        dest += indent + src.substr(pos, epos - pos) + "\n";
        pos = epos + 1;
    } while(pos < src.length());
    if (pos < src.length())
        dest += indent + src.substr(pos);
    return dest;
}

//////////////////////////////////////////////////////////////////////////////////

file_exception::file_exception(const std::string &f)
: message(strerror(errno))
, filename(f)
, container(filename + ":" + message)
{ 
    text = container.c_str();
}

file_exception::file_exception(const std::string &f, const std::string &t)
: message(t)
, filename(f)
, container(filename + ":" + message)
{
    text = container.c_str();
}


/* Returns a list of files in a directory (except the ones that begin with a dot) */
#ifdef _MSC_VER
vector <direntry> list_directory(const string &path)
{
	std::vector <direntry> out;
	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile((path+"\\").c_str(), &data);      // DIRECTORY
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			direntry f;
			f.directory = path;
			f.full_path = path + "\\" + data.cFileName;
			f.name = data.cFileName;
			out.push_back(f);
		} while (FindNextFile(hFind, &data) != 0);
	}
	FindClose(hFind);
	return out;
}
#else
vector <direntry> list_directory(const string &path)
{
    std::vector <direntry> out;
    DIR *dir;
    struct dirent *ent;
    dir = opendir(path.empty() ? "." : path.c_str());
    while ((ent = readdir(dir)) != NULL) {
        direntry f;
        const string file_name = ent->d_name;
        const string full_file_name = path + "/" + file_name;
        if (file_name[0] == '.') continue;
        f.name = file_name;
        f.directory = path;
        f.full_path = full_file_name;
        out.push_back(f);
    }
    closedir(dir);
    return out;
}
#endif
}
