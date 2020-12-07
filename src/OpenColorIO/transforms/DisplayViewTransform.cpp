// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>

#include "ContextVariableUtils.h"
#include "Display.h"
#include "NamedTransform.h"
#include "OpBuilders.h"


namespace OCIO_NAMESPACE
{

DisplayViewTransformRcPtr DisplayViewTransform::Create()
{
    return DisplayViewTransformRcPtr(new DisplayViewTransform(), &deleter);
}

void DisplayViewTransform::deleter(DisplayViewTransform* t)
{
    delete t;
}

class DisplayViewTransform::Impl
{
public:
    TransformDirection m_dir{ TRANSFORM_DIR_FORWARD };
    std::string m_src;
    std::string m_display;
    std::string m_view;

    bool m_looksBypass{ false };
    bool m_dataBypass{ true };

    Impl() = default;
    Impl(const Impl &) = delete;
    ~Impl() = default;
    Impl& operator= (const Impl & rhs) = default;
};

///////////////////////////////////////////////////////////////////////////

DisplayViewTransform::DisplayViewTransform()
    : m_impl(new DisplayViewTransform::Impl)
{
}

TransformRcPtr DisplayViewTransform::createEditableCopy() const
{
    DisplayViewTransformRcPtr transform = DisplayViewTransform::Create();
    *(transform->m_impl) = *m_impl; // Perform a deep copy.
    return transform;
}

DisplayViewTransform::~DisplayViewTransform()
{
    delete m_impl;
    m_impl = nullptr;
}

TransformDirection DisplayViewTransform::getDirection() const noexcept
{
    return getImpl()->m_dir;
}

void DisplayViewTransform::setDirection(TransformDirection dir) noexcept
{
    getImpl()->m_dir = dir;
}

void DisplayViewTransform::validate() const
{
    try
    {
        Transform::validate();
    }
    catch (Exception & ex)
    {
        std::string errMsg("DisplayViewTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }

    if (getImpl()->m_src.empty())
    {
        throw Exception("DisplayViewTransform: empty source color space name.");
    }

    if (getImpl()->m_display.empty())
    {
        throw Exception("DisplayViewTransform: empty display name.");
    }

    if (getImpl()->m_view.empty())
    {
        throw Exception("DisplayViewTransform: empty view name.");
    }
}

void DisplayViewTransform::setSrc(const char * name)
{
    getImpl()->m_src = name ? name : "";
}

const char * DisplayViewTransform::getSrc() const
{
    return getImpl()->m_src.c_str();
}

void DisplayViewTransform::setDisplay(const char * display)
{
    getImpl()->m_display = display ? display : "";
}

const char * DisplayViewTransform::getDisplay() const
{
    return getImpl()->m_display.c_str();
}

void DisplayViewTransform::setView(const char * view)
{
    getImpl()->m_view = view ? view : "";
}

const char * DisplayViewTransform::getView() const
{
    return getImpl()->m_view.c_str();
}

void DisplayViewTransform::setLooksBypass(bool bypass)
{
    getImpl()->m_looksBypass = bypass;
}

bool DisplayViewTransform::getLooksBypass() const
{
    return getImpl()->m_looksBypass;
}

void DisplayViewTransform::setDataBypass(bool bypass) noexcept
{
    getImpl()->m_dataBypass = bypass;
}

bool DisplayViewTransform::getDataBypass() const noexcept
{
    return getImpl()->m_dataBypass;
}

std::ostream& operator<< (std::ostream& os, const DisplayViewTransform& t)
{
    os << "<DisplayViewTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
    os << "src=" << t.getSrc() << ", ";
    os << "display=" << t.getDisplay() << ", ";
    os << "view=" << t.getView() << ", ";
    if (t.getLooksBypass())
    {
        os << ", looksBypass=" << t.getLooksBypass();
    }
    if (!t.getDataBypass())
    {
        os << ", dataBypass=" << t.getDataBypass();
    }
    os << ">";
    return os;
}

///////////////////////////////////////////////////////////////////////////

// Helper function to build the list of ops to convert from the source color space to the display
// color space (using a view transform). This is used when building ops in the forward direction.
void BuildSourceToDisplay(OpRcPtrVec & ops,
                          const Config & config,
                          const ConstContextRcPtr & context,
                          const ConstColorSpaceRcPtr & sourceCS,
                          const ConstViewTransformRcPtr & viewTransform,
                          const ConstColorSpaceRcPtr & displayCS,
                          bool dataBypass)
{
    // DisplayCS is display-referred.

    // Convert the current color space to its reference space.
    BuildColorSpaceToReferenceOps(ops, config, context, sourceCS, dataBypass);

    // If necessary, convert to the type of reference space used by the view transform.
    const auto vtRef = viewTransform->getReferenceSpaceType();
    const auto curCSRef = sourceCS->getReferenceSpaceType();
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
    else
    {
        std::ostringstream os;
        os << "View transform named '" << viewTransform->getName();
        os << "' needs either a transform from or to reference.";
        throw Exception(os.str().c_str());
    }

    // Convert from the display-referred reference space to the displayCS.
    BuildColorSpaceFromReferenceOps(ops, config, context, displayCS, dataBypass);
}

// Helper function to build the list of ops to convert from the display color space (using a view
// transform) to the source color space. This is used when building ops in the inverse direction.
void BuildDisplayToSource(OpRcPtrVec & ops,
                          const Config & config,
                          const ConstContextRcPtr & context,
                          const ConstColorSpaceRcPtr & displayCS,
                          const ConstViewTransformRcPtr & viewTransform,
                          const ConstColorSpaceRcPtr & sourceCS,
                          bool dataBypass)
{
    // Convert to the display-referred reference space from the displayColorSpace.
    BuildColorSpaceToReferenceOps(ops, config, context, displayCS, dataBypass);

    // Apply view transform inverted.
    if (viewTransform->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE))
    {
        BuildOps(ops, config, context,
                 viewTransform->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE),
                 TRANSFORM_DIR_FORWARD);
    }
    else if (viewTransform->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE))
    {
        BuildOps(ops, config, context,
                 viewTransform->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE),
                 TRANSFORM_DIR_INVERSE);
    }
    else
    {
        std::ostringstream os;
        os << "View transform named '" << viewTransform->getName();
        os << "' needs either a transform from or to reference.";
        throw Exception(os.str().c_str());
    }

    // If necessary, convert from the type of reference space used by the view transform to the
    // reference space of the source color space.
    const auto vtRef = viewTransform->getReferenceSpaceType();
    const auto inCSRef = sourceCS->getReferenceSpaceType();
    BuildReferenceConversionOps(ops, config, context, vtRef, inCSRef);

    // Convert from the reference space back to the source color space.
    BuildColorSpaceFromReferenceOps(ops, config, context, sourceCS, dataBypass);
}

