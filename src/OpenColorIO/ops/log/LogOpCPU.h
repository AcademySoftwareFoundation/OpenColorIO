// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LOG_LOGOP_CPU_H
#define INCLUDED_OCIO_LOG_LOGOP_CPU_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/log/LogOpData.h"

namespace OCIO_NAMESPACE
{
ConstOpCPURcPtr GetLogRenderer(ConstLogOpDataRcPtr & log, bool fastExp);

} // namespace OCIO_NAMESPACE

#endif