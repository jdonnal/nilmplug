#!/usr/bin/env python

from setuptools import setup, find_packages

# Run setup
setup(name='nilmplug',
      description='Smart plug to support NILM systems',
      version='0.6.0',
      url='http://git.wattsworth.net/nilm/wemo-firmware.git',
      author='John Donnal',
      author_email='donnal@usna.edu',
      long_description="Smart plug based on Belkin WeMo",
      license="Proprietary",
      install_requires=['requests >= 2.0'
                        ],
      packages=['nilmplug'],
      include_package_data=True,
      entry_points={
          'console_scripts': [
              'nilm-plug = nilmplug.plug_wrapper:main',
              'nilm-plug-reader = nilmplug.modules.reader:main',
              'jim-term = nilmplug.scripts.terminal:main'
          ],
      },
      zip_safe=False,
      )