void BuildNamedTransformToDisplay(OpRcPtrVec & ops,
                                  const Config & config,
                                  const ConstContextRcPtr & context,
                                  const ConstNamedTransformRcPtr & viewNamedTransform,
                                  const ConstColorSpaceRcPtr & displayCS,
                                  bool dataBypass)
{
    // Apply view named transform.
    auto transform = NamedTransformImpl::GetTransform(viewNamedTransform, TRANSFORM_DIR_FORWARD);
    BuildOps(ops, config, context, transform, TRANSFORM_DIR_FORWARD);

    // Convert from the display-referred reference space to the displayCS.
    BuildColorSpaceFromReferenceOps(ops, config, context, displayCS, dataBypass);
}

void BuildDisplayToNamedTransform(OpRcPtrVec & ops,
                                  const Config & config,
                                  const ConstContextRcPtr & context,
                                  const ConstColorSpaceRcPtr & displayCS,
                                  const ConstNamedTransformRcPtr & viewNamedTransform,
                                  bool dataBypass)
{
    // Convert to the display-referred reference space from the displayColorSpace.
    BuildColorSpaceToReferenceOps(ops, config, context, displayCS, dataBypass);

    // Apply view named transform.
    auto transform = NamedTransformImpl::GetTransform(viewNamedTransform, TRANSFORM_DIR_INVERSE);
    BuildOps(ops, config, context, transform, TRANSFORM_DIR_FORWARD);
}

