// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"

#include <cmath>
#include <cstring>
#include <iterator>

namespace OCIO_NAMESPACE
{
DisplayTransformRcPtr DisplayTransform::Create()
{
    return DisplayTransformRcPtr(new DisplayTransform(), &deleter);
}

void DisplayTransform::deleter(DisplayTransform* t)
{
    delete t;
}

class DisplayTransform::Impl
{
public:
    TransformDirection m_dir;
    std::string m_inputColorSpaceName;
    TransformRcPtr m_linearCC;
    TransformRcPtr m_colorTimingCC;
    TransformRcPtr m_channelView;
    std::string m_display;
    std::string m_view;
    TransformRcPtr m_displayCC;

    std::string m_looksOverride;
    bool m_looksOverrideEnabled;

    Impl() :
        m_dir(TRANSFORM_DIR_FORWARD),
        m_looksOverrideEnabled(false)
    { }

    Impl(const Impl &) = delete;

    ~Impl()
    { }

    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            m_dir = rhs.m_dir;
            m_inputColorSpaceName = rhs.m_inputColorSpaceName;

            m_linearCC = rhs.m_linearCC?
                rhs.m_linearCC->createEditableCopy() : rhs.m_linearCC;

            m_colorTimingCC = rhs.m_colorTimingCC?
                rhs.m_colorTimingCC->createEditableCopy()
                : rhs.m_colorTimingCC;

            m_channelView = rhs.m_channelView?
                rhs.m_channelView->createEditableCopy() : rhs.m_channelView;

            m_display = rhs.m_display;
            m_view = rhs.m_view;

            m_displayCC = rhs.m_displayCC?
                rhs.m_displayCC->createEditableCopy() : rhs.m_displayCC;

            m_looksOverride = rhs.m_looksOverride;
            m_looksOverrideEnabled = rhs.m_looksOverrideEnabled;
        }
        return *this;
    }
};

///////////////////////////////////////////////////////////////////////////

DisplayTransform::DisplayTransform()
    : m_impl(new DisplayTransform::Impl)
{
}

TransformRcPtr DisplayTransform::createEditableCopy() const
{
    DisplayTransformRcPtr transform = DisplayTransform::Create();
    *(transform->m_impl) = *m_impl;
    return transform;
}

DisplayTransform::~DisplayTransform()
{
    delete m_impl;
    m_impl = NULL;
}

DisplayTransform& DisplayTransform::operator= (const DisplayTransform & rhs)
{
    if (this != &rhs)
    {
        *m_impl = *rhs.m_impl;
    }
    return *this;
}

TransformDirection DisplayTransform::getDirection() const
{
    return getImpl()->m_dir;
}

void DisplayTransform::setDirection(TransformDirection dir)
{
    getImpl()->m_dir = dir;
}

void DisplayTransform::validate() const
{
    Transform::validate();

    // TODO: Improve DisplayTransform::validate()
}

void DisplayTransform::setInputColorSpaceName(const char * name)
{
    getImpl()->m_inputColorSpaceName = name;
}

const char * DisplayTransform::getInputColorSpaceName() const
{
    return getImpl()->m_inputColorSpaceName.c_str();
}

void DisplayTransform::setLinearCC(const ConstTransformRcPtr & cc)
{
    getImpl()->m_linearCC = cc->createEditableCopy();
}

ConstTransformRcPtr DisplayTransform::getLinearCC() const
{
    return getImpl()->m_linearCC;
}

void DisplayTransform::setColorTimingCC(const ConstTransformRcPtr & cc)
{
    getImpl()->m_colorTimingCC = cc->createEditableCopy();
}

ConstTransformRcPtr DisplayTransform::getColorTimingCC() const
{
    return getImpl()->m_colorTimingCC;
}

void DisplayTransform::setChannelView(const ConstTransformRcPtr & transform)
{
    getImpl()->m_channelView = transform->createEditableCopy();
}

