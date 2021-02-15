// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CDLTRANSFORM_H
#define INCLUDED_OCIO_CDLTRANSFORM_H

#include <map>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/cdl/CDLOpData.h"

namespace OCIO_NAMESPACE
{
static constexpr char METADATA_INPUT_DESCRIPTION[] = "InputDescription";
static constexpr char METADATA_VIEWING_DESCRIPTION[] = "ViewingDescription";
static constexpr char METADATA_SOP_DESCRIPTION[] = "SOPDescription";
static constexpr char METADATA_SAT_DESCRIPTION[] = "SATDescription";

class CDLTransformImpl;
typedef OCIO_SHARED_PTR<CDLTransformImpl> CDLTransformImplRcPtr;

typedef std::map<std::string, CDLTransformImplRcPtr> CDLTransformMap;
typedef std::vector<CDLTransformImplRcPtr> CDLTransformVec;

class CDLTransformImpl : public CDLTransform
{
public:
    static CDLTransformImplRcPtr Create();

    CDLTransformImpl() = default;
    CDLTransformImpl(const CDLTransformImpl &) = delete;
    CDLTransformImpl & operator=(const CDLTransformImpl &) = delete;
    virtual ~CDLTransformImpl() = default;

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    FormatMetadata & getFormatMetadata() noexcept override;
    const FormatMetadata & getFormatMetadata() const noexcept override;

    void validate() const override;

    bool equals(const CDLTransform & other) const noexcept override;

    CDLStyle getStyle() const override;
    void setStyle(CDLStyle style) override;

    void getSlope(double * rgb) const override;
    void setSlope(const double * rgb) override;

    void getOffset(double * rgb) const override;
    void setOffset(const double * rgb) override;

    void getPower(double * rgb) const override;
    void setPower(const double * rgb) override;

    void getSOP(double * vec9) const override;
    void setSOP(const double * vec9) override;

    double getSat() const override;
    void setSat(double sat) override;

    void getSatLumaCoefs(double * rgb) const override;

    const char * getID() const override;
    void setID(const char * id) override;
    const char * getFirstSOPDescription() const override;
    void setFirstSOPDescription(const char * description) override;

    CDLOpData & data() noexcept { return m_data; }
    const CDLOpData & data() const noexcept { return m_data; }

    static void deleter(CDLTransform * t);

private:
    mutable std::string m_xml;
    CDLOpData m_data;
};

} // namespace OCIO_NAMESPACE

#endif
