// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>
#include "OpBuilders.h"

#include <sstream>

OCIO_NAMESPACE_ENTER
{
    GroupTransformRcPtr GroupTransform::Create()
    {
        return GroupTransformRcPtr(new GroupTransform(), &deleter);
    }
    
    void GroupTransform::deleter(GroupTransform* t)
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
        TransformDirection dir_;
        TransformRcPtrVec vec_;
        
        Impl()
            : dir_(TRANSFORM_DIR_FORWARD)
            , m_metadata(METADATA_ROOT)
        { }

        Impl(const Impl &) = delete;

        ~Impl()
        {
            vec_.clear();
        }
        
        Impl& operator= (const Impl & rhs)
        {
            if (this != &rhs)
            {
                dir_ = rhs.dir_;

                vec_.clear();

                for (unsigned int i = 0; i < rhs.vec_.size(); ++i)
                {
                    vec_.push_back(rhs.vec_[i]->createEditableCopy());
                }
            }
            return *this;
        }

        FormatMetadata & getFormatMetadata()
        {
            return m_metadata;
        }

        const FormatMetadata & getFormatMetadata() const
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
        m_impl = NULL;
    }
    
    GroupTransform& GroupTransform::operator= (const GroupTransform & rhs)
    {
        if(this!=&rhs)
        {
            *m_impl = *rhs.m_impl;
        }
        return *this;
    }
    
    TransformDirection GroupTransform::getDirection() const
    {
        return getImpl()->dir_;
    }
    
    void GroupTransform::setDirection(TransformDirection dir)
    {
        getImpl()->dir_ = dir;
    }
    
    void GroupTransform::validate() const
    {
        Transform::validate();

        for (int i = 0; i<size(); ++i)
        {
            getTransform(i)->validate();
        }
    }

    FormatMetadata & GroupTransform::getFormatMetadata()
    {
        return m_impl->getFormatMetadata();
    }

    const FormatMetadata & GroupTransform::getFormatMetadata() const
    {
        return m_impl->getFormatMetadata();
    }

    int GroupTransform::size() const
    {
        return static_cast<int>(getImpl()->vec_.size());
    }
    
    ConstTransformRcPtr GroupTransform::getTransform(int index) const
    {
        if(index < 0 || index >= (int)getImpl()->vec_.size())
        {
            std::ostringstream os;
            os << "Invalid transform index " << index << ".";
            throw Exception(os.str().c_str());
        }
        
        return getImpl()->vec_[index];
    }

    TransformRcPtr & GroupTransform::getTransform(int index)
    {
        if (index < 0 || index >= (int)getImpl()->vec_.size())
        {
            std::ostringstream os;
            os << "Invalid transform index " << index << ".";
            throw Exception(os.str().c_str());
        }

        return getImpl()->vec_[index];
    }

    void GroupTransform::push_back(const ConstTransformRcPtr& transform)
    {
        getImpl()->vec_.push_back(transform->createEditableCopy());
    }
    
    void GroupTransform::push_back(TransformRcPtr& transform)
    {
        getImpl()->vec_.push_back(transform);
    }

    void GroupTransform::clear()
    {
        getImpl()->vec_.clear();
    }
    
    bool GroupTransform::empty() const
    {
        return getImpl()->vec_.empty();
    }
    
    std::ostream& operator<< (std::ostream& os, const GroupTransform& groupTransform)
    {
        os << "<GroupTransform ";
        os << "direction=" << TransformDirectionToString(groupTransform.getDirection()) << ", ";
        os << "transforms=";
        for(int i=0; i<groupTransform.size(); ++i)
        {
            ConstTransformRcPtr transform = groupTransform.getTransform(i);
            os << "\n\t" << *transform;
        }
        os << ">";
        return os;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    void BuildGroupOps(OpRcPtrVec & ops,
                       const Config& config,
                       const ConstContextRcPtr & context,
                       const GroupTransform& groupTransform,
                       TransformDirection dir)
    {
        if (ops.size() == 0)
        {
            // If group is the first transform, copy the group metadata.
            FormatMetadataImpl & processorData = ops.getFormatMetadata();
            processorData = groupTransform.getFormatMetadata();
        }

        TransformDirection combinedDir = CombineTransformDirections(dir,
                                                  groupTransform.getDirection());
        
        if(combinedDir == TRANSFORM_DIR_FORWARD)
        {
            for(int i=0; i<groupTransform.size(); ++i)
            {
                ConstTransformRcPtr childTransform = groupTransform.getTransform(i);
                BuildOps(ops, config, context, childTransform, TRANSFORM_DIR_FORWARD);
            }
        }
        else if(combinedDir == TRANSFORM_DIR_INVERSE)
        {
            for(int i=groupTransform.size()-1; i>=0; --i)
            {
                ConstTransformRcPtr childTransform = groupTransform.getTransform(i);
                BuildOps(ops, config, context, childTransform, TRANSFORM_DIR_INVERSE);
            }
        }
    }

}
OCIO_NAMESPACE_EXIT

////////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OCIO_ADD_TEST(GroupTransform, basic)
{
    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    OCIO_CHECK_EQUAL(group->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    group->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(group->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_ASSERT(group->empty());
    OCIO_CHECK_EQUAL(group->size(), 0);

    auto & groupData = group->getFormatMetadata();
    OCIO_CHECK_EQUAL(std::string(groupData.getName()), OCIO::METADATA_ROOT);
    OCIO_CHECK_EQUAL(groupData.getNumAttributes(), 0);
    OCIO_CHECK_EQUAL(groupData.getNumChildrenElements(), 0);

    OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();
    group->push_back(matrix);
    OCIO::FixedFunctionTransformRcPtr ff =  OCIO::FixedFunctionTransform::Create();
    group->push_back(ff);

    OCIO_CHECK_ASSERT(!group->empty());
    OCIO_CHECK_EQUAL(group->size(), 2);

    auto t0 = group->getTransform(0);
    auto m0 = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(t0);
    OCIO_CHECK_ASSERT(m0);

    auto t1 = group->getTransform(1);
    auto ff1 = OCIO_DYNAMIC_POINTER_CAST<OCIO::FixedFunctionTransform>(t1);
    OCIO_CHECK_ASSERT(ff1);

    auto & metadata = group->getFormatMetadata();
    OCIO_CHECK_EQUAL(std::string(metadata.getName()), OCIO::METADATA_ROOT);
    OCIO_CHECK_EQUAL(std::string(metadata.getValue()), "");
    OCIO_CHECK_EQUAL(metadata.getNumAttributes(), 0);
    OCIO_CHECK_EQUAL(metadata.getNumChildrenElements(), 0);
    metadata.addAttribute("att1", "val1");
    metadata.addChildElement("child1", "content1");

    group->clear();
    OCIO_CHECK_ASSERT(group->empty());

    const auto & metadataCheck = group->getFormatMetadata();
    OCIO_CHECK_EQUAL(metadataCheck.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(metadataCheck.getNumChildrenElements(), 1);
}

#endif