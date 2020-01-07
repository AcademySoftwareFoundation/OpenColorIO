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
    m_impl = NULL;
}

ColorSpaceTransform& ColorSpaceTransform::operator= (const ColorSpaceTransform & rhs)
{
    if (this != &rhs)
    {
        *m_impl = *rhs.m_impl;
    }
    return *this;
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
        throw Exception("ColorSpaceTransform: empty source color space name");
    }

    if (getImpl()->m_dst.empty())
    {
        throw Exception("ColorSpaceTransform: empty destination color space name");
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

void BuildColorSpaceOps(OpRcPtrVec & ops,
                        const Config& config,
                        const ConstContextRcPtr & context,
                        const ColorSpaceTransform & colorSpaceTransform,
                        TransformDirection dir)
{
    TransformDirection combinedDir = CombineTransformDirections(dir,
                                                colorSpaceTransform.getDirection());

    ConstColorSpaceRcPtr src, dst;

    if(combinedDir == TRANSFORM_DIR_FORWARD)
    {
        src = config.getColorSpace( context->resolveStringVar( colorSpaceTransform.getSrc() ) );
        dst = config.getColorSpace( context->resolveStringVar( colorSpaceTransform.getDst() ) );
    }
    else if(combinedDir == TRANSFORM_DIR_INVERSE)
    {
        dst = config.getColorSpace( context->resolveStringVar( colorSpaceTransform.getSrc() ) );
        src = config.getColorSpace( context->resolveStringVar( colorSpaceTransform.getDst() ) );
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

    AllocationData srcAllocation;
    srcAllocation.allocation = srcColorSpace->getAllocation();
    srcAllocation.vars.resize( srcColorSpace->getAllocationNumVars());
    if(srcAllocation.vars.size() > 0)
    {
        srcColorSpace->getAllocationVars(&srcAllocation.vars[0]);
    }

    CreateGpuAllocationNoOp(ops, srcAllocation);

    // Go to the reference space, either by using
    // * cs->ref in the forward direction
    // * ref->cs in the inverse direction
    if(srcColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE))
    {
        BuildOps(ops, config, context, srcColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE), TRANSFORM_DIR_FORWARD);
    }
    else if(srcColorSpace->getTransform(COLORSPACE_DIR_FROM_REFERENCE))
    {
        BuildOps(ops, config, context, srcColorSpace->getTransform(COLORSPACE_DIR_FROM_REFERENCE), TRANSFORM_DIR_INVERSE);
    }
    // Otherwise, both are not defined so its a no-op. This is not an error condition.

    // Go from the reference space, either by using
    // * ref->cs in the forward direction
    // * cs->ref in the inverse direction
    if(dstColorSpace->getTransform(COLORSPACE_DIR_FROM_REFERENCE))
    {
        BuildOps(ops, config, context, dstColorSpace->getTransform(COLORSPACE_DIR_FROM_REFERENCE), TRANSFORM_DIR_FORWARD);
    }
    else if(dstColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE))
    {
        BuildOps(ops, config, context, dstColorSpace->getTransform(COLORSPACE_DIR_TO_REFERENCE), TRANSFORM_DIR_INVERSE);
    }
    // Otherwise, both are not defined so its a no-op. This is not an error condition.

    AllocationData dstAllocation;
    dstAllocation.allocation = dstColorSpace->getAllocation();
    dstAllocation.vars.resize( dstColorSpace->getAllocationNumVars());
    if(dstAllocation.vars.size() > 0)
    {
        dstColorSpace->getAllocationVars(&dstAllocation.vars[0]);
    }

    CreateGpuAllocationNoOp(ops, dstAllocation);
}
} // namespace OCIO_NAMESPACE
