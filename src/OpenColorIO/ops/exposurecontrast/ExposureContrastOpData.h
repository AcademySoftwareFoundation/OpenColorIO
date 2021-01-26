// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_EXPOSURECONTRASTOPDATA_H
#define INCLUDED_OCIO_EXPOSURECONTRASTOPDATA_H

#include <OpenColorIO/OpenColorIO.h>

#include "DynamicProperty.h"
#include "Op.h"

namespace OCIO_NAMESPACE
{

namespace EC
{
constexpr double MIN_PIVOT = 0.001;
constexpr double MIN_CONTRAST = 0.001;
constexpr double VIDEO_OETF_POWER = 0.54644808743169393;  // 1 / 1.83
}

class ExposureContrastOpData;
typedef OCIO_SHARED_PTR<ExposureContrastOpData> ExposureContrastOpDataRcPtr;
typedef OCIO_SHARED_PTR<const ExposureContrastOpData> ConstExposureContrastOpDataRcPtr;

// The ExposureContrast op is designed to enable interactive exposure and
// contrast/gamma adjustments in application viewport windows.  Typically this
// op will be inserted into the view + display transform as part of the viewing
// pipeline that converts the working color space into a display color space.
// 
// The op utilizes the DynamicProperties class in order to allow the exposure /
// contrast controls to be adjusted by the user in real-time, even after the
// Processor has been created and its ops have been finalized.  Three dynamic
// properties (or parameters) are available:
// 
//   Exposure -- The exposure control attempts to simulate an adjustment to
//   the amount of light hitting the scene.  It is in units of photographic
//   stops.  The default is 0, and +1 is twice the amount light, -1 is half
//   the light.  Ideally it is applied in scene-referred linear or log space
//   (before the viewing transform).
// 
//   Contrast & Gamma -- The contrast and gamma controls raise or lower the
//   contrast relative to the pivot point.  Values above the pivot point get
//   brighter as the control is increased, values below the pivot point get
//   darker.
// 
// It is useful to be able to apply a contrast/gamma-type control both before
// and after a viewing transform.  The contrast control is typically applied in
// scene-referred linear or log space (before the viewing transform) and allows
// contrast to be increased or decreased around a mid-gray pivot.  Reducing the
// contrast is similar to switching to a "log view" of the image.  The gamma
// control is typically applied in display space (after the viewing and display
// transform) with a pivot of 1.  This allows the highlights and shadows of the
// final image to be carefully inspected.  Mathematically, the contrast and
// gamma controls are the same, but two controls are provided since applications
// will likely want to make both available to users and hence two properties 
// are required.
// 
// The most common scenario in visual effects is for the application working
// space to be scene-linear.  However there are scenarios where applications
// would like to offer exposure/contrast controls for images in log or video
// color space (without the overhead of converting to scene-linear just to make
// these controls available).  Therefore, the op provides algorithms for linear, 
// log, and video style use-cases.  The math is adjusted so that the user 
// experience remains constant.
// 
// LINEAR STYLE
// 
//   OUT = pivot * ( IN * 2^exposure / pivot )^contrast
// 
// VIDEO STYLE
// 
//   OUT = pivot^G * ( IN * (2^exposure / pivot)^G )^contrast
// 
// LOG STYLE
// 
//   logPivot = log2( pivot / 0.18 ) * logExposureStep + logMidGray
//   OUT = ( IN + exposure * logExposureStep - logPivot ) * contrast + logPivot
// 
// The pivot, logExposureStep, and logMidGray are settable (non-dynamic)
// parameters.
// 
// Consider the following simplistic conversions from scene-linear to video and
// logarithmic space:
// 
//   VIDEO = LINEAR^G
//   LOG = log2( LINEAR / 0.18 ) * logExposureStep + logMidGray
// 
// For this scenario, one may show that the exposure and contrast/gamma controls
// have exactly the same behavior for all three cases.  In practice the video
// and log conversions will be more complicated, but hopefully this provides 
// some insight into the design of the equations.
// 
// The G parameter is set to 1/1.83.  This value was chosen since that power is
// a decent approximation to the ITU-R BT.709 OETF (i.e., camera) curve.  Note
// that for a scene-linear value of 0.18, the BT.709 curve gives 0.409, the
// simple gamma gives 0.392, and the ACES 1.0 Rec.709 Output Transform gives
// 0.389.  (Note the power was chosen as an overall approximation to the tone
// scale, not the value that maps 18% exactly.  The gray values are cited simply
// to give some confidence that it is a reasonable choice.)  The value is
// currently hard-coded but could be brought forward to the API in the future,
// if necessary.
// 
// The logExposureStep parameter is the size of one stop exposure change as
// measured in the [0-1] normalized log encoding.  This is settable in the API
// since there is some variation in typical encodings.  For example, Cineon used
// 90/1023 = 0.088 but more recent encodings try to cover a wider exposure range
// and so the step size is smaller, e.g. LogC is roughly 0.074 and ACEScct is
// 0.057.  The default is 0.088 but in practice the exact value is not critical
// since users are typically using these controls simply to dynamically explore
// their images on-the-fly rather than to bake in precise adjustments.
// 
// The logMidGray parameter is the position of a scene-linear value of 0.18 as
// represented in the [0-1] normalized log encoding.  This is settable in the
// API since there is some variation in typical encodings.  For example, ADX10
// uses 445/1023 = 0.435, whereas ACEScct is 0.414 and LogC is approximately
// 0.39.  The default is 0.435, but again, the exact value is typically not
// critical.  The log-style controls will continue to do something reasonable
// even if the value is not set to match the specific space being viewed.
//
class ExposureContrastOpData : public OpData
{
public:
    enum Style
    {
        STYLE_LINEAR,          // E/C to be applied to a linear space image
        STYLE_LINEAR_REV,      // Inverse of STYLE_LINEAR
        STYLE_VIDEO,           // E/C to be applied to a video space image
        STYLE_VIDEO_REV,       // Inverse of STYLE_VIDEO
        STYLE_LOGARITHMIC,     // E/C to be applied to a log space image
        STYLE_LOGARITHMIC_REV  // Inverse of STYLE_LOG
    };

