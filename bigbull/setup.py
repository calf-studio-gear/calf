#!/usr/bin/env python
from distutils.core import setup, Extension

module1 = Extension('calfpytools',
                    libraries = ['jack'],
                    sources = ['calfpytools.cpp', 'ttl.cpp'],
                    extra_compile_args = ["-g"],
                    extra_link_args = ["-g"]
                    )

setup (name = 'CalfPyTools',
       version = '0.01',
       description = 'Calf Python Tools',
       ext_modules = [module1])
