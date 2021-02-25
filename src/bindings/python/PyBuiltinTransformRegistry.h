// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYBUILTINTRANSFORMREGISTRY_H
#define INCLUDED_OCIO_PYBUILTINTRANSFORMREGISTRY_H

#include "PyOpenColorIO.h"

namespace OCIO_NAMESPACE
{

// Wrapper to preserve the BuiltinTransformRegistry singleton.
class OCIOHIDDEN PyBuiltinTransformRegistry
{
public:
    PyBuiltinTransformRegistry() = default;
    ~PyBuiltinTransformRegistry() = default;

    size_t getNumBuiltins() const noexcept
    {
        return BuiltinTransformRegistry::Get()->getNumBuiltins();
    }

    const char * getBuiltinStyle(size_t idx) const
    {
        return BuiltinTransformRegistry::Get()->getBuiltinStyle(idx);
    }

    const char * getBuiltinDescription(size_t idx) const
    {
        return BuiltinTransformRegistry::Get()->getBuiltinDescription(idx);
    }
};

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_PYBUILTINTRANSFORMREGISTRY_H
