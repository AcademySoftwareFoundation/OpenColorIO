// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "DynamicProperty.h"

OCIO_NAMESPACE_ENTER
{

bool operator==(const DynamicProperty &lhs, const DynamicProperty &rhs)
{
    if (DynamicPropertyImpl const * plhs = dynamic_cast<DynamicPropertyImpl const*>(&lhs))
    {
        if (DynamicPropertyImpl const * prhs = dynamic_cast<DynamicPropertyImpl const*>(&rhs))
        {
            return plhs->equals(*prhs);
        }
    }
    return false;
}

DynamicPropertyImpl::DynamicPropertyImpl(DynamicPropertyType type, double value, bool dynamic)
    :   m_type(type)
    ,   m_valueType(DYNAMIC_PROPERTY_DOUBLE)
    ,   m_value(value)
    ,   m_isDynamic(dynamic)
{
}

DynamicPropertyImpl::DynamicPropertyImpl(DynamicPropertyImpl & rhs)
    :   m_type(rhs.m_type)
    ,   m_valueType(rhs.m_valueType)
    ,   m_value(rhs.m_value)
    ,   m_isDynamic(rhs.m_isDynamic)
{   
}

double DynamicPropertyImpl::getDoubleValue() const
{
    if(m_valueType!=DYNAMIC_PROPERTY_DOUBLE)
    {
        throw Exception("The dynamic property does not hold a double precision value.");
    }

    return m_value;
}

void DynamicPropertyImpl::setValue(double value)
{
    if(m_valueType!=DYNAMIC_PROPERTY_DOUBLE)
    {
        throw Exception("The dynamic property does not hold a double precision value.");
    }

    m_value = value;
}

bool DynamicPropertyImpl::equals(const DynamicPropertyImpl & rhs) const
{
    if (this == &rhs) return true;

    if (m_isDynamic == rhs.m_isDynamic && m_type == rhs.m_type)
    {
        if (!m_isDynamic)
        {
            if (m_value == rhs.m_value)
            {
                // Both not dynamic, same value.
                return true;
            }
            else
            {
                // Both not dynamic, different values.
                return false;
            }
        }
        else
        {
            // Both dynamic, will be same.
            return true;
        }
    }

    // One dynamic, not the other.
    return false;
}

DynamicProperty::DynamicProperty()
{

}

DynamicProperty::~DynamicProperty()
{

}

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include <sstream>
#include "UnitTest.h"
#include "UnitTestUtils.h"

OCIO_ADD_TEST(DynamicPropertyImpl, basic)
{
    OCIO::DynamicPropertyRcPtr dp =
        std::make_shared<OCIO::DynamicPropertyImpl>(OCIO::DYNAMIC_PROPERTY_EXPOSURE, 1.0, false);
    OCIO_REQUIRE_ASSERT(dp);
    OCIO_REQUIRE_EQUAL(dp->getDoubleValue(), 1.0);
    dp->setValue(2.0);
    OCIO_CHECK_EQUAL(dp->getDoubleValue(), 2.0);

    OCIO::DynamicPropertyImplRcPtr dpImpl =
        std::make_shared<OCIO::DynamicPropertyImpl>(OCIO::DYNAMIC_PROPERTY_EXPOSURE, 1.0, false);
    OCIO_REQUIRE_ASSERT(dpImpl);
    OCIO_CHECK_ASSERT(!dpImpl->isDynamic());
    OCIO_CHECK_EQUAL(dpImpl->getDoubleValue(), 1.0);

    dpImpl->makeDynamic();
    OCIO_CHECK_ASSERT(dpImpl->isDynamic());
    dpImpl->setValue(2.0);
    OCIO_CHECK_EQUAL(dpImpl->getDoubleValue(), 2.0);
}

OCIO_ADD_TEST(DynamicPropertyImpl, equal)
{
    OCIO::DynamicPropertyImplRcPtr dpImpl0 =
        std::make_shared<OCIO::DynamicPropertyImpl>(OCIO::DYNAMIC_PROPERTY_EXPOSURE, 1.0, false);
    OCIO::DynamicPropertyRcPtr dp0 = dpImpl0;
    
    OCIO::DynamicPropertyImplRcPtr dpImpl1 =
        std::make_shared<OCIO::DynamicPropertyImpl>(OCIO::DYNAMIC_PROPERTY_EXPOSURE, 1.0, false);
    OCIO::DynamicPropertyRcPtr dp1 = dpImpl1;

    // Both not dynamic, same value.
    OCIO_CHECK_ASSERT(*dp0 == *dp1);
    
    // Both not dynamic, diff values.
    dp0->setValue(2.0);
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));
    
    // Same value.
    dp1->setValue(2.0);
    OCIO_CHECK_ASSERT(*dp0 == *dp1);

    // One dynamic, not the other, same value.
    dpImpl0->makeDynamic();
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));
    
    // Both dynamic, same value.
    dpImpl1->makeDynamic();
    OCIO_CHECK_ASSERT(*dp0 == *dp1);
    
    // Both dynamic, diffent values.
    dp1->setValue(3.0);
    OCIO_CHECK_ASSERT(*dp0 == *dp1);
}

