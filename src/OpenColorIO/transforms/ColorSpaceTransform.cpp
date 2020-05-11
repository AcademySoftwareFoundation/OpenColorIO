// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

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
    TransformDirection m_dir;
    std::string m_src;
    std::string m_dst;

    Impl() :
        m_dir(TRANSFORM_DIR_FORWARD)
    { }

    Impl(const Impl &) = delete;

    ~Impl()
    { }

    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            m_dir = rhs.m_dir;
            m_src = rhs.m_src;
            m_dst = rhs.m_dst;
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
    Transform::validate();

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
    getImpl()->m_src = src;
}

const char * ColorSpaceTransform::getDst() const
{
    return getImpl()->m_dst.c_str();
}

void ColorSpaceTransform::setDst(const char * dst)
{
    getImpl()->m_dst = dst;
}

std::ostream& operator<< (std::ostream& os, const ColorSpaceTransform& t)
{
    os << "<ColorSpaceTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
    os << "src=" << t.getSrc() << ", ";
    os << "dst=" << t.getDst();
    os << ">";
    return os;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
void ThrowMissingCS(const char * srcdst, const char * cs)
{
    std::ostringstream os;
    os << "BuildColorSpaceOps failed: " << srcdst << " color space '" << cs;
    os << "' could not be found.";
    throw Exception(os.str().c_str());
}

constexpr char SRC_CS[] = "source";
constexpr char DST_CS[] = "destination";
}

void BuildColorSpaceOps(OpRcPtrVec & ops,
                        const Config& config,
                        const ConstContextRcPtr & context,
                        const ColorSpaceTransform & colorSpaceTransform,
                        TransformDirection dir)
{
    auto combinedDir = CombineTransformDirections(dir, colorSpaceTransform.getDirection());

    ConstColorSpaceRcPtr src, dst;

    if(combinedDir == TRANSFORM_DIR_FORWARD)
    {
        src = config.getColorSpace( context->resolveStringVar( colorSpaceTransform.getSrc() ) );
        if (!src)
        {
            ThrowMissingCS(SRC_CS, colorSpaceTransform.getSrc());
        }
        dst = config.getColorSpace( context->resolveStringVar( colorSpaceTransform.getDst() ) );
        if (!dst)
        {
            ThrowMissingCS(DST_CS, colorSpaceTransform.getDst());
        }
    }
    else if(combinedDir == TRANSFORM_DIR_INVERSE)
    {
        dst = config.getColorSpace( context->resolveStringVar( colorSpaceTransform.getSrc() ) );
        if (!dst)
        {
            ThrowMissingCS(SRC_CS, colorSpaceTransform.getSrc());
        }
        src = config.getColorSpace( context->resolveStringVar( colorSpaceTransform.getDst() ) );
        if (!src)
        {
            ThrowMissingCS(DST_CS, colorSpaceTransform.getDst());
        }
    }

    BuildColorSpaceOps(ops, config, context, src, dst);
}

namespace
{
bool AreColorSpacesInSameEqualityGroup(const ConstColorSpaceRcPtr & csa,
                                       const ConstColorSpaceRcPtr & csb)
{
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
                        const ConstColorSpaceRcPtr & dstColorSpace)
{
    if(!srcColorSpace)
        throw Exception("BuildColorSpaceOps failed, null srcColorSpace.");
    if(!dstColorSpace)
        throw Exception("BuildColorSpaceOps failed, null dstColorSpace.");

    if(AreColorSpacesInSameEqualityGroup(srcColorSpace, dstColorSpace))
        return;
    if(dstColorSpace->isData() || srcColorSpace->isData())
        return;

    // Consider dt8 -> vd8?
    // One would have to explode the srcColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE);
    // result, and walk through it step by step.  If the dstColorspace family were
    // ever encountered in transit, we'd want to short circuit the result.

    // Go from the srcColorSpace to the reference space.
    BuildColorSpaceToReferenceOps(ops, config, context, srcColorSpace);

    // There are two possible reference spaces, the main (scene-referred) one and the
    // display-referred one.  If the src and dst use different reference spaces, use the
    // default ViewTransform to convert between them.
    BuildReferenceConversionOps(ops, config, context,
                                srcColorSpace->getReferenceSpaceType(),
                                dstColorSpace->getReferenceSpaceType());

    // Go from the reference space to dstColorSpace.
    BuildColorSpaceFromReferenceOps(ops, config, context, dstColorSpace);
}

void BuildColorSpaceToReferenceOps(OpRcPtrVec & ops,
                                   const Config & config,
                                   const ConstContextRcPtr & context,
                                   const ConstColorSpaceRcPtr & srcColorSpace)
{
    if (!srcColorSpace)
        throw Exception("BuildColorSpaceOps failed, null colorSpace.");

    if (srcColorSpace->isData())
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
                                     const ConstColorSpaceRcPtr & dstColorSpace)
{
    if (!dstColorSpace)
        throw Exception("BuildColorSpaceOps failed, null colorSpace.");

    if (dstColorSpace->isData())
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
            // Can not be the case for a sane config.
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

} // namespace OCIO_NAMESPACE
