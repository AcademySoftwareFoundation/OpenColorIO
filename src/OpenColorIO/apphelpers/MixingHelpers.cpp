// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "CategoryHelpers.h"
#include "ColorSpaceHelpers.h"
#include "MixingHelpers.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{
namespace
{

constexpr float GAMMA       = 2.0f;
constexpr float LOGSLOPE    = 0.55f;
constexpr float BREAKPNT    = 0.18f;
constexpr float NEGSLOPE    = 0.5f;
constexpr float INVGAMMA    = 0.5f;                 // 1/GAMMA
constexpr float INVBREAKPNT = 0.42426406871192851f; // BREAKPNT^(1/GAMMA)
constexpr float LOGOFFSET   = 0.83386419090511021f; // BREAKPNT^(1/GAMMA) - log10(BREAKPNT)*LOGSLOPE

float linearToPerceptual(float linear)
{
    if (linear <= 0.f)
        return linear * NEGSLOPE;
    else if (linear > BREAKPNT)
        return LOGOFFSET + std::log10(linear) * LOGSLOPE;
    else
        return std::pow(linear, INVGAMMA);
}

float perceptualToLinear(float percept)
{
    if (percept <= 0.f)
        return percept / NEGSLOPE;
    else if (percept > INVBREAKPNT)
        return std::pow(10.f, (percept - LOGOFFSET) / LOGSLOPE);
    else
        return std::pow(percept, GAMMA);
}

} // anon.


MixingSliderImpl::MixingSliderImpl(MixingColorSpaceManager & mixing)
    :   MixingSlider()
    ,   m_mixing(mixing)
{
}

void MixingSliderImpl::Deleter(MixingSlider * slider)
{
    delete (MixingSliderImpl*)slider;
}

float MixingSliderImpl::getSliderMinEdge() const noexcept
{
    return (!m_mixing.isPerceptuallyUniform()
                ? linearToPerceptual(std::min(m_sliderMinEdge, m_sliderMaxEdge - 0.01f))
                : m_sliderMinEdge);
}

void MixingSliderImpl::setSliderMinEdge(float sliderMixingMinEdge) noexcept
{
    m_sliderMinEdge = sliderMixingMinEdge;
}

float MixingSliderImpl::getSliderMaxEdge() const noexcept
{
    return (!m_mixing.isPerceptuallyUniform()
                ? linearToPerceptual(std::max(m_sliderMaxEdge, m_sliderMinEdge + 0.01f))
                : m_sliderMaxEdge);
}

void MixingSliderImpl::setSliderMaxEdge(float sliderMixingMaxEdge) noexcept
{
    m_sliderMaxEdge = sliderMixingMaxEdge;
}

float MixingSliderImpl::sliderToMixing(float sliderUnits) const noexcept
{
    const float percept
        = getSliderMinEdge() + sliderUnits * (getSliderMaxEdge() - getSliderMinEdge());

    return (!m_mixing.isPerceptuallyUniform() ? perceptualToLinear(percept) : percept);
}

float MixingSliderImpl::mixingToSlider(float mixingUnits) const noexcept
{
    const float percept
        = (!m_mixing.isPerceptuallyUniform() ? linearToPerceptual(mixingUnits) : mixingUnits);

    // The slider is a window onto the perceptual units.
    // Apply affine based on current left/right or min/max edges of the UI.
    const float sliderUnits
        = (percept - getSliderMinEdge()) / (getSliderMaxEdge() - getSliderMinEdge());

    return sliderUnits;
}


std::ostream & operator<<(std::ostream & os, const MixingSlider & ms)
{
    os << "minEdge: " << ms.getSliderMinEdge();
    os << ", maxEdge: " << ms.getSliderMaxEdge();
    return os;
}

MixingColorSpaceManagerRcPtr MixingColorSpaceManager::Create(ConstConfigRcPtr & config)
{
    return std::shared_ptr<MixingColorSpaceManager>(new MixingColorSpaceManagerImpl(config),
                                                    &MixingColorSpaceManagerImpl::Deleter);
}

void MixingColorSpaceManagerImpl::Deleter(MixingColorSpaceManager * incs)
{
    delete (MixingColorSpaceManagerImpl*)incs;
}

