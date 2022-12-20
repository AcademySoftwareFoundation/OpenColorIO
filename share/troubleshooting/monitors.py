# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
# 
# Prints the monitor display name and the ICC profile path.
#
import PyOpenColorIO as OCIO

for m in OCIO.SystemMonitors().getMonitors():
    # Each element is a tuple containing the monitor display name and the ICC profile path.
    print(m)