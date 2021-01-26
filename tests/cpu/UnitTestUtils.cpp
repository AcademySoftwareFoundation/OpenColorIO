// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "Logging.h"
#include "OpBuilders.h"
#include "UnitTestUtils.h"


namespace OCIO_NAMESPACE
{
#ifndef OCIO_UNIT_TEST_FILES_DIR
#error Expecting OCIO_UNIT_TEST_FILES_DIR to be defined for tests. Check relevant CMakeLists.txt
#endif

// For explanation, refer to https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html 
#define _STR(x) #x
#define STR(x) _STR(x)


static const std::string ocioTestFilesDir(STR(OCIO_UNIT_TEST_FILES_DIR));

const std::string & GetTestFilesDir()
{
    return ocioTestFilesDir;
}

// Create a FileTransform.
FileTransformRcPtr CreateFileTransform(const std::string & fileName)
{
    const std::string filePath(GetTestFilesDir() + "/" + fileName);

    // Create a FileTransform
    FileTransformRcPtr pFileTransform = FileTransform::Create();
    pFileTransform->setSrc(filePath.c_str());

    return pFileTransform;
}

void BuildOpsTest(OpRcPtrVec & fileOps,
                  const std::string & fileName,
                  ContextRcPtr & context,
                  TransformDirection dir)
{
    FileTransformRcPtr fileTransform = CreateFileTransform(fileName);

    // Create empty Config to use
    ConfigRcPtr config = Config::Create();
    BuildFileTransformOps(fileOps, *(config.get()), context,
                          *(fileTransform.get()), dir);
}


ConstProcessorRcPtr GetFileTransformProcessor(const std::string & fileName)
{
    FileTransformRcPtr fileTransform = CreateFileTransform(fileName);

    // Create empty Config to use.
    ConfigRcPtr config = Config::Create();
    // Get the processor corresponding to the transform.
    return config->getProcessor(fileTransform);
}

} // namespace OCIO_NAMESPACE


