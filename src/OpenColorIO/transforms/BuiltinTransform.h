// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_BUILTINTRANSFORM_H
#define INCLUDED_OCIO_BUILTINTRANSFORM_H


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

class BuiltinTransformImpl : public BuiltinTransform
{
public:
    BuiltinTransformImpl() = default;
    BuiltinTransformImpl(const BuiltinTransformImpl &) = delete;
    BuiltinTransformImpl & operator=(const BuiltinTransformImpl &) = delete;
    ~BuiltinTransformImpl() override = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    const char * getStyle() const noexcept override;
    void setStyle(const char * style) override;

    const char * getDescription() const noexcept override;

    size_t getTransformIndex() const noexcept;

    static void deleter(BuiltinTransform * t);

private:
    TransformDirection m_direction{ TRANSFORM_DIR_FORWARD };
    size_t m_transformIndex{ 0 }; // Index of the built-in transform.
};


} // namespace OCIO_NAMESPACE

#endif  // INCLUDED_OCIO_BUILTINTRANSFORM_H
