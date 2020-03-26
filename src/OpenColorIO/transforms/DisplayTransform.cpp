// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"

#include <algorithm>


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
    TransformDirection m_dir = TRANSFORM_DIR_FORWARD;
    std::string m_inputColorSpaceName;
    TransformRcPtr m_linearCC;
    TransformRcPtr m_colorTimingCC;
    TransformRcPtr m_channelView;
    std::string m_display;
    std::string m_view;
    TransformRcPtr m_displayCC;

    std::string m_looksOverride;
    bool m_looksOverrideEnabled = false;

    Impl() = default;
    Impl(const Impl &) = delete;
    ~Impl() = default;

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
    *(transform->m_impl) = *m_impl; // Perform a deep copy.
    return transform;
}

DisplayTransform::~DisplayTransform()
{
    delete m_impl;
    m_impl = nullptr;
}

TransformDirection DisplayTransform::getDirection() const noexcept
{
    return getImpl()->m_dir;
}

void DisplayTransform::setDirection(TransformDirection dir) noexcept
{
    getImpl()->m_dir = dir;
}

void DisplayTransform::validate() const
{
    Transform::validate();

    if (getImpl()->m_inputColorSpaceName.empty())
    {
        throw Exception("DisplayTransform: empty input color space name.");
    }

    if (getImpl()->m_display.empty())
    {
        throw Exception("DisplayTransform: empty display name.");
    }

    if (getImpl()->m_view.empty())
    {
        throw Exception("DisplayTransform: empty view name.");
    }
}

void DisplayTransform::setInputColorSpaceName(const char * name)
{
    getImpl()->m_inputColorSpaceName = name ? name : "";
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
    getImpl()->m_display = display ? display : "";
}

const char * DisplayTransform::getDisplay() const
{
    return getImpl()->m_display.c_str();
}

