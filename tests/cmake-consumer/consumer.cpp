// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


// Check that OCIO library can be consumed by a cmake build

#include <cassert>

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO = OCIO_NAMESPACE;


int main(int argc, char** argv)
{
    auto reg = OCIO::BuiltinTransformRegistry::Get();
    assert(reg->getNumBuiltins() > 0);
    return 0;
}
