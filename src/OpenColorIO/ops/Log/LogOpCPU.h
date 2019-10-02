// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LOG_LOGOPCPU
#define INCLUDED_OCIO_LOG_LOGOPCPU

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Log/LogOpData.h"

OCIO_NAMESPACE_ENTER
{
ConstOpCPURcPtr GetLogRenderer(ConstLogOpDataRcPtr & log);
}
OCIO_NAMESPACE_EXIT


#endif