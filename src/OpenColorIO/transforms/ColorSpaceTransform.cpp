// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <string.h>

#include <OpenColorIO/OpenColorIO.h>

#include "ContextVariableUtils.h"
#include "NamedTransform.h"
#include "OpBuilders.h"
#include "ops/noop/NoOps.h"


namespace OCIO_NAMESPACE
{
ColorSpaceTransformRcPtr ColorSpaceTransform::Create()
{
    return ColorSpaceTransformRcPtr(new ColorSpaceTransform(), &deleter);
}

void ColorSpaceTransform::deleter(ColorSpaceTransform* t)
{
    delete t;
}

class ColorSpaceTransform::Impl
{
public:
    TransformDirection m_dir{ TRANSFORM_DIR_FORWARD  };
    std::string m_src;
    std::string m_dst;
    bool m_dataBypass{ true };

    Impl() = default;
    Impl(const Impl &) = delete;

    ~Impl() = default;

    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            m_dir        = rhs.m_dir;
            m_src        = rhs.m_src;
            m_dst        = rhs.m_dst;
            m_dataBypass = rhs.m_dataBypass;
        }
        return *this;
    }
};

///////////////////////////////////////////////////////////////////////////

ColorSpaceTransform::ColorSpaceTransform()
    : m_impl(new ColorSpaceTransform::Impl)
{
}

TransformRcPtr ColorSpaceTransform::createEditableCopy() const
{
    ColorSpaceTransformRcPtr transform = ColorSpaceTransform::Create();
    *(transform->m_impl) = *m_impl;
    return transform;
}

ColorSpaceTransform::~ColorSpaceTransform()
{
    delete m_impl;
    m_impl = nullptr;
}

TransformDirection ColorSpaceTransform::getDirection() const noexcept
{
    return getImpl()->m_dir;
}

void ColorSpaceTransform::setDirection(TransformDirection dir) noexcept
{
    getImpl()->m_dir = dir;
}

