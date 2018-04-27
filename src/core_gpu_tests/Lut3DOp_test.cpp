/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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


OCIO_ADD_GPU_TEST(Lut3DOp, red_only_using_CSP_file)
{
    // Used a format I know to create a 3D lut file.
    //  Any other file format would have been good also.

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
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "1.0 0.0 0.0"                                 << "\n";
    content << "1.0 0.0 0.0"                                 << "\n";
    content << "1.0 0.0 0.0"                                 << "\n";
    content << "1.0 0.0 0.0"                                 << "\n";


    const std::string filename = createTempFile(".csp", content.str());

    // Create the transform & set the unit test

    OCIO::FileTransformRcPtr file = OCIO::FileTransform::Create();
    file->setSrc(filename.c_str());
    file->setInterpolation(OCIO::INTERP_LINEAR);
    test.setContext(file->createEditableCopy(), epsilon);
}

OCIO_ADD_GPU_TEST(Lut3DOp, green_only_using_CSP_file)
{
    // Used a format I know to create a 3D lut file.
    //  Any other file format would have been good also.

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
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 1.0 0.0"                                 << "\n";
    content << "0.0 1.0 0.0"                                 << "\n";
    content << "0.0 1.0 0.0"                                 << "\n";
    content << "0.0 1.0 0.0"                                 << "\n";


    const std::string filename = createTempFile(".csp", content.str());

    // Create the transform & set the unit test

    OCIO::FileTransformRcPtr file = OCIO::FileTransform::Create();
    file->setSrc(filename.c_str());
    file->setInterpolation(OCIO::INTERP_LINEAR);
    test.setContext(file->createEditableCopy(), epsilon);
}

OCIO_ADD_GPU_TEST(Lut3DOp, blue_only_using_CSP_file)
{
    // Used a format I know to create a 3D lut file.
    //  Any other file format would have been good also.

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
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 1.0"                                 << "\n";
    content << "0.0 0.0 1.0"                                 << "\n";
    content << "0.0 0.0 1.0"                                 << "\n";
    content << "0.0 0.0 1.0"                                 << "\n";


    const std::string filename = createTempFile(".csp", content.str());

    // Create the transform & set the unit test

    OCIO::FileTransformRcPtr file = OCIO::FileTransform::Create();
    file->setSrc(filename.c_str());
    file->setInterpolation(OCIO::INTERP_LINEAR);
    test.setContext(file->createEditableCopy(), epsilon);
}


OCIO_ADD_GPU_TEST(Lut3DOp, arbitrary_using_CSP_file)
{
    // Used a format I know to create a 3D lut file.
    //  Any other file format would have been good also.

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
    content << "0.100000 0.100000 0.100000"                  << "\n";
    content << "1.100000 0.100000 0.100000"                  << "\n";
    content << "0.100000 1.100000 0.100000"                  << "\n";
    content << "1.100000 1.100000 0.100000"                  << "\n";
    content << "0.100000 0.100000 1.100000"                  << "\n";
    content << "1.100000 0.100000 1.100000"                  << "\n";
    content << "0.100000 1.100000 1.100000"                  << "\n";
    content << "1.100000 1.100000 1.100000"                  << "\n";


    const std::string filename = createTempFile(".csp", content.str());

    // Create the transform & set the unit test

    OCIO::FileTransformRcPtr file = OCIO::FileTransform::Create();
    file->setSrc(filename.c_str());
    file->setInterpolation(OCIO::INTERP_LINEAR);
    test.setContext(file->createEditableCopy(), 2e-4f);
}
