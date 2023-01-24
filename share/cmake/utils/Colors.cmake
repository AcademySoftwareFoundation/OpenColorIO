# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

string (ASCII 27 ColorEsc)

set(ColorReset       "${ColorEsc}[m")

# Error
set(ColorError       "${ColorEsc}[31m")
# Green
set(ColorSuccess     "${ColorEsc}[32m")
# Yellow
set(ColorWarning     "${ColorEsc}[33m")
set(ColorInfo        "${ColorEsc}[34m")

set(ColorBoldWhite   "${ColorEsc}[1;37m")