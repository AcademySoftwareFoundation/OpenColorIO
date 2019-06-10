/*
Copyright (c) 2019 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/exposurecontrast/ExposureContrastOpCPU.h"
#include "ops/exposurecontrast/ExposureContrastOpGPU.h"
#include "ops/exposurecontrast/ExposureContrastOps.h"

OCIO_NAMESPACE_ENTER
{

namespace
{

class ExposureContrastOp;
typedef OCIO_SHARED_PTR<ExposureContrastOp> ExposureContrastOpRcPtr;
typedef OCIO_SHARED_PTR<const ExposureContrastOp> ConstExposureContrastOpRcPtr;

class ExposureContrastOp : public Op
{
public:
    ExposureContrastOp(ExposureContrastOpDataRcPtr & ec);

    virtual ~ExposureContrastOp();

    OpRcPtr clone() const override;

    std::string getInfo() const override;

    bool isIdentity() const override;
    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

    void finalize() override;

    void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;

    bool hasDynamicProperty(DynamicPropertyType type) const override;
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const override;
    void replaceDynamicProperty(DynamicPropertyType type,
                                DynamicPropertyImplRcPtr prop) override;

    // Note: Only needed by unit tests.
#ifdef OCIO_UNIT_TEST
    OpCPURcPtr getCPUOp() const { return m_cpuOp; }
#endif

protected:
    ConstExposureContrastOpDataRcPtr ecData() const
    { 
        return DynamicPtrCast<const ExposureContrastOpData>(data());
    }
    ExposureContrastOpDataRcPtr ecData()
    { 
        return DynamicPtrCast<ExposureContrastOpData>(data());
    }

    ExposureContrastOp() = delete;
    ExposureContrastOp(const ExposureContrastOp &) = delete;

};


ExposureContrastOp::ExposureContrastOp(ExposureContrastOpDataRcPtr & ec)
    : Op()
{
    data() = ec;
}

OpRcPtr ExposureContrastOp::clone() const
{
    ExposureContrastOpDataRcPtr f = ecData()->clone();
    return std::make_shared<ExposureContrastOp>(f);
}

ExposureContrastOp::~ExposureContrastOp()
{
}

std::string ExposureContrastOp::getInfo() const
{
    return "<ExposureContrastOp>";
}

bool ExposureContrastOp::isIdentity() const
{
    return ecData()->isIdentity();
}

bool ExposureContrastOp::isSameType(ConstOpRcPtr & op) const
{
    ConstExposureContrastOpRcPtr typedRcPtr = DynamicPtrCast<const ExposureContrastOp>(op);
    return (bool)typedRcPtr;
}

bool ExposureContrastOp::isInverse(ConstOpRcPtr & op) const
{
    ConstExposureContrastOpRcPtr typedRcPtr = DynamicPtrCast<const ExposureContrastOp>(op);
    if (!typedRcPtr) return false;

    ConstExposureContrastOpDataRcPtr ecOpData = typedRcPtr->ecData();
    return ecData()->isInverse(ecOpData);
}

bool ExposureContrastOp::canCombineWith(ConstOpRcPtr & /*op*/) const
{
    return false;
}

void ExposureContrastOp::combineWith(OpRcPtrVec & /*ops*/, ConstOpRcPtr & secondOp) const
{
    if (!canCombineWith(secondOp))
    {
        throw Exception("ExposureContrast can't be combined.");
    }
}

void ExposureContrastOp::finalize()
{
    // In this initial implementation, only 32f processing is natively supported.
    ecData()->setInputBitDepth(BIT_DEPTH_F32);
    ecData()->setOutputBitDepth(BIT_DEPTH_F32);

    ecData()->validate();
    ecData()->finalize();

    const ExposureContrastOp & constThis = *this;
    ConstExposureContrastOpDataRcPtr ecOpData = constThis.ecData();
    m_cpuOp = GetExposureContrastCPURenderer(ecOpData);

    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<ExposureContrastOp ";
    cacheIDStream << ecData()->getCacheID() << " ";
    cacheIDStream << ">";

    m_cacheID = cacheIDStream.str();
}

void ExposureContrastOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
{
    if (getInputBitDepth() != BIT_DEPTH_F32 || getOutputBitDepth() != BIT_DEPTH_F32)
    {
        throw Exception("Only 32F bit depth is supported for the GPU shader");
    }

    ConstExposureContrastOpDataRcPtr ecOpData = ecData();
    GetExposureContrastGPUShaderProgram(shaderDesc, ecOpData);
}

