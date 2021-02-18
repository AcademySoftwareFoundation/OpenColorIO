// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "utils/StringUtils.h"
#include "LegacyViewingPipeline.h"

namespace OCIO_NAMESPACE
{

LegacyViewingPipelineRcPtr LegacyViewingPipeline::Create()
{
    return LegacyViewingPipelineRcPtr(new LegacyViewingPipelineImpl(),
                                      &LegacyViewingPipelineImpl::Deleter);

}

void LegacyViewingPipelineImpl::Deleter(LegacyViewingPipeline * vp)
{
    delete static_cast<LegacyViewingPipelineImpl *>(vp);
}

ConstDisplayViewTransformRcPtr LegacyViewingPipelineImpl::getDisplayViewTransform() const noexcept
{
    return m_displayViewTransform;
}

void LegacyViewingPipelineImpl::setDisplayViewTransform(const ConstDisplayViewTransformRcPtr & dt) noexcept
{
    if (dt)
    {
        TransformRcPtr tr = dt->createEditableCopy();
        m_displayViewTransform = OCIO_DYNAMIC_POINTER_CAST<DisplayViewTransform>(tr);
        m_dtOriginalLooksBypass = m_displayViewTransform->getLooksBypass();
        m_displayViewTransform->setLooksBypass(true);
    }
    else
    {
        m_displayViewTransform = DisplayViewTransformRcPtr();
    }
}

ConstTransformRcPtr LegacyViewingPipelineImpl::getLinearCC() const noexcept
{
    return m_linearCC;
}

void LegacyViewingPipelineImpl::setLinearCC(const ConstTransformRcPtr & cc) noexcept
{
    if (cc)
    {
        m_linearCC = cc->createEditableCopy();
    }
    else
    {
        m_linearCC = TransformRcPtr();
    }
}

ConstTransformRcPtr LegacyViewingPipelineImpl::getColorTimingCC() const noexcept
{
    return m_colorTimingCC;
}

void LegacyViewingPipelineImpl::setColorTimingCC(const ConstTransformRcPtr & cc) noexcept
{
    if (cc)
    {
        m_colorTimingCC = cc->createEditableCopy();
    }
    else
    {
        m_colorTimingCC = TransformRcPtr();
    }
}

ConstTransformRcPtr LegacyViewingPipelineImpl::getChannelView() const noexcept
{
    return m_channelView;
}

void LegacyViewingPipelineImpl::setChannelView(const ConstTransformRcPtr & transform) noexcept
{
    if (transform)
    {
        m_channelView = transform->createEditableCopy();
    }
    else
    {
        m_channelView = TransformRcPtr();
    }
}

ConstTransformRcPtr LegacyViewingPipelineImpl::getDisplayCC() const noexcept
{
    return m_displayCC;
}

void LegacyViewingPipelineImpl::setDisplayCC(const ConstTransformRcPtr & cc) noexcept
{
    if (cc)
    {
        m_displayCC = cc->createEditableCopy();
    }
    else
    {
        m_displayCC = TransformRcPtr();
    }
}


void LegacyViewingPipelineImpl::setLooksOverrideEnabled(bool enable)
{
    m_looksOverrideEnabled = enable;
}

bool LegacyViewingPipelineImpl::getLooksOverrideEnabled() const
{
    return m_looksOverrideEnabled;
}

void LegacyViewingPipelineImpl::setLooksOverride(const char * looks)
{
    m_looksOverride = looks ? looks : "";
}

const char * LegacyViewingPipelineImpl::getLooksOverride() const
{
    return m_looksOverride.c_str();
}

void LegacyViewingPipelineImpl::validate() const
{
    if (!m_displayViewTransform)
    {
        throw Exception("LegacyViewingPipeline: can't create a processor without "
                        "a display transform.");
    }

    try
    {
        m_displayViewTransform->validate();

        if (m_linearCC)
        {
            m_linearCC->validate();
        }
        if (m_colorTimingCC)
        {
            m_colorTimingCC->validate();
        }
        if (m_channelView)
        {
            m_channelView->validate();
        }
        if (m_displayCC)
        {
            m_displayCC->validate();
        }
    }
    catch (Exception & e)
    {
        std::ostringstream oss;
        oss << "LegacyViewingPipeline is not valid: "
            << e.what();
        throw Exception(oss.str().c_str());
    }
}

ConstProcessorRcPtr LegacyViewingPipelineImpl::getProcessor(const ConstConfigRcPtr & config) const
{
    return getProcessor(config, config->getCurrentContext());
}

ConstProcessorRcPtr LegacyViewingPipelineImpl::getProcessor(const ConstConfigRcPtr & configIn,
                                                            const ConstContextRcPtr & context) const
{
    validate();

    // Get direction from display transform.
    const TransformDirection dir = m_displayViewTransform->getDirection();

    ConstConfigRcPtr config = configIn;

    const std::string inputColorSpaceName = m_displayViewTransform->getSrc();
    ConstColorSpaceRcPtr inputColorSpace = config->getColorSpace(inputColorSpaceName.c_str());

    if (!inputColorSpace)
    {
        std::ostringstream os;
        os << "LegacyViewingPipeline error: ";
        if (inputColorSpaceName.empty())
        {
            os << "InputColorSpaceName is unspecified.";
        }
        else
        {
            os << "Cannot find inputColorSpace, named '" << inputColorSpaceName << "'.";
        }
        throw Exception(os.str().c_str());
    }

    const std::string display = m_displayViewTransform->getDisplay();
    const std::string view = m_displayViewTransform->getView();

    const std::string viewTransformName = config->getDisplayViewTransformName(display.c_str(),
                                                                              view.c_str());
    ConstViewTransformRcPtr viewTransform;
    if (!viewTransformName.empty())
    {
        viewTransform = config->getViewTransform(viewTransformName.c_str());
    }

    // NB: If the viewTranform is present, then displayColorSpace is a true display color space
    // rather than a traditional color space.
    const std::string name{ config->getDisplayViewColorSpaceName(display.c_str(), view.c_str()) };
    // A shared view containing a view transform may set the color space to USE_DISPLAY_NAME,
    // in which case we look for a display color space with the same name as the display.
    const bool nameFromDisplay = (0 == strcmp(name.c_str(), OCIO_VIEW_USE_DISPLAY_NAME));
    const std::string displayColorSpaceName{ nameFromDisplay ? display : name };
    ConstColorSpaceRcPtr displayColorSpace = config->getColorSpace(displayColorSpaceName.c_str());
    // If this is not a color space it can be a named transform. Error handling (missing color
    // space or named transform) is handled by display view transform.

    const bool dataBypass = m_displayViewTransform->getDataBypass();
    const bool displayData = (!displayColorSpace ||
                              (displayColorSpace && displayColorSpace->isData())) ? true : false;
    bool skipColorSpaceConversions = dataBypass && (inputColorSpace->isData() || displayData);

    if (dataBypass)
    {
        // If we're viewing alpha, also skip all color space conversions.
        // If the user does uses a different transform for the channel view,
        // in place of a simple matrix, they run the risk that when viewing alpha
        // the colorspace transforms will not be skipped. (I.e., filmlook will be applied
        // to alpha.)  If this ever becomes an issue, additional engineering will be
        // added at that time.

        auto typedChannelView = DynamicPtrCast<const MatrixTransform>(m_channelView);
        if (typedChannelView)
        {
            double matrix44[16];
            typedChannelView->getMatrix(matrix44);

            if ((matrix44[3]>0.0) || (matrix44[7]>0.0) || (matrix44[11]>0.0))
            {
                skipColorSpaceConversions = true;
            }
        }
    }

    std::string currentCSName{ inputColorSpaceName };
    ConstColorSpaceRcPtr dtInputColorSpace = inputColorSpace;

    GroupTransformRcPtr group = GroupTransform::Create();

    if (m_linearCC)
    {
        auto linearCC = config->getProcessor(context, m_linearCC, dir);
        // If it is a no-op, dont bother doing the colorspace conversion.
        if (!linearCC->isNoOp())
        {
            auto sceneLinearCS = config->getColorSpace(ROLE_SCENE_LINEAR);
            dtInputColorSpace = sceneLinearCS;
            if (!dtInputColorSpace)
            {
                std::ostringstream os;
                os << "DisplayViewTransform error:";
                os << " LinearCC requires '" << std::string(ROLE_SCENE_LINEAR);
                os << "' role to be defined.";
                throw Exception(os.str().c_str());
            }

            if (!skipColorSpaceConversions)
            {
                auto cst = ColorSpaceTransform::Create();
                cst->setSrc(currentCSName.c_str());
                cst->setDst(ROLE_SCENE_LINEAR);
                currentCSName = ROLE_SCENE_LINEAR;
                group->appendTransform(cst);
            }

            group->appendTransform(m_linearCC);
        }
    }

    if (m_colorTimingCC)
    {
        auto colorTimingCC = config->getProcessor(context, m_colorTimingCC, dir);
        // If it is a no-op, dont bother doing the colorspace conversion.
        if (!colorTimingCC->isNoOp())
        {
            auto colorTimingCS = config->getColorSpace(ROLE_COLOR_TIMING);
            dtInputColorSpace = colorTimingCS;
            if (!dtInputColorSpace)
            {
                std::ostringstream os;
                os << "DisplayViewTransform error:";
                os << " ColorTimingCC requires '" << std::string(ROLE_COLOR_TIMING);
                os << "' role to be defined.";
                throw Exception(os.str().c_str());
            }

            if (!skipColorSpaceConversions)
            {
                auto cst = ColorSpaceTransform::Create();
                cst->setSrc(currentCSName.c_str());
                cst->setDst(ROLE_COLOR_TIMING);
                currentCSName = ROLE_COLOR_TIMING;
                group->appendTransform(cst);
            }
            group->appendTransform(m_colorTimingCC);
        }
    }

    TransformRcPtr trans = m_displayViewTransform->createEditableCopy();
    DisplayViewTransformRcPtr dt = OCIO_DYNAMIC_POINTER_CAST<DisplayViewTransform>(trans);
    dt->setDirection(TRANSFORM_DIR_FORWARD);

    // Adjust display transform input color space.
    dt->setSrc(currentCSName.c_str());

    // NB: If looksOverrideEnabled is true, always apply the look, even to data color spaces.
    // In other cases, follow what the DisplayViewTransform would do, except skip color space conversions
    // to the process space for Look transforms for data spaces (DisplayViewTransform never skips).
    std::string looks;
    if (m_looksOverrideEnabled)
    {
        looks = m_looksOverride;
    }
    else if (!m_dtOriginalLooksBypass && !skipColorSpaceConversions)
    {
        looks = config->getDisplayViewLooks(display.c_str(), view.c_str());
    }

    if (!looks.empty())
    {
        const char * inCS = dtInputColorSpace->getName();
        const char * outCS = skipColorSpaceConversions ? inCS :
                             LookTransform::GetLooksResultColorSpace(configIn, context,
                                                                     looks.c_str());
        auto lt = LookTransform::Create();
        lt->setSrc(inCS);
        lt->setDst(outCS);
        lt->setLooks(looks.c_str());
        lt->setSkipColorSpaceConversion(skipColorSpaceConversions);

        group->appendTransform(lt);

        // Adjust display transform input color space.
        dt->setSrc(outCS);
    }

    if (m_channelView)
    {
        group->appendTransform(m_channelView);
    }

    // If there is no displayColorSpace it should be a named transform and it has to be applied.
    if (!skipColorSpaceConversions || !displayColorSpace)
    {
        group->appendTransform(dt);
    }

    if (m_displayCC)
    {
        group->appendTransform(m_displayCC);
    }

    return config->getProcessor(context, group, dir);
}

std::ostream & operator<<(std::ostream & os, const LegacyViewingPipeline & pipeline)
{
    bool first = true;
    if (pipeline.getDisplayViewTransform())
    {
        os << "DisplayViewTransform: " << *(pipeline.getDisplayViewTransform());
        first = false;
    }
    if (pipeline.getLinearCC())
    {
        if (!first)
        {
            os << ", ";
        }
        os << "LinearCC: " << *(pipeline.getLinearCC());
        first = false;
    }
    if (pipeline.getColorTimingCC())
    {
        if (!first)
        {
            os << ", ";
        }
        os << "ColorTimingCC: " << *(pipeline.getColorTimingCC());
        first = false;
    }
    if (pipeline.getChannelView())
    {
        if (!first)
        {
            os << ", ";
        }
        os << "ChannelView: " << *(pipeline.getChannelView());
        first = false;
    }
    if (pipeline.getDisplayCC())
    {
        if (!first)
        {
            os << ", ";
        }
        os << "DisplayCC: " << *(pipeline.getDisplayCC());
        first = false;
    }
    if (pipeline.getLooksOverrideEnabled())
    {
        if (!first)
        {
            os << ", ";
        }
        os << "LooksOveerideEnabled";
        first = false;
    }
    const std::string lo{ pipeline.getLooksOverride() };
    if (!lo.empty())
    {
        if (!first)
        {
            os << ", ";
        }
        os << "LooksOveeride: " << lo;
        first = false;
    }
    return os;
}

} // namespace OCIO_NAMESPACE

