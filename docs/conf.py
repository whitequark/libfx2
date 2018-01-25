#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys, os
import sphinx_rtd_theme
from mock import Mock as MagicMock

# Configure our load path
sys.path.insert(0, os.path.abspath('../software'))

# Mock out C dependencies for readthedocs
class Mock(MagicMock):
    @classmethod
    def __getattr__(cls, name):
            return MagicMock()

MOCK_MODULES = ['usb1']
sys.modules.update((mod_name, Mock()) for mod_name in MOCK_MODULES)

# Configure Breathe and doxygen
breathe_projects_source = {
    'libfx2': (
        '../firmware/library/include', [
            'usb.h',
            'fx2regs.h', 'fx2ints.h', 'fx2lib.h',
            'fx2delay.h', 'fx2i2c.h', 'fx2eeprom.h',
            'fx2usb.h',
        ]
    )
}
breathe_default_project = 'libfx2'
breathe_doxygen_config_options = {
    'EXTRACT_ALL':            'YES',
    'OPTIMIZE_OUTPUT_FOR_C':  'YES',
    'SORT_MEMBER_DOCS':       'NO',
    'ENABLE_PREPROCESSING':   'YES',
    'MACRO_EXPANSION':        'YES',
    'EXPAND_ONLY_PREDEF':     'YES',
    'PREDEFINED': " ".join([
        'DOXYGEN',
        '_SFR(addr)="volatile sfr8_t"',
        '_SFR16(addr)="volatile sfr16_t"',
        '_SBIT(addr)="volatile sbit_t"',
        '_IOR(addr)="volatile ior8_t"',
        '_IOR16(addr)="volatile ior16_t"',
        '__at(x)=',
        '__xdata=',
    ])
}

# Configure Sphinx
extensions = ['sphinx.ext.autodoc', 'sphinxarg.ext', 'sphinx.ext.viewcode', 'breathe']
autodoc_member_order = 'bysource'
source_suffix = '.rst'
master_doc = 'index'
project = 'libfx2 Reference'
author = 'whitequark'
copyright = '2018, whitequark'
pygments_style = 'sphinx'
html_theme = 'sphinx_rtd_theme'