// Build ops for a display/view transform.
void BuildDisplayOps(OpRcPtrVec & ops,
                     const Config & config,
                     const ConstContextRcPtr & context,
                     const DisplayViewTransform & displayViewTransform,
                     TransformDirection dir)
{
    // There are two ways of specifying a DisplayViewTransform: either with a colorspace, or with
    // a view_transform plus display_colorspace.  It is permitted to substitute a named_transform
    // for either the colorspace or the view_transform.

    // Validate src color space.
    const std::string srcColorSpaceName = displayViewTransform.getSrc();
    ConstColorSpaceRcPtr srcColorSpace = config.getColorSpace(srcColorSpaceName.c_str());
    if (!srcColorSpace)
    {
        std::ostringstream os;
        os << "DisplayViewTransform error.";
        if (srcColorSpaceName.empty())
        {
            os << " The source color space is unspecified.";
        }
        else
        {
            os << " Cannot find source color space named '" << srcColorSpaceName << "'.";
        }
        throw Exception(os.str().c_str());
    }

    const std::string display = displayViewTransform.getDisplay();
    if (config.getNumViews(display.c_str()) == 0)
    {
        std::ostringstream os;
        os << "DisplayViewTransform error.";
        os << " Display '" << display << "' not found.";
        throw Exception(os.str().c_str());
    }
    const std::string view = displayViewTransform.getView();

    // Get the view transform if any: if it exists, it can be a view transform or a named transform.
    const std::string viewTransformName = config.getDisplayViewTransformName(display.c_str(),
                                                                             view.c_str());
    ConstViewTransformRcPtr viewTransform;
    ConstNamedTransformRcPtr viewNamedTransform;
    if (!viewTransformName.empty())
    {
        viewTransform = config.getViewTransform(viewTransformName.c_str());
        if (!viewTransform)
        {
            viewNamedTransform = config.getNamedTransform(viewTransformName.c_str());
            if (!viewNamedTransform)
            {
                // Config::validate catch this.
                std::ostringstream os;
                os << "DisplayViewTransform error. The view transform '";
                os << viewTransformName << "' is neither a view transform nor a named transform.";
                throw Exception(os.str().c_str());
            }
        }
    }

    // Get the color space associated to the (display, view) pair.
    const char * csName = config.getDisplayViewColorSpaceName(display.c_str(), view.c_str());

    // A shared view containing a view transform may set the color space to USE_DISPLAY_NAME,
    // in which case we look for a display color space with the same name as the display.
    const std::string displayColorSpaceName = View::UseDisplayName(csName) ? display : csName;
    ConstColorSpaceRcPtr displayColorSpace = config.getColorSpace(displayColorSpaceName.c_str());
    ConstNamedTransformRcPtr displayNamedTransform;
    if (!displayColorSpace)
    {
        if (displayColorSpaceName.empty())
        {
            std::ostringstream os;
            os << "DisplayViewTransform error.";
            os << " Display color space name is unspecified.";
            throw Exception(os.str().c_str());
        }

        // If there is a view transform, the display color space can't be a named transform.
        if (viewTransform || viewNamedTransform)
        {
            // Config::validate catch this.
            std::ostringstream os;
            os << "DisplayViewTransform error." << " The view '";
            os << viewTransformName << "' refers to a display color space '";
            os << displayColorSpaceName << "' that can't be found.";
            throw Exception(os.str().c_str());
        }
        displayNamedTransform = config.getNamedTransform(displayColorSpaceName.c_str());
        if (!displayNamedTransform)
        {
            std::ostringstream os;
            os << "DisplayViewTransform error.";
            os << " Cannot find color space or named transform, named '";
            os << displayColorSpaceName << "'.";
            throw Exception(os.str().c_str());
        }
    }

    // By default, data color spaces are not processed.
    const bool dataBypass = displayViewTransform.getDataBypass();
    const bool srcData = (srcColorSpace && srcColorSpace->isData()) ? true : false;
    const bool displayData = (displayColorSpace && displayColorSpace->isData()) ? true : false;
    if (dataBypass && (srcData || displayData))
    {
        return;
    }

    // Get looks to be applied, if specified.
    LookParseResult looks;
    if (!displayViewTransform.getLooksBypass())
    {
        looks.parse(config.getDisplayViewLooks(display.c_str(), view.c_str()));
    }

    // Now that all the inputs are found and validated, the following code builds the list of ops
    // for the forward or the inverse direction.

    const auto combinedDir = CombineTransformDirections(dir, displayViewTransform.getDirection());
    switch (combinedDir)
    {
    case TRANSFORM_DIR_FORWARD:
    {
        // Start from src color space.
        ConstColorSpaceRcPtr currentCS = srcColorSpace;

        // Apply looks if needed.
        if (!looks.empty())
        {
            // Note that this updates the currentCS to be the process space of the last look
            // applied.
            BuildLookOps(ops, currentCS, false, config, context, looks);
        }

        if (displayNamedTransform)
        {
            // Ignore currentCS.  The forward direction NamedTransform is used for the forward
            // direction DisplayViewTransform.
            auto transform = NamedTransformImpl::GetTransform(displayNamedTransform,
                                                              TRANSFORM_DIR_FORWARD);
            BuildOps(ops, config, context, transform, TRANSFORM_DIR_FORWARD);
        }
        else if (viewNamedTransform)
        {
            BuildNamedTransformToDisplay(ops, config, context, viewNamedTransform,
                                         displayColorSpace, dataBypass);
        }
        else if (viewTransform)
        {
            BuildSourceToDisplay(ops, config, context, currentCS, viewTransform,
                                 displayColorSpace, dataBypass);
        }
        else
        {
            // Apply the conversion from the current color space to the display color space.
            BuildColorSpaceOps(ops, config, context, currentCS, displayColorSpace, dataBypass);
        }
        break;
    }
    case TRANSFORM_DIR_INVERSE:
    {
        // The source color space of the view transform might need to be computed. In forward,
        // looks (if present) are applied and will change the current color space that is used
        // as the starting point of the view transform. Looks need to be used in order to find
        // what color space to use for the view transform in the inverse direction.
        ConstColorSpaceRcPtr vtSourceCS = srcColorSpace;
        if (!looks.empty())
        {
            // Get the result color space of applying the looks in the forward direction.
            const char * csRes = LooksResultColorSpace(config, context, looks);
            vtSourceCS = config.getColorSpace(csRes);
        }

        if (displayNamedTransform)
        {
            // Ignore currentCS.
            auto transform = NamedTransformImpl::GetTransform(displayNamedTransform,
                                                              TRANSFORM_DIR_INVERSE);
            BuildOps(ops, config, context, transform, TRANSFORM_DIR_FORWARD);
        }
        else if (viewNamedTransform)
        {
            BuildDisplayToNamedTransform(ops, config, context, displayColorSpace,
                                         viewNamedTransform, dataBypass);
        }
        else if (viewTransform)
        {
            BuildDisplayToSource(ops, config, context, displayColorSpace, viewTransform,
                                 vtSourceCS, dataBypass);
        }
        else
        {
            // Apply the conversion from the display color space to the vtSourceCS.
            BuildColorSpaceOps(ops, config, context, displayColorSpace, vtSourceCS, dataBypass);
        }

        if (!looks.empty())
        {
            // Apply looks in inverse direction.  Note that vtSourceCS is updated.
            looks.reverse();
            BuildLookOps(ops, vtSourceCS, false, config, context, looks);

            // End in srcColorSpace.
            BuildColorSpaceOps(ops, config, context, vtSourceCS, srcColorSpace, dataBypass);
        }
        break;
    }
    }
}

