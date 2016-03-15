#!/usr/bin/python

# To release a new version, tag it:
#   git tag -a nilmplug-1.1 -m "Version 1.1"
#   git push --tags
# Then just package it up:
#   python setup.py sdist

# This is supposed to be using Distribute:
#
#   distutils provides a "setup" method.
#   setuptools is a set of monkeypatches on top of that.
#   distribute is a particular version/implementation of setuptools.
#
# So we don't really know if this is using the old setuptools or the
# Distribute-provided version of setuptools.

import traceback
import sys
import os

try:
    from setuptools import setup, find_packages
    import distutils.version
except ImportError:
    traceback.print_exc()
    print "Please install the prerequisites listed in README.txt"
    sys.exit(1)

# Versioneer manages version numbers from git tags.
# https://github.com/warner/python-versioneer
import versioneer
versioneer.versionfile_source = 'nilmplug/_version.py'
versioneer.versionfile_build = 'nilmplug/_version.py'
versioneer.tag_prefix = 'nilmplug-'
versioneer.parentdir_prefix = ''#nilmplug-'

# Hack to workaround logging/multiprocessing issue:
# https://groups.google.com/d/msg/nose-users/fnJ-kAUbYHQ/_UsLN786ygcJ
try: import multiprocessing
except: pass

# We need a MANIFEST.in.  Generate it here rather than polluting the
# repository with yet another setup-related file.
with open("MANIFEST.in", "w") as m:
    m.write("""
# Root
include README.txt
include setup.py
include versioneer.py
include Makefile
""")

# Run setup
setup(name='nilmplug',
      version = versioneer.get_version(),
      cmdclass = versioneer.get_cmdclass(),
      url = 'http://git.wattsworth.net/nilm/wemo-firmware.git',
      author = 'John Donnal',
      description = "Smart plug based on Belkin WeMo",
      long_description = "Smart plug based on Belkin WeMo",
      license = "Proprietary",
      author_email = 'jim@jtan.com, jdonnal@mit.edu',
      install_requires = [ 'requests >= 2.0'
                           ],
      packages = [ 'nilmplug',
                   ],
      entry_points = {
          'console_scripts': [
              'nilm-plug = nilmplug.plug_wrapper:main',
              'jim-term = nilmplug.terminal:main',
              ],
          },
      zip_safe = False,
      )
