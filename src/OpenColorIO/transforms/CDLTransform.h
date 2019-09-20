// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CDLTRANSFORM_H
#define INCLUDED_OCIO_CDLTRANSFORM_H

#include <map>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

OCIO_NAMESPACE_ENTER
{
static constexpr const char * METADATA_INPUT_DESCRIPTION = "InputDescription";
static constexpr const char * METADATA_VIEWING_DESCRIPTION = "ViewingDescription";
static constexpr const char * METADATA_SOP_DESCRIPTION = "SOPDescription";
static constexpr const char * METADATA_SAT_DESCRIPTION = "SATDescription";

typedef std::map<std::string,CDLTransformRcPtr> CDLTransformMap;
typedef std::vector<CDLTransformRcPtr> CDLTransformVec;

void ClearCDLTransformFileCache();

void LoadCDL(CDLTransform * cdl, const char * xml);

}
OCIO_NAMESPACE_EXIT

#endif