bool CollectContextVariables(const Config & config,
                             const Context & context,
                             const DisplayViewTransform & tr,
                             ContextRcPtr & usedContextVars)
{
    bool foundContextVars = false;

    // NB: The search could return false positive but should not miss anything i.e. it looks
    // in both directions even if only one will be used, and it only roughly mimics the op creation.

    ConstColorSpaceRcPtr src = config.getColorSpace(tr.getSrc());
    if (CollectContextVariables(config, context, src, usedContextVars))
    {
        foundContextVars = true;
    }

    const char * csName = config.getDisplayViewColorSpaceName(tr.getDisplay(), tr.getView());
    if (csName && *csName)
    {
        src = config.getColorSpace(csName);
        if (CollectContextVariables(config, context, src, usedContextVars))
        {
            foundContextVars = true;
        }
    }

    const char * vtName = config.getDisplayViewTransformName(tr.getDisplay(), tr.getView());
    if (vtName && *vtName)
    {
        ConstViewTransformRcPtr vt = config.getViewTransform(vtName);
        if (vt)
        {
            ConstTransformRcPtr to = vt->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE);
            if (to && CollectContextVariables(config, context, to, usedContextVars))
            {
                foundContextVars = true;
            }

            ConstTransformRcPtr from = vt->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE);
            if (from && CollectContextVariables(config, context, from, usedContextVars))
            {
                foundContextVars = true;
            }
        }
    }

    // TODO: The LooksBypass must be a DynamicProperty to allow live on/off.
    if (!tr.getLooksBypass())
    {
        const std::string looksStr = config.getDisplayViewLooks(tr.getDisplay(), tr.getView());
        LookParseResult looks;
        looks.parse(looksStr);

        const LookParseResult::Options & options = looks.getOptions();
        for (const auto & tokens : options)
        {
            for (const auto & token : tokens)
            {
                ConstLookRcPtr look = config.getLook(token.name.c_str());
                if (look && CollectContextVariables(config, context, token.dir, *look, usedContextVars))
                {
                    foundContextVars = true;
                }
            }
        }
    }

    return foundContextVars;
}

} // namespace OCIO_NAMESPACE
