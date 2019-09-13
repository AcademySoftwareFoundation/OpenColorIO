/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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

#include "Processor.cpp"

#include "ops/exposurecontrast/ExposureContrastOps.h"
#include "UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(Processor, basic)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);
    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    auto processorEmptyGroup = config->getProcessor(group);
    OCIO_CHECK_EQUAL(processorEmptyGroup->getNumTransforms(), 0);
    OCIO_CHECK_EQUAL(std::string(processorEmptyGroup->getCacheID()), "<NOOP>");

    auto mat = OCIO::MatrixTransform::Create();
    double offset[4]{ 0.1, 0.2, 0.3, 0.4 };
    mat->setOffset(offset);

    auto processorMat = config->getProcessor(mat);
    OCIO_CHECK_EQUAL(processorMat->getNumTransforms(), 1);

    OCIO_CHECK_EQUAL(std::string(processorMat->getCacheID()), "$c15dfc9b251ee075f33c4ccb3eb1e4b8");
}

OCIO_ADD_TEST(Processor, shared_dynamic_properties)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO::ExposureContrastOpDataRcPtr data =
        std::make_shared<OCIO::ExposureContrastOpData>();

    data->setExposure(1.2);
    data->setPivot(0.5);
    data->getExposureProperty()->makeDynamic();

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    data = data->clone();
    data->setExposure(2.2);

    OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, data, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_REQUIRE_ASSERT(ops[1]);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    auto data0 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::ExposureContrastOpData>(op0->data());
    auto data1 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::ExposureContrastOpData>(op1->data());

    OCIO::DynamicPropertyImplRcPtr dp0 = data0->getExposureProperty();
    OCIO::DynamicPropertyImplRcPtr dp1 = data1->getExposureProperty();

    OCIO_CHECK_NE(dp0->getDoubleValue(), dp1->getDoubleValue());

    OCIO::UnifyDynamicProperties(ops);

    OCIO::DynamicPropertyImplRcPtr dp0_post = data0->getExposureProperty();
    OCIO::DynamicPropertyImplRcPtr dp1_post = data1->getExposureProperty();

    OCIO_CHECK_EQUAL(dp0_post->getDoubleValue(), dp1_post->getDoubleValue());

    // Both share the same pointer.
    OCIO_CHECK_EQUAL(dp0_post.get(), dp1_post.get());

    // The first pointer is the one that gets shared.
    OCIO_CHECK_EQUAL(dp0.get(), dp0_post.get());
}

namespace
{
void GetFormatName(const std::string & extension, std::string & name)
{
    std::string requestedExt{ extension };
    std::transform(requestedExt.begin(), requestedExt.end(), requestedExt.begin(), ::tolower);
    for (int i = 0; i < OCIO::Processor::getNumWriteFormats(); ++i)
    {
        std::string formatExt(OCIO::Processor::getFormatExtensionByIndex(i));
        if (requestedExt == formatExt)
        {
            name = OCIO::Processor::getFormatNameByIndex(i);
            break;
        }
    }
}
}

OCIO_ADD_TEST(Processor, write_formats)
{
    std::string clfFileFormat;
    GetFormatName("CLF", clfFileFormat);
    OCIO_CHECK_EQUAL(clfFileFormat, OCIO::FILEFORMAT_CLF);

    std::string ctfFileFormat;
    GetFormatName("CTF", ctfFileFormat);
    OCIO_CHECK_EQUAL(ctfFileFormat, OCIO::FILEFORMAT_CTF);

    std::string noFileFormat;
    GetFormatName("XXX", noFileFormat);
    OCIO_CHECK_ASSERT(noFileFormat.empty());
}
