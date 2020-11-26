# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# -----------------------------------------------------------------------------
#
# This Sphinx extension supports RST read-time variable substitution. It is 
# derived from a code snippet by Adam Szmyd (https://github.com/aszmyd) in 
# Sphinx issue #4054:
#
#   https://github.com/sphinx-doc/sphinx/issues/4054#issuecomment-329097229
#
# This extension is used as a Python-only replacement of CMake"s 
# configure_file *.in substitution to support being run on readthedocs.org.
#
# -----------------------------------------------------------------------------


def expand_vars(app, docname, source):
    result = source[0]
    for key, value in app.config.expandvars_define.items():
        result = result.replace("${{{}}}".format(key), value)
    source[0] = result


def setup(app):
   app.add_config_value("expandvars_define", {}, "env")
   app.connect("source-read", expand_vars)
