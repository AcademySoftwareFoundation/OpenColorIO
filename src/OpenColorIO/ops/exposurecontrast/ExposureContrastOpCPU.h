// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_EXPOSURECONTRAST_CPU_H
#define INCLUDED_OCIO_EXPOSURECONTRAST_CPU_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/exposurecontrast/ExposureContrastOpData.h"


namespace OCIO_NAMESPACE
{

OpCPURcPtr GetExposureContrastCPURenderer(ConstExposureContrastOpDataRcPtr & ec);

} // namespace OCIO_NAMESPACE

#endif