ConstTransformRcPtr DisplayTransform::getChannelView() const
{
    return getImpl()->m_channelView;
}

void DisplayTransform::setDisplay(const char * display)
{
    getImpl()->m_display = display;
}

const char * DisplayTransform::getDisplay() const
{
    return getImpl()->m_display.c_str();
}

void DisplayTransform::setView(const char * view)
{
    getImpl()->m_view = view;
}

const char * DisplayTransform::getView() const
{
    return getImpl()->m_view.c_str();
}

void DisplayTransform::setDisplayCC(const ConstTransformRcPtr & cc)
{
    getImpl()->m_displayCC = cc->createEditableCopy();
}

ConstTransformRcPtr DisplayTransform::getDisplayCC() const
{
    return getImpl()->m_displayCC;
}

void DisplayTransform::setLooksOverride(const char * looks)
{
    getImpl()->m_looksOverride = looks;
}

const char * DisplayTransform::getLooksOverride() const
{
    return getImpl()->m_looksOverride.c_str();
}

void DisplayTransform::setLooksOverrideEnabled(bool enabled)
{
    getImpl()->m_looksOverrideEnabled = enabled;
}

bool DisplayTransform::getLooksOverrideEnabled() const
{
    return getImpl()->m_looksOverrideEnabled;
}

std::ostream& operator<< (std::ostream& os, const DisplayTransform& t)
{
    os << "<DisplayTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
    os << "inputColorSpace=" << t.getInputColorSpaceName() << ", ";
    os << "display=" << t.getDisplay() << ", ";
    os << "view=" << t.getView();
    if (t.getLooksOverrideEnabled())
    {
        os << ", looksOverride=" << t.getLooksOverride();
    }
    ConstTransformRcPtr transform(t.getLinearCC());
    if (transform)
    {
        os << ", linearCC: " << *transform;
    }
    transform = t.getColorTimingCC();
    if (transform)
    {
        os << ", colorTimingCC: " << *transform;
    }
    transform = t.getChannelView();
    if (transform)
    {
        os << ", channelView: " << *transform;
    }
    transform = t.getDisplayCC();
    if (transform)
    {
        os << ", displayCC: " << *transform;
    }
    os << ">";
    return os;
}

///////////////////////////////////////////////////////////////////////////

