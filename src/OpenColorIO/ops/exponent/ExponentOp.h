// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_EXPONENTOP_H
#define INCLUDED_OCIO_EXPONENTOP_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"

namespace OCIO_NAMESPACE
{
class ExponentOpData;
typedef OCIO_SHARED_PTR<ExponentOpData> ExponentOpDataRcPtr;
typedef OCIO_SHARED_PTR<const ExponentOpData> ConstExponentOpDataRcPtr;

class ExponentOpData : public OpData
{
public:
    ExponentOpData();
    ExponentOpData(const ExponentOpData & rhs);
    explicit ExponentOpData(const double * exp4);
    virtual ~ExponentOpData() {}

    ExponentOpData & operator = (const ExponentOpData & rhs);

    virtual Type getType() const override { return ExponentType; }

    virtual bool isNoOp() const override;
    virtual bool isIdentity() const override;

    virtual bool hasChannelCrosstalk() const override { return false; }

    double m_exp4[4];

    std::string getCacheID() const override;
    void validate() const override;
};

// If the exponent is 1.0, this will return without clamping
// Otherwise, will be clamped between [0.0, inf]

void CreateExponentOp(OpRcPtrVec & ops,
                        const double(&vec4)[4],
                        TransformDirection direction);

void CreateExponentOp(OpRcPtrVec & ops,
                        ExponentOpDataRcPtr & expData,
                        TransformDirection direction);

// Create a copy of the exponent transform in the op and append it to the GroupTransform.
void CreateExponentTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

} // namespace OCIO_NAMESPACE

#endif
