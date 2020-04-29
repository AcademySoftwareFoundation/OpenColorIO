// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>

#include "fileformats/ctf/CTFReaderUtils.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{

namespace
{
static constexpr char INTERPOLATION_1D_LINEAR[] = "linear";

static constexpr char INTERPOLATION_3D_LINEAR[] = "trilinear";
static constexpr char INTERPOLATION_3D_TETRAHEDRAL[] = "tetrahedral";
}

Interpolation GetInterpolation1D(const char * str)
{
    if (str && *str)
    {
        if (0 == Platform::Strcasecmp(str, INTERPOLATION_1D_LINEAR))
        {
            return INTERP_LINEAR;
        }

        std::ostringstream oss;
        oss << "1D LUT interpolation not recongnized: '" << str << "'.";
        throw Exception(oss.str().c_str());
    }

    throw Exception("1D LUT missing interpolation value.");
}

const char * GetInterpolation1DName(Interpolation interp)
{
    switch (interp)
    {
    case INTERP_LINEAR:
        return INTERPOLATION_1D_LINEAR;

    // Note: In CLF v3, some options were removed and the only legal Lut1D value is now "linear".

    case INTERP_CUBIC:
    case INTERP_DEFAULT:
    case INTERP_NEAREST:
    case INTERP_TETRAHEDRAL:
    case INTERP_BEST:
    case INTERP_UNKNOWN:
        return nullptr;
    };

    return nullptr;
}

Interpolation GetInterpolation3D(const char * str)
{
    if (str && *str)
    {
        if (0 == Platform::Strcasecmp(str, INTERPOLATION_3D_LINEAR))
        {
            return INTERP_LINEAR;
        }
        else if (0 == Platform::Strcasecmp(str, INTERPOLATION_3D_TETRAHEDRAL))
        {
            return INTERP_TETRAHEDRAL;
        }

        std::ostringstream oss;
        oss << "3D LUT interpolation not recongnized: '" << str << "'.";
        throw Exception(oss.str().c_str());
    }

    throw Exception("3D LUT missing interpolation value.");
}

const char * GetInterpolation3DName(Interpolation interp)
{
    switch (interp)
    {
    case INTERP_LINEAR:
        return INTERPOLATION_3D_LINEAR;
    case INTERP_TETRAHEDRAL:
        return INTERPOLATION_3D_TETRAHEDRAL;

    case INTERP_DEFAULT:
    case INTERP_NEAREST:
    case INTERP_CUBIC:
    case INTERP_BEST:
    case INTERP_UNKNOWN:
        return nullptr;
    };

    return nullptr;
}

} // namespace OCIO_NAMESPACE