void BuildDisplayOps(OpRcPtrVec & ops,
                        const Config & config,
                        const ConstContextRcPtr & context,
                        const DisplayTransform & displayTransform,
                        TransformDirection dir)
{
    TransformDirection combinedDir = CombineTransformDirections(dir,
                                                displayTransform.getDirection());
    if(combinedDir != TRANSFORM_DIR_FORWARD)
    {
        std::ostringstream os;
        os << "DisplayTransform can only be applied in the forward direction.";
        throw Exception(os.str().c_str());
    }

    std::string inputColorSpaceName = displayTransform.getInputColorSpaceName();
    ConstColorSpaceRcPtr inputColorSpace = config.getColorSpace(inputColorSpaceName.c_str());
    if(!inputColorSpace)
    {
        std::ostringstream os;
        os << "DisplayTransform error.";
        if(inputColorSpaceName.empty()) os << " InputColorSpaceName is unspecified.";
        else os <<  " Cannot find inputColorSpace, named '" << inputColorSpaceName << "'.";
        throw Exception(os.str().c_str());
    }

    std::string display = displayTransform.getDisplay();
    std::string view = displayTransform.getView();

    std::string displayColorSpaceName = config.getDisplayColorSpaceName(display.c_str(), view.c_str());
    ConstColorSpaceRcPtr displayColorspace = config.getColorSpace(displayColorSpaceName.c_str());
    if(!displayColorspace)
    {
        std::ostringstream os;
        os << "DisplayTransform error.";
        os <<  " Cannot find display colorspace,  '" << displayColorSpaceName << "'.";
        throw Exception(os.str().c_str());
    }

    bool skipColorSpaceConversions = (inputColorSpace->isData() || displayColorspace->isData());

    // If we're viewing alpha, also skip all color space conversions.
    // If the user does uses a different transform for the channel view,
    // in place of a simple matrix, they run the risk that when viewing alpha
    // the colorspace transforms will not be skipped. (I.e., filmlook will be applied
    // to alpha.)  If this ever becomes an issue, additional engineering will be
    // added at that time.

    ConstMatrixTransformRcPtr typedChannelView = DynamicPtrCast<const MatrixTransform>(
        displayTransform.getChannelView());
    if(typedChannelView)
    {
        double matrix44[16];
        typedChannelView->getMatrix(matrix44);

        if((matrix44[3]>0.0) || (matrix44[7]>0.0) || (matrix44[11]>0.0))
        {
            skipColorSpaceConversions = true;
        }
    }

    ConstColorSpaceRcPtr currentColorSpace = inputColorSpace;

    // Apply a transform in ROLE_SCENE_LINEAR
    ConstTransformRcPtr linearCC = displayTransform.getLinearCC();
    if(linearCC)
    {
        // Put the new ops into a temp array, to see if it's a no-op
        // If it is a no-op, dont bother doing the colorspace conversion.
        OpRcPtrVec tmpOps;
        BuildOps(tmpOps, config, context, linearCC, TRANSFORM_DIR_FORWARD);

        if(!IsOpVecNoOp(tmpOps))
        {
            ConstColorSpaceRcPtr targetColorSpace = config.getColorSpace(ROLE_SCENE_LINEAR);

            if(!skipColorSpaceConversions)
            {
                BuildColorSpaceOps(ops, config, context,
                                    currentColorSpace,
                                    targetColorSpace);
                currentColorSpace = targetColorSpace;
            }

            ops += tmpOps;
        }
    }

    // Apply a color correction, in ROLE_COLOR_TIMING
    ConstTransformRcPtr colorTimingCC = displayTransform.getColorTimingCC();
    if(colorTimingCC)
    {
        // Put the new ops into a temp array, to see if it's a no-op
        // If it is a no-op, dont bother doing the colorspace conversion.
        OpRcPtrVec tmpOps;
        BuildOps(tmpOps, config, context, colorTimingCC, TRANSFORM_DIR_FORWARD);

        if(!IsOpVecNoOp(tmpOps))
        {
            ConstColorSpaceRcPtr targetColorSpace = config.getColorSpace(ROLE_COLOR_TIMING);

            if(!skipColorSpaceConversions)
            {
                BuildColorSpaceOps(ops, config, context,
                                    currentColorSpace,
                                    targetColorSpace);
                currentColorSpace = targetColorSpace;
            }

            ops += tmpOps;
        }
    }

    // Apply a look, if specified
    LookParseResult looks;
    if(displayTransform.getLooksOverrideEnabled())
    {
        looks.parse(displayTransform.getLooksOverride());
    }
    else if(!skipColorSpaceConversions)
    {
        looks.parse(config.getDisplayLooks(display.c_str(), view.c_str()));
    }

    if(!looks.empty())
    {
        BuildLookOps(ops,
                        currentColorSpace,
                        skipColorSpaceConversions,
                        config,
                        context,
                        looks);
    }

    // Apply a channel view
    ConstTransformRcPtr channelView = displayTransform.getChannelView();
    if(channelView)
    {
        BuildOps(ops, config, context, channelView, TRANSFORM_DIR_FORWARD);
    }

    // Apply the conversion to the display color space
    if(!skipColorSpaceConversions)
    {
        BuildColorSpaceOps(ops, config, context,
                            currentColorSpace,
                            displayColorspace);
        currentColorSpace = displayColorspace;
    }

    // Apply a display cc
    ConstTransformRcPtr displayCC = displayTransform.getDisplayCC();
    if(displayCC)
    {
        BuildOps(ops, config, context, displayCC, TRANSFORM_DIR_FORWARD);
    }

}
} // namespace OCIO_NAMESPACE