void DisplayTransform::setView(const char * view)
{
    getImpl()->m_view = view ? view : "";
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
    auto combinedDir = CombineTransformDirections(dir, displayTransform.getDirection());
    if (combinedDir == TRANSFORM_DIR_UNKNOWN)
    {
        std::ostringstream os;
        os << "Cannot build display transform: unspecified transform direction.";
        throw Exception(os.str().c_str());
    }

    const std::string inputColorSpaceName = displayTransform.getInputColorSpaceName();
    ConstColorSpaceRcPtr inputColorSpace = config.getColorSpace(inputColorSpaceName.c_str());
    if (!inputColorSpace)
    {
        std::ostringstream os;
        os << "DisplayTransform error.";
        if (inputColorSpaceName.empty())
        {
            os << " InputColorSpaceName is unspecified.";
        }
        else
        {
            os <<  " Cannot find inputColorSpace, named '" << inputColorSpaceName << "'.";
        }
        throw Exception(os.str().c_str());
    }

    const std::string display = displayTransform.getDisplay();
    const std::string view = displayTransform.getView();

    const std::string viewTransformName = config.getDisplayViewTransformName(display.c_str(), view.c_str());
    ConstViewTransformRcPtr viewTransform;
    if (!viewTransformName.empty())
    {
        viewTransform = config.getViewTransform(viewTransformName.c_str());
    }

    // NB: If the viewTranform is present, then displayColorSpace is a true display color space
    // rather than a traditional color space.
    const std::string displayColorSpaceName = config.getDisplayColorSpaceName(display.c_str(), view.c_str());
    ConstColorSpaceRcPtr displayColorSpace = config.getColorSpace(displayColorSpaceName.c_str());
    if (!displayColorSpace)
    {
        std::ostringstream os;
        os << "DisplayTransform error.";
        if (displayColorSpaceName.empty())
        {
            os << " displayColorSpaceName is unspecified.";
        }
        else
        {
            os <<  " Cannot find display colorspace,  '" << displayColorSpaceName << "'.";
        }
        throw Exception(os.str().c_str());
    }

    bool skipColorSpaceConversions = (inputColorSpace->isData() || displayColorSpace->isData());

    // If we're viewing alpha, also skip all color space conversions.
    // If the user does uses a different transform for the channel view,
    // in place of a simple matrix, they run the risk that when viewing alpha
    // the colorspace transforms will not be skipped. (I.e., filmlook will be applied
    // to alpha.)  If this ever becomes an issue, additional engineering will be
    // added at that time.

    ConstMatrixTransformRcPtr typedChannelView = DynamicPtrCast<const MatrixTransform>(
        displayTransform.getChannelView());
    if (typedChannelView)
    {
        double matrix44[16];
        typedChannelView->getMatrix(matrix44);

        if ((matrix44[3]>0.0) || (matrix44[7]>0.0) || (matrix44[11]>0.0))
        {
            skipColorSpaceConversions = true;
        }
    }

    ConstColorSpaceRcPtr currentColorSpace = inputColorSpace;

    // Apply a transform in ROLE_SCENE_LINEAR.
    ConstTransformRcPtr linearCC = displayTransform.getLinearCC();
    if (linearCC)
    {
        // Put the new ops into a temp array, to see if it's a no-op
        // If it is a no-op, dont bother doing the colorspace conversion.
        OpRcPtrVec tmpOps;
        BuildOps(tmpOps, config, context, linearCC, TRANSFORM_DIR_FORWARD);

        if (!tmpOps.isNoOp())
        {
            ConstColorSpaceRcPtr targetColorSpace = config.getColorSpace(ROLE_SCENE_LINEAR);

            if (!targetColorSpace)
            {
                std::ostringstream os;
                os << "DisplayTransform error.";
                os << " LinearCC requires '" << std::string(ROLE_SCENE_LINEAR);
                os << "' role to be defined.";
                throw Exception(os.str().c_str());
            }

            if (!skipColorSpaceConversions)
            {
                BuildColorSpaceOps(ops, config, context, currentColorSpace, targetColorSpace);
                currentColorSpace = targetColorSpace;
            }

            ops += tmpOps;
        }
    }

    // Apply a color correction, in ROLE_COLOR_TIMING.
    ConstTransformRcPtr colorTimingCC = displayTransform.getColorTimingCC();
    if (colorTimingCC)
    {
        // Put the new ops into a temp array, to see if it's a no-op
        // If it is a no-op, dont bother doing the colorspace conversion.
        OpRcPtrVec tmpOps;
        BuildOps(tmpOps, config, context, colorTimingCC, TRANSFORM_DIR_FORWARD);

        if (!tmpOps.isNoOp())
        {
            ConstColorSpaceRcPtr targetColorSpace = config.getColorSpace(ROLE_COLOR_TIMING);

            if (!targetColorSpace)
            {
                std::ostringstream os;
                os << "DisplayTransform error.";
                os << " ColorTimingCC requires '" << std::string(ROLE_COLOR_TIMING);
                os << "' role to be defined.";
                throw Exception(os.str().c_str());
            }

            if(!skipColorSpaceConversions)
            {
                BuildColorSpaceOps(ops, config, context, currentColorSpace, targetColorSpace);
                currentColorSpace = targetColorSpace;
            }

            ops += tmpOps;
        }
    }

    // Apply a look, if specified.
    LookParseResult looks;
    if (displayTransform.getLooksOverrideEnabled())
    {
        looks.parse(displayTransform.getLooksOverride());
    }
    else if (!skipColorSpaceConversions)
    {
        looks.parse(config.getDisplayLooks(display.c_str(), view.c_str()));
    }

    if (!looks.empty())
    {
        BuildLookOps(ops,
                     currentColorSpace,
                     skipColorSpaceConversions,
                     config,
                     context,
                     looks);
    }

    // Apply a channel view.
    ConstTransformRcPtr channelView = displayTransform.getChannelView();
    if (channelView)
    {
        BuildOps(ops, config, context, channelView, TRANSFORM_DIR_FORWARD);
    }

    // Apply the conversion to the display color space.
    if (!skipColorSpaceConversions)
    {
        if (viewTransform)
        {
            // DisplayColorSpace is display-referred.

            // Convert the currentColorSpace to its reference space.
            BuildColorSpaceToReferenceOps(ops, config, context, currentColorSpace);

            // If necessary, convert to the type of reference space used by the view transform.
            const auto vtRef = viewTransform->getReferenceSpaceType();
            const auto curCSRef = currentColorSpace->getReferenceSpaceType();
            BuildReferenceConversionOps(ops, config, context, curCSRef, vtRef);

            // Apply view transform.
            if (viewTransform->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE))
            {
                BuildOps(ops, config, context,
                         viewTransform->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE),
                         TRANSFORM_DIR_FORWARD);
            }
            else if (viewTransform->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE))
            {
                BuildOps(ops, config, context,
                         viewTransform->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE),
                         TRANSFORM_DIR_INVERSE);
            }

            // Convert from the display-referred reference space to the displayColorSpace.
            BuildColorSpaceFromReferenceOps(ops, config, context, displayColorSpace);
        }
        else
        {
            BuildColorSpaceOps(ops, config, context, currentColorSpace, displayColorSpace);
        }
        currentColorSpace = displayColorSpace;
    }

    // Apply a display cc.
    ConstTransformRcPtr displayCC = displayTransform.getDisplayCC();
    if (displayCC)
    {
        BuildOps(ops, config, context, displayCC, TRANSFORM_DIR_FORWARD);
    }

    // TODO: Implement inverse direction. It is not simply inverting the result, because the
    // from_reference or to_reference transform should be chosen from the direction (when they
    // both exist).

    // TODO: The view transform & display color space pipeline for (display, view) pair requires
    // some adjustements to the inverse computation as it's more accurate to use the
    // ViewTransform inverse if available.

    // Invert the display transform, if requested.
    if (combinedDir == TRANSFORM_DIR_INVERSE)
    {
        ops = ops.invert();
    }
}

} // namespace OCIO_NAMESPACE
