/*
    Made by Autodesk Inc. under the terms of the OpenColorIO BSD 3 Clause License
*/


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO = OCIO_NAMESPACE;
#include "GPUUnitTest.h"
#include "GPUHelpers.h"

#include <stdio.h>
#include <sstream>
#include <string>

OCIO_NAMESPACE_USING


const float epsilon = 1e-4f;


OCIO_ADD_GPU_TEST(Lut3DOp, using_CSP_file)
{
    std::ostringstream content;
    content << "CSPLUTV100"                                  << "\n";
    content << "3D"                                          << "\n";
    content << ""                                            << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << ""                                            << "\n";
    content << "2 2 2"                                       << "\n";
    content << "0.00 0.0 0.0"                                << "\n";
    content << "0.00 0.0 0.0"                                << "\n";
    content << "0.00 0.0 0.0"                                << "\n";
    content << "0.00 0.0 0.0"                                << "\n";
    content << "1.00 0.0 0.0"                                << "\n";
    content << "1.00 0.0 0.0"                                << "\n";
    content << "1.00 0.0 0.0"                                << "\n";
    content << "1.00 0.0 0.0"                                << "\n";


    const std::string filename = createTempFile(".csp", content.str());

    OCIO::FileTransformRcPtr file = OCIO::FileTransform::Create();
    file->setSrc(filename.c_str());
    file->setInterpolation(OCIO::INTERP_LINEAR);
    test.setContext(file->createEditableCopy(), epsilon);
}