void ColorSpaceTransform::validate() const
{
    try
    {
        Transform::validate();
    }
    catch (Exception & ex)
    {
        std::string errMsg("ColorSpaceTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }

    if (getImpl()->m_src.empty())
    {
        throw Exception("ColorSpaceTransform: empty source color space name.");
    }

    if (getImpl()->m_dst.empty())
    {
        throw Exception("ColorSpaceTransform: empty destination color space name.");
    }
}

const char * ColorSpaceTransform::getSrc() const
{
    return getImpl()->m_src.c_str();
}

void ColorSpaceTransform::setSrc(const char * src)
{
    getImpl()->m_src = src ? src : "";
}

const char * ColorSpaceTransform::getDst() const
{
    return getImpl()->m_dst.c_str();
}

void ColorSpaceTransform::setDst(const char * dst)
{
    getImpl()->m_dst = dst ? dst : "";
}

bool ColorSpaceTransform::getDataBypass() const noexcept
{
    return getImpl()->m_dataBypass;
}

void ColorSpaceTransform::setDataBypass(bool bypass) noexcept
{
    getImpl()->m_dataBypass = bypass;
}

std::ostream& operator<< (std::ostream& os, const ColorSpaceTransform& t)
{
    os << "<ColorSpaceTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
    os << "src=" << t.getSrc() << ", ";
    os << "dst=" << t.getDst();
    const bool bypass = t.getDataBypass();
    if (!bypass)
    {
        os << "dataBypass=" << bypass;
    }
    os << ">";
    return os;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
void ThrowMissingCS(const char * cs)
{
    std::ostringstream os;
    os << "Color space '" << cs << "' could not be found.";
    throw Exception(os.str().c_str());
}
}

void BuildColorSpaceOps(OpRcPtrVec & ops,
                        const Config & config,
                        const ConstContextRcPtr & context,
                        const ColorSpaceTransform & colorSpaceTransform,
                        TransformDirection dir)
{
    const auto combinedDir = CombineTransformDirections(dir, colorSpaceTransform.getDirection());
    const bool forward = (combinedDir == TRANSFORM_DIR_FORWARD);

    const std::string transSrcName{ colorSpaceTransform.getSrc() };
    const std::string transDstName{ colorSpaceTransform.getDst() };
    const std::string srcName = forward ? transSrcName : transDstName;
    const std::string dstName = forward ? transDstName : transSrcName;

    ConstColorSpaceRcPtr src = config.getColorSpace( context->resolveStringVar(srcName.c_str()) );
    ConstColorSpaceRcPtr dst = config.getColorSpace( context->resolveStringVar(dstName.c_str()) );

    ConstNamedTransformRcPtr srcNamedTransform;
    ConstNamedTransformRcPtr dstNamedTransform;

    if (!src)
    {
        srcNamedTransform = config.getNamedTransform(srcName.c_str());
        if (!srcNamedTransform)
        {
            ThrowMissingCS(srcName.c_str());
        }
    }
    if (!dst)
    {
        dstNamedTransform = config.getNamedTransform(dstName.c_str());
        if (!dstNamedTransform)
        {
            ThrowMissingCS(dstName.c_str());
        }
    }

    // There are 4 cases:
    // * (srcNamedTransform, dstNamedTransform): srcNamedTransform->forward +
    //                                           dstNamedTransform->inverse.
    // * (srcNamedTransform, dst): srcNamedTransform->forward (dst ignored).
    // * (src, dstNamedTransform): dstNamedTransform->inverse (src ignored).
    // * (src, dst): full color space transform.

    if (srcNamedTransform || dstNamedTransform)
    {
        ConstTransformRcPtr tr = GetTransform(srcNamedTransform, dstNamedTransform);
        BuildOps(ops, config, context, tr, TRANSFORM_DIR_FORWARD);
        return;
    }

    BuildColorSpaceOps(ops, config, context, src, dst, colorSpaceTransform.getDataBypass());
}

namespace
{
bool AreColorSpacesInSameEqualityGroup(const ConstColorSpaceRcPtr & csa,
                                       const ConstColorSpaceRcPtr & csb)
{
    // See issue #602. Using names in case one of the color space would be a copy.
    if (StringUtils::Compare(csa->getName(), csb->getName())) return true;

    std::string a = csa->getEqualityGroup();
    std::string b = csb->getEqualityGroup();

    if(!a.empty()) return (a==b);
    return false;
}

}

void BuildColorSpaceOps(OpRcPtrVec & ops,
                        const Config & config,
                        const ConstContextRcPtr & context,
                        const ConstColorSpaceRcPtr & srcColorSpace,
                        const ConstColorSpaceRcPtr & dstColorSpace,
                        bool dataBypass)
{
    if(!srcColorSpace)
        throw Exception("BuildColorSpaceOps failed, null srcColorSpace.");
    if(!dstColorSpace)
        throw Exception("BuildColorSpaceOps failed, null dstColorSpace.");

    if(AreColorSpacesInSameEqualityGroup(srcColorSpace, dstColorSpace))
        return;
    if(dataBypass && (dstColorSpace->isData() || srcColorSpace->isData()))
        return;

    // Consider dt8 -> vd8?
    // One would have to explode the srcColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE);
    // result, and walk through it step by step.  If the dstColorspace family were
    // ever encountered in transit, we'd want to short circuit the result.

    // Go from the srcColorSpace to the reference space.
    BuildColorSpaceToReferenceOps(ops, config, context, srcColorSpace, dataBypass);

    // There are two possible reference spaces, the main (scene-referred) one and the
    // display-referred one.  If the src and dst use different reference spaces, use the
    // default ViewTransform to convert between them.
    BuildReferenceConversionOps(ops, config, context,
                                srcColorSpace->getReferenceSpaceType(),
                                dstColorSpace->getReferenceSpaceType());

    // Go from the reference space to dstColorSpace.
    BuildColorSpaceFromReferenceOps(ops, config, context, dstColorSpace, dataBypass);
}

void BuildColorSpaceToReferenceOps(OpRcPtrVec & ops,
                                   const Config & config,
                                   const ConstContextRcPtr & context,
                                   const ConstColorSpaceRcPtr & srcColorSpace,
                                   bool dataBypass)
{
    if (!srcColorSpace)
        throw Exception("BuildColorSpaceOps failed, null colorSpace.");

    if (dataBypass && srcColorSpace->isData())
        return;

    AllocationData srcAllocation;
    srcAllocation.allocation = srcColorSpace->getAllocation();
    srcAllocation.vars.resize(srcColorSpace->getAllocationNumVars());
    if (srcAllocation.vars.size() > 0)
    {
        srcColorSpace->getAllocationVars(&srcAllocation.vars[0]);
    }

    CreateGpuAllocationNoOp(ops, srcAllocation);

    // Go to the reference space, either by using:
    // * cs->ref in the forward direction.
    // * ref->cs in the inverse direction.
    if (srcColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE))
    {
        BuildOps(ops, config, context, srcColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE),
                 TRANSFORM_DIR_FORWARD);
    }
    else if (srcColorSpace->getTransform(COLORSPACE_DIR_FROM_REFERENCE))
    {
        BuildOps(ops, config, context, srcColorSpace->getTransform(COLORSPACE_DIR_FROM_REFERENCE),
                 TRANSFORM_DIR_INVERSE);
    }
    // Otherwise, both are not defined so its a no-op. This is not an error condition.
}

