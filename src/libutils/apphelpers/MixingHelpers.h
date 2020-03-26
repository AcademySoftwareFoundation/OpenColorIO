// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_MIXING_HELPERS_H
#define INCLUDED_OCIO_MIXING_HELPERS_H


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

// The MixingSlider and MixingColorSpaceManager classes are to help applications implement correct 
// color pickers.  The term "color mixing" is used here to describe what is done in a typical 
// application "color picker" user interface.
//
// A user may want to mix colors in different color spaces.  The two most common mixing space 
// options are a scene-linear working space or the display space. 
//
// Since scene-linear color spaces are not perceptually uniform, it is necessary to compensate UI 
// widgets such as sliders.  For example, it is nice if mid-gray falls near the center of mixing
// controls rather than way over near the black end.  This may be done by using a mapping from 
// linear into an approximately perceptually uniform space. 
//
// Also note that a color picking/mixing UI may want to present a given color space in several
// different encodings.  The most common two encodings for color mixing are RGB and HSV.
//
// Note that these helpers anticipate that a user may want to mix colors using values that extend
// outside the typical [0,1] domain.


class MixingSlider
{
public:
    // Set the minimum edge of a UI slider for conversion to mixing space.
    virtual void setSliderMinEdge(float sliderMixingMinEdge) noexcept = 0;

    // Minimum edge of a UI slider for conversion to mixing space.
    virtual float getSliderMinEdge() const noexcept = 0;

    // Set the maximum edge of a UI slider for conversion to mixing space.
    virtual void setSliderMaxEdge(float sliderMixingMaxEdge) noexcept = 0;

    // Maximum edge of a UI slider for conversion to mixing space.
    virtual float getSliderMaxEdge() const noexcept = 0;

    // Convert from units in distance along the slider to mixing space units.
    virtual float sliderToMixing(float sliderUnits) const noexcept = 0;

    // Convert from mixing space units to distance along the slider.
    virtual float mixingToSlider(float mixingUnits) const noexcept = 0;

protected:
    MixingSlider() = default;
    virtual ~MixingSlider() = default;

private:
    MixingSlider(const MixingSlider &) = delete;
    MixingSlider & operator=(const MixingSlider &) = delete;
};


class MixingColorSpaceManager;
using MixingColorSpaceMenuRcPtr = OCIO_SHARED_PTR<MixingColorSpaceManager>;


class MixingColorSpaceManager
{
public:

    static MixingColorSpaceMenuRcPtr Create(ConstConfigRcPtr & config);

    // Access to the mixing spaces.
    virtual size_t getNumMixingSpaces() const noexcept = 0;
    virtual const char * getMixingSpaceUIName(size_t idx) const = 0;
    virtual size_t getSelectedMixingSpaceIdx() const noexcept = 0;
    virtual void setSelectedMixingSpaceIdx(size_t idx) = 0;
    virtual void setSelectedMixingSpace(const char * mixingSpace) = 0;

    virtual bool isPerceptuallyUniform() const noexcept = 0;

    // Access to the mixing encodings.
    virtual size_t getNumMixingEncodings() const noexcept = 0;
    virtual const char * getMixingEncodingName(size_t idx) const = 0;
    virtual size_t getSelectedMixingEncodingIdx() const noexcept = 0;
    virtual void setSelectedMixingEncodingIdx(size_t idx) = 0;
    virtual void setSelectedMixingEncoding(const char * mixingEncoding) = 0;

    // Refresh the instance (i.e. needed following a configuration change for example).
    virtual void refresh(ConstConfigRcPtr config) = 0;

    virtual ConstProcessorRcPtr getProcessor(const char * workingName,
                                             const char * displayName,
                                             const char * viewName,
                                             TransformDirection direction) const = 0;

    virtual MixingSlider & getSlider() noexcept = 0;
    virtual MixingSlider & getSlider(float sliderMixingMinEdge,
                                    float sliderMixingMaxEdge) noexcept = 0;

protected:
    MixingColorSpaceManager() = default;
    virtual ~MixingColorSpaceManager() = default;

private:
    MixingColorSpaceManager(const MixingColorSpaceManager &) = delete;
    MixingColorSpaceManager & operator=(const MixingColorSpaceManager &) = delete;
};


}  // namespace OCIO_NAMESPACE


#endif // INCLUDED_OCIO_MIXING_HELPERS_H