MixingColorSpaceManagerImpl::MixingColorSpaceManagerImpl(ConstConfigRcPtr & config)
    :   MixingColorSpaceManager()
    ,   m_config(config)
    ,   m_slider(*this)
{
    refresh();
}

void MixingColorSpaceManagerImpl::refresh(ConstConfigRcPtr config)
{
    m_config = config;
    refresh();
}

void MixingColorSpaceManagerImpl::refresh()
{
    // Add the mixing spaces.

    m_selectedMixingSpaceIdx = 0;
    m_mixingSpaces.clear();

    m_colorPicker.reset();

    if (m_config->hasRole(ROLE_COLOR_PICKING))
    {
        m_colorPicker = ColorSpaceInfo::CreateFromSingleRole(m_config, ROLE_COLOR_PICKING);
        m_mixingSpaces.push_back(m_colorPicker->getUIName());
    }
    else
    {
        // TODO: Replace the 'Display Space' entry (i.e. the color space of the monitor)
        // by the list of all the display color spaces from the configuration 
        // when the feature is in.
        m_mixingSpaces.push_back("Rendering Space");
        m_mixingSpaces.push_back("Display Space");
    }

    // The mixing encodings.

    m_selectedMixingEncodingIdx = 0;
}

size_t MixingColorSpaceManagerImpl::getNumMixingSpaces() const noexcept
{
    return m_mixingSpaces.size();
}

const char * MixingColorSpaceManagerImpl::getMixingSpaceUIName(size_t idx) const
{
    if (idx < m_mixingSpaces.size())
    {
        return m_mixingSpaces[idx].c_str();
    }

    std::stringstream ss;
    ss << "Invalid mixing space index " << idx
       << " where size is " << m_mixingSpaces.size() << ".";

    throw Exception(ss.str().c_str());
}

size_t MixingColorSpaceManagerImpl::getSelectedMixingSpaceIdx() const noexcept
{
    return m_selectedMixingSpaceIdx;
}

void MixingColorSpaceManagerImpl::setSelectedMixingSpaceIdx(size_t idx)
{
    if (idx >= m_mixingSpaces.size())
    {
        std::stringstream ss;
        ss << "Invalid idx for the mixing space index " << idx
           << " where size is " << m_mixingSpaces.size() << ".";
        throw Exception(ss.str().c_str());
    }

    m_selectedMixingSpaceIdx = idx;
}

void MixingColorSpaceManagerImpl::setSelectedMixingSpace(const char * mixingSpace)
{
    for (size_t idx = 0 ; idx < m_mixingSpaces.size(); ++idx)
    {
        if (m_mixingSpaces[idx] == mixingSpace)
        {
            m_selectedMixingSpaceIdx = idx;
            return;
        }
    }

    std::stringstream ss;
    ss << "Invalid mixing space name: '" << mixingSpace << "'.";
    throw Exception(ss.str().c_str());
}

bool MixingColorSpaceManagerImpl::isPerceptuallyUniform() const noexcept
{
    // TODO: This response should vary as a function of the mixing space.
    // (The limited options above allow us to hard-code for now.)

    // Only Display color spaces are perceptually linear.
    // TODO: It's probably not always reasonable to assume the color_picking role is 
    // perceptually uniform.
    return !m_colorPicker ? getSelectedMixingSpaceIdx()!=0 : true;
}

size_t MixingColorSpaceManagerImpl::getNumMixingEncodings() const noexcept
{
    return m_mixingEncodings.size();
}

const char * MixingColorSpaceManagerImpl::getMixingEncodingName(size_t idx) const
{
    if (idx < m_mixingEncodings.size())
    {
        return m_mixingEncodings[idx].c_str();
    }

    std::stringstream ss;
    ss << "Invalid mixing encoding index " << idx
       << " where size is " << m_mixingEncodings.size() << ".";

    throw Exception(ss.str().c_str());
}

size_t MixingColorSpaceManagerImpl::getSelectedMixingEncodingIdx() const noexcept
{
    return m_selectedMixingEncodingIdx;
}

