// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "OCIOColorSpace.h"
#include "OCIODisplay.h"

void OFX::Plugin::getPluginIDs(OFX::PluginFactoryArray & ids)
{
    static OCIOColorSpaceFactory ocioColorSpace("OpenColorIO.OCIOColorSpace", 1, 0);
    ids.push_back(&ocioColorSpace);

    static OCIODisplayFactory ocioDisplay("OpenColorIO.OCIODisplay", 1, 0);
    ids.push_back(&ocioDisplay);
}
