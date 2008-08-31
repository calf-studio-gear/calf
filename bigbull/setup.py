#!/usr/bin/env python
from distutils.core import setup, Extension

module1 = Extension('calfpytools',
                    libraries = ['jack'],
                    sources = ['calfpytools.cpp']
                    )

setup (name = 'CalfPyTools',
       version = '0.01',
       description = 'Calf Python Tools',
       ext_modules = [module1])