void MixingColorSpaceManagerImpl::setSelectedMixingEncodingIdx(size_t idx)
{
    if (idx >= m_mixingEncodings.size())
    {
        std::stringstream ss;
        ss << "Invalid idx for the mixing encoding index " << idx
           << " where size is " << m_mixingEncodings.size() << ".";
        throw Exception(ss.str().c_str());
    }

    m_selectedMixingEncodingIdx = idx;
}

void MixingColorSpaceManagerImpl::setSelectedMixingEncoding(const char * mixingEncoding)
{
    for (size_t idx = 0 ; idx < m_mixingEncodings.size(); ++idx)
    {
        if (m_mixingEncodings[idx] == mixingEncoding)
        {
            m_selectedMixingEncodingIdx = idx;
            return;
        }
    }

    std::stringstream ss;
    ss << "Invalid mixing encoding: '" << mixingEncoding << "'.";
    throw Exception(ss.str().c_str());
}

// Get a processor to convert from the working/rendering space to the mixing space 
// (using the RGB encoding rather than HSV).
ConstProcessorRcPtr MixingColorSpaceManagerImpl::getProcessorWithoutEncoding(const char * workingName,
                                                                             const char * displayName,
                                                                             const char * viewName) const
{
    if (m_colorPicker)
    {
        // Mix colors in the color_picker role color space.
        return m_config->getProcessor(workingName, m_colorPicker->getName());
    }
    else
    {
        if (getSelectedMixingSpaceIdx()>0)
        {
            // Mix colors in the selected (display, view) space.
            auto dt = DisplayViewTransform::Create();
            dt->setDisplay(displayName);
            dt->setView(viewName);
            dt->setSrc(workingName);
            return m_config->getProcessor(dt);
        }
        else
        {
            // Mix colors directly in the working/rendering space.
            MatrixTransformRcPtr tr = MatrixTransform::Create();
            return m_config->getProcessor(tr);
        }
    }
}

ConstProcessorRcPtr MixingColorSpaceManagerImpl::getProcessor(const char * workingName,
                                                              const char * displayName,
                                                              const char * viewName,
                                                              TransformDirection direction) const
{
    GroupTransformRcPtr group = GroupTransform::Create();

    ConstProcessorRcPtr processor 
        = getProcessorWithoutEncoding(workingName, displayName, viewName);

    group->appendTransform(processor->createGroupTransform());

    if (getSelectedMixingEncodingIdx()==1)  // i.e. HSV
    {
        FixedFunctionTransformRcPtr tr = FixedFunctionTransform::Create(FIXED_FUNCTION_RGB_TO_HSV);

        group->appendTransform(tr);
    }

    return m_config->getProcessor(group, direction);
}

MixingSlider & MixingColorSpaceManagerImpl::getSlider() noexcept
{
    return m_slider;
}

MixingSlider & MixingColorSpaceManagerImpl::getSlider(float sliderMixingMinEdge,
                                                      float sliderMixingMaxEdge) noexcept
{
    m_slider.setSliderMinEdge(sliderMixingMinEdge);
    m_slider.setSliderMaxEdge(sliderMixingMaxEdge);
    return m_slider;
}

std::ostream & MixingColorSpaceManagerImpl::serialize(std::ostream & os) const
{
    os << "config: " << m_config->getCacheID();
    os << ", slider: [" << m_slider << "]";
    if (!m_mixingSpaces.empty())
    {
        os << ", mixingSpaces: [";
        bool first = true;
        for (const auto & cs : m_mixingSpaces)
        {
            if (!first)
            {
                os << ", ";
            }
            os << cs;
            first = false;
        }
        os << "]";
    }
    os << ", selectedMixingSpaceIdx: " << m_selectedMixingSpaceIdx;
    os << ", selectedMixingEncodingIdx: " << m_selectedMixingEncodingIdx;
    if (m_colorPicker)
    {
        os << ", colorPicking";
    }
    return os;
}

std::ostream & operator<<(std::ostream & os, const MixingColorSpaceManager & mcsm)
{
    const auto & impl = dynamic_cast<const MixingColorSpaceManagerImpl &>(mcsm);
    impl.serialize(os);
    return os;
}

} // namespace OCIO_NAMESPACE
