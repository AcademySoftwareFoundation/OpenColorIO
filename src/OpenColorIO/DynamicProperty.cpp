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

DynamicPropertyImpl::DynamicPropertyImpl(double value, bool dynamic)
    : m_valueType(DYNAMIC_PROPERTY_DOUBLE)
    , m_value(value)
    , m_isDynamic(dynamic)
{
}


bool DynamicPropertyImpl::equals(const DynamicPropertyImpl & rhs) const
{
    if (this == &rhs) return true;

    if (m_isDynamic == rhs.m_isDynamic && m_valueType == rhs.m_valueType)
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

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include <sstream>
#include "unittest.h"
#include "UnitTestUtils.h"

OIIO_ADD_TEST(DynamicPropertyImpl, basic)
{
    OCIO::DynamicPropertyRcPtr dp =
        std::make_shared<OCIO::DynamicPropertyImpl>(1.0, false);
    OIIO_REQUIRE_ASSERT(dp);
    OIIO_CHECK_EQUAL(dp->getDoubleValue(), 1.0);
    dp->setValue(2.0);
    OIIO_CHECK_EQUAL(dp->getDoubleValue(), 2.0);

    OCIO::DynamicPropertyImplRcPtr dpImpl =
        std::make_shared<OCIO::DynamicPropertyImpl>(1.0, false);
    OIIO_REQUIRE_ASSERT(dpImpl);
    OIIO_CHECK_ASSERT(!dpImpl->isDynamic());
    OIIO_CHECK_EQUAL(dpImpl->getDoubleValue(), 1.0);

    dpImpl->makeDynamic();
    OIIO_CHECK_ASSERT(dpImpl->isDynamic());
    dpImpl->setValue(2.0);
    OIIO_CHECK_EQUAL(dpImpl->getDoubleValue(), 2.0);
}

OIIO_ADD_TEST(DynamicPropertyImpl, equal)
{
    OCIO::DynamicPropertyImplRcPtr dpImpl0 =
        std::make_shared<OCIO::DynamicPropertyImpl>(1.0, false);
    OCIO::DynamicPropertyRcPtr dp0 = dpImpl0;
    
    OCIO::DynamicPropertyImplRcPtr dpImpl1 =
        std::make_shared<OCIO::DynamicPropertyImpl>(1.0, false);
    OCIO::DynamicPropertyRcPtr dp1 = dpImpl1;

    // Both not dynamic, same value.
    OIIO_CHECK_ASSERT(*dp0 == *dp1);
    
    // Both not dynamic, diff values.
    dp0->setValue(2.0);
    OIIO_CHECK_ASSERT(!(*dp0 == *dp1));
    
    // Same value.
    dp1->setValue(2.0);
    OIIO_CHECK_ASSERT(*dp0 == *dp1);

    // One dynamic, not the other, same value.
    dpImpl0->makeDynamic();
    OIIO_CHECK_ASSERT(!(*dp0 == *dp1));
    
    // Both dynamic, same value.
    dpImpl1->makeDynamic();
    OIIO_CHECK_ASSERT(*dp0 == *dp1);
    
    // Both dynamic, diffent values.
    dp1->setValue(3.0);
    OIIO_CHECK_ASSERT(*dp0 == *dp1);
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
OIIO_ADD_TEST(DynamicProperty, get_dynamic_via_processor)
{
    const std::string ctfFile("exposure_contrast_video_dp.ctf");
    OCIO::ConstProcessorRcPtr processor;
    OIIO_CHECK_NO_THROW(processor = LoadTransformFile(ctfFile));

    float pixel[3] = { 0.5f, 0.4f, 0.2f };
    processor->applyRGB(pixel);

    float error = 1e-5f;
    OIIO_CHECK_CLOSE(pixel[0], 0.57495f, error);
    OIIO_CHECK_CLOSE(pixel[1], 0.43988f, error);
    OIIO_CHECK_CLOSE(pixel[2], 0.19147f, error);

    OCIO::DynamicPropertyType dpt = OCIO::DYNAMIC_PROPERTY_EXPOSURE;
    OCIO::DynamicPropertyRcPtr dp;
    OIIO_CHECK_NO_THROW(dp = processor->getDynamicProperty(dpt));
    const double fileValue = dp->getDoubleValue();
    dp->setValue(0.4);

    pixel[0] = 0.5f;
    pixel[1] = 0.4f;
    pixel[2] = 0.2f;
    processor->applyRGB(pixel);

    // Adjust error for SSE approximation.
    OIIO_CHECK_CLOSE(pixel[0], 0.62966f, error*2.0f);
    OIIO_CHECK_CLOSE(pixel[1], 0.48175f, error);
    OIIO_CHECK_CLOSE(pixel[2], 0.20969f, error);

    // Restore file value.
    dp->setValue(fileValue);

    pixel[0] = 0.5f;
    pixel[1] = 0.4f;
    pixel[2] = 0.2f;
    processor->applyRGB(pixel);

    OIIO_CHECK_CLOSE(pixel[0], 0.57495f, error);
    OIIO_CHECK_CLOSE(pixel[1], 0.43988f, error);
    OIIO_CHECK_CLOSE(pixel[2], 0.19147f, error);

    // Note: The CTF does not define gamma as being dynamic.
    OIIO_CHECK_THROW_WHAT(processor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA),
                          OCIO::Exception,
                          "Cannot find dynamic property");
}


#endif