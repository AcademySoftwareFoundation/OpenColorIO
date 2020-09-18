// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_FILE_FORMAT_ICC_H
#define INCLUDED_OCIO_FILE_FORMAT_ICC_H

#include <string>

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

std::string GetProfileDescriptionFromICCProfile(const char * ICCProfileFilepath);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_FILE_FORMAT_ICC_H
