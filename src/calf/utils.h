/* Calf DSP Library
 * Utilities
 *
 * Copyright (C) 2008 Krzysztof Foltman
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
#ifndef __CALF_UTILS_H
#define __CALF_UTILS_H

#include <pthread.h>
#include <map>
#include <string>

namespace calf_utils
{

class ptmutex
{
public:
    pthread_mutex_t pm;
    
    ptmutex(int type = PTHREAD_MUTEX_RECURSIVE)
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, type);
        pthread_mutex_init(&pm, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    
    bool lock()
    {
        return pthread_mutex_lock(&pm) == 0;
    }
    
    bool trylock()
    {
        return pthread_mutex_trylock(&pm) == 0;
    }
    
    void unlock()
    {
        pthread_mutex_unlock(&pm);
    }

    ~ptmutex()
    {
        pthread_mutex_destroy(&pm);
    }
};

class ptlock
{
    ptmutex &mutex;
    bool locked;
    
public:
    ptlock(ptmutex &_m) : mutex(_m), locked(true)
    {
        mutex.lock();
    }
    void unlock()
    {
        mutex.unlock();
        locked = false;
    }
    void unlocked()
    {
        locked = false;
    }
    ~ptlock()
    {
        if (locked)
            mutex.unlock();
    }
};

template<class T>
class scope_assign
{
    T &data, old_value;
public:
    scope_assign(T &_data, T new_value)
    : data(_data), old_value(_data)
    {
        data = new_value;
    }
    ~scope_assign()
    {
        data = old_value;
    }
};

typedef std::map<std::string, std::string> dictionary;

extern std::string encodeMap(const dictionary &data);
extern void decodeMap(dictionary &data, const std::string &src);

static inline std::string i2s(int value)
{
    char buf[32];
    sprintf(buf, "%d", value);
    
    return std::string(buf);
}

static inline std::string f2s(double value)
{
    // XXXKF might not work with some locale settings
    char buf[64];
    sprintf(buf, "%g", value);
    return buf;
}

};

#endif
