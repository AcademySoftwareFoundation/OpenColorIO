// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_FILEFORMATS_CTF_CTFREADERUTILS_H
#define INCLUDED_OCIO_FILEFORMATS_CTF_CTFREADERUTILS_H

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{

Interpolation GetInterpolation1D(const char * str);
const char * GetInterpolation1DName(Interpolation interp);
Interpolation GetInterpolation3D(const char * str);
const char * GetInterpolation3DName(Interpolation interp);
void ConvertStringToGradingStyleAndDir(const char * str,
                                       GradingStyle & style,
                                       TransformDirection & dir);
const char * ConvertGradingStyleAndDirToString(GradingStyle style, TransformDirection dir);


static constexpr char TAG_ACES[] = "ACES";
static constexpr char TAG_ACES_PARAMS[] = "ACESParams";
static constexpr char TAG_ARRAY[] = "Array";
static constexpr char TAG_CDL[] = "ASC_CDL";
static constexpr char TAG_CURVE_CTRL_PNTS[] = "ControlPoints";
static constexpr char TAG_CURVE_SLOPES[] = "Slopes";
static constexpr char TAG_DYN_PROP_CONTRAST[] = "CONTRAST";
static constexpr char TAG_DYN_PROP_EXPOSURE[] = "EXPOSURE";
static constexpr char TAG_DYN_PROP_GAMMA[] = "GAMMA";
static constexpr char TAG_DYN_PROP_PRIMARY[] = "PRIMARY";
static constexpr char TAG_DYN_PROP_RGBCURVE[] = "RGB_CURVE";
static constexpr char TAG_DYN_PROP_TONE[] = "TONE";
static constexpr char TAG_DYN_PROP_LOOK[] = "LOOK_SWITCH";
static constexpr char TAG_DYNAMIC_PARAMETER[] = "DynamicParameter";
static constexpr char TAG_EXPONENT[] = "Exponent";
static constexpr char TAG_EXPONENT_PARAMS[] = "ExponentParams";
static constexpr char TAG_EXPOSURE_CONTRAST[] = "ExposureContrast";
static constexpr char TAG_EC_PARAMS[] = "ECParams";
static constexpr char TAG_FIXED_FUNCTION[] = "FixedFunction";
static constexpr char TAG_FUNCTION[] = "Function";
static constexpr char TAG_GAMMA[] = "Gamma";
static constexpr char TAG_GAMMA_PARAMS[] = "GammaParams";
static constexpr char TAG_INDEX_MAP[] = "IndexMap";
static constexpr char TAG_INFO[] = "Info";
static constexpr char TAG_INVLUT1D[] = "InverseLUT1D";
static constexpr char TAG_INVLUT3D[] = "InverseLUT3D";
static constexpr char TAG_LOG[] = "Log";
static constexpr char TAG_LOG_PARAMS[] = "LogParams";
static constexpr char TAG_LUT1D[] = "LUT1D";
static constexpr char TAG_LUT3D[] = "LUT3D";
static constexpr char TAG_MATRIX[] = "Matrix";
static constexpr char TAG_MAX_IN_VALUE[] = "maxInValue";
static constexpr char TAG_MAX_OUT_VALUE[] = "maxOutValue";
static constexpr char TAG_MIN_IN_VALUE[] = "minInValue";
static constexpr char TAG_MIN_OUT_VALUE[] = "minOutValue";
static constexpr char TAG_PRIMARY[] = "GradingPrimary";
static constexpr char TAG_PRIMARY_BRIGHTNESS[] = "Brightness";
static constexpr char TAG_PRIMARY_CLAMP[] = "Clamp";
static constexpr char TAG_PRIMARY_CONTRAST[] = "Contrast";
static constexpr char TAG_PRIMARY_EXPOSURE[] = "Exposure";
static constexpr char TAG_PRIMARY_GAIN[] = "Gain";
static constexpr char TAG_PRIMARY_GAMMA[] = "Gamma";
static constexpr char TAG_PRIMARY_LIFT[] = "Lift";
static constexpr char TAG_PRIMARY_OFFSET[] = "Offset";
static constexpr char TAG_PRIMARY_PIVOT[] = "Pivot";
static constexpr char TAG_PRIMARY_SATURATION[] = "Saturation";
static constexpr char TAG_PROCESS_LIST[] = "ProcessList";
static constexpr char TAG_RANGE[] = "Range";
static constexpr char TAG_REFERENCE[] = "Reference";
static constexpr char TAG_RGB_CURVE[] = "GradingRGBCurve";
static constexpr char TAG_RGB_CURVE_BLUE[] = "Blue";
static constexpr char TAG_RGB_CURVE_GREEN[] = "Green";
static constexpr char TAG_RGB_CURVE_MASTER[] = "Master";
static constexpr char TAG_RGB_CURVE_RED[] = "Red";
static constexpr char TAG_TONE[] = "GradingTone";
static constexpr char TAG_TONE_BLACKS[] = "Blacks";
static constexpr char TAG_TONE_HIGHLIGHTS[] = "Highlights";
static constexpr char TAG_TONE_MIDTONES[] = "Midtones";
static constexpr char TAG_TONE_SCONTRAST[] = "SContrast";
static constexpr char TAG_TONE_SHADOWS[] = "Shadows";
static constexpr char TAG_TONE_WHITES[] = "Whites";

static constexpr char ATTR_ALIAS[] = "alias";
static constexpr char ATTR_BASE[] = "base";
static constexpr char ATTR_BASE_PATH[] = "basePath";
static constexpr char ATTR_BITDEPTH_IN[] = "inBitDepth";
static constexpr char ATTR_BITDEPTH_OUT[] = "outBitDepth";
static constexpr char ATTR_BYPASS[] = "bypass";
static constexpr char ATTR_BYPASS_LIN_TO_LOG[] = "bypassLinToLog";
static constexpr char ATTR_CENTER[] = "center";
static constexpr char ATTR_CHAN[] = "channel";
static constexpr char ATTR_COMP_CLF_VERSION[] = "compCLFversion";
static constexpr char ATTR_CONTRAST[] = "contrast";
static constexpr char ATTR_DIMENSION[] = "dim";
static constexpr char ATTR_DIRECTION[] = "dir";
static constexpr char ATTR_EXPONENT[] = "exponent";
static constexpr char ATTR_EXPOSURE[] = "exposure";
static constexpr char ATTR_GAMMA[] = "gamma";
static constexpr char ATTR_HALF_DOMAIN[] = "halfDomain";
static constexpr char ATTR_HIGHLIGHT[] = "highlight";
static constexpr char ATTR_HUE_ADJUST[] = "hueAdjust";
static constexpr char ATTR_INTERPOLATION[] = "interpolation";
static constexpr char ATTR_INVERSE_OF[] = "inverseOf";
static constexpr char ATTR_IS_INVERTED[] = "inverted";
static constexpr char ATTR_LINEARSLOPE[] = "linearSlope";
static constexpr char ATTR_LINSIDEBREAK[] = "linSideBreak";
static constexpr char ATTR_LINSIDESLOPE[] = "linSideSlope";
static constexpr char ATTR_LINSIDEOFFSET[] = "linSideOffset";
static constexpr char ATTR_LOGEXPOSURESTEP[] = "logExposureStep";
static constexpr char ATTR_LOGMIDGRAY[] = "logMidGray";
static constexpr char ATTR_LOGSIDESLOPE[] = "logSideSlope";
static constexpr char ATTR_LOGSIDEOFFSET[] = "logSideOffset";
static constexpr char ATTR_MASTER[] = "master";
static constexpr char ATTR_OFFSET[] = "offset";
static constexpr char ATTR_PARAM[] = "param";
static constexpr char ATTR_PARAMS[] = "params";
static constexpr char ATTR_PATH[] = "path";
static constexpr char ATTR_PIVOT[] = "pivot";
static constexpr char ATTR_PRIMARY_BLACK[] = "black";
static constexpr char ATTR_PRIMARY_CONTRAST[] = "contrast";
static constexpr char ATTR_PRIMARY_WHITE[] = "white";
static constexpr char ATTR_RAW_HALFS[] = "rawHalfs";
static constexpr char ATTR_REFBLACK[] = "refBlack";
static constexpr char ATTR_REFWHITE[] = "refWhite";
static constexpr char ATTR_RGB[] = "rgb";
static constexpr char ATTR_SHADOW[] = "shadow";
static constexpr char ATTR_START[] = "start";
static constexpr char ATTR_STYLE[] = "style";
static constexpr char ATTR_VERSION[] = "version";
static constexpr char ATTR_WIDTH[] = "width";

} // namespace OCIO_NAMESPACE

#endif