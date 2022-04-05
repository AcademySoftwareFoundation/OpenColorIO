// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_BAKING_UTILS_H
#define INCLUDED_OCIO_BAKING_UTILS_H

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

ConstCPUProcessorRcPtr GetInputToShaperProcessor(const Baker & baker);

ConstCPUProcessorRcPtr GetShaperToInputProcessor(const Baker & baker);

ConstCPUProcessorRcPtr GetInputToTargetProcessor(const Baker & baker);

ConstCPUProcessorRcPtr GetShaperToTargetProcessor(const Baker & baker);

void GetShaperRange(const Baker & baker, float& start, float& end);

void GetTargetRange(const Baker & baker, float& start, float& end);


} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_BAKING_UTILS_H
