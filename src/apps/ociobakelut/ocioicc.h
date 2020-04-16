// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_OCIOICC_H
#define INCLUDED_OCIO_OCIOICC_H

#include <string>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

namespace OCIO_NAMESPACE
{

//
// Build an ICC profile for doing soft proofing
//
// N-component LUT-based display profile required tags
// ------------------------------------------------------------------------------
// Tag Name                 General Description
// ------------------------------------------------------------------------------
// profileDescriptionTag    Structure containing invariant and localizable
//                          versions of the profile name for display
// AToB0Tag                 Device to PCS: 8-bit or 16-bit data: intent of 0
// BToA0Tag                 PCS to Device space: 8-bit or 16-bit data: intent of 0
// mediaWhitePointTag       Media XYZ white point
// copyrightTag             Profile copyright information
// chromaticAdaptationTag   Converts XYZ colour from the actual illumination
//                          source to PCS illuminant. Required only if the actual
//                          illumination source is not D50.

void SaveICCProfileToFile(const std::string & outputfile,
                          ConstCPUProcessorRcPtr & processor,
                          int cubesize,
                          int whitepointtemp,
                          const std::string & displayicc,
                          const std::string & description,
                          const std::string & copyright,
                          bool verbose);

} // namespace OCIO_NAMESPACE

#endif

