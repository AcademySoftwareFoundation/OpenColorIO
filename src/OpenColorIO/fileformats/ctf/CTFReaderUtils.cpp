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
    case INTERP_BEST:
        return INTERPOLATION_1D_LINEAR;

    // Note: In CLF v3, some options were removed and the only legal Lut1D value is now "linear".

    case INTERP_CUBIC:
    case INTERP_DEFAULT:
    case INTERP_NEAREST:
    case INTERP_TETRAHEDRAL:
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
    case INTERP_BEST:
        return INTERPOLATION_3D_TETRAHEDRAL;

    case INTERP_DEFAULT:
    case INTERP_NEAREST:
    case INTERP_CUBIC:
    case INTERP_UNKNOWN:
        return nullptr;
    };

    return nullptr;
}

namespace
{
constexpr char GRADING_STYLE_LOG_FWD[] = "log";
constexpr char GRADING_STYLE_LIN_FWD[] = "linear";
constexpr char GRADING_STYLE_VIDEO_FWD[] = "video";
constexpr char GRADING_STYLE_LOG_REV[] = "logRev";
constexpr char GRADING_STYLE_LIN_REV[] = "linearRev";
constexpr char GRADING_STYLE_VIDEO_REV[] = "videoRev";
}

void ConvertStringToGradingStyleAndDir(const char * str,
                                       GradingStyle & style,
                                       TransformDirection & dir)
{
    if (str && *str)
    {
        if (0 == Platform::Strcasecmp(str, GRADING_STYLE_LOG_FWD))
        {
            style = GRADING_LOG;
            dir = TRANSFORM_DIR_FORWARD;
        }
        else if (0 == Platform::Strcasecmp(str, GRADING_STYLE_LOG_REV))
        {
            style = GRADING_LOG;
            dir = TRANSFORM_DIR_INVERSE;
        }
        else if (0 == Platform::Strcasecmp(str, GRADING_STYLE_LIN_FWD))
        {
            style = GRADING_LIN;
            dir = TRANSFORM_DIR_FORWARD;
        }
        else if (0 == Platform::Strcasecmp(str, GRADING_STYLE_LIN_REV))
        {
            style = GRADING_LIN;
            dir = TRANSFORM_DIR_INVERSE;
        }
        else if (0 == Platform::Strcasecmp(str, GRADING_STYLE_VIDEO_FWD))
        {
            style = GRADING_VIDEO;
            dir = TRANSFORM_DIR_FORWARD;
        }
        else if (0 == Platform::Strcasecmp(str, GRADING_STYLE_VIDEO_REV))
        {
            style = GRADING_VIDEO;
            dir = TRANSFORM_DIR_INVERSE;
        }
        else
        {
            std::ostringstream os;
            os << "Unknown grading style: '" << str << "'.";

            throw Exception(os.str().c_str());
        }
        return;
    }

    throw Exception("Missing grading style.");
}

const char * ConvertGradingStyleAndDirToString(GradingStyle style, TransformDirection dir)
{
    const bool fwd = (dir == TRANSFORM_DIR_FORWARD);
    switch (style)
    {
    case GRADING_LOG:
        return fwd ? GRADING_STYLE_LOG_FWD : GRADING_STYLE_LOG_REV;
    case GRADING_LIN:
        return fwd ? GRADING_STYLE_LIN_FWD : GRADING_STYLE_LIN_REV;
    case GRADING_VIDEO:
        return fwd ? GRADING_STYLE_VIDEO_FWD : GRADING_STYLE_VIDEO_REV;
    }
    std::ostringstream os;
    os << "Unknown grading style: " << style;

    throw Exception(os.str().c_str());
}
} // namespace OCIO_NAMESPACE