bool ExposureContrastOp::hasDynamicProperty(DynamicPropertyType type) const
{
    return ecData()->hasDynamicProperty(type);
}

DynamicPropertyRcPtr ExposureContrastOp::getDynamicProperty(DynamicPropertyType type) const
{
    return ecData()->getDynamicProperty(type);
}

void ExposureContrastOp::replaceDynamicProperty(DynamicPropertyType type,
                                                DynamicPropertyImplRcPtr prop)
{
    ecData()->replaceDynamicProperty(type, prop);
}

}  // Anon namespace


///////////////////////////////////////////////////////////////////////////

void CreateExposureContrastOp(OpRcPtrVec & ops,
                              ExposureContrastOpDataRcPtr & data,
                              TransformDirection direction)
{
    if (data->isNoOp()) return;

    if (direction == TRANSFORM_DIR_FORWARD)
    {
        ops.push_back(std::make_shared<ExposureContrastOp>(data));
    }
    else if(direction == TRANSFORM_DIR_INVERSE)
    {
        ExposureContrastOpDataRcPtr dataInv = data->inverse();
        ops.push_back(std::make_shared<ExposureContrastOp>(dataInv));
    }
    else
    {
        throw Exception("Cannot apply ExposureContrast op, "
                        "unspecified transform direction.");
    }
}

}
OCIO_NAMESPACE_EXIT


////////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

OIIO_ADD_TEST(ExposureContrastOp, create)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::ExposureContrastOpDataRcPtr data =
        std::make_shared<OCIO::ExposureContrastOpData>();
    OCIO::OpRcPtrVec ops;

    // Make it dynamic so that it is not a no op.
    data->getExposureProperty()->makeDynamic();
    OIIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_REQUIRE_ASSERT(ops[0]);
    OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<ExposureContrastOp>");

    data = data->clone();
    data->getContrastProperty()->makeDynamic();
    OIIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OIIO_REQUIRE_ASSERT(ops[1]);
    OCIO::ConstOpRcPtr op1 = ops[1];
    OIIO_CHECK_ASSERT(ops[0]->isSameType(op1));
}

OIIO_ADD_TEST(ExposureContrastOp, inverse)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::ExposureContrastOpDataRcPtr data =
        std::make_shared<OCIO::ExposureContrastOpData>();

    data->setExposure(1.2);
    data->setPivot(0.5);

    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_REQUIRE_ASSERT(ops[0]);

    data = data->clone();
    direction = OCIO::TRANSFORM_DIR_INVERSE;
    OIIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OIIO_REQUIRE_ASSERT(ops[1]);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OIIO_CHECK_ASSERT(op0->isInverse(op1));
    OIIO_CHECK_ASSERT(op1->isInverse(op0));

    // Create op2 similar to op1 with different exposure.
    data = data->clone();
    data->setExposure(1.3);
    OIIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OIIO_REQUIRE_EQUAL(ops.size(), 3);
    OIIO_REQUIRE_ASSERT(ops[2]);

    OCIO::ConstOpRcPtr op2 = ops[2];
    // As exposure from E/C is not dynamic and exposure is different,
    // op1 and op2 are different.
    OIIO_CHECK_ASSERT(op1 != op2);
    OIIO_CHECK_ASSERT(!op0->isInverse(op2));

    // With dynamic property.
    data = data->clone();
    data->getExposureProperty()->makeDynamic();
    OCIO::DynamicPropertyRcPtr dp3 = data->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE);
    OIIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OIIO_REQUIRE_EQUAL(ops.size(), 4);
    OIIO_REQUIRE_ASSERT(ops[3]);
    OCIO::ConstOpRcPtr op3 = ops[3];

    data = data->clone();
    OCIO::DynamicPropertyRcPtr dp4 = data->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE);

    direction = OCIO::TRANSFORM_DIR_FORWARD;
    OIIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OIIO_REQUIRE_EQUAL(ops.size(), 5);
    OIIO_REQUIRE_ASSERT(ops[4]);

    // Exposure dynamic, same value, opposite direction.
    OIIO_CHECK_ASSERT(ops[4]->isInverse(op3));
    OIIO_CHECK_ASSERT(!ops[4]->isInverse(op1));
    OIIO_CHECK_ASSERT(!ops[4]->isInverse(op0));

    // Value is not taken into account when dynamic property is enabled.
    dp4->setValue(-1.);
    OIIO_CHECK_ASSERT(dp3->getDoubleValue() != dp4->getDoubleValue());
    OIIO_CHECK_ASSERT(ops[4]->isInverse(op3));
}

#endif