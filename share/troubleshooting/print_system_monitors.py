# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
# 
# This script will print the information OCIO has for each active monitor/display 
# connected to the system. The information consists of a Monitor Name and a path 
# to the monitor's ICC profile. 
# 
# OCIO attempts to build the unique Monitor Name based on information available 
# from the operating system, but sometimes that information may not be that helpful. 
# Ideally the Monitor Names should be descriptive enough to allow a user to determine 
# which name corresponds to which physical display and yet brief enough that they are 
# able to be used in menus that list the available displays. 
# 
# This script is an easy way to check the information OCIO detects on a given system 
# and may be useful when submitting bug reports.
#
import PyOpenColorIO as OCIO

for m in OCIO.SystemMonitors().getMonitors():
    # Each element is a tuple containing the monitor display name and the ICC profile path.
    print(m)