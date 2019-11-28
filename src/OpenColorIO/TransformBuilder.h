// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_TRANSFORMBUILDER_H
#define INCLUDED_OCIO_TRANSFORMBUILDER_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"

namespace OCIO_NAMESPACE
{

void CreateTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

} // namespace OCIO_NAMESPACE

#endif
