// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatUtils.h"

#include "Logging.h"

namespace OCIO_NAMESPACE
{
// First time an interpolation is used for a given file, convert the interpolation to a valid one
// and remember the interpolation.  Next calls, if the interpolation is valid and is not similar
// to the cached one, clone the LUT and use that new interpolation, else, use the cached LUT.
template<class Lut>
OCIO_SHARED_PTR<Lut> HandleLUT(const OCIO_SHARED_PTR<Lut> & fileLut,
                               Interpolation fileInterp,
                               Interpolation cachedInterp,
                               bool & fileInterpUsed)
{
    OCIO_SHARED_PTR<Lut> lut;
    const bool firstInterp = cachedInterp == INTERP_UNKNOWN;

    if (fileLut)
    {
        const bool valid = Lut::IsValidInterpolation(fileInterp);
        fileInterpUsed |= valid;

        // If first: change interpolation of the cached LUT.
        if (firstInterp)
        {
            fileInterp = valid ? fileInterp : INTERP_DEFAULT;
            fileLut->setInterpolation(fileInterp);
        }

        const auto lutInterp = fileLut->getInterpolation();
        if (!valid ||
            Lut::GetConcreteInterpolation(lutInterp) == Lut::GetConcreteInterpolation(fileInterp))
        {
            // Same concrete interpolation or invalid, no clone needed.
            lut = fileLut;
        }
        else
        {
            // As the FileTransform interpolation (from the config file) is valid and is different
            // from the cached interpolation, clone the LUT and use that new interpolation.
            lut = fileLut->clone();
            lut->setInterpolation(valid ? fileInterp : INTERP_DEFAULT);
        }
    }
    return lut;
}

Lut1DOpDataRcPtr HandleLUT1D(const Lut1DOpDataRcPtr & fileLut1D,
                             Interpolation fileInterp,
                             Interpolation cachedInterp,
                             bool & fileInterpUsed)
{
    return HandleLUT<Lut1DOpData>(fileLut1D, fileInterp, cachedInterp, fileInterpUsed);
}

Lut3DOpDataRcPtr HandleLUT3D(const Lut3DOpDataRcPtr & fileLut3D,
                             Interpolation fileInterp,
                             Interpolation cachedInterp,
                             bool & fileInterpUsed)
{
    return HandleLUT<Lut3DOpData>(fileLut3D, fileInterp, cachedInterp, fileInterpUsed);
}

void LogWarningInterpolationNotUsed(Interpolation interp, const FileTransform & fileTransform)
{
    std::ostringstream oss;
    oss << "Interpolation specified by FileTransform '";
    oss << InterpolationToString(interp);
    oss << "' is not allowed with the given file: '";
    oss << std::string(fileTransform.getSrc()) << "'.";
    LogWarning(oss.str());
}

TransformDirection CombineDirections(TransformDirection dir, const FileTransform & fileTransform)
{
    const auto  newDir = CombineTransformDirections(dir, fileTransform.getDirection());
    if (newDir == TRANSFORM_DIR_UNKNOWN)
    {
        std::ostringstream oss;
        oss << "Cannot build file transform, unspecified transform direction for file: '";
        oss << std::string(fileTransform.getSrc()) << "'.";
        throw Exception(oss.str().c_str());
    }
    return newDir;
}
} // OCIO_NAMESPACE
