# -*- coding: utf-8 -*-
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# See:
# http://sphinx.pocoo.org/config.html

import os
import shutil 
import sys
import re


# -- Build options ------------------------------------------------------------

# OCIO API has been modified. Update frozen API docs in source tree. 
# Dependent on cmake build (so not RTD).
build_frozen = '@OCIO_BUILD_FROZEN_DOCS@' == 'ON'

# Building docs for RTD
on_rtd = os.environ.get('READTHEDOCS') == 'True'

# -- Add PyOpenColorIO to sys.path ---------------------------------------------

# Python bindings unavailable when building on RTD
sys.path.append('@CMAKE_BINARY_DIR@/src/bindings/python')

# -- Add local Sphinx extensions -----------------------------------------------

if on_rtd:
    # No CMake configuration
    HERE = os.path.dirname(os.path.realpath(__file__))
    ROOT = os.path.abspath(os.path.join(HERE, os.pardir))
    sys.path.append(os.path.join(ROOT, 'share', 'docs'))
else:
    sys.path.append('@CMAKE_SOURCE_DIR@/share/docs')

import expandvars
import prettymethods

if build_frozen:
    import frozendoc
else:
    frozendoc = None

if on_rtd:
    # No CMake configuration
    from ocio_namespace import get_ocio_namespace
else:
    get_ocio_namespace = None

# TODO: On RTD, run doxygen from here

# -- General configuration -----------------------------------------------------

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.todo',
    'sphinx.ext.ifconfig',
    'sphinx.ext.napoleon',
    'recommonmark',
    'sphinx_tabs.tabs',
    'breathe',
    'expandvars',
    'prettymethods',
]
templates_path = ['templates']
source_suffix = {
  '.rst': 'restructuredtext',
  '.md': 'markdown',
  '.txt': 'markdown',
}
master_doc = 'index'
project = u'OpenColorIO'
copyright = u'Copyright Contributors to the OpenColorIO Project'

exclude_patterns = ['build', '*-prefix']

rst_prolog = """
.. |OCIO| replace:: *OCIO*
.. |OpenColorIO| replace:: **OpenColorIO**
.. _Academy Software Foundation: https://www.aswf.io/
.. |Academy Software Foundation| replace:: `Academy Software Foundation`_
.. _Jeremy Selan: mailto:jeremy.selan@gmail.com
.. |Jeremy Selan| replace:: `Jeremy Selan`_
.. _ocio-user: https://lists.aswf.io/g/ocio-user
.. _ocio-dev: https://lists.aswf.io/g/ocio-dev
"""

# -- Extension Configuration ---------------------------------------------------

# autodoc
if build_frozen:
    frozendoc.patch()

# breathe
breathe_projects = {
    'OpenColorIO': '_doxygen/xml',
}
breathe_default_project = 'OpenColorIO'

# pygments
pygments_style = 'friendly'

# napoleon
napoleon_use_param = False
napoleon_include_init_with_doc = True

# expandvars
expandvars_define = {
    'PYDIR': 
        'frozen' if on_rtd else 'src',
    'OCIO_NAMESPACE': 
        '@OCIO_NAMESPACE@' if not on_rtd else get_ocio_namespace(),
}

# -- Options for HTML output ---------------------------------------------------

html_theme = 'press'
html_logo = '_static/ocio_b.svg'
html_static_path = ['_static']

html_theme_options = {
  "external_links": [
      ("Github", "https://github.com/AcademySoftwareFoundation/OpenColorIO"),
  ]
}

# -- Options for LaTeX output --------------------------------------------------

latex_documents = [
  ('index', 'OpenColorIO.tex', u'OpenColorIO Documentation',
   u'Academy Software Foundation', 'manual', False),
]
latex_elements = {
  'preamble': '\setcounter{tocdepth}{2}',
  'footer': 'test...123'
}

latex_domain_indices = ['cpp-modindex', 'py-modindex']

# -- Options for manual page output --------------------------------------------

man_pages = [
    ('index', 'opencolorio', u'OpenColorIO Documentation',
    [u'Academy Software Foundation'], 1)
]

# -- Options for Texinfo output ------------------------------------------------

texinfo_documents = [
  ('index', 'OpenColorIO', u'OpenColorIO Documentation', u'Academy Software Foundation',
   'OpenColorIO', 'One line description of project.', 'Miscellaneous'),
]
texinfo_appendices = []

# -- Options for Epub output ---------------------------------------------------

epub_title = u'OpenColorIO'
epub_author = u'Contributors to the OpenColorIO Project'
epub_publisher = u'Contributors to the OpenColorIO Project'
epub_copyright = u'Copyright Contributors to the OpenColorIO Project'
