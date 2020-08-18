# -*- coding: utf-8 -*-
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# See:
# http://sphinx.pocoo.org/config.html

import sys, os

# -- Add PyOpenColorIO to sys.path ---------------------------------------------

sys.path.insert(0, "@CMAKE_BINARY_DIR@/src/bindings/python")

# -- Add installed ext packages to sys.path ------------------------------------

sys.path.insert(1, "@CMAKE_BINARY_DIR@/ext/dist/lib@LIB_SUFFIX@/site-packages")

# -- General configuration -----------------------------------------------------

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.todo',
    'sphinx.ext.ifconfig',
    'sphinx.ext.napoleon',
    'sphinx_tabs.tabs',
    'recommonmark',
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

# Pygments
pygments_style = 'friendly'

# # Breathe
# breathe_projects = {
#   u'OpenColorIO': "./_doxygen/xml"
# }
# breathe_default_project = u'OpenColorIO'

# Napoleon
napoleon_use_param = False
napoleon_include_init_with_doc = True


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