namespace
{
OCIO::ConstProcessorRcPtr LoadTransformFile(const std::string & fileName)
{
    const std::string filePath(std::string(OCIO::getTestFilesDir()) + "/"
        + fileName);
    // Create a FileTransform.
    OCIO::FileTransformRcPtr pFileTransform
        = OCIO::FileTransform::Create();
    // A tranform file does not define any interpolation (contrary to config
    // file), this is to avoid exception when creating the operation.
    pFileTransform->setInterpolation(OCIO::INTERP_LINEAR);
    pFileTransform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    pFileTransform->setSrc(filePath.c_str());

    // Create empty Config to use.
    OCIO::ConfigRcPtr pConfig = OCIO::Config::Create();

    // Get the processor corresponding to the transform.
    return pConfig->getProcessor(pFileTransform);
}
}

// Test several aspects of dynamic properties, especially the ability to set
// values via the processor.
OCIO_ADD_TEST(DynamicProperty, get_dynamic_via_processor)
{
    const std::string ctfFile("exposure_contrast_video_dp.ctf");

    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = LoadTransformFile(ctfFile));

    OCIO::ConstCPUProcessorRcPtr cpuProcessor;
    OCIO_CHECK_NO_THROW(cpuProcessor = processor->getDefaultCPUProcessor());

    float pixel[3] = { 0.5f, 0.4f, 0.2f };
    cpuProcessor->applyRGB(pixel);

    float error = 1e-5f;
    OCIO_CHECK_CLOSE(pixel[0], 0.57495f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.43988f, error);
    OCIO_CHECK_CLOSE(pixel[2], 0.19147f, error);

    OCIO::DynamicPropertyType dpt = OCIO::DYNAMIC_PROPERTY_EXPOSURE;
    OCIO::DynamicPropertyRcPtr dp;
    OCIO_CHECK_NO_THROW(dp = cpuProcessor->getDynamicProperty(dpt));
    const double fileValue = dp->getDoubleValue();
    dp->setValue(0.4);

    pixel[0] = 0.5f;
    pixel[1] = 0.4f;
    pixel[2] = 0.2f;
    cpuProcessor->applyRGB(pixel);

    // Adjust error for SSE approximation.
    OCIO_CHECK_CLOSE(pixel[0], 0.62966f, error*2.0f);
    OCIO_CHECK_CLOSE(pixel[1], 0.48175f, error);
    OCIO_CHECK_CLOSE(pixel[2], 0.20969f, error);

    // Restore file value.
    dp->setValue(fileValue);

    pixel[0] = 0.5f;
    pixel[1] = 0.4f;
    pixel[2] = 0.2f;
    cpuProcessor->applyRGB(pixel);

    OCIO_CHECK_CLOSE(pixel[0], 0.57495f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.43988f, error);
    OCIO_CHECK_CLOSE(pixel[2], 0.19147f, error);

    // Note: The CTF does not define gamma as being dynamic.
    OCIO_CHECK_THROW_WHAT(cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA),
                          OCIO::Exception,
                          "Cannot find dynamic property");
}


#endif