// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GROUPTRANSFORM_H
#define INCLUDED_OCIO_GROUPTRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

class GroupTransformImpl : public GroupTransform
{
public:

    GroupTransformImpl();
    GroupTransformImpl(const GroupTransformImpl &) = delete;
    GroupTransformImpl& operator=(const GroupTransformImpl &) = delete;
    ~GroupTransformImpl() override = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    TransformType getTransformType() const noexcept override
    {
        return TRANSFORM_TYPE_GROUP;
    }

    void validate() const override;

    FormatMetadata & getFormatMetadata() noexcept override
    {
        return m_metadata;
    }

    const FormatMetadata & getFormatMetadata() const noexcept override
    {
        return m_metadata;
    }

    ConstTransformRcPtr getTransform(int index) const override;

    TransformRcPtr & getTransform(int index) override;

    int getNumTransforms() const noexcept override;

    void appendTransform(TransformRcPtr transform) noexcept override;

    void prependTransform(TransformRcPtr transform) noexcept override;

    void write(const ConstConfigRcPtr & config,
               const char * formatName,
               std::ostream & os) const override;

    static void Deleter(GroupTransform * t);

private:
    typedef std::vector<TransformRcPtr> TransformRcPtrVec;

    FormatMetadataImpl m_metadata;
    TransformDirection m_dir;
    TransformRcPtrVec m_vec;
};

}

#endif