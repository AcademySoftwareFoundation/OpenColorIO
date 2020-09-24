// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_VIEWINGPIPELINE_H
#define INCLUDED_OCIO_VIEWINGPIPELINE_H


#include <string>

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

// Whereas the DisplayViewTransform in OCIO core simply applies a specific view from an OCIO
// display, the ViewingPipeline provides an example of a complete viewing pipeline of the sort
// that could be used to implement a viewport in a typical application.  It therefore adds, around
// the DisplayViewTransform, various optional color correction steps and RGBA channel view
// swizzling. The direction of the DisplayViewTranform is used as the direction of the pipeline.
// Note: The ViewingPipeline class provides the same functionality as the OCIO v1 DisplayTransform.
//
// Viewing pipeline:
// * Start in display transform input color space.
// * If linearCC is provided:
//   * Go to scene_linear colorspace.
//   * Apply linearCC transform.
// * If colorTimingCC is provided:
//   * Go to color_timing colorspace.
//   * Apply colorTimingCC transform.
// * Apply looks (from display transform or from looks override).
//   * Go to first look color space.
//   * Apply first look transform.
//   * Iterate for all looks.
// * Apply channelView transform.
// * Apply display transform (without looks).
// * Apply displayCC.
// Note that looks are applied even if the display transform involves data color spaces.

class ViewingPipeline
{
public:
    ViewingPipeline() = default;
    ~ViewingPipeline() = default;

    ViewingPipeline(const ViewingPipeline &) = delete;
    ViewingPipeline & operator=(const ViewingPipeline &) = delete;

    ConstDisplayViewTransformRcPtr getDisplayViewTransform() const noexcept;
    void setDisplayViewTransform(const ConstDisplayViewTransformRcPtr & dt) noexcept;

    ConstTransformRcPtr getLinearCC() const noexcept;
    void setLinearCC(const ConstTransformRcPtr & cc) noexcept;

    ConstTransformRcPtr getColorTimingCC() const noexcept;
    void setColorTimingCC(const ConstTransformRcPtr & cc) noexcept;

    ConstTransformRcPtr getChannelView() const noexcept;
    void setChannelView(const ConstTransformRcPtr & transform) noexcept;

    ConstTransformRcPtr getDisplayCC() const noexcept;
    void setDisplayCC(const ConstTransformRcPtr & cc) noexcept;

    // Specify whether the lookOverride should be used, or not. This is a separate flag, as
    // it's often useful to override "looks" to an empty string.
    void setLooksOverrideEnabled(bool enable);
    bool getLooksOverrideEnabled() const;

    // A user can optionally override the looks that are,  by default, used with the expected
    // display / view combination.  A common use case for this functionality is in an image
    // viewing app, where per-shot looks are supported.  If for some reason a per-shot look is
    // not defined for the current Context, the Config::getProcessor fcn will not succeed by
    // default.  Thus, with this mechanism the viewing app could override to looks = "", and
    // this will allow image display to continue (though hopefully) the interface would reflect
    // this fallback option.)
    //
    // Looks is a potentially comma (or colon) delimited list of lookNames, Where +/- prefixes
    // are optionally allowed to denote forward/inverse look specification. (And forward is
    // assumed in the absence of either)
    void setLooksOverride(const std::string & looks);
    const std::string & getLooksOverride() const;

    ConstProcessorRcPtr getProcessor(const ConstConfigRcPtr & config,
                                     const ConstContextRcPtr & context) const;

    ConstProcessorRcPtr getProcessor(const ConstConfigRcPtr & config) const;

protected:
    void validate() const;

private:
    TransformRcPtr m_linearCC;
    TransformRcPtr m_colorTimingCC;
    TransformRcPtr m_channelView;
    TransformRcPtr m_displayCC;
    DisplayViewTransformRcPtr m_displayViewTransform;
    // Looks from DisplayViewTransform are applied separately.
    bool m_dtOriginalLooksBypass{ false };

    bool m_looksOverrideEnabled{ false };
    std::string m_looksOverride;
};

} // namespace OCIO_NAMESPACE


#endif // INCLUDED_OCIO_VIEWINGPIPELINETRANSFORM_H
