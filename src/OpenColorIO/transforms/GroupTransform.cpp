// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"

namespace OCIO_NAMESPACE
{
GroupTransformRcPtr GroupTransform::Create()
{
    return GroupTransformRcPtr(new GroupTransform(), &deleter);
}

void GroupTransform::deleter(GroupTransform * t)
{
    delete t;
}

namespace
{
typedef std::vector<TransformRcPtr> TransformRcPtrVec;
}

class GroupTransform::Impl
{
public:
    TransformDirection m_dir;
    TransformRcPtrVec m_vec;

    Impl()
        : m_dir(TRANSFORM_DIR_FORWARD)
        , m_metadata()
    { }

    Impl(const Impl &) = delete;

    ~Impl()
    {
        m_vec.clear();
    }

    Impl & operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            m_dir = rhs.m_dir;

            m_vec.clear();

            for (unsigned int i = 0; i < rhs.m_vec.size(); ++i)
            {
                m_vec.push_back(rhs.m_vec[i]->createEditableCopy());
            }
        }
        return *this;
    }

    FormatMetadata & getFormatMetadata() noexcept
    {
        return m_metadata;
    }

    const FormatMetadata & getFormatMetadata() const noexcept
    {
        return m_metadata;
    }

private:
    FormatMetadataImpl m_metadata;
};

///////////////////////////////////////////////////////////////////////////

GroupTransform::GroupTransform()
    : m_impl(new GroupTransform::Impl)
{
}

TransformRcPtr GroupTransform::createEditableCopy() const
{
    GroupTransformRcPtr transform = GroupTransform::Create();
    *(transform->m_impl) = *m_impl;
    return transform;
}

GroupTransform::~GroupTransform()
{
    delete m_impl;
    m_impl = nullptr;
}

TransformDirection GroupTransform::getDirection() const noexcept
{
    return getImpl()->m_dir;
}

void GroupTransform::setDirection(TransformDirection dir) noexcept
{
    getImpl()->m_dir = dir;
}

void GroupTransform::validate() const
{
    Transform::validate();

    for (int i = 0; i<getNumTransforms(); ++i)
    {
        getTransform(i)->validate();
    }
}

FormatMetadata & GroupTransform::getFormatMetadata() noexcept
{
    return m_impl->getFormatMetadata();
}

const FormatMetadata & GroupTransform::getFormatMetadata() const noexcept
{
    return m_impl->getFormatMetadata();
}

int GroupTransform::getNumTransforms() const
{
    return static_cast<int>(getImpl()->m_vec.size());
}

ConstTransformRcPtr GroupTransform::getTransform(int index) const
{
    if (index < 0 || index >= (int)getImpl()->m_vec.size())
    {
        std::ostringstream os;
        os << "Invalid transform index " << index << ".";
        throw Exception(os.str().c_str());
    }

    return getImpl()->m_vec[index];
}

TransformRcPtr & GroupTransform::getTransform(int index)
{
    if (index < 0 || index >= (int)getImpl()->m_vec.size())
    {
        std::ostringstream os;
        os << "Invalid transform index " << index << ".";
        throw Exception(os.str().c_str());
    }

    return getImpl()->m_vec[index];
}

void GroupTransform::appendTransform(TransformRcPtr transform)
{
    getImpl()->m_vec.push_back(transform);
}

void GroupTransform::prependTransform(TransformRcPtr transform)
{
    getImpl()->m_vec.insert(getImpl()->m_vec.begin(), transform);
}

std::ostream & operator<< (std::ostream & os, const GroupTransform & groupTransform)
{
    os << "<GroupTransform ";
    os << "direction=" << TransformDirectionToString(groupTransform.getDirection()) << ", ";
    os << "transforms=";
    for (int i = 0; i < groupTransform.getNumTransforms(); ++i)
    {
        ConstTransformRcPtr transform = groupTransform.getTransform(i);
        os << "\n\t" << *transform;
    }
    os << ">";
    return os;
}

///////////////////////////////////////////////////////////////////////////

void BuildGroupOps(OpRcPtrVec & ops,
                    const Config & config,
                    const ConstContextRcPtr & context,
                    const GroupTransform & groupTransform,
                    TransformDirection dir)
{
    if (ops.size() == 0)
    {
        // If group is the first transform, copy the group metadata.
        FormatMetadataImpl & processorData = ops.getFormatMetadata();
        processorData = groupTransform.getFormatMetadata();
    }

    auto combinedDir = CombineTransformDirections(dir, groupTransform.getDirection());

    if (combinedDir == TRANSFORM_DIR_FORWARD)
    {
        for (int i = 0; i < groupTransform.getNumTransforms(); ++i)
        {
            ConstTransformRcPtr childTransform = groupTransform.getTransform(i);
            BuildOps(ops, config, context, childTransform, TRANSFORM_DIR_FORWARD);
        }
    }
    else if (combinedDir == TRANSFORM_DIR_INVERSE)
    {
        for (int i = groupTransform.getNumTransforms() - 1; i >= 0; --i)
        {
            ConstTransformRcPtr childTransform = groupTransform.getTransform(i);
            BuildOps(ops, config, context, childTransform, TRANSFORM_DIR_INVERSE);
        }
    }
}

} // namespace OCIO_NAMESPACE