    static Style ConvertStringToStyle(const char * str);
    static const char * ConvertStyleToString(Style style);

    static ExposureContrastStyle ConvertStyle(ExposureContrastOpData::Style style);
    static Style ConvertStyle(ExposureContrastStyle style, TransformDirection dir);

    ExposureContrastOpData();

    ExposureContrastOpData(const ExposureContrastOpData &) = delete;

    ExposureContrastOpData(Style style);

    virtual ~ExposureContrastOpData();

    ExposureContrastOpDataRcPtr clone() const;

    Type getType() const override { return ExposureContrastType; }

    void validate() const override;

    bool isNoOp() const override;
    bool isIdentity() const override;
    bool hasChannelCrosstalk() const override { return false; }

    bool isDynamic() const;
    bool isInverse(ConstExposureContrastOpDataRcPtr & r) const;
    ExposureContrastOpDataRcPtr inverse() const;

    std::string getCacheID() const override;

    bool operator==(const OpData & other) const override;

    bool hasDynamicProperty(DynamicPropertyType type) const;

    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;
    void replaceDynamicProperty(DynamicPropertyType type, DynamicPropertyDoubleImplRcPtr & prop);
    void removeDynamicProperties();

    ExposureContrastOpData & operator=(const ExposureContrastOpData & rhs);

    Style getStyle() const noexcept { return m_style; }
    void setStyle(Style style) noexcept { m_style = style; }

    TransformDirection getDirection() const noexcept;
    void setDirection(TransformDirection dir) noexcept;

    double getExposure() const { return m_exposure->getValue(); }
    void setExposure(double exposure) { m_exposure->setValue(exposure); }

    double getContrast() const { return m_contrast->getValue(); }
    void setContrast(double contrast) { m_contrast->setValue(contrast); }

    double getGamma() const { return m_gamma->getValue(); }
    void setGamma(double gamma) { m_gamma->setValue(gamma); }

    double getPivot() const { return m_pivot; }
    void setPivot(double pivot) { m_pivot = pivot; }

    double getLogExposureStep() const { return m_logExposureStep; }
    void setLogExposureStep(double step) { m_logExposureStep = step; }

    double getLogMidGray() const { return m_logMidGray; }
    void setLogMidGray(double midGray) { m_logMidGray = midGray;  }

    DynamicPropertyDoubleImplRcPtr getExposureProperty() const
    { 
        return m_exposure;
    }
    DynamicPropertyDoubleImplRcPtr getContrastProperty() const
    {
        return m_contrast;
    }
    DynamicPropertyDoubleImplRcPtr getGammaProperty() const
    {
        return m_gamma;
    }

    static constexpr double LOGEXPOSURESTEP_DEFAULT = 0.088;
    static constexpr double LOGMIDGRAY_DEFAULT = 0.435;

private:
    void invert() noexcept;

    Style  m_style = STYLE_LINEAR;
    DynamicPropertyDoubleImplRcPtr m_exposure;
    DynamicPropertyDoubleImplRcPtr m_contrast;
    DynamicPropertyDoubleImplRcPtr m_gamma;
    double m_pivot = 0.18;
    double m_logExposureStep = LOGEXPOSURESTEP_DEFAULT;
    double m_logMidGray = LOGMIDGRAY_DEFAULT;
};

} // namespace OCIO_NAMESPACE

#endif
