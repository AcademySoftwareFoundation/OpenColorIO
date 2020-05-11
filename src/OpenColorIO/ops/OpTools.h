// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_OPTOOLS_H
#define INCLUDED_OCIO_OPTOOLS_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "PrivateTypes.h"


namespace OCIO_NAMESPACE
{

void EvalTransform(const float * in, float * out,
                   long numPixels,
                   OpRcPtrVec & ops);

} // namespace OCIO_NAMESPACE

#endif
