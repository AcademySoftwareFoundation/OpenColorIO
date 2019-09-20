// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_EXPOSURECONTRAST_CPU
#define INCLUDED_OCIO_EXPOSURECONTRAST_CPU


#include <OpenColorIO/OpenColorIO.h>

#include "ops/exposurecontrast/ExposureContrastOpData.h"


OCIO_NAMESPACE_ENTER
{

OpCPURcPtr GetExposureContrastCPURenderer(ConstExposureContrastOpDataRcPtr & ec);

}
OCIO_NAMESPACE_EXIT

#endif
