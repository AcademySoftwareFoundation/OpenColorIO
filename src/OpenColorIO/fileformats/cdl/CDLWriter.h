// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_FILEFORMATS_CDL_CDLWRITER_H
#define INCLUDED_OCIO_FILEFORMATS_CDL_CDLWRITER_H

#include <OpenColorIO/OpenColorIO.h>

#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
{

class XmlFormatter;

void Write(XmlFormatter & fmt, const ConstCDLTransformRcPtr & cdl);

void WriteStrings(XmlFormatter & fmt, const char * tag, const StringUtils::StringVec & strings);

void ExtractCDLMetadata(const FormatMetadata & metadata,
                        StringUtils::StringVec & mainDesc,
                        StringUtils::StringVec & inputDesc,
                        StringUtils::StringVec & viewingDesc,
                        StringUtils::StringVec & sopDesc,
                        StringUtils::StringVec & satDesc);

} // namespace OCIO_NAMESPACE

#endif
