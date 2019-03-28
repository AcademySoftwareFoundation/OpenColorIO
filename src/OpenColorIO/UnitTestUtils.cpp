/*
Copyright (c) 2019 Autodesk Inc., et al.
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

#ifdef OCIO_UNIT_TEST

#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"
#include "UnitTestUtils.h"

OCIO_NAMESPACE_ENTER
{
#ifndef OCIO_UNIT_TEST_FILES_DIR
#error Expecting OCIO_UNIT_TEST_FILES_DIR to be defined for tests. Check relevant CMakeLists.txt
#endif

// For explanation, refer to https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html 
#define _STR(x) #x
#define STR(x) _STR(x)

static const std::string ocioTestFilesDir(STR(OCIO_UNIT_TEST_FILES_DIR));

const char * getTestFilesDir()
{
    return ocioTestFilesDir.c_str();
}

void BuildOps(const std::string & fileName,
              OpRcPtrVec & fileOps,
              TransformDirection dir)
{
    const std::string filePath(std::string(getTestFilesDir()) + "/"
                               + fileName);

    // Create a FileTransform
    FileTransformRcPtr pFileTransform = FileTransform::Create();
    // A transform file does not define any interpolation (contrary to config
    // file), this is to avoid exception when creating the operation.
    pFileTransform->setInterpolation(INTERP_LINEAR);
    pFileTransform->setDirection(TRANSFORM_DIR_FORWARD);
    pFileTransform->setSrc(filePath.c_str());

    // Create empty Config to use
    ConfigRcPtr pConfig = Config::Create();

    ContextRcPtr pContext = Context::Create();

    BuildFileOps(fileOps, *(pConfig.get()), pContext,
                 *(pFileTransform.get()), dir);
}

}
OCIO_NAMESPACE_EXIT


#endif // OCIO_UNIT_TEST