void BuildColorSpaceFromReferenceOps(OpRcPtrVec & ops,
                                     const Config & config,
                                     const ConstContextRcPtr & context,
                                     const ConstColorSpaceRcPtr & dstColorSpace,
                                     bool dataBypass)
{
    if (!dstColorSpace)
        throw Exception("BuildColorSpaceOps failed, null colorSpace.");

    if (dataBypass && dstColorSpace->isData())
        return;

    // Go from the reference space, either by using:
    // * ref->cs in the forward direction.
    // * cs->ref in the inverse direction.
    if (dstColorSpace->getTransform(COLORSPACE_DIR_FROM_REFERENCE))
    {
        BuildOps(ops, config, context, dstColorSpace->getTransform(COLORSPACE_DIR_FROM_REFERENCE),
                    TRANSFORM_DIR_FORWARD);
    }
    else if (dstColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE))
    {
        BuildOps(ops, config, context, dstColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE),
                    TRANSFORM_DIR_INVERSE);
    }
    // Otherwise, both are not defined so its a no-op. This is not an error condition.

    AllocationData dstAllocation;
    dstAllocation.allocation = dstColorSpace->getAllocation();
    dstAllocation.vars.resize(dstColorSpace->getAllocationNumVars());
    if (dstAllocation.vars.size() > 0)
    {
        dstColorSpace->getAllocationVars(&dstAllocation.vars[0]);
    }

    CreateGpuAllocationNoOp(ops, dstAllocation);
}

void BuildReferenceConversionOps(OpRcPtrVec & ops,
                                 const Config & config,
                                 const ConstContextRcPtr & context,
                                 ReferenceSpaceType srcReferenceSpace,
                                 ReferenceSpaceType dstReferenceSpace)
{
    if (srcReferenceSpace != dstReferenceSpace)
    {
        auto view = config.getDefaultSceneToDisplayViewTransform();
        if (!view)
        {
            // Can not be the case for a valid config.
            throw Exception("There is no view transform between the main scene-referred space "
                            "and the display-referred space.");
        }
        if (srcReferenceSpace == REFERENCE_SPACE_SCENE) // convert scene-referred to display-referred
        {
            if (view->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE))
            {
                BuildOps(ops, config, context,
                         view->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE),
                         TRANSFORM_DIR_FORWARD);
            }
            else if (view->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE))
            {
                BuildOps(ops, config, context,
                         view->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE),
                         TRANSFORM_DIR_INVERSE);
            }
        }
        else // convert display-referred to scene-referred
        {
            if (view->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE))
            {
                BuildOps(ops, config, context,
                         view->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE),
                         TRANSFORM_DIR_FORWARD);
            }
            else if (view->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE))
            {
                BuildOps(ops, config, context,
                         view->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE),
                         TRANSFORM_DIR_INVERSE);
            }
        }
    }
}

bool CollectContextVariables(const Config & config, 
                             const Context & context,
                             ConstColorSpaceRcPtr & cs,
                             ContextRcPtr & usedContextVars)
{
    bool foundContextVars = false;

    if (cs)
    {
        ConstTransformRcPtr to = cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
        if (to && CollectContextVariables(config, context, to, usedContextVars))
        {
            foundContextVars = true;
        }

        ConstTransformRcPtr from = cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if (from && CollectContextVariables(config, context, from, usedContextVars))
        {
            foundContextVars = true;
        }
    }

    return foundContextVars;
}

bool CollectContextVariables(const Config & config, 
                             const Context & context,
                             const ColorSpaceTransform & tr,
                             ContextRcPtr & usedContextVars)
{
    bool foundContextVars = false;
    
    // NB: The search could return false positive but should not miss anything i.e. it looks
    // for context variables in both directions even if only one will be used.

    const std::string srcName(context.resolveStringVar(tr.getSrc(), usedContextVars));
    if (0 != strcmp(srcName.c_str(), tr.getSrc()))
    {
        foundContextVars = true;
    }

    const std::string dstName(context.resolveStringVar(tr.getDst(), usedContextVars));
    if (0 != strcmp(dstName.c_str(), tr.getDst()))
    {
        foundContextVars = true;
    }

    ConstColorSpaceRcPtr src = config.getColorSpace(srcName.c_str());
    if (CollectContextVariables(config, context, src, usedContextVars))
    {
        foundContextVars = true;
    }

    ConstColorSpaceRcPtr dst = config.getColorSpace(dstName.c_str());
    if (CollectContextVariables(config, context, dst, usedContextVars))
    {
        foundContextVars = true;
    }

    return foundContextVars;
}

} // namespace OCIO_NAMESPACE
