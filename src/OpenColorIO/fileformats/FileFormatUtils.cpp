// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FileFormatUtils.h"

#include "Logging.h"

namespace OCIO_NAMESPACE
{
// Handle the interpolation supplied by the FileTransform.  If the interpolation matches what is
// in the cached file, return that LUT.  Otherwise clone the LUT and set the desired interpolation.
// It sets fileInterpUsed to indicate that the fileInterp was legal for the given LUT.
template<class Lut>
OCIO_SHARED_PTR<Lut> HandleLUT(const OCIO_SHARED_PTR<Lut> & fileLut,
                               Interpolation fileInterp,
                               bool & fileInterpUsed)
{
    OCIO_SHARED_PTR<Lut> lut;
    if (fileLut)
    {
        const bool valid = Lut::IsValidInterpolation(fileInterp);
        fileInterpUsed |= valid;

        fileInterp = valid ? fileInterp : INTERP_DEFAULT;

        const auto lutInterp = fileLut->getInterpolation();
        if (Lut::GetConcreteInterpolation(lutInterp) == Lut::GetConcreteInterpolation(fileInterp))
        {
            // Same concrete interpolation, no clone needed.
            lut = fileLut;
        }
        else
        {
            // As the FileTransform interpolation (from the config file) is different from the
            // cached interpolation, clone the LUT and use that new interpolation.
            lut = fileLut->clone();
            lut->setInterpolation(fileInterp);
        }
    }
    return lut;
}

Lut1DOpDataRcPtr HandleLUT1D(const Lut1DOpDataRcPtr & fileLut1D,
                             Interpolation fileInterp,
                             bool & fileInterpUsed)
{
    return HandleLUT<Lut1DOpData>(fileLut1D, fileInterp, fileInterpUsed);
}

Lut3DOpDataRcPtr HandleLUT3D(const Lut3DOpDataRcPtr & fileLut3D,
                             Interpolation fileInterp,
                             bool & fileInterpUsed)
{
    return HandleLUT<Lut3DOpData>(fileLut3D, fileInterp, fileInterpUsed);
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
} // OCIO_NAMESPACE
