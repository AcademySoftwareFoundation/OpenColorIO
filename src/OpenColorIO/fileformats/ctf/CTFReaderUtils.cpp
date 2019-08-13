// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>

#include "fileformats/ctf/CTFReaderUtils.h"

OCIO_NAMESPACE_ENTER
{

Interpolation GetInterpolation1D(const char * str)
{
    if (str && *str)
    {
        if (0 == strcmp(str, "linear"))
        {
            return INTERP_LINEAR;
        }
        else if (0 == strcmp(str, "cubic"))
        {
            return INTERP_CUBIC;
        }
        else if (0 == strcmp(str, "default"))
        {
            return INTERP_DEFAULT;
        }

        std::ostringstream oss;
        oss << "1D LUT interpolation not recongnized: '" << str << "'.";
        throw Exception(oss.str().c_str());
    }

    throw Exception("1D LUT missing interpolation value.");
}

Interpolation GetInterpolation3D(const char * str)
{
    if (str && *str)
    {
        if (0 == strcmp(str, "trilinear"))
        {
            return INTERP_LINEAR;
        }
        else if (0 == strcmp(str, "tetrahedral"))
        {
            return INTERP_TETRAHEDRAL;
        }
        else if (0 == strcmp(str, "4pt tetrahedral"))
        {
            return INTERP_TETRAHEDRAL;
        }
        else if (0 == strcmp(str, "default"))
        {
            return INTERP_DEFAULT;
        }

        std::ostringstream oss;
        oss << "3D LUT interpolation not recongnized: '" << str << "'.";
        throw Exception(oss.str().c_str());
    }

    throw Exception("3D LUT missing interpolation value.");
}

}
OCIO_NAMESPACE_EXIT
