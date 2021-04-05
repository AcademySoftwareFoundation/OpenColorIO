// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "OCIOColorSpace.h"

void OFX::Plugin::getPluginIDs(OFX::PluginFactoryArray & ids)
{
    static OCIOColorSpaceFactory p("OpenColorIO.OCIOColorSpace", 1, 0);
    ids.push_back(&p);
}  
