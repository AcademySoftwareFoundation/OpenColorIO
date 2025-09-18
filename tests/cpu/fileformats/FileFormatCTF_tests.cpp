// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "BitDepthUtils.h"
#include "fileformats/FileFormatCTF.cpp"
#include "ops/fixedfunction/FixedFunctionOp.h"
#include "ops/gradingrgbcurve/GradingRGBCurve.h"
#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


///////////////////////////////////////////////////////////////////////////////
//
// READER TESTS
//
///////////////////////////////////////////////////////////////////////////////

namespace
{
OCIO::LocalCachedFileRcPtr LoadCLFFile(const std::string & fileName)
{
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(
        fileName, std::ios_base::in);
}
}

OCIO_ADD_TEST(FileFormatCTF, missing_file)
{
    // Test LoadCLFFile helper function with missing file.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("xxxxxxxxxxxxxxxxx.xxxxx");
    OCIO_CHECK_THROW_WHAT(cachedFile = LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Error opening test file.");
}

OCIO_ADD_TEST(FileFormatCTF, clf_examples)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("clf/lut1d_example.clf");
        OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
        OCIO_REQUIRE_ASSERT((bool)cachedFile);
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getName(),
                         "transform example lut1d");
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getID(), "exlut1");
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().size(), 1);
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[0],
                         "1D LUT with legal out of range values");
        const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
        OCIO_REQUIRE_EQUAL(opList.size(), 1);
        OCIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::Lut1DType);
        OCIO_CHECK_EQUAL(opList[0]->getName(), "65valueLut");
        OCIO_CHECK_EQUAL(opList[0]->getID(), "lut-23");
        auto lut = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(opList[0]);
        OCIO_REQUIRE_ASSERT(lut);
        OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        StringUtils::StringVec desc;
        GetElementsValues(opList[0]->getFormatMetadata().getChildrenElements(),
                          OCIO::TAG_DESCRIPTION, desc);
        OCIO_REQUIRE_EQUAL(desc.size(), 2);
        OCIO_CHECK_EQUAL(desc[0], "Note that the bit-depth does not constrain the legal range of values.");
        OCIO_CHECK_EQUAL(desc[1], "Formula: flipud(1.25 - 1.5 * x^2.2)");
    }

    {
        const std::string ctfFile("clf/lut3d_identity_12i_16f.clf");
        OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
        OCIO_REQUIRE_ASSERT((bool)cachedFile);
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getName(),
                         "transform example lut3d");
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getID(), "exlut2");
        OCIO_REQUIRE_EQUAL(cachedFile->m_transform->getDescriptions().size(), 1);
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[0],
                         " 3D LUT example ");
        const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
        OCIO_REQUIRE_EQUAL(opList.size(), 1);
        OCIO_CHECK_EQUAL(opList[0]->getName(), "identity");
        OCIO_CHECK_EQUAL(opList[0]->getID(), "lut-24");
        auto lut = OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(opList[0]);
        OCIO_REQUIRE_ASSERT(lut);
        OCIO_CHECK_EQUAL(lut->getInterpolation(), OCIO::INTERP_TETRAHEDRAL);
        OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F16);
        StringUtils::StringVec desc;
        GetElementsValues(opList[0]->getFormatMetadata().getChildrenElements(),
                          OCIO::TAG_DESCRIPTION, desc);
        OCIO_REQUIRE_EQUAL(desc.size(), 1);
        OCIO_CHECK_EQUAL(desc[0], " 3D LUT ");
    }

    {
        const std::string ctfFile("clf/matrix_3x4_example.clf");
        OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
        OCIO_REQUIRE_ASSERT((bool)cachedFile);
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getName(),
                         "transform example matrix");
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getID(), "exmat1");
        OCIO_REQUIRE_EQUAL(cachedFile->m_transform->getDescriptions().size(), 2);
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[0],
                         " Matrix example ");
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[1],
                         " Used by unit tests ");
        const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
        OCIO_REQUIRE_EQUAL(opList.size(), 1);
        OCIO_CHECK_EQUAL(opList[0]->getName(), "colorspace conversion");
        OCIO_CHECK_EQUAL(opList[0]->getID(), "mat-25");
        auto mat = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(opList[0]);
        OCIO_REQUIRE_ASSERT(mat);
        OCIO_CHECK_EQUAL(mat->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OCIO_CHECK_EQUAL(mat->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        StringUtils::StringVec desc;
        GetElementsValues(opList[0]->getFormatMetadata().getChildrenElements(),
                          OCIO::TAG_DESCRIPTION, desc);
        OCIO_REQUIRE_EQUAL(desc.size(), 1);
        OCIO_CHECK_EQUAL(desc[0], " 3x4 Matrix , 4th column is offset ");

       // In file, matrix is defined by a 4x4 array.
        const OCIO::ArrayDouble & array = mat->getArray();
        OCIO_CHECK_EQUAL(array.getLength(), 4);
        OCIO_CHECK_EQUAL(array.getNumColorComponents(), 4);
        OCIO_CHECK_EQUAL(array.getNumValues(),
                         array.getLength()*array.getLength());

        const double oscale = OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT12);
        const double scale =  oscale / OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT10);

        // Check matrix ...
        OCIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
        OCIO_CHECK_EQUAL(array.getValues()[0] * scale,  3.60);
        OCIO_CHECK_EQUAL(array.getValues()[1] * scale,  0.10);
        OCIO_CHECK_EQUAL(array.getValues()[2] * scale, -0.20);
        OCIO_CHECK_EQUAL(array.getValues()[3],  0.0);

        OCIO_CHECK_EQUAL(array.getValues()[4] * scale,  0.20);
        OCIO_CHECK_EQUAL(array.getValues()[5] * scale,  3.50);
        OCIO_CHECK_EQUAL(array.getValues()[6] * scale,  0.10);
        OCIO_CHECK_EQUAL(array.getValues()[7],  0.0);

        OCIO_CHECK_EQUAL(array.getValues()[8]  * scale, 0.10);
        OCIO_CHECK_EQUAL(array.getValues()[9]  * scale,-0.30);
        OCIO_CHECK_EQUAL(array.getValues()[10] * scale, 3.40);
        OCIO_CHECK_EQUAL(array.getValues()[11], 0.0);

        OCIO_CHECK_EQUAL(array.getValues()[12], 0.0);
        OCIO_CHECK_EQUAL(array.getValues()[13], 0.0);
        OCIO_CHECK_EQUAL(array.getValues()[14], 0.0);
        OCIO_CHECK_EQUAL(array.getValues()[15], 1.0);

        const OCIO::MatrixOpData::Offsets & offsets = mat->getOffsets();
        // ... offsets.
        OCIO_CHECK_EQUAL(offsets[0] * oscale,  0.30);
        OCIO_CHECK_EQUAL(offsets[1] * oscale, -0.05);
        OCIO_CHECK_EQUAL(offsets[2] * oscale, -0.40);
        OCIO_CHECK_EQUAL(offsets[3], 0.0);
    }

    {
        // Test two-entries IndexMap support.
        const std::string ctfFile("indexMap_test_clfv2.clf");
        OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
        OCIO_REQUIRE_ASSERT((bool)cachedFile);
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getName(),
                         "transform example lut IndexMap");
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getID(), "exlut3");
        OCIO_REQUIRE_EQUAL(cachedFile->m_transform->getDescriptions().size(), 1);
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[0],
                         " IndexMap LUT example from spec ");
        const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
        OCIO_REQUIRE_EQUAL(opList.size(), 2);
        auto pR = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[0]);
        OCIO_REQUIRE_ASSERT(pR);
        OCIO_CHECK_EQUAL(pR->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OCIO_CHECK_EQUAL(pR->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OCIO_CHECK_EQUAL(pR->getMinInValue(), 64. / 1023.);
        OCIO_CHECK_EQUAL(pR->getMaxInValue(), 940. / 1023.);
        OCIO_CHECK_EQUAL(pR->getMinOutValue(), 0. / 1023.);
        OCIO_CHECK_EQUAL(pR->getMaxOutValue(), 1023. / 1023.);

        OCIO_CHECK_EQUAL(opList[1]->getName(), "IndexMap LUT");
        OCIO_CHECK_EQUAL(opList[1]->getID(), "lut-26");
        auto lut = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(opList[1]);
        OCIO_REQUIRE_ASSERT(lut);
        OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F16);
        StringUtils::StringVec desc;
        GetElementsValues(opList[1]->getFormatMetadata().getChildrenElements(),
                          OCIO::TAG_DESCRIPTION, desc);
        OCIO_REQUIRE_EQUAL(desc.size(), 1);
        OCIO_CHECK_EQUAL(desc[0], " 1D LUT with IndexMap ");
    }
}

OCIO_ADD_TEST(FileFormatCTF, matrix4x4)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("matrix_example4x4.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::CTFVersion ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OCIO_CHECK_ASSERT(OCIO::CTF_PROCESS_LIST_VERSION_1_2 == ctfVersion);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pMatrix = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pMatrix);

    OCIO_CHECK_ASSERT(cachedFile->m_transform->getInputDescriptor() == "XYZ");
    OCIO_CHECK_ASSERT(cachedFile->m_transform->getOutputDescriptor() == "RGB");

    OCIO_CHECK_EQUAL(pMatrix->getFileInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(pMatrix->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    // In file, matrix is defined by a 4x4 array.
    const OCIO::ArrayDouble & array = pMatrix->getArray();
    OCIO_CHECK_EQUAL(array.getLength(), 4);
    OCIO_CHECK_EQUAL(array.getNumColorComponents(), 4);
    OCIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength());

    OCIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
    OCIO_CHECK_EQUAL(array.getValues()[0],  3.24);
    OCIO_CHECK_EQUAL(array.getValues()[1], -1.537);
    OCIO_CHECK_EQUAL(array.getValues()[2], -0.49850);
    OCIO_CHECK_EQUAL(array.getValues()[3],  0.0);

    OCIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
    OCIO_CHECK_EQUAL(array.getValues()[5],  1.876);
    OCIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
    OCIO_CHECK_EQUAL(array.getValues()[7],  0.0);

    OCIO_CHECK_EQUAL(array.getValues()[8],  0.05560);
    OCIO_CHECK_EQUAL(array.getValues()[9], -0.204);
    // Validate double precision can be read both matrix and ...
    OCIO_CHECK_EQUAL(array.getValues()[10], 1.123456789012);
    OCIO_CHECK_EQUAL(array.getValues()[11], 0.0);

    OCIO_CHECK_EQUAL(array.getValues()[12], 0.0);
    OCIO_CHECK_EQUAL(array.getValues()[13], 0.0);
    OCIO_CHECK_EQUAL(array.getValues()[14], 0.0);
    OCIO_CHECK_EQUAL(array.getValues()[15], 1.0);

    const OCIO::MatrixOpData::Offsets & offsets = pMatrix->getOffsets();
    // ... offset
    OCIO_CHECK_EQUAL(offsets[0], 0.987654321098);
    OCIO_CHECK_EQUAL(offsets[1], 0.2);
    OCIO_CHECK_EQUAL(offsets[2], 0.3);
    OCIO_CHECK_EQUAL(offsets[3], 0.0);
}

OCIO_ADD_TEST(FileFormatCTF, matrix_with_offset)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("matrix_offsets_example.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);
    // Note that the ProcessList does not have a version attribute and
    // therefore defaults to 1.2.
    // The "4x4x3" Array syntax is only allowed in versions 1.2 or earlier.
    const OCIO::CTFVersion ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OCIO_CHECK_EQUAL(OCIO::CTF_PROCESS_LIST_VERSION_1_2, ctfVersion);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pMatrix = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pMatrix);

    const OCIO::ArrayDouble & array = pMatrix->getArray();
    OCIO_CHECK_EQUAL(array.getLength(), 4);
    OCIO_CHECK_EQUAL(array.getNumColorComponents(), 4);
    OCIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength());

    OCIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
    OCIO_CHECK_EQUAL(array.getValues()[0],  3.24);
    OCIO_CHECK_EQUAL(array.getValues()[1], -1.537);
    OCIO_CHECK_EQUAL(array.getValues()[2], -0.49850);
    OCIO_CHECK_EQUAL(array.getValues()[3],  0.0);
    
    OCIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
    OCIO_CHECK_EQUAL(array.getValues()[5],  1.876);
    OCIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
    OCIO_CHECK_EQUAL(array.getValues()[7],  0.0);
    
    OCIO_CHECK_EQUAL(array.getValues()[8],  0.05560);
    OCIO_CHECK_EQUAL(array.getValues()[9], -0.204);
    OCIO_CHECK_EQUAL(array.getValues()[10], 1.0573);
    OCIO_CHECK_EQUAL(array.getValues()[11], 0.0);
    
    OCIO_CHECK_EQUAL(array.getValues()[12], 0.0);
    OCIO_CHECK_EQUAL(array.getValues()[13], 0.0);
    OCIO_CHECK_EQUAL(array.getValues()[14], 0.0);
    OCIO_CHECK_EQUAL(array.getValues()[15], 1.0);
    
    OCIO_CHECK_EQUAL(pMatrix->getOffsets()[0], 1.0);
    OCIO_CHECK_EQUAL(pMatrix->getOffsets()[1], 2.0);
    OCIO_CHECK_EQUAL(pMatrix->getOffsets()[2], 3.0);
}

OCIO_ADD_TEST(FileFormatCTF, matrix_with_offset_1_3)
{
    // Matrix 4 4 3 only valid up to version 1.2.
    const std::string ctfFile("matrix_offsets_example_1_3.ctf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Illegal array dimensions 4 4 3");
}

OCIO_ADD_TEST(FileFormatCTF, matrix_1_3_3x3)
{
    // Version 1.3, array 3x3x3: matrix with no alpha and no offsets.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("matrix_example_1_3_3x3.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::CTFVersion & ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OCIO_CHECK_ASSERT(OCIO::CTF_PROCESS_LIST_VERSION_1_3 == ctfVersion);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pMatrix = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pMatrix);

    OCIO_CHECK_ASSERT(cachedFile->m_transform->getInputDescriptor() == "XYZ");
    OCIO_CHECK_ASSERT(cachedFile->m_transform->getOutputDescriptor() == "RGB");

    OCIO_CHECK_EQUAL(pMatrix->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OCIO_CHECK_EQUAL(pMatrix->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    // 3x3 array gets extended to 4x4.
    const OCIO::ArrayDouble & array = pMatrix->getArray();
    OCIO_CHECK_EQUAL(array.getLength(), 4);
    OCIO_CHECK_EQUAL(array.getNumColorComponents(), 4);
    OCIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength());

    OCIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
    OCIO_CHECK_EQUAL(array.getValues()[0],  3.24);
    OCIO_CHECK_EQUAL(array.getValues()[1], -1.537);
    OCIO_CHECK_EQUAL(array.getValues()[2], -0.49850);
    OCIO_CHECK_EQUAL(array.getValues()[3],  0.0);

    OCIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
    OCIO_CHECK_EQUAL(array.getValues()[5],  1.876);
    OCIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
    OCIO_CHECK_EQUAL(array.getValues()[7],  0.0);

    OCIO_CHECK_EQUAL(array.getValues()[8],  0.05560);
    OCIO_CHECK_EQUAL(array.getValues()[9], -0.204);
    OCIO_CHECK_EQUAL(array.getValues()[10], 1.0573);
    OCIO_CHECK_EQUAL(array.getValues()[11], 0.0);

    OCIO_CHECK_EQUAL(array.getValues()[12], 0.0);
    OCIO_CHECK_EQUAL(array.getValues()[13], 0.0);
    OCIO_CHECK_EQUAL(array.getValues()[14], 0.0);
    OCIO_CHECK_EQUAL(array.getValues()[15], 1.0);

    const OCIO::MatrixOpData::Offsets & offsets = pMatrix->getOffsets();
    OCIO_CHECK_EQUAL(offsets[1], 0.0);
    OCIO_CHECK_EQUAL(offsets[2], 0.0);
    OCIO_CHECK_EQUAL(offsets[3], 0.0);
    OCIO_CHECK_EQUAL(offsets[0], 0.0);
}

OCIO_ADD_TEST(FileFormatCTF, matrix_1_3_4x4)
{
    // Version 1.3, array 4x4x4, matrix with alpha and no offsets.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("matrix_example_1_3_4x4.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::CTFVersion & ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OCIO_CHECK_ASSERT(OCIO::CTF_PROCESS_LIST_VERSION_1_3 == ctfVersion);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pMatrix = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pMatrix);

    const OCIO::ArrayDouble & array = pMatrix->getArray();
    OCIO_CHECK_EQUAL(array.getLength(), 4);
    OCIO_CHECK_EQUAL(array.getNumColorComponents(), 4);
    OCIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength());

    OCIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());

    OCIO_CHECK_EQUAL(array.getValues()[0],  3.24);
    OCIO_CHECK_EQUAL(array.getValues()[1], -1.537);
    OCIO_CHECK_EQUAL(array.getValues()[2], -0.49850);
    OCIO_CHECK_EQUAL(array.getValues()[3], -0.1);

    OCIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
    OCIO_CHECK_EQUAL(array.getValues()[5],  1.876);
    OCIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
    OCIO_CHECK_EQUAL(array.getValues()[7], -0.2);

    OCIO_CHECK_EQUAL(array.getValues()[8],   0.05560);
    OCIO_CHECK_EQUAL(array.getValues()[9],  -0.204);
    OCIO_CHECK_EQUAL(array.getValues()[10],  1.0573);
    OCIO_CHECK_EQUAL(array.getValues()[11], -0.3);

    OCIO_CHECK_EQUAL(array.getValues()[12], 0.11);
    OCIO_CHECK_EQUAL(array.getValues()[13], 0.22);
    OCIO_CHECK_EQUAL(array.getValues()[14], 0.33);
    OCIO_CHECK_EQUAL(array.getValues()[15], 0.4);

    const OCIO::MatrixOpData::Offsets & offsets = pMatrix->getOffsets();
    OCIO_CHECK_EQUAL(offsets[0], 0.0);
    OCIO_CHECK_EQUAL(offsets[1], 0.0);
    OCIO_CHECK_EQUAL(offsets[2], 0.0);
    OCIO_CHECK_EQUAL(offsets[3], 0.0);
}

OCIO_ADD_TEST(FileFormatCTF, matrix_1_3_offsets)
{
    // Version 1.3, array 3x4x3: matrix only with offsets and no alpha.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("matrix_example_1_3_offsets.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::CTFVersion ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OCIO_CHECK_ASSERT(OCIO::CTF_PROCESS_LIST_VERSION_1_3 == ctfVersion);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pMatrix = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pMatrix);

    const OCIO::ArrayDouble & array = pMatrix->getArray();
    OCIO_CHECK_EQUAL(array.getLength(), 4);
    OCIO_CHECK_EQUAL(array.getNumColorComponents(), 4);
    OCIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength());

    OCIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
    OCIO_CHECK_EQUAL(array.getValues()[0],  3.24);
    OCIO_CHECK_EQUAL(array.getValues()[1], -1.537);
    OCIO_CHECK_EQUAL(array.getValues()[2], -0.49850);
    OCIO_CHECK_EQUAL(array.getValues()[3],  0.0f);

    OCIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
    OCIO_CHECK_EQUAL(array.getValues()[5],  1.876);
    OCIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
    OCIO_CHECK_EQUAL(array.getValues()[7],  0.0);

    OCIO_CHECK_EQUAL(array.getValues()[8],  0.05560);
    OCIO_CHECK_EQUAL(array.getValues()[9], -0.204);
    OCIO_CHECK_EQUAL(array.getValues()[10], 1.0573);
    OCIO_CHECK_EQUAL(array.getValues()[11], 0.0);

    OCIO_CHECK_EQUAL(array.getValues()[12], 0.0);
    OCIO_CHECK_EQUAL(array.getValues()[13], 0.0);
    OCIO_CHECK_EQUAL(array.getValues()[14], 0.0);
    OCIO_CHECK_EQUAL(array.getValues()[15], 1.0);

    const OCIO::MatrixOpData::Offsets & offsets = pMatrix->getOffsets();
    OCIO_CHECK_EQUAL(offsets[0], 0.1);
    OCIO_CHECK_EQUAL(offsets[1], 0.2);
    OCIO_CHECK_EQUAL(offsets[2], 0.3);
    OCIO_CHECK_EQUAL(offsets[3], 0.0);
}

OCIO_ADD_TEST(FileFormatCTF, matrix_1_3_alpha_offsets)
{
    // Version 1.3, array 4x5x4: matrix with alpha and offsets.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("matrix_example_1_3_alpha_offsets.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::CTFVersion & ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OCIO_CHECK_ASSERT(OCIO::CTF_PROCESS_LIST_VERSION_1_3 == ctfVersion);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pMatrix = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pMatrix);

    const OCIO::ArrayDouble & array = pMatrix->getArray();
    OCIO_CHECK_EQUAL(array.getLength(), 4);
    OCIO_CHECK_EQUAL(array.getNumColorComponents(), 4);
    OCIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength());

    OCIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
    OCIO_CHECK_EQUAL(array.getValues()[0],  3.24);
    OCIO_CHECK_EQUAL(array.getValues()[1], -1.537);
    OCIO_CHECK_EQUAL(array.getValues()[2], -0.49850);
    OCIO_CHECK_EQUAL(array.getValues()[3],  0.6);

    OCIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
    OCIO_CHECK_EQUAL(array.getValues()[5],  1.876);
    OCIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
    OCIO_CHECK_EQUAL(array.getValues()[7],  0.7);

    OCIO_CHECK_EQUAL(array.getValues()[8],  0.05560);
    OCIO_CHECK_EQUAL(array.getValues()[9], -0.204);
    OCIO_CHECK_EQUAL(array.getValues()[10], 1.0573);
    OCIO_CHECK_EQUAL(array.getValues()[11], 0.8);

    OCIO_CHECK_EQUAL(array.getValues()[12], 1.2);
    OCIO_CHECK_EQUAL(array.getValues()[13], 1.3);
    OCIO_CHECK_EQUAL(array.getValues()[14], 1.4);
    OCIO_CHECK_EQUAL(array.getValues()[15], 1.5);

    const OCIO::MatrixOpData::Offsets & offsets = pMatrix->getOffsets();
    OCIO_CHECK_EQUAL(offsets[0], 0.1);
    OCIO_CHECK_EQUAL(offsets[1], 0.2);
    OCIO_CHECK_EQUAL(offsets[2], 0.3);
    OCIO_CHECK_EQUAL(offsets[3], 0.4);
}

namespace
{
void CheckIdentity(std::istringstream & ctfStream, unsigned line)
{
    // Load file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr file;
    OCIO_CHECK_NO_THROW_FROM(file = tester.read(ctfStream, emptyString, OCIO::INTERP_DEFAULT), line);
    OCIO::LocalCachedFileRcPtr cachedFile = OCIO_DYNAMIC_POINTER_CAST<OCIO::LocalCachedFile>(file);
    const auto & fileOps = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL_FROM(fileOps.size(), 1, line);
    const auto op = fileOps[0];
    auto mat = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(op);
    OCIO_REQUIRE_ASSERT_FROM(mat, line);
    OCIO_CHECK_ASSERT_FROM(mat->isIdentity(), line);
}
}

OCIO_ADD_TEST(FileFormatCTF, matrix_identity)
{
    std::istringstream ctf;

    // Pre version 1.3 matrix parsing.

    ctf.str(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList id="none">
    <Description>RGB matrix Identity, 10i to 12i</Description>
    <Matrix inBitDepth="10i" outBitDepth="12i">
        <Array dim="3 3 3">
4.0029325513196481 0 0
0 4.0029325513196481 0
0 0 4.0029325513196481
        </Array>
    </Matrix>
</ProcessList>
)");
    CheckIdentity(ctf, __LINE__);

    ctf.clear();
    ctf.str(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList id="none" version="1.2">
    <Description>RGB matrix + offset Identity, 10i to 12i</Description>
    <Matrix inBitDepth="10i" outBitDepth="12i">
        <Array dim="4 4 3">
4.0029325513196481 0 0 0
0 4.0029325513196481 0 0
0 0 4.0029325513196481 0
0 0                  0 0
        </Array>
    </Matrix>
</ProcessList>
)");
    CheckIdentity(ctf, __LINE__);

    // Version 1.3 and onward matrix parsing.

    ctf.clear();
    ctf.str(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList id="none" version="1.3">
    <Description>RGB matrix Identity, 10i to 12i</Description>
    <Matrix inBitDepth="10i" outBitDepth="12i">
        <Array dim="3 3 3">
4.0029325513196481 0 0
0 4.0029325513196481 0
0 0 4.0029325513196481
        </Array>
    </Matrix>
</ProcessList>
)");
    CheckIdentity(ctf, __LINE__);
    ctf.clear();

    ctf.str(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList id="none" version="1.3">
    <Description>RGBA matrix Identity, 10i to 12i</Description>
    <Matrix inBitDepth="10i" outBitDepth="12i">
        <Array dim="4 4 4">
4.0029325513196481 0 0 0
0 4.0029325513196481 0 0
0 0 4.0029325513196481 0
0 0 0 4.0029325513196481
        </Array>
    </Matrix>
</ProcessList>
)");
    CheckIdentity(ctf, __LINE__);

    ctf.clear();
    ctf.str(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList id="none" version="1.3">
    <Description>RGB matrix + offset Identity, 10i to 12i</Description>
    <Matrix inBitDepth="10i" outBitDepth="12i">
        <Array dim="3 4 3">
4.0029325513196481 0 0 0
0 4.0029325513196481 0 0
0 0 4.0029325513196481 0
        </Array>
    </Matrix>
</ProcessList>
)");
    CheckIdentity(ctf, __LINE__);
}

OCIO_ADD_TEST(FileFormatCTF, lut_1d)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("lut1d_32_10i_10i.ctf");
        OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
        OCIO_REQUIRE_ASSERT((bool)cachedFile);
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getName(), "1d-lut example");
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getID(),
                         "9843a859-e41e-40a8-a51c-840889c3774e");
        OCIO_REQUIRE_EQUAL(cachedFile->m_transform->getDescriptions().size(), 1);
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[0],
                         "Apply a 1/2.2 gamma.");
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getInputDescriptor(), "RGB");
        OCIO_CHECK_EQUAL(cachedFile->m_transform->getOutputDescriptor(), "RGB");
        const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
        OCIO_REQUIRE_EQUAL(opList.size(), 1);

        auto pLut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[0]);
        OCIO_REQUIRE_ASSERT(pLut);

        StringUtils::StringVec desc;
        GetElementsValues(pLut->getFormatMetadata().getChildrenElements(),
                          OCIO::TAG_DESCRIPTION, desc);
        OCIO_REQUIRE_EQUAL(desc.size(), 1);

        OCIO_CHECK_ASSERT(!pLut->isInputHalfDomain());
        OCIO_CHECK_ASSERT(!pLut->isOutputRawHalfs());
        OCIO_CHECK_EQUAL(pLut->getHueAdjust(), OCIO::HUE_NONE);

        OCIO_CHECK_EQUAL(pLut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OCIO_CHECK_ASSERT(pLut->getName() == "1d-lut example op");

        // TODO: bypass is for CTF
        // OCIO_CHECK_ASSERT(!pLut->getBypass()->isDynamic());

        // LUT is defined with a 32x1 array.
        // Array is extended to 32x3 by duplicating the available component.
        const OCIO::Array & array = pLut->getArray();
        OCIO_CHECK_EQUAL(array.getLength(), 32);
        OCIO_CHECK_EQUAL(array.getNumColorComponents(), 1);
        OCIO_CHECK_EQUAL(array.getNumValues(),
                         array.getLength()
                         *pLut->getArray().getMaxColorComponents());

        OCIO_REQUIRE_EQUAL(array.getValues().size(), 96);
        OCIO_CHECK_EQUAL(array.getValues()[0], 0.0f);
        OCIO_CHECK_EQUAL(array.getValues()[1], 0.0f);
        OCIO_CHECK_EQUAL(array.getValues()[2], 0.0f);
        OCIO_CHECK_EQUAL(array.getValues()[3], 215.0f / 1023.0f);
        OCIO_CHECK_EQUAL(array.getValues()[4], 215.0f / 1023.0f);
        OCIO_CHECK_EQUAL(array.getValues()[5], 215.0f / 1023.0f);
        OCIO_CHECK_EQUAL(array.getValues()[6], 294.0f / 1023.0f);
        // and many more
        OCIO_CHECK_EQUAL(array.getValues()[92], 1008.0f / 1023.0f);
        OCIO_CHECK_EQUAL(array.getValues()[93], 1023.0f / 1023.0f);
        OCIO_CHECK_EQUAL(array.getValues()[94], 1023.0f / 1023.0f);
        OCIO_CHECK_EQUAL(array.getValues()[95], 1023.0f / 1023.0f);
    }

    // Test the hue adjust attribute.
    {
        const std::string ctfFile("lut1d_hue_adjust_test.ctf");
        OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
        OCIO_REQUIRE_ASSERT((bool)cachedFile);

        const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
        OCIO_REQUIRE_EQUAL(opList.size(), 1);
        auto pLut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[0]);
        OCIO_REQUIRE_ASSERT(pLut);
        OCIO_CHECK_EQUAL(pLut->getHueAdjust(), OCIO::HUE_DW3);
    }
}

OCIO_ADD_TEST(FileFormatCTF, lut1d_hue_adjust_invalid_style)
{
    const std::string ctfFile("lut1d_hue_adjust_invalid_style.ctf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "Illegal 'hueAdjust' attribute");
}

OCIO_ADD_TEST(FileFormatCTF, lut_3by1d_with_nan_infinity)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("lut3by1d_nan_infinity_example.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pLut1d = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pLut1d);

    const OCIO::Array & array = pLut1d->getArray();

    OCIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
    OCIO_CHECK_ASSERT(OCIO::IsNan(array.getValues()[0]));
    OCIO_CHECK_ASSERT(OCIO::IsNan(array.getValues()[1]));
    OCIO_CHECK_ASSERT(OCIO::IsNan(array.getValues()[2]));
    OCIO_CHECK_ASSERT(OCIO::IsNan(array.getValues()[3]));
    OCIO_CHECK_ASSERT(OCIO::IsNan(array.getValues()[4]));
    OCIO_CHECK_EQUAL(array.getValues()[5],
                     std::numeric_limits<float>::infinity());
    OCIO_CHECK_EQUAL(array.getValues()[6],
                     std::numeric_limits<float>::infinity());
    OCIO_CHECK_EQUAL(array.getValues()[7],
                     std::numeric_limits<float>::infinity());
    OCIO_CHECK_EQUAL(array.getValues()[8],
                     -std::numeric_limits<float>::infinity());
    OCIO_CHECK_EQUAL(array.getValues()[9],
                     -std::numeric_limits<float>::infinity());
}

OCIO_ADD_TEST(FileFormatCTF, lut1d_half_domain_set_false)
{
    // Should throw an exception because the 'half_domain' tag
    // was found but set to something other than 'true'.
    const std::string ctfFile("clf/illegal/lut1d_half_domain_set_false.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Illegal 'halfDomain' attribute");
}

OCIO_ADD_TEST(FileFormatCTF, lut1d_raw_half_set_false)
{
    // Should throw an exception because the 'raw_halfs' tag
    // was found but set to something other than 'true'.
    const std::string ctfFile("clf/illegal/lut1d_raw_half_set_false.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Illegal 'rawHalfs' attribute");
}

OCIO_ADD_TEST(FileFormatCTF, lut1d_half_domain_raw_half_set)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/lut1d_half_domain_raw_half_set.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pLut1d = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pLut1d);

    OCIO_CHECK_ASSERT(pLut1d->isInputHalfDomain());
    OCIO_CHECK_ASSERT(pLut1d->isOutputRawHalfs());
    
    OCIO_CHECK_EQUAL(pLut1d->getArray().getValues()[0],
                     OCIO::ConvertHalfBitsToFloat(44646));
    OCIO_CHECK_EQUAL(pLut1d->getArray().getValues()[3],
                     OCIO::ConvertHalfBitsToFloat(44637));
    OCIO_CHECK_EQUAL(pLut1d->getArray().getValues()[6],
                     OCIO::ConvertHalfBitsToFloat(44634));
    OCIO_CHECK_EQUAL(pLut1d->getArray().getValues()[9],
                     OCIO::ConvertHalfBitsToFloat(44631));
    OCIO_CHECK_EQUAL(pLut1d->getArray().getValues()[12],
                     OCIO::ConvertHalfBitsToFloat(44629));
}

OCIO_ADD_TEST(FileFormatCTF, lut1d_half_domain_missing_values)
{
    const std::string ctfFile("clf/illegal/lut1d_half_domain_missing_values.clf");
    // This should fail with invalid entries exception because the number
    // of entries in the op is not 65536 (required when using half domain).
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "65536 required for halfDomain");
}

OCIO_ADD_TEST(FileFormatCTF, 3by1d_lut)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/xyz_to_rgb.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 3);
    auto pMatrix = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pMatrix);

    const OCIO::ArrayDouble & a1 = pMatrix->getArray();
    OCIO_CHECK_EQUAL(a1.getLength(), 4);
    OCIO_CHECK_EQUAL(a1.getNumColorComponents(), 4);
    OCIO_CHECK_EQUAL(a1.getNumValues(), a1.getLength()*a1.getLength());

    OCIO_REQUIRE_EQUAL(a1.getValues().size(), a1.getNumValues());
    OCIO_CHECK_EQUAL(a1.getValues()[0],  3.24);
    OCIO_CHECK_EQUAL(a1.getValues()[1], -1.537);
    OCIO_CHECK_EQUAL(a1.getValues()[2], -0.49850);
    OCIO_CHECK_EQUAL(a1.getValues()[3],  0.0);

    OCIO_CHECK_EQUAL(a1.getValues()[4], -0.96930);
    OCIO_CHECK_EQUAL(a1.getValues()[5],  1.876);
    OCIO_CHECK_EQUAL(a1.getValues()[6],  0.04156);
    OCIO_CHECK_EQUAL(a1.getValues()[7],  0.0);

    OCIO_CHECK_EQUAL(a1.getValues()[8],  0.05560);
    OCIO_CHECK_EQUAL(a1.getValues()[9], -0.204);
    OCIO_CHECK_EQUAL(a1.getValues()[10], 1.0573);
    OCIO_CHECK_EQUAL(a1.getValues()[11], 0.0);

    OCIO_CHECK_EQUAL(a1.getValues()[12], 0.0);
    OCIO_CHECK_EQUAL(a1.getValues()[13], 0.0);
    OCIO_CHECK_EQUAL(a1.getValues()[14], 0.0);
    OCIO_CHECK_EQUAL(a1.getValues()[15], 1.0);

    auto pRng =
        std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[1]);
    OCIO_REQUIRE_ASSERT(pRng);

    auto pLut =
        std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[2]);
    OCIO_REQUIRE_ASSERT(pLut);
    OCIO_CHECK_EQUAL(pLut->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(pLut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    const OCIO::Array & a2 = pLut->getArray();
    OCIO_CHECK_EQUAL(a2.getLength(), 128);
    OCIO_CHECK_EQUAL(a2.getNumColorComponents(), 3);
    OCIO_CHECK_EQUAL(a2.getNumValues(),
                     a2.getLength()*pLut->getArray().getMaxColorComponents());

    OCIO_REQUIRE_EQUAL(a2.getValues().size(), a2.getNumValues());
    OCIO_CHECK_EQUAL(a2.getValues()[0], 0.0f);
    OCIO_CHECK_EQUAL(a2.getValues()[1], 0.0f);
    OCIO_CHECK_EQUAL(a2.getValues()[2], 0.0f);
    OCIO_CHECK_EQUAL(a2.getValues()[3], 0.06780f);

    OCIO_CHECK_EQUAL(a2.getValues()[21], 0.19986f);
    OCIO_CHECK_EQUAL(a2.getValues()[22], 0.18986f);
    OCIO_CHECK_EQUAL(a2.getValues()[23], 0.17987f);

    OCIO_CHECK_EQUAL(a2.getValues()[48], 0.31636f);
    OCIO_CHECK_EQUAL(a2.getValues()[49], 0.30054f);
    OCIO_CHECK_EQUAL(a2.getValues()[50], 0.28472f);
}

OCIO_ADD_TEST(FileFormatCTF, lut1d_long_lut)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/lut1d_long.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pLut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pLut);

    const OCIO::Array & a = pLut->getArray();
    OCIO_CHECK_EQUAL(a.getLength(), 131072);
    OCIO_CHECK_EQUAL(a.getNumColorComponents(), 1);
    OCIO_CHECK_EQUAL(a.getNumValues(),
                     a.getLength()*pLut->getArray().getMaxColorComponents());
    OCIO_CHECK_EQUAL(a.getValues()[393215], 1.293f);
}

OCIO_ADD_TEST(FileFormatCTF, lut1d_inv)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("lut1d_inv.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 2);

    auto pMatrix = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pMatrix);

    const OCIO::ArrayDouble & a1 = pMatrix->getArray();
    OCIO_CHECK_EQUAL(a1.getLength(), 4);
    OCIO_CHECK_EQUAL(a1.getNumColorComponents(), 4);
    OCIO_CHECK_EQUAL(a1.getNumValues(), a1.getLength()*a1.getLength());

    OCIO_REQUIRE_EQUAL(a1.getValues().size(), a1.getNumValues());
    OCIO_CHECK_EQUAL(a1.getValues()[0],  3.24);
    OCIO_CHECK_EQUAL(a1.getValues()[1], -1.537);
    OCIO_CHECK_EQUAL(a1.getValues()[2], -0.49850);
    OCIO_CHECK_EQUAL(a1.getValues()[3],  0.0);

    OCIO_CHECK_EQUAL(a1.getValues()[4], -0.96930);
    OCIO_CHECK_EQUAL(a1.getValues()[5],  1.876);
    OCIO_CHECK_EQUAL(a1.getValues()[6],  0.04156);
    OCIO_CHECK_EQUAL(a1.getValues()[7],  0.0);

    OCIO_CHECK_EQUAL(a1.getValues()[8],  0.05560);
    OCIO_CHECK_EQUAL(a1.getValues()[9], -0.204);
    OCIO_CHECK_EQUAL(a1.getValues()[10], 1.0573);
    OCIO_CHECK_EQUAL(a1.getValues()[11], 0.0);

    OCIO_CHECK_EQUAL(a1.getValues()[12], 0.0);
    OCIO_CHECK_EQUAL(a1.getValues()[13], 0.0);
    OCIO_CHECK_EQUAL(a1.getValues()[14], 0.0);
    OCIO_CHECK_EQUAL(a1.getValues()[15], 1.0);

    auto pLut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[1]);
    OCIO_REQUIRE_ASSERT(pLut);
    OCIO_CHECK_EQUAL(pLut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(pLut->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    const OCIO::Array & a2 = pLut->getArray();
    OCIO_CHECK_EQUAL(a2.getNumColorComponents(), 3);

    OCIO_CHECK_EQUAL(a2.getLength(), 17);
    OCIO_CHECK_EQUAL(a2.getNumValues(),
                     a2.getLength()*a2.getMaxColorComponents());

    const float error = 1e-6f;
    OCIO_REQUIRE_EQUAL(a2.getValues().size(), a2.getNumValues());

    OCIO_CHECK_CLOSE(a2.getValues()[0], 0.0f, error);
    OCIO_CHECK_CLOSE(a2.getValues()[1], 0.0f, error);
    OCIO_CHECK_CLOSE(a2.getValues()[2], 0.0f, error);
    OCIO_CHECK_CLOSE(a2.getValues()[3], 0.28358f, error);

    OCIO_CHECK_CLOSE(a2.getValues()[21], 0.68677f, error);
    OCIO_CHECK_CLOSE(a2.getValues()[22], 0.68677f, error);
    OCIO_CHECK_CLOSE(a2.getValues()[23], 0.68677f, error);

    OCIO_CHECK_CLOSE(a2.getValues()[48], 1.0f, error);
    OCIO_CHECK_CLOSE(a2.getValues()[49], 1.0f, error);
    OCIO_CHECK_CLOSE(a2.getValues()[50], 1.0f, error);
}

OCIO_ADD_TEST(FileFormatCTF, lut1d_inv_scaling)
{
    // Validate that the InverseLUT1D array values are scaled based on inBitDepth.
    // (The previous example had inBitDepth=32f, so it does not validate that.)

    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("lut1d_inverse_halfdom_slog_fclut.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);

    auto pLut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pLut);
    // For an InverseLUT1D, the file "out" depth is actually taken from inBitDepth.
    OCIO_CHECK_EQUAL(pLut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT16);
    OCIO_CHECK_EQUAL(pLut->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    const OCIO::Array & a2 = pLut->getArray();
    OCIO_CHECK_EQUAL(a2.getNumColorComponents(), 1);

    OCIO_CHECK_EQUAL(a2.getLength(), 65536);
    OCIO_CHECK_EQUAL(a2.getNumValues(),
                     a2.getLength()*a2.getMaxColorComponents());

    const float error = 1e-6f;
    OCIO_REQUIRE_EQUAL(a2.getValues().size(), a2.getNumValues());

    // Input value 17830 scaled by 65535.
    OCIO_CHECK_CLOSE(a2.getValues()[0], 0.27206836f, error);
    // Input value 55070 scaled by 65535.
    OCIO_CHECK_CLOSE(a2.getValues()[31743 * 3], 0.84031434f, error);
}

namespace
{
OCIO::LocalCachedFileRcPtr ParseString(const std::string & str)
{
    std::istringstream ctf;
    ctf.str(str);

    // Parse stream.
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr file = tester.read(ctf, emptyString, OCIO::INTERP_DEFAULT);

    return OCIO_DYNAMIC_POINTER_CAST<OCIO::LocalCachedFile>(file);
}
}

OCIO_ADD_TEST(FileFormatCTF, invlut1d_clf)
{
    const std::string clf{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="UIDLUT42">
    <InverseLUT1D id="lut01" name="test-lut" inBitDepth="32f" outBitDepth="10i">
        <Array dim="16 3">
   0    1    2
   3    4    5
   6    7    8
   9   10   11
  12   13   14
  15   16   17
  18   19   20
  21   22   23
  24   25   26
  27   28   29
  30   31   32
  33   34   35
  36   37   38
  39   40   41
  42   43   44
  45   46   47
        </Array>
    </InverseLUT1D>
</ProcessList>
)" };

    OCIO_CHECK_THROW_WHAT(ParseString(clf), OCIO::Exception,
                          "CLF file version '3' does not support operator 'InverseLUT1D'");
}

OCIO_ADD_TEST(FileFormatCTF, lut3d)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/lut3d_17x17x17_10i_12i.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);

    auto pLut = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pLut);
    OCIO_CHECK_EQUAL(pLut->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(pLut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
    // Interpolation is not defined in the file.
    OCIO_CHECK_EQUAL(pLut->getInterpolation(), OCIO::INTERP_DEFAULT);

    const OCIO::Array & array = pLut->getArray();
    OCIO_CHECK_EQUAL(array.getLength(), 17);
    OCIO_CHECK_EQUAL(array.getNumColorComponents(), 3);
    OCIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength()
                     *array.getLength()
                     *pLut->getArray().getMaxColorComponents());

    OCIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
    const float tol = 2e-8f;
    OCIO_CHECK_CLOSE(array.getValues()[0],   0.0f / 4095.0f, tol);
    OCIO_CHECK_CLOSE(array.getValues()[1],  12.0f / 4095.0f, tol);
    OCIO_CHECK_CLOSE(array.getValues()[2],  13.0f / 4095.0f, tol);

    OCIO_CHECK_CLOSE(array.getValues()[18],   0.0f / 4095.0f, tol);
    OCIO_CHECK_CLOSE(array.getValues()[19], 203.0f / 4095.0f, tol);
    OCIO_CHECK_CLOSE(array.getValues()[20], 399.0f / 4095.0f, tol);

    OCIO_CHECK_CLOSE(array.getValues()[30],  54.0f / 4095.0f, tol);
    OCIO_CHECK_CLOSE(array.getValues()[31], 490.0f / 4095.0f, tol);
    OCIO_CHECK_CLOSE(array.getValues()[32], 987.0f / 4095.0f, tol);
}

OCIO_ADD_TEST(FileFormatCTF, lut3d_inv)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("lut3d_example_Inv.ctf");

    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);

    auto pLut = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pLut);

    // For an InverseLUT3D, the file "out" depth is set by the inBitDepth of the file.
    OCIO_CHECK_EQUAL(pLut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    OCIO_CHECK_EQUAL(pLut->getInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OCIO_CHECK_EQUAL(pLut->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    const OCIO::Array & array = pLut->getArray();
    OCIO_CHECK_EQUAL(array.getNumColorComponents(), 3);
    OCIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength()
                     *array.getLength()*array.getMaxColorComponents());
    OCIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());

    // Validate that the array was scaled by the inBitDepth of the file.
    OCIO_CHECK_EQUAL(array.getLength(), 17);
    OCIO_CHECK_CLOSE(array.getValues()[0], 25.0f / 4095.0f, 1e-8f);
    OCIO_CHECK_CLOSE(array.getValues()[1], 30.0f / 4095.0f, 1e-8f);
    OCIO_CHECK_EQUAL(array.getValues()[2], 33.0f / 4095.0f);

    OCIO_CHECK_CLOSE(array.getValues()[18], 26.0f / 4095.0f, 1e-8f);
    OCIO_CHECK_EQUAL(array.getValues()[19], 308.0f / 4095.0f);
    OCIO_CHECK_EQUAL(array.getValues()[20], 580.0f / 4095.0f);

    OCIO_CHECK_EQUAL(array.getValues()[30], 0.0f);
    OCIO_CHECK_EQUAL(array.getValues()[31], 586.0f / 4095.0f);
    OCIO_CHECK_EQUAL(array.getValues()[32], 1350.0f / 4095.0f);
}

OCIO_ADD_TEST(FileFormatCTF, lut3d_unequal_size)
{
    std::string fileName("clf/illegal/lut3d_unequal_size.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(fileName), OCIO::Exception,
                          "Illegal array dimensions 2 2 3 3");
}

OCIO_ADD_TEST(FileFormatCTF, tabluation_support)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    // This clf file contains tabulations used as delimiters for a
    // series of numbers.
    const std::string ctfFile("clf/tabulation_support.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_CHECK_EQUAL(cachedFile->m_transform->getID(), "e0a0ae4b-adc2-4c25-ad70-fa6f31ba219d");
    OCIO_REQUIRE_EQUAL(opList.size(), 1);

    auto pL = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pL);

    OCIO_CHECK_EQUAL(pL->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OCIO_CHECK_EQUAL(pL->getInterpolation(), OCIO::INTERP_TETRAHEDRAL);

    const OCIO::Array & array = pL->getArray();
    OCIO_CHECK_EQUAL(array.getLength(), 3U);
    OCIO_CHECK_EQUAL(array.getNumColorComponents(), 3U);
    OCIO_CHECK_EQUAL(array.getNumValues(), 81U);
    OCIO_REQUIRE_EQUAL(array.getValues().size(), 81U);

    const float scale = (float)OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT10);
    OCIO_CHECK_EQUAL(array.getValues()[0] * scale,-60.0f);
    OCIO_CHECK_EQUAL(array.getValues()[1] * scale,  5.0f);
    OCIO_CHECK_EQUAL(array.getValues()[2] * scale, 75.0f);

    OCIO_CHECK_EQUAL(array.getValues()[3] * scale, -10.0f);
    OCIO_CHECK_CLOSE(array.getValues()[4] * scale,  50.0f, 1e-5f);
    OCIO_CHECK_CLOSE(array.getValues()[5] * scale, 400.0f, 1e-4f);

    OCIO_CHECK_EQUAL(array.getValues()[6] * scale,    0.0f);
    OCIO_CHECK_CLOSE(array.getValues()[7] * scale,  100.0f, 1e-4f);
    OCIO_CHECK_EQUAL(array.getValues()[8] * scale, 1200.0f);

    OCIO_CHECK_EQUAL(array.getValues()[9]  * scale, -40.0f);
    OCIO_CHECK_EQUAL(array.getValues()[10] * scale, 500.0f);
    OCIO_CHECK_EQUAL(array.getValues()[11] * scale, -30.0f);

    OCIO_CHECK_EQUAL(array.getValues()[3 * 26 + 0] * scale, 1110.0f);
    OCIO_CHECK_EQUAL(array.getValues()[3 * 26 + 1] * scale,  900.0f);
    OCIO_CHECK_EQUAL(array.getValues()[3 * 26 + 2] * scale, 1200.0f);
}

OCIO_ADD_TEST(FileFormatCTF, matrix_windows_eol)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    // This file uses windows end of line character and does not start
    // with the ?xml header.
    const std::string ctfFile("clf/matrix_windows.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_CHECK_EQUAL(cachedFile->m_transform->getID(), "42");
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    OCIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::MatrixType);
    OCIO_CHECK_EQUAL(opList[0]->getID(), "");
    OCIO_CHECK_EQUAL(opList[0]->getName(), "identity matrix");
}

OCIO_ADD_TEST(FileFormatCTF, matrix_no_newlines)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/matrix_no_newlines.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    OCIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::MatrixType);
    auto pMatrix = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pMatrix);
    const OCIO::ArrayDouble & array = pMatrix->getArray();

    const float scale = 4095.f / 1023.f;
    OCIO_CHECK_CLOSE(array.getValues()[0] * scale, 3.6f, 1e-6f);
    OCIO_CHECK_CLOSE(array.getValues()[1] * scale, 0.1f, 1e-6f);
    OCIO_CHECK_CLOSE(array.getValues()[2] * scale,-0.2f, 1e-6f);
    OCIO_CHECK_CLOSE(array.getValues()[3] * scale, 0.0f, 1e-6f);
    OCIO_CHECK_CLOSE(array.getValues()[4] * scale, 0.2f, 1e-6f);
    OCIO_CHECK_CLOSE(array.getValues()[5] * scale, 3.5f, 1e-6f);
    OCIO_CHECK_CLOSE(array.getValues()[6] * scale, 0.1f, 1e-6f);
    OCIO_CHECK_CLOSE(array.getValues()[7] * scale, 0.0f, 1e-6f);
    OCIO_CHECK_CLOSE(array.getValues()[8] * scale, 0.1f, 1e-6f);
    OCIO_CHECK_CLOSE(array.getValues()[9] * scale,-0.3f, 1e-6f);
    OCIO_CHECK_CLOSE(array.getValues()[10] * scale,3.4f, 1e-6f);
    OCIO_CHECK_CLOSE(array.getValues()[11] * scale,0.0f, 1e-6f);

    const OCIO::MatrixOpData::Offsets & offsets = pMatrix->getOffsets();
    const float oscale = 4095.f;
    OCIO_CHECK_CLOSE(offsets[0] * oscale, 0.3f, 1e-6f);
    OCIO_CHECK_CLOSE(offsets[1] * oscale,-0.05f,1e-6f);
    OCIO_CHECK_CLOSE(offsets[2] * oscale,-0.4f, 1e-6f);
    OCIO_CHECK_CLOSE(offsets[3] * oscale, 0.0f, 1e-6f);
}

OCIO_ADD_TEST(FileFormatCTF, check_utf8)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/matrix_example_utf8.clf");

    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    StringUtils::StringVec descList;
    GetElementsValues(opList[0]->getFormatMetadata().getChildrenElements(),
                      OCIO::TAG_DESCRIPTION, descList);
    OCIO_REQUIRE_EQUAL(descList.size(), 1);
    const std::string & desc = descList[0];
    const std::string utf8Test
        ("\xE6\xA8\x99\xE6\xBA\x96\xE8\x90\xAC\xE5\x9C\x8B\xE7\xA2\xBC");
    OCIO_CHECK_EQUAL(desc, utf8Test);
    const std::string utf8TestWrong
        ("\xE5\xA8\x99\xE6\xBA\x96\xE8\x90\xAC\xE5\x9C\x8B\xE7\xA2\xBC");
    OCIO_CHECK_NE(desc, utf8TestWrong);

}

OCIO_ADD_TEST(FileFormatCTF, info_example)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/info_example.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    OCIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().size(), 2);
    OCIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[0],
                     "Example of using the Info element");
    OCIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[1],
                     "A second description");
    OCIO_CHECK_EQUAL(cachedFile->m_transform->getInputDescriptor(),
                     "input desc");
    OCIO_CHECK_EQUAL(cachedFile->m_transform->getOutputDescriptor(),
                     "output desc");

    // Ensure ops were not affected by metadata parsing.
    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);

    auto pMatrix =
        std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pMatrix);
    OCIO_CHECK_EQUAL(pMatrix->getName(), "identity");

    OCIO_CHECK_EQUAL(pMatrix->getFileInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(pMatrix->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    const OCIO::FormatMetadataImpl & info = cachedFile->m_transform->getInfoMetadata();

    // Check element values.
    //
    OCIO_CHECK_EQUAL(std::string(info.getElementName()), OCIO::METADATA_INFO);
    auto items = info.getChildrenElements();
    OCIO_REQUIRE_EQUAL(items.size(), 6);
    OCIO_CHECK_EQUAL(std::string(items[0].getElementName()), "Copyright");
    OCIO_CHECK_EQUAL(std::string(items[0].getElementValue()),
                     "Copyright Contributors to the OpenColorIO Project.");
    OCIO_CHECK_EQUAL(std::string(items[1].getElementName()), "AppRelease");
    OCIO_CHECK_EQUAL(std::string(items[1].getElementValue()), "2020.0.63");
    OCIO_CHECK_EQUAL(std::string(items[2].getElementName()), "Revision");
    OCIO_CHECK_EQUAL(std::string(items[2].getElementValue()), "1");

    OCIO_CHECK_EQUAL(std::string(items[3].getElementName()), "Category");
    OCIO_CHECK_EQUAL(std::string(items[3].getElementValue()), "");
    auto catItems = items[3].getChildrenElements();
    OCIO_REQUIRE_EQUAL(catItems.size(), 1);
    OCIO_CHECK_EQUAL(std::string(catItems[0].getElementName()), "Tags");
    auto tagsItems = catItems[0].getChildrenElements();
    OCIO_REQUIRE_EQUAL(tagsItems.size(), 2);
    OCIO_CHECK_EQUAL(std::string(tagsItems[0].getElementName()), "SceneLinearWorkingSpace");
    OCIO_CHECK_EQUAL(std::string(tagsItems[0].getElementValue()), "");
    OCIO_CHECK_EQUAL(std::string(tagsItems[1].getElementName()), "Input");
    OCIO_CHECK_EQUAL(std::string(tagsItems[1].getElementValue()), "");

    OCIO_CHECK_EQUAL(std::string(items[4].getElementName()), "InputColorSpace");
    OCIO_CHECK_EQUAL(std::string(items[4].getElementValue()), "");
    auto icItems = items[4].getChildrenElements();
    OCIO_REQUIRE_EQUAL(icItems.size(), 4);
    OCIO_CHECK_EQUAL(std::string(icItems[0].getElementName()), OCIO::METADATA_DESCRIPTION);
    OCIO_CHECK_EQUAL(std::string(icItems[0].getElementValue()), "Input color space description");
    OCIO_CHECK_EQUAL(std::string(icItems[1].getElementName()), "ImageState");
    OCIO_CHECK_EQUAL(std::string(icItems[1].getElementValue()), "video");
    OCIO_CHECK_EQUAL(std::string(icItems[2].getElementName()), "ShortName");
    OCIO_CHECK_EQUAL(std::string(icItems[2].getElementValue()), "no_version");
    OCIO_CHECK_EQUAL(std::string(icItems[3].getElementName()), "ID");
    OCIO_CHECK_EQUAL(std::string(icItems[3].getElementValue()),
                     "387b23d1-f1ce-3f69-8544-e5601f45f78b");

    OCIO_CHECK_EQUAL(std::string(items[5].getElementName()), "OutputColorSpace");
    OCIO_CHECK_EQUAL(std::string(items[5].getElementValue()), "");
    auto ocItems = items[5].getChildrenElements();
    OCIO_REQUIRE_EQUAL(ocItems.size(), 3);
    auto attribs = items[5].getAttributes();
    OCIO_REQUIRE_EQUAL(attribs.size(), 2);
    OCIO_CHECK_EQUAL(attribs[0].first, "att1");
    OCIO_CHECK_EQUAL(attribs[0].second, "test1");
    OCIO_CHECK_EQUAL(attribs[1].first, "att2");
    OCIO_CHECK_EQUAL(attribs[1].second, "test2");
    OCIO_CHECK_EQUAL(std::string(ocItems[0].getElementName()), "ImageState");
    OCIO_CHECK_EQUAL(std::string(ocItems[0].getElementValue()), "scene");
    OCIO_CHECK_EQUAL(std::string(ocItems[1].getElementName()), "ShortName");
    OCIO_CHECK_EQUAL(std::string(ocItems[1].getElementValue()), "ACES");
    OCIO_CHECK_EQUAL(std::string(ocItems[2].getElementName()), "ID");
    OCIO_CHECK_EQUAL(std::string(ocItems[2].getElementValue()), "1");
}

OCIO_ADD_TEST(FileFormatCTF, difficult_syntax)
{
    // This file contains a lot of unusual (but still legal) ways of writing the XML.
    // It is intended to stress test that the XML parsing is working robustly.

    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/difficult_syntax.clf");

    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::CTFVersion clfVersion = cachedFile->m_transform->getCLFVersion();
    const OCIO::CTFVersion ver(3, 0, 0);
    OCIO_CHECK_EQUAL(clfVersion, ver);

    OCIO_CHECK_EQUAL(cachedFile->m_transform->getID(), "id1");

    OCIO_REQUIRE_EQUAL(cachedFile->m_transform->getDescriptions().size(), 2);
    OCIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[0],
                     "This is the ProcessList description.");
    OCIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[1],
                     "yet 'another' \"valid\" desc");

    const OCIO::FormatMetadataImpl & info = cachedFile->m_transform->getInfoMetadata();
    OCIO_CHECK_EQUAL(std::string(info.getElementName()), OCIO::METADATA_INFO);
    auto items = info.getChildrenElements();
    OCIO_REQUIRE_EQUAL(items.size(), 1);
    OCIO_CHECK_EQUAL(std::string(items[0].getElementName()), "Stuff");
    OCIO_CHECK_EQUAL(std::string(items[0].getElementValue()),
                     "This is a \"difficult\" but 'legal' color transform file.");

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 2);
    {
        auto pMatrix = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
        OCIO_REQUIRE_ASSERT(pMatrix);

        OCIO_CHECK_EQUAL(pMatrix->getID(), "'mat-25'");
        OCIO_CHECK_EQUAL(pMatrix->getName(), "\"quote\"");

        StringUtils::StringVec desc;
        GetElementsValues(pMatrix->getFormatMetadata().getChildrenElements(),
                          OCIO::TAG_DESCRIPTION, desc);
        OCIO_REQUIRE_EQUAL(desc.size(), 1);
        OCIO_CHECK_EQUAL(desc[0], "third array dim value is ignored");

        const OCIO::ArrayDouble & array = pMatrix->getArray();
        OCIO_CHECK_EQUAL(array.getLength(), 4u);
        OCIO_CHECK_EQUAL(array.getNumColorComponents(), 4u);
        OCIO_CHECK_EQUAL(array.getNumValues(), array.getLength()*array.getLength());

        OCIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
        OCIO_CHECK_EQUAL(array.getValues()[0],  3.24);
        OCIO_CHECK_EQUAL(array.getValues()[1], -1.537);
        OCIO_CHECK_EQUAL(array.getValues()[2], -0.4985);
        OCIO_CHECK_EQUAL(array.getValues()[3],  0.0);

        OCIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
        OCIO_CHECK_EQUAL(array.getValues()[5],  1.876);
        OCIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
        OCIO_CHECK_EQUAL(array.getValues()[7],  0.0);

        OCIO_CHECK_EQUAL(array.getValues()[8],  0.0556);
        OCIO_CHECK_EQUAL(array.getValues()[9], -0.204);
        OCIO_CHECK_EQUAL(array.getValues()[10], 0.105730e+1);
        OCIO_CHECK_EQUAL(array.getValues()[11], 0.0);

        OCIO_CHECK_EQUAL(array.getValues()[12], 0.0);
        OCIO_CHECK_EQUAL(array.getValues()[13], 0.0);
        OCIO_CHECK_EQUAL(array.getValues()[14], 0.0);
        OCIO_CHECK_EQUAL(array.getValues()[15], 1.0);
    }
    {
        auto pLut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[1]);
        OCIO_REQUIRE_ASSERT(pLut);

        OCIO_CHECK_EQUAL(pLut->getName(), "a multi-line  name");

        StringUtils::StringVec desc;
        GetElementsValues(pLut->getFormatMetadata().getChildrenElements(),
                          OCIO::TAG_DESCRIPTION, desc);
        OCIO_REQUIRE_EQUAL(desc.size(), 3);
        OCIO_CHECK_EQUAL(desc[0], "the n–dash description");  // the string here uses opt-dash
        OCIO_CHECK_EQUAL(desc[1], "another valid description element    ");
        OCIO_CHECK_EQUAL(desc[2], "& another <valid> desc");

        const OCIO::Array & array2 = pLut->getArray();
        OCIO_CHECK_EQUAL(array2.getLength(), 128);
        OCIO_CHECK_EQUAL(array2.getNumColorComponents(), 3);
        OCIO_CHECK_EQUAL(array2.getNumValues(),
                         array2.getLength()
                         *pLut->getArray().getMaxColorComponents());

        OCIO_REQUIRE_EQUAL(array2.getValues().size(), 384);
        OCIO_CHECK_EQUAL(array2.getValues()[0], 0.0f);
        OCIO_CHECK_EQUAL(array2.getValues()[1], 0.0f);
        OCIO_CHECK_EQUAL(array2.getValues()[2], 0.0f);
        OCIO_CHECK_EQUAL(array2.getValues()[3], 0.06780f);
        OCIO_CHECK_EQUAL(array2.getValues()[4], 0.06441f);
        OCIO_CHECK_EQUAL(array2.getValues()[5], 0.06102f);
        OCIO_CHECK_EQUAL(array2.getValues()[6], 0.09965f);
        OCIO_CHECK_EQUAL(array2.getValues()[378], 0.99562f);
        OCIO_CHECK_EQUAL(array2.getValues()[379], 0.94584f);
        OCIO_CHECK_EQUAL(array2.getValues()[380], 0.89606f);
    }
}

OCIO_ADD_TEST(FileFormatCTF, difficult_xml_unknown_elements)
{
    OCIO::LocalCachedFileRcPtr cachedFile;

    {
        constexpr char ErrorOutputs[11][1024] = 
        {
            "(10): Unrecognized element 'Ignore' where its parent is 'ProcessList' (8): Unknown element",
            "(22): Unrecognized attribute 'id' of 'Array'",
            "(22): Unrecognized attribute 'foo' of 'Array'",
            "(27): Unrecognized element 'ProcessList' where its parent is 'ProcessList' (8): The Transform already exists",
            "(30): Unrecognized element 'Array' where its parent is 'Matrix' (16): Only one Array allowed per op",
            "(37): Unrecognized element 'just_ignore' where its parent is 'ProcessList' (8): Unknown element",
            "(69): Unrecognized element 'just_ignore' where its parent is 'Description' (66)",
            "(70): Unrecognized element 'just_ignore' where its parent is 'just_ignore' (69)",
            "(75): Unrecognized element 'Matrix' where its parent is 'LUT1D' (", // Line number is expat library version dependent.
            "(76): Unrecognized element 'Description' where its parent is 'Matrix' (75)",
            "(77): Unrecognized element 'Array' where its parent is 'Matrix' (75)"
        };

        OCIO::LogGuard guard;
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_WARNING);

        const std::string ctfFile("difficult_test1_v1.ctf");
        OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
        OCIO_REQUIRE_ASSERT((bool)cachedFile);

        const StringUtils::StringVec parts = StringUtils::SplitByLines(StringUtils::RightTrim(guard.output()));
        OCIO_REQUIRE_EQUAL(parts.size(), 11);

        for (size_t i = 0; i < parts.size(); ++i)
        {
            OCIO_CHECK_NE(std::string::npos, StringUtils::Find(parts[i], ErrorOutputs[i]));
        }
    }

    // Defaults to 1.2
    const OCIO::CTFVersion ctfVersion = cachedFile->m_transform->getCTFVersion();
    OCIO_CHECK_ASSERT(OCIO::CTF_PROCESS_LIST_VERSION_1_2 == ctfVersion);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 2);

    auto pMatrix = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pMatrix);

    const OCIO::ArrayDouble & array = pMatrix->getArray();
    OCIO_CHECK_EQUAL(array.getLength(), 4u);
    OCIO_CHECK_EQUAL(array.getNumColorComponents(), 4u);
    OCIO_CHECK_EQUAL(array.getNumValues(), array.getLength()*array.getLength());

    OCIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
    OCIO_CHECK_EQUAL(array.getValues()[0],  3.24);
    OCIO_CHECK_EQUAL(array.getValues()[1], -1.537);
    OCIO_CHECK_EQUAL(array.getValues()[2], -0.4985);
    OCIO_CHECK_EQUAL(array.getValues()[3],  0.0);

    OCIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
    OCIO_CHECK_EQUAL(array.getValues()[5],  1.876);
    OCIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
    OCIO_CHECK_EQUAL(array.getValues()[7],  0.0);

    OCIO_CHECK_EQUAL(array.getValues()[8],  0.0556);
    OCIO_CHECK_EQUAL(array.getValues()[9], -0.204);
    OCIO_CHECK_EQUAL(array.getValues()[10], 0.105730e+1);
    OCIO_CHECK_EQUAL(array.getValues()[11], 0.0);

    OCIO_CHECK_EQUAL(array.getValues()[12], 0.0);
    OCIO_CHECK_EQUAL(array.getValues()[13], 0.0);
    OCIO_CHECK_EQUAL(array.getValues()[14], 0.0);
    OCIO_CHECK_EQUAL(array.getValues()[15], 1.0);

    auto pLut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[1]);
    OCIO_REQUIRE_ASSERT(pLut);

    const OCIO::Array & array2 = pLut->getArray();
    OCIO_CHECK_EQUAL(array2.getLength(), 17);
    OCIO_CHECK_EQUAL(array2.getNumColorComponents(), 3);
    OCIO_CHECK_EQUAL(array2.getNumValues(),
                     array2.getLength()
                     *pLut->getArray().getMaxColorComponents());

    OCIO_REQUIRE_EQUAL(array2.getValues().size(), 51);
    OCIO_CHECK_EQUAL(array2.getValues()[0], 0.0f);
    OCIO_CHECK_EQUAL(array2.getValues()[1], 0.0f);
    OCIO_CHECK_EQUAL(array2.getValues()[2], 0.0f);
    OCIO_CHECK_EQUAL(array2.getValues()[3], 0.28358f);
    OCIO_CHECK_EQUAL(array2.getValues()[4], 0.28358f);
    OCIO_CHECK_EQUAL(array2.getValues()[5], 0.28358f);
    OCIO_CHECK_EQUAL(array2.getValues()[6], 0.38860f);
    OCIO_CHECK_EQUAL(array2.getValues()[45], 0.97109f);
    OCIO_CHECK_EQUAL(array2.getValues()[46], 0.97109f);
    OCIO_CHECK_EQUAL(array2.getValues()[47], 0.97109f);
}

OCIO_ADD_TEST(FileFormatCTF, unknown_elements)
{
    OCIO::LocalCachedFileRcPtr cachedFile;

    {
        constexpr static const char ErrorOutputs[3][1024] = 
        {
            "(34): Unrecognized element 'B' where its parent is 'ProcessList' (2): Unknown element",
            "(34): Unrecognized element 'C' where its parent is 'B' (34)",
            "(36): Unrecognized element 'A' where its parent is 'Description' (36)"
        };

        OCIO::LogGuard guard;
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_WARNING);

        // NB: This file has some added unknown elements A, B, and C as a test.
        const std::string ctfFile("clf/illegal/unknown_elements.clf");
        OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
        OCIO_REQUIRE_ASSERT((bool)cachedFile);

        const StringUtils::StringVec parts = StringUtils::SplitByLines(StringUtils::RightTrim(guard.output()));
        OCIO_REQUIRE_EQUAL(parts.size(), 3);

        for (size_t i = 0; i < parts.size(); ++i)
        {
            OCIO_CHECK_NE(std::string::npos, StringUtils::Find(parts[i], ErrorOutputs[i]));
        }
    }

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 4);

    auto pMatrix = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pMatrix);

    const OCIO::ArrayDouble & a1 = pMatrix->getArray();
    OCIO_CHECK_EQUAL(a1.getLength(), 4);
    OCIO_CHECK_EQUAL(a1.getNumColorComponents(), 4);
    OCIO_CHECK_EQUAL(a1.getNumValues(), a1.getLength()*a1.getLength());

    OCIO_REQUIRE_EQUAL(a1.getValues().size(), a1.getNumValues());
    OCIO_CHECK_EQUAL(a1.getValues()[0],  3.24);
    OCIO_CHECK_EQUAL(a1.getValues()[4], -0.96930);
    OCIO_CHECK_EQUAL(a1.getValues()[10], 1.0573);

    auto pLut1 = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[1]);
    OCIO_REQUIRE_ASSERT(pLut1);

    const OCIO::Array & a2 = pLut1->getArray();
    OCIO_CHECK_EQUAL(a2.getLength(), 17);
    OCIO_CHECK_EQUAL(a2.getNumColorComponents(), 3);
    OCIO_CHECK_EQUAL(a2.getNumValues(), 
                     a2.getLength()
                     *pLut1->getArray().getMaxColorComponents());

    OCIO_REQUIRE_EQUAL(a2.getValues().size(), a2.getNumValues());
    OCIO_CHECK_EQUAL(a2.getValues()[3], 0.28358f);
    OCIO_CHECK_EQUAL(a2.getValues()[4], 0.28358f);
    OCIO_CHECK_EQUAL(a2.getValues()[5], 100.0f);
    OCIO_CHECK_EQUAL(a2.getValues()[50], 1.0f);

    auto pLut2 = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[2]);
    OCIO_REQUIRE_ASSERT(pLut2);
    OCIO_CHECK_EQUAL(pLut2->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    const OCIO::Array & array = pLut2->getArray();
    OCIO_CHECK_EQUAL(array.getLength(), 32);
    OCIO_CHECK_EQUAL(array.getNumColorComponents(), 1);
    OCIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()
                     *pLut2->getArray().getMaxColorComponents());

    OCIO_REQUIRE_EQUAL(array.getValues().size(), 96);
    OCIO_CHECK_EQUAL(array.getValues()[0], 0.0f);
    OCIO_CHECK_EQUAL(array.getValues()[1], 0.0f);
    OCIO_CHECK_EQUAL(array.getValues()[2], 0.0f);
    OCIO_CHECK_EQUAL(array.getValues()[3], 215.0f / 1023.0f);
    OCIO_CHECK_EQUAL(array.getValues()[4], 215.0f / 1023.0f);
    OCIO_CHECK_EQUAL(array.getValues()[5], 215.0f / 1023.0f);
    OCIO_CHECK_EQUAL(array.getValues()[6], 294.0f / 1023.0f);
    // and many more
    OCIO_CHECK_EQUAL(array.getValues()[92], 1008.0f / 1023.0f);
    OCIO_CHECK_EQUAL(array.getValues()[93], 1023.0f / 1023.0f);
    OCIO_CHECK_EQUAL(array.getValues()[94], 1023.0f / 1023.0f);
    OCIO_CHECK_EQUAL(array.getValues()[95], 1023.0f / 1023.0f);

    auto pLut3 = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opList[3]);
    OCIO_REQUIRE_ASSERT(pLut3);
    OCIO_CHECK_EQUAL(pLut3->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    const OCIO::Array & a3 = pLut3->getArray();
    OCIO_CHECK_EQUAL(a3.getLength(), 3);
    OCIO_CHECK_EQUAL(a3.getNumColorComponents(), 3);
    OCIO_CHECK_EQUAL(a3.getNumValues(),
                     a3.getLength()*a3.getLength()*a3.getLength()
                     *pLut3->getArray().getMaxColorComponents());

    OCIO_REQUIRE_EQUAL(a3.getValues().size(), a3.getNumValues());
    OCIO_CHECK_EQUAL(a3.getValues()[0], 0.0f);
    OCIO_CHECK_EQUAL(a3.getValues()[1], 30.0f / 1023.0f);
    OCIO_CHECK_EQUAL(a3.getValues()[2], 33.0f / 1023.0f);
    OCIO_CHECK_EQUAL(a3.getValues()[3], 0.0f);
    OCIO_CHECK_EQUAL(a3.getValues()[4], 0.0f);
    OCIO_CHECK_EQUAL(a3.getValues()[5], 133.0f / 1023.0f);

    OCIO_CHECK_EQUAL(a3.getValues()[78], 1023.0f / 1023.0f);
    OCIO_CHECK_EQUAL(a3.getValues()[79], 1023.0f / 1023.0f);
    OCIO_CHECK_EQUAL(a3.getValues()[80], 1023.0f / 1023.0f);
}

OCIO_ADD_TEST(FileFormatCTF, wrong_format)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("logtolin_8to8.lut");
        OCIO_CHECK_THROW_WHAT(cachedFile = LoadCLFFile(ctfFile),
                              OCIO::Exception,
                              "not a CTF/CLF file.");
        OCIO_CHECK_ASSERT(!(bool)cachedFile);
    }
}

OCIO_ADD_TEST(FileFormatCTF, binary_file)
{
    const std::string ctfFile("clf/illegal/image_png.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, "is not a CTF/CLF file.");
}

OCIO_ADD_TEST(FileFormatCTF, process_list_invalid_version)
{
    const std::string ctfFile("process_list_invalid_version.ctf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "is not a valid version");
}

OCIO_ADD_TEST(FileFormatCTF, clf_process_list_bad_version)
{
    std::string fileName("clf/illegal/process_list_bad_version.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(fileName), 
                          OCIO::Exception,
                          "is not a valid version");
}

OCIO_ADD_TEST(FileFormatCTF, process_list_valid_version)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("process_list_valid_version.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    
    const OCIO::CTFVersion ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OCIO_CHECK_EQUAL(ctfVersion, OCIO::CTF_PROCESS_LIST_VERSION_1_4);
}

OCIO_ADD_TEST(FileFormatCTF, process_list_higher_version)
{
    const std::string ctfFile("process_list_higher_version.ctf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Unsupported transform file version");
}

OCIO_ADD_TEST(FileFormatCTF, clf_process_list_higher_version)
{
    const std::string ctfFile("clf/illegal/process_list_higher_version.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Unsupported transform file version");
}

OCIO_ADD_TEST(FileFormatCTF, process_list_version_revision)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("process_list_version_revision.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));

    const OCIO::CTFVersion ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    const OCIO::CTFVersion ver(1, 3, 10);
    OCIO_CHECK_EQUAL(ctfVersion, ver);
    OCIO_CHECK_ASSERT(OCIO::CTF_PROCESS_LIST_VERSION_1_3 < ctfVersion);
    OCIO_CHECK_ASSERT(ctfVersion < OCIO::CTF_PROCESS_LIST_VERSION_1_4);
}

OCIO_ADD_TEST(FileFormatCTF, process_list_no_version)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("process_list_no_version.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));

    const OCIO::CTFVersion ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OCIO_CHECK_EQUAL(ctfVersion, OCIO::CTF_PROCESS_LIST_VERSION_1_2);
}

OCIO_ADD_TEST(FileFormatCTF, info_element_version_test)
{
    // VALID - No Version.
    {
        const std::string ctfFile("info_version_without.ctf");
        OCIO_CHECK_NO_THROW(LoadCLFFile(ctfFile));
    }
    // VALID - Minor Version.
    {
        const std::string ctfFile("info_version_valid_minor.ctf");
        OCIO_CHECK_NO_THROW(LoadCLFFile(ctfFile));
    }
    // INVALID - Invalid Version.
    {
        const std::string ctfFile("info_version_invalid.ctf");
        OCIO_CHECK_THROW_WHAT(
            LoadCLFFile(ctfFile), OCIO::Exception,
                        "Invalid Info element version attribute");
    }
    // INVALID - Unsupported Version.
    {
        const std::string ctfFile("info_version_unsupported.ctf");
        OCIO_CHECK_THROW_WHAT(
            LoadCLFFile(ctfFile), OCIO::Exception,
            "Unsupported Info element version attribute");
    }
    // INVALID - Empty Version.
    {
        const std::string ctfFile("info_version_empty.ctf");
        OCIO_CHECK_THROW_WHAT(
            LoadCLFFile(ctfFile), OCIO::Exception,
            "Invalid Info element version attribute");
    }
}

OCIO_ADD_TEST(FileFormatCTF, process_list_missing)
{
    const std::string ctfFile("clf/illegal/process_list_missing.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "is not a CTF/CLF file.");
}

OCIO_ADD_TEST(FileFormatCTF, transform_missing)
{
    const std::string ctfFile("clf/illegal/transform_missing.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "is not a CTF/CLF file.");
}

OCIO_ADD_TEST(FileFormatCTF, transform_element_end_missing)
{
    const std::string ctfFile("clf/illegal/transform_element_end_missing.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, "no element found");
}

OCIO_ADD_TEST(FileFormatCTF, transform_missing_id)
{
    const std::string ctfFile("clf/illegal/transform_missing_id.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Required attribute 'id'");
}

OCIO_ADD_TEST(FileFormatCTF, transform_missing_inbitdepth)
{
    const std::string ctfFile("clf/illegal/transform_missing_inbitdepth.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "inBitDepth is missing");
}

OCIO_ADD_TEST(FileFormatCTF, transform_missing_outbitdepth)
{
    const std::string ctfFile("clf/illegal/transform_missing_outbitdepth.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "outBitDepth is missing");
}

OCIO_ADD_TEST(FileFormatCTF, array_missing_values)
{
    const std::string ctfFile("clf/illegal/array_missing_values.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Expected 3x3 Array values");
}

OCIO_ADD_TEST(FileFormatCTF, array_bad_value)
{
    const std::string ctfFile("clf/illegal/array_bad_value.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), 
                          OCIO::Exception,
                          "Illegal values");
}

OCIO_ADD_TEST(FileFormatCTF, array_bad_dimension)
{
    const std::string ctfFile("clf/illegal/array_bad_dimension.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Illegal array dimensions");
}

OCIO_ADD_TEST(FileFormatCTF, array_too_many_values)
{
    const std::string ctfFile("clf/illegal/array_too_many_values.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Expected 3x3 Array, found too many values");
}

OCIO_ADD_TEST(FileFormatCTF, matrix_end_missing)
{
    const std::string ctfFile("clf/illegal/matrix_end_missing.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "no closing tag for 'Matrix'");
}

OCIO_ADD_TEST(FileFormatCTF, transform_bad_outdepth)
{
    const std::string ctfFile("clf/illegal/transform_bad_outdepth.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "outBitDepth unknown value");
}

OCIO_ADD_TEST(FileFormatCTF, transform_end_missing)
{
    const std::string ctfFile("clf/illegal/transform_element_end_missing.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), 
                          OCIO::Exception,
                          "no element found");
}

OCIO_ADD_TEST(FileFormatCTF, transform_corrupted_tag)
{
    const std::string ctfFile("clf/illegal/transform_corrupted_tag.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "no closing tag");
}

OCIO_ADD_TEST(FileFormatCTF, transform_empty)
{
    const std::string ctfFile("clf/illegal/transform_empty.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "No color operator");
}

OCIO_ADD_TEST(FileFormatCTF, transform_id_empty)
{
    const std::string ctfFile("clf/illegal/transform_id_empty.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Required attribute 'id' does not have a value");
}

OCIO_ADD_TEST(FileFormatCTF, transform_with_bitdepth_mismatch)
{
    // Even though we normalize the bit-depths after reading, any mismatches in
    // the file are an indication of improper/unreliable formatting and an
    // exception should be thrown.
    const std::string ctfFile("clf/illegal/transform_bitdepth_mismatch.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Bit-depth mismatch");
}

OCIO_ADD_TEST(FileFormatCTF, inverse_of_id_test)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/inverseOf_id_test.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    OCIO_CHECK_ASSERT(cachedFile->m_transform->getInverseOfId() ==
                      "inverseOfIdTest");
}

OCIO_ADD_TEST(FileFormatCTF, range_default)
{
    // If style is not present, it defaults to clamp.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/range.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pR = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pR);

    OCIO_CHECK_EQUAL(pR->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT16);
    OCIO_CHECK_EQUAL(pR->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT16);
    // NB: All exactly representable as float.
    OCIO_CHECK_EQUAL(pR->getMinInValue(), 16320. / 65535.);
    OCIO_CHECK_EQUAL(pR->getMaxInValue(), 32640. / 65535.);
    OCIO_CHECK_EQUAL(pR->getMinOutValue(), 16320. / 65535.);
    OCIO_CHECK_EQUAL(pR->getMaxOutValue(), 32640. / 65535.);

    OCIO_CHECK_ASSERT(!pR->minIsEmpty());
    OCIO_CHECK_ASSERT(!pR->maxIsEmpty());
}

OCIO_ADD_TEST(FileFormatCTF, range_test1_clamp)
{
    // Style == clamp.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/range_test1_clamp.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pR = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pR);

    OCIO_CHECK_EQUAL(pR->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(pR->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    // NB: All exactly representable as float.
    OCIO_CHECK_EQUAL(pR->getMinInValue(), 16. / 255.);
    OCIO_CHECK_EQUAL(pR->getMaxInValue(), 240. / 255.);
    OCIO_CHECK_EQUAL(pR->getMinOutValue(), -0.5);
    OCIO_CHECK_EQUAL(pR->getMaxOutValue(), 2.);

    OCIO_CHECK_ASSERT(!pR->minIsEmpty());
    OCIO_CHECK_ASSERT(!pR->maxIsEmpty());
}

OCIO_ADD_TEST(FileFormatCTF, range_test1_noclamp)
{
    // Style == noClamp.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/range_test1_noclamp.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);

    // Check that the noClamp style Range became a Matrix.
    auto matOpData = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(matOpData);
    OCIO_CHECK_EQUAL(matOpData->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(matOpData->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    const double outScale = OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_F32);
    const double matScale = outScale / OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT8);
    const OCIO::ArrayDouble & array = matOpData->getArray();
    OCIO_CHECK_EQUAL(array.getLength(), 4u);
    OCIO_CHECK_EQUAL(array.getNumColorComponents(), 4u);
    OCIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength());

    const float scalef = (1.05f + 0.05f) / (272.f + 16.f);
    const float offsetf = -0.05f - scalef * -16.f;
    const float prec = 10000.f;
    const int scale = (int)(prec * scalef);
    const int offset = (int)(prec * offsetf);

    OCIO_CHECK_ASSERT(matOpData->isDiagonal());

    // Check values on the diagonal.
    OCIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
    OCIO_CHECK_EQUAL((int)(prec * array.getValues()[0] * matScale), scale);
    OCIO_CHECK_EQUAL((int)(prec * array.getValues()[5] * matScale), scale);
    OCIO_CHECK_EQUAL((int)(prec * array.getValues()[10] * matScale), scale);
    OCIO_CHECK_EQUAL(array.getValues()[15], 1.0);

    // Check the offsets.
    const OCIO::MatrixOpData::Offsets & offsets = matOpData->getOffsets();
    OCIO_CHECK_EQUAL((int)(prec * offsets[0] * outScale), offset);
    OCIO_CHECK_EQUAL((int)(prec * offsets[1] * outScale), offset);
    OCIO_CHECK_EQUAL((int)(prec * offsets[2] * outScale), offset);
    OCIO_CHECK_EQUAL(offsets[3], 0.f);
}

OCIO_ADD_TEST(FileFormatCTF, range_test2)
{
    // Style == clamp.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/range_test2.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pR = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pR);

    OCIO_CHECK_EQUAL(pR->getFileInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(pR->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F16);
    OCIO_CHECK_EQUAL(pR->getMinInValue(), 0.1);
    OCIO_CHECK_EQUAL(pR->getMinOutValue(), 0.1);
    OCIO_CHECK_ASSERT(pR->maxIsEmpty());
}

OCIO_ADD_TEST(FileFormatCTF, range_nonmatching_clamp)
{
    const std::string ctfFile("clf/illegal/range_nonmatching_clamp.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "In and out minimum limits must be equal");
}

OCIO_ADD_TEST(FileFormatCTF, range_empty)
{
    const std::string ctfFile("clf/illegal/range_empty.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "At least minimum or maximum limits must be set");
}

OCIO_ADD_TEST(FileFormatCTF, range_bad_noclamp)
{
    const std::string ctfFile("clf/illegal/range_bad_noclamp.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "Non-clamping Range min & max values have to be set");
}

OCIO_ADD_TEST(FileFormatCTF, range_bad_values)
{
    const std::string ctfFile("clf/illegal/range_bad_values.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "Range maxInValue is too close to minInValue");
}

OCIO_ADD_TEST(FileFormatCTF, indexMap_test)
{
    const std::string ctfFile("indexMap_test.ctf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Only two entry IndexMaps are supported");
}

OCIO_ADD_TEST(FileFormatCTF, indexMap_test1_clfv2)
{
    // IndexMaps were allowed in CLF v2 (were removed in v3).
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("indexMap_test1_clfv2.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 2);
    auto pR = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pR);

    // Check that the indexMap caused a Range to be inserted.
    OCIO_CHECK_EQUAL(pR->getMinInValue() * 1023., 64.5);
    OCIO_CHECK_EQUAL(pR->getMaxInValue() * 1023., 940.);
    OCIO_CHECK_EQUAL(pR->getMinOutValue() * 1023.0, 132.0);  // 4*1023/31
    OCIO_CHECK_EQUAL(pR->getMaxOutValue() * 1023.0, 1089.0); // 33*1023/31
    OCIO_CHECK_EQUAL(pR->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OCIO_CHECK_EQUAL(pR->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    // Check the LUT is ok.
    auto pL = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[1]);
    OCIO_REQUIRE_ASSERT(pL);
    OCIO_CHECK_EQUAL(pL->getArray().getLength(), 32u);
    OCIO_CHECK_EQUAL(pL->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
}

OCIO_ADD_TEST(FileFormatCTF, indexMap_test2_clfv2)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("indexMap_test2_clfv2.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 2);
    auto pR = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pR);
    OCIO_CHECK_EQUAL(pR->getMinInValue(), -0.1f);
    OCIO_CHECK_EQUAL(pR->getMaxInValue(), 19.f);
    OCIO_CHECK_EQUAL(pR->getMinOutValue(), 0.f);
    OCIO_CHECK_EQUAL(pR->getMaxOutValue(), 1.f);
    OCIO_CHECK_EQUAL(pR->getFileInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(pR->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    // Check the LUT is ok.
    auto pL = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opList[1]);
    OCIO_REQUIRE_ASSERT(pL);
    OCIO_CHECK_EQUAL(pL->getArray().getLength(), 2u);
    OCIO_CHECK_EQUAL(pL->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
}

OCIO_ADD_TEST(FileFormatCTF, clf3_index_map)
{
    // Same as previous, but setting compCLFversion=3.0.
    const std::string ctfFile("clf/illegal/indexMap_test2.clf");
    static constexpr char Warning[1024] = 
        "Element 'IndexMap' is not valid since CLF 3 (or CTF 2)";

    OCIO::LogGuard guard;
    OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_WARNING);

    OCIO::LocalCachedFileRcPtr cachedFile;
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);
    OCIO_CHECK_NE(std::string::npos, 
                  StringUtils::Find( StringUtils::RightTrim(guard.output()), Warning ));
}

OCIO_ADD_TEST(FileFormatCTF, indexMap_test3)
{
    const std::string ctfFile("indexMap_test3.ctf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "Only one IndexMap allowed per LUT");
}

OCIO_ADD_TEST(FileFormatCTF, indexMap_test4_clfv2)
{
    const std::string ctfFile("indexMap_test4_clfv2.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "Only two entry IndexMaps are supported");
}

OCIO_ADD_TEST(FileFormatCTF, gamma_test1)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_test1.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    OCIO_CHECK_EQUAL(cachedFile->m_transform->getID(), "id");

    OCIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().size(), 1);
    OCIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[0], "2.4 gamma");

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pG);

    OCIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::BASIC_FWD);

    OCIO::GammaOpData::Params params;
    params.push_back(2.4);

    OCIO_CHECK_ASSERT(pG->getRedParams() == params);
    OCIO_CHECK_ASSERT(pG->getGreenParams() == params);
    OCIO_CHECK_ASSERT(pG->getBlueParams() == params);
    // Version of the ctf is less than 1.5, so alpha must be identity.
    OCIO_CHECK_ASSERT(OCIO::GammaOpData::isIdentityParameters(
                          pG->getAlphaParams(),
                          pG->getStyle()));

    OCIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OCIO_CHECK_ASSERT(pG->isNonChannelDependent()); // RGB are equal, A is an identity
}

OCIO_ADD_TEST(FileFormatCTF, gamma_test2)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_test2.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pG);

    OCIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::BASIC_REV);
    OCIO::GammaOpData::Params paramsR;
    paramsR.push_back(2.4);
    OCIO::GammaOpData::Params paramsG;
    paramsG.push_back(2.35);
    OCIO::GammaOpData::Params paramsB;
    paramsB.push_back(2.2);

    OCIO_CHECK_ASSERT(pG->getRedParams() == paramsR);
    OCIO_CHECK_ASSERT(pG->getGreenParams() == paramsG);
    OCIO_CHECK_ASSERT(pG->getBlueParams() == paramsB);
    OCIO_CHECK_ASSERT(OCIO::GammaOpData::isIdentityParameters(
                          pG->getAlphaParams(),
                          pG->getStyle()));

    OCIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OCIO_CHECK_ASSERT(!pG->isNonChannelDependent());
}

OCIO_ADD_TEST(FileFormatCTF, gamma_test3)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_test3.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pG);

    OCIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::MONCURVE_FWD);
    OCIO::GammaOpData::Params params;
    params.push_back(1. / 0.45);
    params.push_back(0.099);

    // This is a precision test to ensure we can recreate a double that is
    // exactly equal to 1/0.45, which is required to implement rec 709 exactly.
    OCIO_CHECK_ASSERT(pG->getRedParams() == params);
    OCIO_CHECK_ASSERT(pG->getGreenParams() == params);
    OCIO_CHECK_ASSERT(pG->getBlueParams() == params);
    OCIO_CHECK_ASSERT(OCIO::GammaOpData::isIdentityParameters(
                          pG->getAlphaParams(),
                          pG->getStyle()));

    OCIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OCIO_CHECK_ASSERT(pG->isNonChannelDependent());
}

OCIO_ADD_TEST(FileFormatCTF, gamma_test4)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_test4.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pG);

    OCIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::MONCURVE_REV);
    OCIO::GammaOpData::Params paramsR;
    paramsR.push_back(2.2);
    paramsR.push_back(0.001);
    OCIO::GammaOpData::Params paramsG;
    paramsG.push_back(2.4);
    paramsG.push_back(0.01);
    OCIO::GammaOpData::Params paramsB;
    paramsB.push_back(2.6);
    paramsB.push_back(0.1);

    OCIO_CHECK_ASSERT(pG->getRedParams() == paramsR);
    OCIO_CHECK_ASSERT(pG->getGreenParams() == paramsG);
    OCIO_CHECK_ASSERT(pG->getBlueParams() == paramsB);
    OCIO_CHECK_ASSERT(OCIO::GammaOpData::isIdentityParameters(
                          pG->getAlphaParams(),
                          pG->getStyle()));

    OCIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OCIO_CHECK_ASSERT(!pG->isNonChannelDependent());
}

OCIO_ADD_TEST(FileFormatCTF, gamma_test5)
{
    // This test is for an old (< 1.5) transform file that contains
    // an invalid GammaParams for the A channel.
    const std::string ctfFile("gamma_test5.ctf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "Invalid channel");
}

OCIO_ADD_TEST(FileFormatCTF, gamma_test6)
{
    // This test is for an old (< 1.5) transform file that contains
    // a single GammaParams with identity values:
    // - R, G and B set to identity parameters (identity test).
    // - A set to identity.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_test6.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pG);

    OCIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::MONCURVE_FWD);
    OCIO_CHECK_ASSERT(pG->areAllComponentsEqual());
    OCIO_CHECK_ASSERT(pG->isNonChannelDependent()); 
    OCIO_CHECK_ASSERT(pG->isIdentity());
}

OCIO_ADD_TEST(FileFormatCTF, gamma_alpha_test1)
{
    // This test is for a new (>= 1.5) transform file that contains
    // a single GammaParams:
    // - R, G and B set to same parameters.
    // - A set to identity.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_alpha_test1.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pG);

    OCIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::BASIC_FWD);

    OCIO::GammaOpData::Params params;
    params.push_back(2.4);

    OCIO_CHECK_ASSERT(pG->getRedParams() == params);
    OCIO_CHECK_ASSERT(pG->getGreenParams() == params);
    OCIO_CHECK_ASSERT(pG->getBlueParams() == params);
    OCIO_CHECK_ASSERT(OCIO::GammaOpData::isIdentityParameters(
                          pG->getAlphaParams(),
                          pG->getStyle()));

    OCIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OCIO_CHECK_ASSERT(pG->isNonChannelDependent());
}

OCIO_ADD_TEST(FileFormatCTF, gamma_alpha_test2)
{
    // This test is for a new (>= 1.5) transform file that contains
    // a different GammaParams for every channel:
    // - R, G, B and A set to different parameters.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_alpha_test2.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pG);

    OCIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::BASIC_REV);

    OCIO::GammaOpData::Params paramsR;
    paramsR.push_back(2.4);
    OCIO::GammaOpData::Params paramsG;
    paramsG.push_back(2.35);
    OCIO::GammaOpData::Params paramsB;
    paramsB.push_back(2.2);
    OCIO::GammaOpData::Params paramsA;
    paramsA.push_back(2.5);

    OCIO_CHECK_ASSERT(pG->getRedParams() == paramsR);
    OCIO_CHECK_ASSERT(pG->getGreenParams() == paramsG);
    OCIO_CHECK_ASSERT(pG->getBlueParams() == paramsB);
    OCIO_CHECK_ASSERT(pG->getAlphaParams() == paramsA);

    OCIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OCIO_CHECK_ASSERT(!pG->isNonChannelDependent());
}

OCIO_ADD_TEST(FileFormatCTF, gamma_alpha_test3)
{
    // This test is for a new (>= 1.5) transform file that contains
    // a single GammaParams:
    // - R, G and B set to same parameters (precision test).
    // - A set to identity.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_alpha_test3.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pG);

    OCIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::MONCURVE_FWD);

    OCIO::GammaOpData::Params params;
    params.push_back(1. / 0.45);
    params.push_back(0.099);

    OCIO_CHECK_ASSERT(pG->getRedParams() == params);
    OCIO_CHECK_ASSERT(pG->getGreenParams() == params);
    OCIO_CHECK_ASSERT(pG->getBlueParams() == params);
    OCIO_CHECK_ASSERT(OCIO::GammaOpData::isIdentityParameters(
                          pG->getAlphaParams(),
                          pG->getStyle()));

    OCIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OCIO_CHECK_ASSERT(pG->isNonChannelDependent());
}

OCIO_ADD_TEST(FileFormatCTF, gamma_alpha_test4)
{
    // This test is for a new (>= 1.5) transform file that contains
    // a different GammaParams for every channel:
    // - R, G, B and A set to different parameters (attributes order test).
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_alpha_test4.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pG);

    OCIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::MONCURVE_REV);

    OCIO::GammaOpData::Params paramsR;
    paramsR.push_back(2.2);
    paramsR.push_back(0.001);
    OCIO::GammaOpData::Params paramsG;
    paramsG.push_back(2.4);
    paramsG.push_back(0.01);
    OCIO::GammaOpData::Params paramsB;
    paramsB.push_back(2.6);
    paramsB.push_back(0.1);
    OCIO::GammaOpData::Params paramsA;
    paramsA.push_back(2.0);
    paramsA.push_back(0.0001);

    OCIO_CHECK_ASSERT(pG->getRedParams() == paramsR);
    OCIO_CHECK_ASSERT(pG->getGreenParams() == paramsG);
    OCIO_CHECK_ASSERT(pG->getBlueParams() == paramsB);
    OCIO_CHECK_ASSERT(pG->getAlphaParams() == paramsA);

    OCIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OCIO_CHECK_ASSERT(!pG->isNonChannelDependent());
}

OCIO_ADD_TEST(FileFormatCTF, gamma_alpha_test5)
{
    // This test is for a new (>= 1.5) transform file that contains
    // a GammaParams with no channel specified:
    // - R, G and B set to same parameters.
    // and a GammaParams for the A channel:
    // - A set to different parameters.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_alpha_test5.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pG);

    OCIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::MONCURVE_FWD);

    OCIO::GammaOpData::Params params;
    params.push_back(1. / 0.45);
    params.push_back(0.099);
    OCIO::GammaOpData::Params paramsA;
    paramsA.push_back(1.7);
    paramsA.push_back(0.33);

    OCIO_CHECK_ASSERT(pG->getRedParams() == params);
    OCIO_CHECK_ASSERT(pG->getGreenParams() == params);
    OCIO_CHECK_ASSERT(pG->getBlueParams() == params);
    OCIO_CHECK_ASSERT(pG->getAlphaParams() == paramsA);

    OCIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OCIO_CHECK_ASSERT(!pG->isNonChannelDependent());
}

OCIO_ADD_TEST(FileFormatCTF, gamma_alpha_test6)
{
    // This test is for an new (>= 1.5) transform file that contains
    // an invalid GammaParams for the A channel (missing offset attribute).
    const std::string ctfFile("gamma_alpha_test6.ctf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "Missing required offset parameter");
}

OCIO_ADD_TEST(FileFormatCTF, exponent_bad_value)
{
    // The moncurve style requires a gamma value >= 1.
    const std::string ctfFile("clf/illegal/exponent_bad_value.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "is less than lower bound");
}

OCIO_ADD_TEST(FileFormatCTF, exponent_bad_param)
{
    // The basic style cannot use offset.
    const std::string ctfFile("clf/illegal/exponent_bad_param.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "Illegal offset parameter");
}

OCIO_ADD_TEST(FileFormatCTF, exponent_all_styles)
{
    // Note: This is somewhat repetitive of the CTF Gamma tests above, but it is worth
    // having both due to changes in the format over time (e.g. moncurveFwd->monCurveFwd,
    // and gamma->exponent), and the fact that CLF and early CTF does not support alpha.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string fileName("clf/exponent_all_styles.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 12);

    {   // Op 0 == basicFwd.
        auto opData = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
        OCIO_REQUIRE_ASSERT(opData);
        StringUtils::StringVec desc;
        GetElementsValues(opData->getFormatMetadata().getChildrenElements(),
                          OCIO::TAG_DESCRIPTION, desc);
        OCIO_REQUIRE_EQUAL(desc.size(), 1);
        OCIO_CHECK_EQUAL(desc[0], "If there is only one Params, use it for R, G, and B.");
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(opData->getStyle(), OCIO::GammaOpData::BASIC_FWD);
        OCIO_CHECK_ASSERT(opData->isNonChannelDependent()); // RGB are equal, A is an identity
        OCIO_CHECK_ASSERT(opData->isAlphaComponentIdentity());
        OCIO::GammaOpData::Params params;
        params.push_back(2.4);
        OCIO_CHECK_ASSERT(opData->getRedParams() == params);
    }
    {   // Op 1 == basicRev.
        auto opData = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[1]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getID(), "a1");
        OCIO_CHECK_EQUAL(opData->getName(), "gamma");
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_EQUAL(opData->getStyle(), OCIO::GammaOpData::BASIC_REV);
        OCIO_CHECK_ASSERT(!opData->isNonChannelDependent());
        OCIO_CHECK_ASSERT(opData->isAlphaComponentIdentity());
        OCIO::GammaOpData::Params paramsR;
        paramsR.push_back(2.4);
        OCIO::GammaOpData::Params paramsG;
        paramsG.push_back(2.35);
        OCIO::GammaOpData::Params paramsB;
        paramsB.push_back(2.2);
        OCIO_CHECK_ASSERT(opData->getRedParams()   == paramsR);
        OCIO_CHECK_ASSERT(opData->getGreenParams() == paramsG);
        OCIO_CHECK_ASSERT(opData->getBlueParams()  == paramsB);
    }
    {   // Op 2 == monCurveFwd.
        auto opData = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[2]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(opData->getStyle(), OCIO::GammaOpData::MONCURVE_FWD);
        OCIO_CHECK_ASSERT(opData->isNonChannelDependent()); // RGB are equal, A is an identity
        OCIO_CHECK_ASSERT(opData->isAlphaComponentIdentity());
        OCIO::GammaOpData::Params params;
        params.push_back(1./0.45);
        params.push_back(0.099);
        OCIO_CHECK_ASSERT(opData->getRedParams() == params);
    }
    {   // Op 3 == monCurveRev.
        auto opData = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[3]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_EQUAL(opData->getStyle(), OCIO::GammaOpData::MONCURVE_REV);
        OCIO_CHECK_ASSERT(!opData->isNonChannelDependent());
        OCIO_CHECK_ASSERT(opData->isAlphaComponentIdentity());
        OCIO::GammaOpData::Params paramsR;
        paramsR.push_back(2.2);
        paramsR.push_back(0.001);
        OCIO::GammaOpData::Params paramsG;
        paramsG.push_back(2.4);
        paramsG.push_back(0.01);
        OCIO::GammaOpData::Params paramsB;
        paramsB.push_back(2.6);
        paramsB.push_back(0.1);
        OCIO_CHECK_ASSERT(opData->getRedParams()   == paramsR);
        OCIO_CHECK_ASSERT(opData->getGreenParams() == paramsG);
        OCIO_CHECK_ASSERT(opData->getBlueParams()  == paramsB);
    }
    {   // Op 4 == monCurveFwd.
        auto opData = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[4]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(opData->getStyle(), OCIO::GammaOpData::MONCURVE_FWD);
        OCIO_CHECK_ASSERT(opData->areAllComponentsEqual());
        OCIO_CHECK_ASSERT(opData->isNonChannelDependent()); // RGB are equal, A is an identity
        OCIO_CHECK_ASSERT(opData->isAlphaComponentIdentity());
        OCIO_CHECK_ASSERT(OCIO::GammaOpData::isIdentityParameters(
                          opData->getRedParams(),
                          opData->getStyle()));
    }
    {   // Op 5 == basicMirrorFwd.
        auto opData = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[5]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(opData->getStyle(), OCIO::GammaOpData::BASIC_MIRROR_FWD);
        OCIO_CHECK_ASSERT(!opData->areAllComponentsEqual());
        OCIO_CHECK_ASSERT(opData->isNonChannelDependent()); // RGB are equal, A is an identity
        OCIO_CHECK_ASSERT(opData->isAlphaComponentIdentity());
    }
    {   // Op 6 == basicMirrorRev.
        auto opData = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[6]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_EQUAL(opData->getStyle(), OCIO::GammaOpData::BASIC_MIRROR_REV);
        OCIO_CHECK_ASSERT(opData->isNonChannelDependent()); // RGB are equal, A is an identity
    }
    {   // Op 7 == basicPassThruFwd.
        auto opData = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[7]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(opData->getStyle(), OCIO::GammaOpData::BASIC_PASS_THRU_FWD);
        OCIO_CHECK_ASSERT(opData->isNonChannelDependent()); // RGB are equal, A is an identity
    }
    {   // Op 8 == basicPassThruRev.
        auto opData = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[8]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_EQUAL(opData->getStyle(), OCIO::GammaOpData::BASIC_PASS_THRU_REV);
        OCIO_CHECK_ASSERT(opData->isNonChannelDependent()); // RGB are equal, A is an identity
    }
    {   // Op 9 == monCurveMirrorFwd.
        auto opData = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[9]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(opData->getStyle(), OCIO::GammaOpData::MONCURVE_MIRROR_FWD);
        OCIO_CHECK_ASSERT(opData->isNonChannelDependent()); // RGB are equal, A is an identity
    }
    {   // Op 10 == monCurveMirrorRev.
        auto opData = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[10]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_EQUAL(opData->getStyle(), OCIO::GammaOpData::MONCURVE_MIRROR_REV);
        OCIO_CHECK_ASSERT(!opData->isNonChannelDependent());
        OCIO::GammaOpData::Params paramsR;
        paramsR.push_back(3.0);
        paramsR.push_back(0.16);
        OCIO_CHECK_ASSERT(opData->getRedParams() == paramsR);
        OCIO_CHECK_ASSERT(OCIO::GammaOpData::isIdentityParameters(
                          opData->getGreenParams(),
                          opData->getStyle()));
        OCIO_CHECK_ASSERT(OCIO::GammaOpData::isIdentityParameters(
                          opData->getBlueParams(),
                          opData->getStyle()));
    }
    {   // Op 11 == Range.
        auto opData = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[11]);
        OCIO_REQUIRE_ASSERT(opData);
    }
}

OCIO_ADD_TEST(FileFormatCTF, clf2_exponent_parse)
{
    const std::string gammaCLF2{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="2" id="UID42">
    <Exponent inBitDepth="32f" outBitDepth="32f" style="basicRev">
        <ExponentParams gamma="2.6" />
    </Exponent>
</ProcessList>
)" };

    OCIO_CHECK_THROW_WHAT(ParseString(gammaCLF2), OCIO::Exception,
                          "CLF file version '2' does not support operator 'Exponent'");

    const std::string gammaCLFAlpha{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="UID42">
    <Exponent inBitDepth="32f" outBitDepth="32f" style="basicRev">
        <ExponentParams gamma="2.6" />
        <ExponentParams channel="A" gamma="1.7" offset="0.33" />
    </Exponent>
</ProcessList>
)" };

    OCIO_CHECK_THROW_WHAT(ParseString(gammaCLFAlpha), OCIO::Exception,
                          "Invalid channel: A");

    const std::string gammaCTFMirror1_7{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.7" id="UID42">
    <Exponent inBitDepth="32f" outBitDepth="32f" style="basicMirrorRev">
        <ExponentParams gamma="2.6" />
    </Exponent>
</ProcessList>
)" };

    OCIO_CHECK_THROW_WHAT(ParseString(gammaCTFMirror1_7), OCIO::Exception,
                                      "Style not handled: 'basicMirrorRev'");
}

OCIO_ADD_TEST(FileFormatCTF, cdl_clamp_fwd)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/cdl_clamp_fwd.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_CHECK_EQUAL(cachedFile->m_transform->getInputDescriptor(),
                     "inputDesc");
    OCIO_CHECK_EQUAL(cachedFile->m_transform->getOutputDescriptor(),
                     "outputDesc");
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pCDL);

    OCIO_CHECK_EQUAL(pCDL->getID(), std::string("look 1"));
    OCIO_CHECK_EQUAL(pCDL->getName(), std::string("cdl"));

    StringUtils::StringVec descriptions;
    GetElementsValues(pCDL->getFormatMetadata().getChildrenElements(),
                      OCIO::TAG_DESCRIPTION, descriptions);

    OCIO_REQUIRE_EQUAL(descriptions.size(), 1u);
    OCIO_CHECK_EQUAL(descriptions[0], std::string("ASC CDL operation"));

    OCIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);
    std::string styleName(OCIO::CDLOpData::GetStyleName(pCDL->getStyle()));
    OCIO_CHECK_EQUAL(styleName, "Fwd");

    OCIO_CHECK_ASSERT(pCDL->getSlopeParams() 
                      == OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71));
    OCIO_CHECK_ASSERT(pCDL->getOffsetParams()
                      == OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11));
    OCIO_CHECK_ASSERT(pCDL->getPowerParams()
                      == OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27));
    OCIO_CHECK_EQUAL(pCDL->getSaturation(), 1.239);
}

OCIO_ADD_TEST(FileFormatCTF, cdl_missing_style)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/cdl_missing_style.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pCDL);

    // Note: Default for CLF is different from OCIO default.
    OCIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);

    OCIO_CHECK_ASSERT(pCDL->getSlopeParams() 
                      == OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71));
    OCIO_CHECK_ASSERT(pCDL->getOffsetParams()
                      == OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11));
    OCIO_CHECK_ASSERT(pCDL->getPowerParams()
                      == OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27));
    OCIO_CHECK_EQUAL(pCDL->getSaturation(), 1.239);
}

OCIO_ADD_TEST(FileFormatCTF, cdl_all_styles)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/cdl_all_styles.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 4);

    auto pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pCDL);
    OCIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[1]);
    OCIO_REQUIRE_ASSERT(pCDL);
    OCIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_V1_2_REV);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[2]);
    OCIO_REQUIRE_ASSERT(pCDL);
    OCIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_FWD);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[3]);
    OCIO_REQUIRE_ASSERT(pCDL);
    OCIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_REV);
}

OCIO_ADD_TEST(FileFormatCTF, cdl_bad_slope)
{
    const std::string ctfFile("clf/illegal/cdl_bad_slope.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "SOPNode: 3 values required");
}
        
OCIO_ADD_TEST(FileFormatCTF, cdl_bad_sat)
{
    const std::string ctfFile("clf/illegal/cdl_bad_sat.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "SatNode: non-single value");
}

OCIO_ADD_TEST(FileFormatCTF, cdl_bad_power)
{
    const std::string ctfFile("clf/illegal/cdl_bad_power.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "CDLOpData: Invalid 'power' 0 should be greater than 0.");
}

OCIO_ADD_TEST(FileFormatCTF, cdl_missing_slope)
{
    const std::string ctfFile("clf/illegal/cdl_missing_slope.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "Required node 'Slope' is missing");
}

OCIO_ADD_TEST(FileFormatCTF, cdl_missing_offset)
{
    const std::string ctfFile("clf/illegal/cdl_missing_offset.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "Required node 'Offset' is missing");
}

OCIO_ADD_TEST(FileFormatCTF, cdl_missing_power)
{
    const std::string ctfFile("clf/illegal/cdl_missing_power.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "Required node 'Power' is missing");
}

OCIO_ADD_TEST(FileFormatCTF, cdl_bad_style)
{
    const std::string ctfFile("clf/illegal/cdl_bad_style.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "Unknown style for CDL");
}

OCIO_ADD_TEST(FileFormatCTF, cdl_missing_sop)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/cdl_missing_sop.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pCDL);

    OCIO_CHECK_ASSERT(pCDL->getSlopeParams()
                      == OCIO::CDLOpData::ChannelParams(1.0));
    OCIO_CHECK_ASSERT(pCDL->getOffsetParams()
                      == OCIO::CDLOpData::ChannelParams(0.0));
    OCIO_CHECK_ASSERT(pCDL->getPowerParams()
                      == OCIO::CDLOpData::ChannelParams(1.0));
    OCIO_CHECK_EQUAL(pCDL->getSaturation(), 1.239);
}

OCIO_ADD_TEST(FileFormatCTF, cdl_missing_sat)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/cdl_missing_sat.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pCDL);

    OCIO_CHECK_ASSERT(pCDL->getSlopeParams()
                      == OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71));
    OCIO_CHECK_ASSERT(pCDL->getOffsetParams()
                      == OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11));
    OCIO_CHECK_ASSERT(pCDL->getPowerParams()
                      == OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27));
    OCIO_CHECK_EQUAL(pCDL->getSaturation(), 1.0);
}

OCIO_ADD_TEST(FileFormatCTF, cdl_various_in_ctf)
{
    // When CDL was added to the CLF spec in v2, the style names were changed.
    // Test that both the new and old style names work.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("cdl_various.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 8);

    auto pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pCDL);
    OCIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[1]);
    OCIO_REQUIRE_ASSERT(pCDL);
    OCIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[2]);
    OCIO_REQUIRE_ASSERT(pCDL);
    OCIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_V1_2_REV);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[3]);
    OCIO_REQUIRE_ASSERT(pCDL);
    OCIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_V1_2_REV);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[4]);
    OCIO_REQUIRE_ASSERT(pCDL);
    OCIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_FWD);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[5]);
    OCIO_REQUIRE_ASSERT(pCDL);
    OCIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_FWD);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[6]);
    OCIO_REQUIRE_ASSERT(pCDL);
    OCIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_REV);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[7]);
    OCIO_REQUIRE_ASSERT(pCDL);
    OCIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_REV);
}

OCIO_ADD_TEST(FileFormatCTF, log_all_styles)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string fileName("clf/log_all_styles.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 11);
    double error = 1e-9;

    {   // Op 0 == antiLog2.
        auto opData = std::dynamic_pointer_cast<const OCIO::LogOpData>(opList[0]);
        OCIO_REQUIRE_ASSERT(opData);
        StringUtils::StringVec desc;
        GetElementsValues(opData->getFormatMetadata().getChildrenElements(),
                          OCIO::TAG_DESCRIPTION, desc);
        OCIO_REQUIRE_EQUAL(desc.size(), 1);
        OCIO_CHECK_EQUAL(desc[0], "AntiLog2 logarithm operation");
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_ASSERT(opData->isLog2());
    }
    {   // Op 1 == log2.
        auto opData = std::dynamic_pointer_cast<const OCIO::LogOpData>(opList[1]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getID(), "a1");
        OCIO_CHECK_EQUAL(opData->getName(), "logarithm");
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_ASSERT(opData->isLog2());
        OCIO_CHECK_ASSERT(!opData->isLog10());
        OCIO_CHECK_ASSERT(!opData->isCamera());
    }
    {   // Op 2 == linToLog.
        auto opData = std::dynamic_pointer_cast<const OCIO::LogOpData>(opList[2]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_ASSERT(!opData->isLog2());
        OCIO_CHECK_ASSERT(!opData->isLog10());
        OCIO_CHECK_ASSERT(!opData->isCamera());
        OCIO_CHECK_ASSERT(opData->allComponentsEqual());
        auto & param = opData->getRedParams();
        OCIO_REQUIRE_EQUAL(param.size(), 4);
        OCIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_SLOPE],  0.29325513196, error);
        OCIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_OFFSET], 0.66959921799, error);
        OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_SLOPE],  0.98920224838, error);
        OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_OFFSET], 0.01079775162, error);
        OCIO_CHECK_EQUAL(opData->getBase(), 10.);
    }
    {   // Op 3 == antiLog10.
        auto opData = std::dynamic_pointer_cast<const OCIO::LogOpData>(opList[3]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_ASSERT(!opData->isLog2());
        OCIO_CHECK_ASSERT(opData->isLog10());
    }
    {   // Op 4 == log10.
        auto opData = std::dynamic_pointer_cast<const OCIO::LogOpData>(opList[4]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_ASSERT(!opData->isLog2());
        OCIO_CHECK_ASSERT(opData->isLog10());
    }
    {   // Op 5 == logToLin.
        auto opData = std::dynamic_pointer_cast<const OCIO::LogOpData>(opList[5]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_ASSERT(!opData->isLog2());
        OCIO_CHECK_ASSERT(!opData->isLog10());
        OCIO_CHECK_ASSERT(!opData->isCamera());
        OCIO_CHECK_ASSERT(opData->allComponentsEqual());
        auto & param = opData->getRedParams();
        OCIO_REQUIRE_EQUAL(param.size(), 4);
        OCIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_SLOPE],  0.29325513196, error);
        OCIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_OFFSET], 0.66959921799, error);
        OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_SLOPE],  0.98920224838, error);
        OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_OFFSET], 0.01079775162, error);
        OCIO_CHECK_EQUAL(opData->getBase(), 10.);
    }
    {   // Op 6 == cameraLinToLog.
        auto opData = std::dynamic_pointer_cast<const OCIO::LogOpData>(opList[6]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_ASSERT(!opData->isLog2());
        OCIO_CHECK_ASSERT(!opData->isLog10());
        OCIO_CHECK_ASSERT(opData->isCamera());
        OCIO_CHECK_ASSERT(opData->allComponentsEqual());
        auto & param = opData->getRedParams();
        OCIO_REQUIRE_EQUAL(param.size(), 5);
        OCIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_SLOPE],  0.05707762557, error);
        OCIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_OFFSET], 0.55479452050, error);
        OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_SLOPE],  1.           , error);
        OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_OFFSET], 0.           , error);
        OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_BREAK],  0.00781250000, error);
        // Default base value is 2.
        OCIO_CHECK_EQUAL(opData->getBase(), 2.);
    }
    {   // Op 7 == cameraLogToLin.
        auto opData = std::dynamic_pointer_cast<const OCIO::LogOpData>(opList[7]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_ASSERT(!opData->isLog2());
        OCIO_CHECK_ASSERT(!opData->isLog10());
        OCIO_CHECK_ASSERT(opData->isCamera());
        OCIO_CHECK_ASSERT(opData->allComponentsEqual());
        auto & param = opData->getRedParams();
        OCIO_REQUIRE_EQUAL(param.size(), 5);
        OCIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_SLOPE],  0.05707762557, error);
        OCIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_OFFSET], 0.55479452050, error);
        OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_SLOPE],  1.           , error);
        OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_OFFSET], 0.           , error);
        OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_BREAK],  0.00781250000, error);
        OCIO_CHECK_EQUAL(opData->getBase(), 2.);
    }
    {   // Op 8 == cameraLogToLin.
        auto opData = std::dynamic_pointer_cast<const OCIO::LogOpData>(opList[8]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_ASSERT(!opData->isLog2());
        OCIO_CHECK_ASSERT(!opData->isLog10());
        OCIO_CHECK_ASSERT(opData->isCamera());
        OCIO_CHECK_ASSERT(opData->allComponentsEqual());
        auto & param = opData->getRedParams();
        OCIO_REQUIRE_EQUAL(param.size(), 6);
        OCIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_SLOPE],  0.25562072336, error);
        OCIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_OFFSET], 0.41055718475, error);
        OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_SLOPE],  5.26315789474, error);
        OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_OFFSET], 0.05263157895, error);
        OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_BREAK],  0.01125000000, error);
        OCIO_CHECK_CLOSE(param[OCIO::LINEAR_SLOPE],    6.62194371178, error);
        OCIO_CHECK_EQUAL(opData->getBase(), 10.);
    }
    {   // Op 9 == linToLog.
        auto opData = std::dynamic_pointer_cast<const OCIO::LogOpData>(opList[9]);
        OCIO_REQUIRE_ASSERT(opData);
        OCIO_CHECK_EQUAL(opData->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_ASSERT(!opData->isLog2());
        OCIO_CHECK_ASSERT(!opData->isLog10());
        OCIO_CHECK_ASSERT(!opData->isCamera());
        OCIO_CHECK_ASSERT(!opData->allComponentsEqual());
        {
            auto & param = opData->getRedParams();
            OCIO_REQUIRE_EQUAL(param.size(), 4);
            OCIO_CHECK_EQUAL(param[OCIO::LOG_SIDE_SLOPE],  0.3);
            OCIO_CHECK_EQUAL(param[OCIO::LOG_SIDE_OFFSET], 0.6);
            OCIO_CHECK_EQUAL(param[OCIO::LIN_SIDE_SLOPE],  0.9);
            OCIO_CHECK_EQUAL(param[OCIO::LIN_SIDE_OFFSET], 0.05);
            OCIO_CHECK_EQUAL(opData->getBase(), 8.);
        }
        {
            auto & param = opData->getGreenParams();
            OCIO_REQUIRE_EQUAL(param.size(), 4);
            OCIO_CHECK_EQUAL(param[OCIO::LOG_SIDE_SLOPE],  0.25);
            OCIO_CHECK_EQUAL(param[OCIO::LOG_SIDE_OFFSET], 0.4);
            OCIO_CHECK_EQUAL(param[OCIO::LIN_SIDE_SLOPE],  5.0);
            OCIO_CHECK_EQUAL(param[OCIO::LIN_SIDE_OFFSET], 0.05);
            OCIO_CHECK_EQUAL(opData->getBase(), 8.);
        }
        {
            auto & param = opData->getBlueParams();
            OCIO_REQUIRE_EQUAL(param.size(), 4);
            OCIO_CHECK_EQUAL(param[OCIO::LOG_SIDE_SLOPE],  0.28);
            OCIO_CHECK_EQUAL(param[OCIO::LOG_SIDE_OFFSET], 0.5);
            OCIO_CHECK_EQUAL(param[OCIO::LIN_SIDE_SLOPE],  2.0);
            OCIO_CHECK_EQUAL(param[OCIO::LIN_SIDE_OFFSET], 0.1);
            OCIO_CHECK_EQUAL(opData->getBase(), 8.);
        }
    }
    {   // Op 10 == Range.
        auto opData = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[10]);
        OCIO_REQUIRE_ASSERT(opData);
    }
}

OCIO_ADD_TEST(FileFormatCTF, log_logtolin)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("log_logtolin.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    auto op = fileOps[0];
    auto log = std::dynamic_pointer_cast<const OCIO::LogOpData>(op);
    OCIO_REQUIRE_ASSERT(log);

    OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(!log->isLog2());
    OCIO_CHECK_ASSERT(!log->isLog10());
    OCIO_CHECK_ASSERT(log->allComponentsEqual());
    auto & param = log->getRedParams();
    OCIO_REQUIRE_EQUAL(param.size(), 4);
    double error = 1e-9;
    // This file uses the original CTF/Cineon style params, verify they are converted properly
    // to the new OCIO style params.
    OCIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_SLOPE],  0.29325513196, error);
    OCIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_OFFSET], 0.66959921799, error);
    OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_SLOPE],  0.98969709693, error);
    OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_OFFSET], 0.01030290307, error);
}

OCIO_ADD_TEST(FileFormatCTF, log_logtolinv2)
{
    // Same as previous test, but CTF version set to 2.
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("log_logtolinv2.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    auto op = fileOps[0];
    auto log = std::dynamic_pointer_cast<const OCIO::LogOpData>(op);
    OCIO_REQUIRE_ASSERT(log);

    OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(!log->isLog2());
    OCIO_CHECK_ASSERT(!log->isLog10());
    OCIO_CHECK_ASSERT(log->allComponentsEqual());
    auto & param = log->getRedParams();
    OCIO_REQUIRE_EQUAL(param.size(), 4);
    double error = 1e-9;
    // This file uses the original CTF/Cineon style params, verify they are converted properly
    // to the new OCIO style params.
    OCIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_SLOPE],  0.29325513196, error);
    OCIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_OFFSET], 0.66959921799, error);
    OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_SLOPE],  0.98969709693, error);
    OCIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_OFFSET], 0.01030290307, error);
}

OCIO_ADD_TEST(FileFormatCTF, log_lintolog_3chan)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("log_lintolog_3chan.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    auto op = fileOps[0];
    auto log = std::dynamic_pointer_cast<const OCIO::LogOpData>(op);
    OCIO_REQUIRE_ASSERT(log);

    OCIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_ASSERT(!log->allComponentsEqual());

    auto & rParam = log->getRedParams();
    OCIO_REQUIRE_EQUAL(rParam.size(), 4);
    double error = 1e-9;
    // This file uses the original CTF/Cineon style params, verify they are converted properly
    // to the new OCIO style params.
    OCIO_CHECK_CLOSE(rParam[OCIO::LOG_SIDE_SLOPE],   0.244379276637, error);
    OCIO_CHECK_CLOSE(rParam[OCIO::LOG_SIDE_OFFSET],  0.665689149560, error);
    OCIO_CHECK_CLOSE(rParam[OCIO::LIN_SIDE_SLOPE],   1.111637101285, error);
    OCIO_CHECK_CLOSE(rParam[OCIO::LIN_SIDE_OFFSET], -0.000473391157, error);

    auto & gParam = log->getGreenParams();
    OCIO_REQUIRE_EQUAL(gParam.size(), 4);
    OCIO_CHECK_CLOSE(gParam[OCIO::LOG_SIDE_SLOPE],  0.293255131964, error);
    OCIO_CHECK_CLOSE(gParam[OCIO::LOG_SIDE_OFFSET], 0.666666666667, error);
    OCIO_CHECK_CLOSE(gParam[OCIO::LIN_SIDE_SLOPE],  0.991514003046, error);
    OCIO_CHECK_CLOSE(gParam[OCIO::LIN_SIDE_OFFSET], 0.008485996954, error);

    auto & bParam = log->getBlueParams();
    OCIO_REQUIRE_EQUAL(bParam.size(), 4);
    OCIO_CHECK_CLOSE(bParam[OCIO::LOG_SIDE_SLOPE],  0.317693059628, error);
    OCIO_CHECK_CLOSE(bParam[OCIO::LOG_SIDE_OFFSET], 0.667644183773, error);
    OCIO_CHECK_CLOSE(bParam[OCIO::LIN_SIDE_SLOPE],  1.236287104632, error);
    OCIO_CHECK_CLOSE(bParam[OCIO::LIN_SIDE_OFFSET], 0.010970316295, error);
}

OCIO_ADD_TEST(FileFormatCTF, log_bad_style)
{
    std::string fileName("clf/illegal/log_bad_style.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(fileName), OCIO::Exception,
                          "is invalid");
}

OCIO_ADD_TEST(FileFormatCTF, log_bad_version)
{
    std::string fileName("clf/illegal/log_bad_version.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(fileName), OCIO::Exception,
                          "CLF file version '2' does not support operator 'Log'");
}

OCIO_ADD_TEST(FileFormatCTF, log_bad_param)
{
    std::string fileName("clf/illegal/log_bad_param.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(fileName), OCIO::Exception,
                          "Parameter 'linSideBreak' is only allowed for style");
}

OCIO_ADD_TEST(FileFormatCTF, log_missing_breakpnt)
{
    std::string fileName("clf/illegal/log_missing_breakpnt.clf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(fileName), OCIO::Exception,
                          "Parameter 'linSideBreak' should be defined for style");
}

OCIO_ADD_TEST(FileFormatCTF, log_ocio_params_channels)
{
    // NB: The blue channel is missing and will use default values.
    // Base can be specified in any channel but has to be specified.
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='2'>\n";
    strebuf << "<Log inBitDepth='10i' outBitDepth='16f' style='linToLog'>\n";
    strebuf << "<LogParams channel='R' linSideSlope='1.1' linSideOffset='0.1' logSideSlope='0.9' logSideOffset='0.2' base='10.0' />\n";
    strebuf << "<LogParams channel='G' logSideSlope='0.9' logSideOffset='0.23456' />\n";
    strebuf << "</Log>\n";
    strebuf << "</ProcessList>\n";

    OCIO::LocalCachedFileRcPtr cachedFile = ParseString(strebuf.str());
    OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    auto op = fileOps[0];
    auto log = std::dynamic_pointer_cast<const OCIO::LogOpData>(op);
    OCIO_REQUIRE_ASSERT(log);
    OCIO_CHECK_EQUAL(log->getBase(), 10.0);
    OCIO_CHECK_ASSERT(!log->allComponentsEqual());
    const auto & rParams = log->getRedParams();
    OCIO_CHECK_EQUAL(rParams[OCIO::LIN_SIDE_SLOPE], 1.1);
    OCIO_CHECK_EQUAL(rParams[OCIO::LIN_SIDE_OFFSET], 0.1);
    OCIO_CHECK_EQUAL(rParams[OCIO::LOG_SIDE_SLOPE], 0.9);
    OCIO_CHECK_EQUAL(rParams[OCIO::LOG_SIDE_OFFSET], 0.2);
    const auto & gParams = log->getGreenParams();
    OCIO_CHECK_EQUAL(gParams[OCIO::LIN_SIDE_SLOPE], 1.0);
    OCIO_CHECK_EQUAL(gParams[OCIO::LIN_SIDE_OFFSET], 0.0);
    OCIO_CHECK_EQUAL(gParams[OCIO::LOG_SIDE_SLOPE], 0.9);
    OCIO_CHECK_EQUAL(gParams[OCIO::LOG_SIDE_OFFSET], 0.23456);
    const auto & bParams = log->getBlueParams();
    OCIO_CHECK_EQUAL(bParams[OCIO::LIN_SIDE_SLOPE], 1.0);
    OCIO_CHECK_EQUAL(bParams[OCIO::LIN_SIDE_OFFSET], 0.0);
    OCIO_CHECK_EQUAL(bParams[OCIO::LOG_SIDE_SLOPE], 1.0);
    OCIO_CHECK_EQUAL(bParams[OCIO::LOG_SIDE_OFFSET], 0.0);
}

OCIO_ADD_TEST(FileFormatCTF, log_ocio_params_base_missmatch)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='2'>\n";
    strebuf << "<Log inBitDepth='32f' outBitDepth='32f' style='linToLog'>\n";
    strebuf << "<LogParams channel='R' linSideSlope='1.1' base='2.0'/>\n";
    strebuf << "<LogParams channel='G' linSideSlope='1.2' base='2.5'/>\n";
    strebuf << "</Log>\n";
    strebuf << "</ProcessList>\n";

    OCIO_CHECK_THROW_WHAT(ParseString(strebuf.str()), OCIO::Exception,
                          "base has to be the same");
}

OCIO_ADD_TEST(FileFormatCTF, log_default_params)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='2'>\n";
    strebuf << "<Log inBitDepth='32f' outBitDepth='32f' style='linToLog' />\n";
    strebuf << "<Log inBitDepth='32f' outBitDepth='32f' style='cameraLinToLog'>\n";
    strebuf << "<LogParams linSideBreak='0.1'/>\n";
    strebuf << "</Log>\n";
    strebuf << "</ProcessList>\n";

    OCIO::LocalCachedFileRcPtr cachedFile;
    OCIO_CHECK_NO_THROW(cachedFile = ParseString(strebuf.str()));
    OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(fileOps.size(), 2);
    auto op = fileOps[0];
    auto log = std::dynamic_pointer_cast<const OCIO::LogOpData>(op);
    OCIO_REQUIRE_ASSERT(log);
    // Default value is 2.
    OCIO_CHECK_EQUAL(log->getBase(), 2.0);
    const auto & redParams = log->getRedParams();
    OCIO_CHECK_EQUAL(redParams.size(), 4);
    OCIO_CHECK_EQUAL(redParams[OCIO::LIN_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(redParams[OCIO::LIN_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(redParams[OCIO::LOG_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(redParams[OCIO::LOG_SIDE_OFFSET], 0.);

    op = fileOps[1];
    log = std::dynamic_pointer_cast<const OCIO::LogOpData>(op);
    OCIO_REQUIRE_ASSERT(log);
    // Default value is 2.
    OCIO_CHECK_EQUAL(log->getBase(), 2.0);
    const auto & greenParams = log->getGreenParams();
    OCIO_CHECK_EQUAL(greenParams.size(), 5);
    OCIO_CHECK_EQUAL(greenParams[OCIO::LIN_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(greenParams[OCIO::LIN_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(greenParams[OCIO::LOG_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(greenParams[OCIO::LOG_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(greenParams[OCIO::LIN_SIDE_BREAK], 0.1);
}

OCIO_ADD_TEST(FileFormatCTF, multiple_ops)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("clf/multiple_ops.clf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 9);

    {   // Op 0 == CDL.
        auto cdlOpData = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[0]);
        OCIO_REQUIRE_ASSERT(cdlOpData);
        StringUtils::StringVec desc;
        GetElementsValues(cdlOpData->getFormatMetadata().getChildrenElements(),
                          OCIO::TAG_DESCRIPTION, desc);
        OCIO_REQUIRE_EQUAL(desc.size(), 1);
        OCIO_CHECK_EQUAL(desc[0], "scene 1 exterior look");
        OCIO_CHECK_EQUAL(cdlOpData->getStyle(), OCIO::CDLOpData::CDL_V1_2_REV);
        OCIO_CHECK_ASSERT(cdlOpData->getSlopeParams()
                          == OCIO::CDLOpData::ChannelParams(1.1, 1., 0.8));
        OCIO_CHECK_ASSERT(cdlOpData->getOffsetParams()
                          == OCIO::CDLOpData::ChannelParams(-0.01, 0., 0.05));
        OCIO_CHECK_ASSERT(cdlOpData->getPowerParams()
                          == OCIO::CDLOpData::ChannelParams(1.05, 1.15, 0.8));
        OCIO_CHECK_EQUAL(cdlOpData->getSaturation(), 0.85);
    }
    {   // Op 1 == Lut1D.
        auto l1OpData = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[1]);
        OCIO_REQUIRE_ASSERT(l1OpData);
        OCIO_CHECK_EQUAL(l1OpData->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        StringUtils::StringVec desc;
        GetElementsValues(l1OpData->getFormatMetadata().getChildrenElements(),
                          OCIO::TAG_DESCRIPTION, desc);
        OCIO_CHECK_EQUAL(desc.size(), 0);
        OCIO_CHECK_EQUAL(l1OpData->getArray().getLength(), 65u);
    }
    {   // Op 2 == Range.
        // Check that the noClamp style Range became a Matrix.
        auto matOpData = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[2]);
        OCIO_REQUIRE_ASSERT(matOpData);
        OCIO_CHECK_EQUAL(matOpData->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OCIO_CHECK_EQUAL(matOpData->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

        const double outScale = OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT10);
        const double matScale = outScale / OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT12);
        const OCIO::ArrayDouble & array = matOpData->getArray();
        OCIO_CHECK_EQUAL(array.getLength(), 4u);
        OCIO_CHECK_EQUAL(array.getNumColorComponents(), 4u);
        OCIO_CHECK_EQUAL(array.getNumValues(),
                         array.getLength()*array.getLength());

        const float scalef = (1200.f - 20.f) / (3760.f - 256.f);
        const float offsetf = 20.f - scalef * 256.f;
        const float prec = 10000.f;
        const int scale = (int)(prec * scalef);
        const int offset = (int)(prec * offsetf);

        OCIO_CHECK_ASSERT(matOpData->isDiagonal());

        // Check values on the diagonal.
        OCIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
        OCIO_CHECK_EQUAL((int)(prec * array.getValues()[0] * matScale), scale);
        OCIO_CHECK_EQUAL((int)(prec * array.getValues()[5] * matScale), scale);
        OCIO_CHECK_EQUAL((int)(prec * array.getValues()[10] * matScale), scale);
        OCIO_CHECK_EQUAL(array.getValues()[15], 1.0);

        // Check the offsets.
        const OCIO::MatrixOpData::Offsets & offsets = matOpData->getOffsets();
        OCIO_CHECK_EQUAL((int)(prec * offsets[0] * outScale), offset);
        OCIO_CHECK_EQUAL((int)(prec * offsets[1] * outScale), offset);
        OCIO_CHECK_EQUAL((int)(prec * offsets[2] * outScale), offset);
        OCIO_CHECK_EQUAL(offsets[3], 0.f);
    }
    {   // Op 3 == Range with Clamp.
        auto rangeOpData = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[3]);
        OCIO_REQUIRE_ASSERT(rangeOpData);
        OCIO_CHECK_EQUAL(rangeOpData->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OCIO_CHECK_EQUAL(rangeOpData->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    }
    {   // Op 4 == Range with Clamp.
        // A range without style defaults to clamp.
        auto rangeOpData = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[4]);
        OCIO_REQUIRE_ASSERT(rangeOpData);
        OCIO_CHECK_EQUAL(rangeOpData->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OCIO_CHECK_EQUAL(rangeOpData->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    }
    {   // Op 5 == Log.
        auto logOpData = std::dynamic_pointer_cast<const OCIO::LogOpData>(opList[5]);
        OCIO_REQUIRE_ASSERT(logOpData);
    }
    {   // Op 6 == Matrix with offset.
        auto matOpData2 = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[6]);
        OCIO_REQUIRE_ASSERT(matOpData2);
        const OCIO::ArrayDouble & array2 = matOpData2->getArray();
        OCIO_CHECK_EQUAL(array2.getLength(), 4u);
        OCIO_CHECK_EQUAL(array2.getValues()[2], -0.2);
        const OCIO::MatrixOpData::Offsets & offsets2 = matOpData2->getOffsets();
        OCIO_CHECK_EQUAL(offsets2[1], -0.11);
    }
    {   // Op 7 == Exponent.
        auto expOpData = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[7]);
        OCIO_REQUIRE_ASSERT(expOpData);
    }
    {   // Op 8 == Lut3D.
        auto lut3OpData = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opList[8]);
        OCIO_REQUIRE_ASSERT(lut3OpData);
    }
}

//
// NOTE: These tests are on the ReferenceOpData itself, before it gets replaced
// with the ops from the file it is referencing.  Please see RefereceOpData.cpp
// for tests involving the resolved ops.
//
OCIO_ADD_TEST(FileFormatCTF, reference_load_alias)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("reference_alias.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::ConstOpDataRcPtr op = fileOps[0];
    OCIO::ConstReferenceOpDataRcPtr ref = std::dynamic_pointer_cast<const OCIO::ReferenceOpData>(op);
    OCIO_REQUIRE_ASSERT(ref);
    OCIO_CHECK_EQUAL(ref->getName(), "name");
    OCIO_CHECK_EQUAL(ref->getID(), "uuid");
    OCIO_CHECK_EQUAL(ref->getReferenceStyle(), OCIO::REF_ALIAS);
    OCIO_CHECK_EQUAL(ref->getPath(), "");
    OCIO_CHECK_EQUAL(ref->getAlias(), "alias");
    OCIO_CHECK_EQUAL(ref->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
}

OCIO_ADD_TEST(FileFormatCTF, reference_load_path)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("reference_path_missing_file.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    
    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::ConstOpDataRcPtr op = fileOps[0];
    OCIO::ConstReferenceOpDataRcPtr ref = std::dynamic_pointer_cast<const OCIO::ReferenceOpData>(op);
    OCIO_REQUIRE_ASSERT(ref);
    OCIO_CHECK_EQUAL(ref->getReferenceStyle(), OCIO::REF_PATH);
    OCIO_CHECK_EQUAL(ref->getPath(), "toto/toto.ctf");
    OCIO_CHECK_EQUAL(ref->getAlias(), "");
    OCIO_CHECK_EQUAL(ref->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
}

OCIO_ADD_TEST(FileFormatCTF, reference_load_multiple)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    // File contains 2 references, 1 range and 1 reference.
    std::string fileName("references_some_inverted.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    
    OCIO_REQUIRE_EQUAL(fileOps.size(), 4);
    OCIO::ConstOpDataRcPtr op0 = fileOps[0];
    OCIO::ConstReferenceOpDataRcPtr ref0 = std::dynamic_pointer_cast<const OCIO::ReferenceOpData>(op0);
    OCIO_REQUIRE_ASSERT(ref0);
    OCIO_CHECK_EQUAL(ref0->getReferenceStyle(), OCIO::REF_PATH);
    OCIO_CHECK_EQUAL(ref0->getPath(), "matrix_example_1_3_offsets.ctf");
    OCIO_CHECK_EQUAL(ref0->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    
    OCIO::ConstOpDataRcPtr op1 = fileOps[1];
    OCIO::ConstReferenceOpDataRcPtr ref1 = std::dynamic_pointer_cast<const OCIO::ReferenceOpData>(op1);
    OCIO_REQUIRE_ASSERT(ref1);
    OCIO_CHECK_EQUAL(ref1->getReferenceStyle(), OCIO::REF_PATH);
    OCIO_CHECK_EQUAL(ref1->getPath(), "clf/xyz_to_rgb.clf");
    OCIO_CHECK_EQUAL(ref1->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::ConstOpDataRcPtr op2 = fileOps[2];
    OCIO::ConstRangeOpDataRcPtr range2 = std::dynamic_pointer_cast<const OCIO::RangeOpData>(op2);
    OCIO_REQUIRE_ASSERT(range2);

    OCIO::ConstOpDataRcPtr op3 = fileOps[3];
    OCIO::ConstReferenceOpDataRcPtr ref3 = std::dynamic_pointer_cast<const OCIO::ReferenceOpData>(op3);
    OCIO_REQUIRE_ASSERT(ref3);
    OCIO_CHECK_EQUAL(ref3->getReferenceStyle(), OCIO::REF_PATH);
    OCIO_CHECK_EQUAL(ref3->getPath(), "clf/cdl_clamp_fwd.clf");
    // Note: This tests that the "inverted" attribute set to anything other than
    // true does not result in an inverted transform.
    OCIO_CHECK_EQUAL(ref3->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
}

OCIO_ADD_TEST(FileFormatCTF, reference_load_path_utf8)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("reference_utf8.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::ConstOpDataRcPtr op = fileOps[0];
    OCIO::ConstReferenceOpDataRcPtr ref = std::dynamic_pointer_cast<const OCIO::ReferenceOpData>(op);
    OCIO_REQUIRE_ASSERT(ref);
    OCIO_CHECK_EQUAL(ref->getReferenceStyle(), OCIO::REF_PATH);
    OCIO_CHECK_EQUAL(ref->getPath(), "\xE6\xA8\x99\xE6\xBA\x96\xE8\x90\xAC\xE5\x9C\x8B\xE7\xA2\xBC");
    OCIO_CHECK_EQUAL(ref->getAlias(), "");
}

OCIO_ADD_TEST(FileFormatCTF, reference_load_alias_path)
{
    std::string fileName("reference_alias_path.ctf");
    // Can't have alias and path at the same time.
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(fileName), OCIO::Exception,
                          "alias & path attributes for Reference should not be both defined");
}

OCIO_ADD_TEST(FileFormatCTF, exposure_contrast_video)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("exposure_contrast_video.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT(cachedFile);
    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(opList.size(), 2);

    OCIO_REQUIRE_ASSERT(opList[0]);
    auto pEC = std::dynamic_pointer_cast<const OCIO::ExposureContrastOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pEC);

    OCIO_CHECK_EQUAL(pEC->getStyle(), OCIO::ExposureContrastOpData::STYLE_VIDEO);

    OCIO_CHECK_EQUAL(pEC->getExposure(), -1.0);
    OCIO_CHECK_EQUAL(pEC->getContrast(), 1.5);
    OCIO_CHECK_EQUAL(pEC->getPivot(), 0.5);

    OCIO_CHECK_ASSERT(pEC->isDynamic());
    OCIO_CHECK_ASSERT(pEC->getExposureProperty()->isDynamic());
    OCIO_CHECK_ASSERT(pEC->getContrastProperty()->isDynamic());
    OCIO_CHECK_ASSERT(!pEC->getGammaProperty()->isDynamic());

    OCIO_REQUIRE_ASSERT(opList[1]);
    auto pECRev = std::dynamic_pointer_cast<const OCIO::ExposureContrastOpData>(opList[1]);
    OCIO_REQUIRE_ASSERT(pECRev);
    OCIO_CHECK_ASSERT(!pECRev->isDynamic());

    OCIO_CHECK_EQUAL(pECRev->getStyle(), OCIO::ExposureContrastOpData::STYLE_VIDEO_REV);
}

OCIO_ADD_TEST(FileFormatCTF, exposure_contrast_log)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("exposure_contrast_log.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT(cachedFile);
    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(opList.size(), 2);

    OCIO_REQUIRE_ASSERT(opList[0]);
    auto pEC = std::dynamic_pointer_cast<const OCIO::ExposureContrastOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pEC);

    OCIO_CHECK_EQUAL(pEC->getStyle(), OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC);

    OCIO_CHECK_EQUAL(pEC->getExposure(), -1.5);
    OCIO_CHECK_EQUAL(pEC->getContrast(), 0.5);
    OCIO_CHECK_EQUAL(pEC->getGamma(), 1.2);
    OCIO_CHECK_EQUAL(pEC->getPivot(), 0.18);

    OCIO_CHECK_ASSERT(pEC->isDynamic());
    OCIO_CHECK_ASSERT(pEC->getExposureProperty()->isDynamic());
    OCIO_CHECK_ASSERT(pEC->getContrastProperty()->isDynamic());
    OCIO_CHECK_ASSERT(!pEC->getGammaProperty()->isDynamic());

    OCIO_REQUIRE_ASSERT(opList[1]);
    auto pECRev = std::dynamic_pointer_cast<const OCIO::ExposureContrastOpData>(opList[1]);
    OCIO_REQUIRE_ASSERT(pECRev);

    OCIO_CHECK_EQUAL(pECRev->getStyle(), OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC_REV);
    OCIO_CHECK_ASSERT(pECRev->isDynamic());
    OCIO_CHECK_ASSERT(!pECRev->getExposureProperty()->isDynamic());
    OCIO_CHECK_ASSERT(!pECRev->getContrastProperty()->isDynamic());
    OCIO_CHECK_ASSERT(pECRev->getGammaProperty()->isDynamic());
}

OCIO_ADD_TEST(FileFormatCTF, exposure_contrast_linear)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("exposure_contrast_linear.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT(cachedFile);
    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(opList.size(), 2);

    OCIO_REQUIRE_ASSERT(opList[0]);
    auto pEC = std::dynamic_pointer_cast<const OCIO::ExposureContrastOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pEC);

    OCIO_CHECK_EQUAL(pEC->getStyle(), OCIO::ExposureContrastOpData::STYLE_LINEAR);

    OCIO_CHECK_EQUAL(pEC->getExposure(), 0.65);
    OCIO_CHECK_EQUAL(pEC->getContrast(), 1.2);
    OCIO_CHECK_EQUAL(pEC->getGamma(), 0.5);
    OCIO_CHECK_EQUAL(pEC->getPivot(), 1.0);

    OCIO_CHECK_ASSERT(pEC->isDynamic());
    OCIO_CHECK_ASSERT(pEC->getExposureProperty()->isDynamic());
    OCIO_CHECK_ASSERT(pEC->getContrastProperty()->isDynamic());
    OCIO_CHECK_ASSERT(pEC->getGammaProperty()->isDynamic());

    OCIO_REQUIRE_ASSERT(opList[1]);
    auto pECRev = std::dynamic_pointer_cast<const OCIO::ExposureContrastOpData>(opList[1]);
    OCIO_REQUIRE_ASSERT(pECRev);

    OCIO_CHECK_EQUAL(pECRev->getStyle(), OCIO::ExposureContrastOpData::STYLE_LINEAR_REV);
    OCIO_CHECK_ASSERT(!pECRev->isDynamic());
    OCIO_CHECK_ASSERT(!pECRev->getExposureProperty()->isDynamic());
    OCIO_CHECK_ASSERT(!pECRev->getContrastProperty()->isDynamic());
    OCIO_CHECK_ASSERT(!pECRev->getGammaProperty()->isDynamic());
}

OCIO_ADD_TEST(FileFormatCTF, exposure_contrast_no_gamma) 
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("exposure_contrast_no_gamma.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OCIO_REQUIRE_ASSERT(cachedFile);
    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(opList.size(), 1);

    OCIO_REQUIRE_ASSERT(opList[0]);
    auto pEC = std::dynamic_pointer_cast<const OCIO::ExposureContrastOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(pEC);

    OCIO_CHECK_EQUAL(pEC->getStyle(), OCIO::ExposureContrastOpData::STYLE_VIDEO);

    OCIO_CHECK_EQUAL(pEC->getExposure(), 0.2);
    OCIO_CHECK_EQUAL(pEC->getContrast(), 0.65);
    OCIO_CHECK_EQUAL(pEC->getPivot(), 0.23);

    OCIO_CHECK_EQUAL(pEC->getGamma(), 1.0);

    OCIO_CHECK_ASSERT(!pEC->isDynamic());
    OCIO_CHECK_ASSERT(!pEC->getExposureProperty()->isDynamic());
    OCIO_CHECK_ASSERT(!pEC->getContrastProperty()->isDynamic());
    OCIO_CHECK_ASSERT(!pEC->getGammaProperty()->isDynamic());
}

OCIO_ADD_TEST(FileFormatCTF, exposure_contrast_failures)
{
    const std::string ecBadStyle("exposure_contrast_bad_style.ctf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ecBadStyle), OCIO::Exception,
                          "Unknown exposure contrast style");

    const std::string ecMissingParam("exposure_contrast_missing_param.ctf");
    OCIO_CHECK_THROW_WHAT(LoadCLFFile(ecMissingParam), OCIO::Exception,
                          "exposure missing");
}

OCIO_ADD_TEST(FileFormatCTF, attribute_float_parse_extra_values)
{
    // Test attribute float parsing will throw if extra values are present
    // (using E/C for this test).
    std::istringstream ctf;
    ctf.str(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList id="empty" version="1.7">
   <ExposureContrast inBitDepth="32f" outBitDepth="32f" style="log">
      <ECParams exposure="-1.5 1.2" contrast="0.5" gamma="1.2" pivot="0.18" />
   </ExposureContrast>
</ProcessList>
)");

    // Load file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO_CHECK_THROW_WHAT(tester.read(ctf, emptyString, OCIO::INTERP_DEFAULT), OCIO::Exception,
                          "Expecting 1 value, found 2 values");
}

OCIO_ADD_TEST(FileFormatCTF, attribute_float_parse_leading_spaces)
{
    // Test attribute float parsing will not fail if extra leading white space
    // is present (using E/C for this test).
    std::istringstream ctf;
    ctf.str(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList id="empty" version="1.7">
   <ExposureContrast inBitDepth="32f" outBitDepth="32f" style="log">
      <ECParams exposure="    -1.5 " contrast="0.5" gamma="1.2" pivot="0.18" />
   </ExposureContrast>
</ProcessList>
)");

    // Load file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr file;
    OCIO_CHECK_NO_THROW(file = tester.read(ctf, emptyString, OCIO::INTERP_DEFAULT));
    OCIO::LocalCachedFileRcPtr cachedFile = OCIO_DYNAMIC_POINTER_CAST<OCIO::LocalCachedFile>(file);
    const auto & fileOps = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    const auto op = fileOps[0];
    auto ec = std::dynamic_pointer_cast<const OCIO::ExposureContrastOpData>(op);
    OCIO_REQUIRE_ASSERT(ec);

    OCIO_CHECK_EQUAL(ec->getExposure(), -1.5);
}

OCIO_ADD_TEST(FileFormatCTF, load_deprecated_ops_file)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("deprecated_ops.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(fileOps.size(), 3);

    // Test ACES RedMod03 (deprecated) conversion to the modern representation.
    {
        auto op = fileOps[0];
        auto func = std::dynamic_pointer_cast<const OCIO::FixedFunctionOpData>(op);
        OCIO_REQUIRE_ASSERT(func);
        OCIO_CHECK_EQUAL(func->getStyle(), OCIO::FixedFunctionOpData::ACES_RED_MOD_03_INV);
        OCIO_CHECK_NO_THROW(func->validate());
        OCIO_CHECK_ASSERT(func->getParams().empty());
    }

    // Test ACES Surround (deprecated) conversion to the modern representation.
    {
        auto op = fileOps[1];
        auto func = std::dynamic_pointer_cast<const OCIO::FixedFunctionOpData>(op);
        OCIO_REQUIRE_ASSERT(func);
        OCIO_CHECK_EQUAL(func->getStyle(), OCIO::FixedFunctionOpData::REC2100_SURROUND_FWD);
        OCIO_CHECK_NO_THROW(func->validate());

        OCIO::FixedFunctionOpData::Params params;
        params.push_back(1.2);
        OCIO_CHECK_ASSERT(func->getParams() == params);
    }

    // Test Function (deprecated) conversion to the modern representation.
    {
        auto op = fileOps[2];
        auto func = std::dynamic_pointer_cast<const OCIO::FixedFunctionOpData>(op);
        OCIO_REQUIRE_ASSERT(func);
        OCIO_CHECK_EQUAL(func->getStyle(), OCIO::FixedFunctionOpData::HSV_TO_RGB);
        OCIO_CHECK_NO_THROW(func->validate());
        OCIO_CHECK_ASSERT(func->getParams().empty());
    }
}

OCIO_ADD_TEST(FileFormatCTF, load_fixed_function_file)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("fixed_function.ctf");
    OCIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(fileOps.size(), 2);

    // Test FixedFunction with the REC2100_SURROUND_FWD style.
    {
        auto op = fileOps[0];
        auto func = std::dynamic_pointer_cast<const OCIO::FixedFunctionOpData>(op);
        OCIO_REQUIRE_ASSERT(func);
        OCIO_CHECK_EQUAL(func->getStyle(), OCIO::FixedFunctionOpData::REC2100_SURROUND_FWD);
        OCIO_CHECK_NO_THROW(func->validate());

        OCIO::FixedFunctionOpData::Params params;
        params.push_back(0.8);
        OCIO_CHECK_ASSERT(func->getParams() == params);       
    }

    // Test FixedFunction with the HSV_to_RGB style.
    {
        auto op = fileOps[1];
        auto func = std::dynamic_pointer_cast<const OCIO::FixedFunctionOpData>(op);
        OCIO_REQUIRE_ASSERT(func);
        OCIO_CHECK_EQUAL(func->getStyle(), OCIO::FixedFunctionOpData::HSV_TO_RGB);
        OCIO_CHECK_NO_THROW(func->validate());
        OCIO_CHECK_ASSERT(func->getParams().empty());
    }

}

namespace
{
void WriteGroupCTF(OCIO::ConstGroupTransformRcPtr group, std::ostringstream & outputTransform)
{
    OCIO::ConstConfigRcPtr cfg = OCIO::Config::CreateRaw();
    group->write(cfg, OCIO::FILEFORMAT_CTF, outputTransform);
}

void WriteGroupCLF(OCIO::ConstGroupTransformRcPtr group, std::ostringstream & outputTransform)
{
    OCIO::ConstConfigRcPtr cfg = OCIO::Config::CreateRaw();
    group->write(cfg, OCIO::FILEFORMAT_CLF, outputTransform);
}

void ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::Style style,
                                       const std::string & vers, int lineNo)
{
    // Validate the load & save for any FixedFunction style without parameters.

    std::ostringstream ffStr;
    ffStr << "<FixedFunction inBitDepth=\"32f\" outBitDepth=\"32f\" style=\""
          << OCIO::FixedFunctionOpData::ConvertStyleToString(style, false) << "\">";

    std::ostringstream strebuf;
    strebuf << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            << "<ProcessList version=\"" << vers << "\" id=\"ABCD\">\n"
            << "    " << ffStr.str() << "\n"
            << "    </FixedFunction>\n"
            << "</ProcessList>\n";

    OCIO::LocalCachedFileRcPtr cachedFile;
    OCIO_CHECK_NO_THROW_FROM(cachedFile = ParseString(strebuf.str()), lineNo);
    OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL_FROM(fileOps.size(), 1, lineNo);
    auto opData = fileOps[0];
    auto ffOpData = std::dynamic_pointer_cast<const OCIO::FixedFunctionOpData>(opData);
    OCIO_REQUIRE_ASSERT_FROM(ffOpData, lineNo);
    OCIO_CHECK_EQUAL_FROM(ffOpData->getStyle(), style, lineNo);

    auto clonedOpData = ffOpData->clone();
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW_FROM(
        OCIO::CreateFixedFunctionOp(ops, clonedOpData, OCIO::TRANSFORM_DIR_FORWARD), lineNo);
    OCIO_REQUIRE_EQUAL_FROM(ops.size(), 1, lineNo);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "ABCD");
    OCIO::ConstOpRcPtr constOp = ops[0];
    OCIO_CHECK_NO_THROW_FROM(OCIO::CreateFixedFunctionTransform(group, constOp), lineNo);
    OCIO_REQUIRE_EQUAL_FROM(group->getNumTransforms(), 1, lineNo);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW_FROM(WriteGroupCTF(group, outputTransform), lineNo);

    if (outputTransform.str() != strebuf.str())
    {
        std::ostringstream err;
        err << "Expected is: \n"
            << strebuf.str()
            << "where output is: \n"
            << outputTransform.str();

        OCIO_CHECK_ASSERT_MESSAGE_FROM(0, err.str(), lineNo);
    }
}
}

OCIO_ADD_TEST(FileFormatCTF, ff_load_save_ctf)
{
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_RED_MOD_03_FWD, "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_RED_MOD_03_INV, "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD, "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_RED_MOD_10_INV, "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD   , "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_GLOW_03_INV   , "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_GLOW_10_FWD   , "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_GLOW_10_INV   , "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD, "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV, "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::RGB_TO_HSV         , "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::HSV_TO_RGB         , "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::XYZ_TO_xyY         , "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::xyY_TO_XYZ         , "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::XYZ_TO_uvY         , "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::uvY_TO_XYZ         , "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::XYZ_TO_LUV         , "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::LUV_TO_XYZ         , "2", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::PQ_TO_LIN          , "2.4", __LINE__);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::LIN_TO_PQ          , "2.4", __LINE__);
}

OCIO_ADD_TEST(FileFormatCTF, load_ff_fail_version)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='1.5'>\n";
    strebuf << "    <FixedFunction inBitDepth='8i' outBitDepth='32f' ";
    strebuf << "params = '0.8' ";
    strebuf << "style = 'Rec2100SurroundFwd' />\n";
    strebuf << "</ProcessList>\n";

    OCIO_CHECK_THROW_WHAT(ParseString(strebuf.str()), OCIO::Exception,
                          "CTF file version '1.5' does not support operator 'FixedFunction'");
}

OCIO_ADD_TEST(FileFormatCTF, load_ff_fail_params)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='2'>\n";
    strebuf << "    <FixedFunction inBitDepth='8i' outBitDepth='32f' ";
    strebuf << "params = '0.8 2.0' ";
    strebuf << "style = 'Rec2100SurroundFwd' />\n";
    strebuf << "</ProcessList>\n";

    OCIO_CHECK_THROW_WHAT(ParseString(strebuf.str()), OCIO::Exception,
                          "must have one parameter but 2 found");
}

OCIO_ADD_TEST(FileFormatCTF, load_ff_fail_style)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='2.0'>\n";
    strebuf << "    <FixedFunction inBitDepth='16i' outBitDepth='32f' style='UnknownStyle' />\n";
    strebuf << "</ProcessList>\n";

    OCIO_CHECK_THROW_WHAT(ParseString(strebuf.str()), OCIO::Exception,
                          "Unknown FixedFunction style");
}

OCIO_ADD_TEST(FileFormatCTF, load_ff_aces_fail_gamma_param)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='1.5'>\n";
    strebuf << "    <ACES inBitDepth='16i' outBitDepth='32f' style='Surround'>\n";
    strebuf << "        <ACESParams wrongParam='1.2' />\n";
    strebuf << "    </ACES>\n";
    strebuf << "</ProcessList>\n";

    OCIO::LogGuard guard;
    OCIO_CHECK_THROW_WHAT(ParseString(strebuf.str()), OCIO::Exception,
                          "Missing required parameter");
}

OCIO_ADD_TEST(FileFormatCTF, load_ff_aces_fail_gamma_twice)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='1.5'>\n";
    strebuf << "    <ACES inBitDepth='16i' outBitDepth='32f' style='Surround'>\n";
    strebuf << "        <ACESParams gamma='1.2' />\n";
    strebuf << "        <ACESParams gamma='1.4' />\n";
    strebuf << "    </ACES>\n";
    strebuf << "</ProcessList>\n";

    OCIO_CHECK_THROW_WHAT(ParseString(strebuf.str()), OCIO::Exception,
                          "only 1 gamma parameter");
}

OCIO_ADD_TEST(FileFormatCTF, load_ff_aces_fail_missing_param)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='1.5'>\n";
    strebuf << "    <ACES inBitDepth='16i' outBitDepth='32f' style='Surround'>\n";
    strebuf << "    </ACES>\n";
    strebuf << "</ProcessList>\n";

    OCIO_CHECK_THROW_WHAT(ParseString(strebuf.str()), OCIO::Exception,
                          "must have one parameter");
}

OCIO_ADD_TEST(FileFormatCTF, load_grading_primary_log)
{
    std::istringstream ctfLog;
    ctfLog.str(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingPrimary inBitDepth="32f" outBitDepth="32f" style="log">
        <Brightness rgb="0.1 0 0" master="0" />
        <Contrast rgb="1 1.1 1" master="1" />
        <Gamma rgb="1 1 1" master="1.1" />
        <Saturation master="1.1" />
        <Pivot contrast="-0.1" black="0.1" white="1.1" />
        <Clamp black="0" white="1" />
    </GradingPrimary>
    <GradingPrimary inBitDepth="32f" outBitDepth="32f" style="logRev">
        <DynamicParameter param="PRIMARY" />
    </GradingPrimary>
    <GradingPrimary inBitDepth="32f" outBitDepth="16f" style="log">
        <Brightness rgb="0.1 0 0" master="0" />
        <Gamma rgb="1 1 1" master="1.1" />
        <Saturation master="1.1" />
        <Pivot black="0.1" />
        <Clamp white="1" />
    </GradingPrimary>
    <GradingPrimary inBitDepth="16f" outBitDepth="32f" style="log">
        <Brightness rgb="-0.25 0.5 7.62939453125e-06" master="1" />
        <Contrast rgb="1.25 1.5 -1.25" master="103125e-5" />
        <Gamma rgb="0.75 0.625 1.25" master="0.9e+1" />
        <Pivot contrast="0.75" />
        <Saturation master="1.03125" />
        <DynamicParameter param="PRIMARY" />
    </GradingPrimary>
</ProcessList>
)");

    // Load file.
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr file;
    OCIO_CHECK_NO_THROW(file = tester.read(ctfLog, emptyString, OCIO::INTERP_DEFAULT));
    OCIO::LocalCachedFileRcPtr cachedFile = OCIO_DYNAMIC_POINTER_CAST<OCIO::LocalCachedFile>(file);
    OCIO_REQUIRE_ASSERT(cachedFile);
    const auto & fileOps = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(fileOps.size(), 4);
    const auto op0 = fileOps[0];
    const auto gradingPrimary0 = std::dynamic_pointer_cast<const OCIO::GradingPrimaryOpData>(op0);
    OCIO_REQUIRE_ASSERT(gradingPrimary0);
    OCIO_CHECK_EQUAL(OCIO::GRADING_LOG, gradingPrimary0->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, gradingPrimary0->getDirection());
    const auto vals0 = gradingPrimary0->getValue();
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(0.1, 0., 0., 0.), vals0.m_brightness);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(1., 1.1, 1., 1.), vals0.m_contrast);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(1., 1., 1., 1.1), vals0.m_gamma);
    OCIO_CHECK_EQUAL(1.1, vals0.m_saturation);
    OCIO_CHECK_EQUAL(-0.1, vals0.m_pivot);
    OCIO_CHECK_EQUAL(0.1, vals0.m_pivotBlack);
    OCIO_CHECK_EQUAL(1.1, vals0.m_pivotWhite);
    OCIO_CHECK_EQUAL(0., vals0.m_clampBlack);
    OCIO_CHECK_EQUAL(1., vals0.m_clampWhite);
    OCIO_CHECK_ASSERT(!gradingPrimary0->isDynamic());

    const auto op1 = fileOps[1];
    const auto gradingPrimary1 = std::dynamic_pointer_cast<const OCIO::GradingPrimaryOpData>(op1);
    OCIO_REQUIRE_ASSERT(gradingPrimary1);
    OCIO_CHECK_EQUAL(OCIO::GRADING_LOG, gradingPrimary1->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, gradingPrimary1->getDirection());
    const OCIO::GradingPrimary defLog{ OCIO::GRADING_LOG };
    OCIO_CHECK_EQUAL(defLog, gradingPrimary1->getValue());
    OCIO_CHECK_ASSERT(gradingPrimary1->isDynamic());

    const auto op2 = fileOps[2];
    const auto gradingPrimary2 = std::dynamic_pointer_cast<const OCIO::GradingPrimaryOpData>(op2);
    OCIO_REQUIRE_ASSERT(gradingPrimary2);
    OCIO_CHECK_EQUAL(OCIO::GRADING_LOG, gradingPrimary2->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, gradingPrimary2->getDirection());
    const auto vals2 = gradingPrimary2->getValue();
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(0.1, 0., 0., 0.), vals2.m_brightness);
    OCIO_CHECK_EQUAL(defLog.m_contrast, vals2.m_contrast);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(1., 1., 1., 1.1), vals2.m_gamma);
    OCIO_CHECK_EQUAL(1.1, vals2.m_saturation);
    OCIO_CHECK_EQUAL(defLog.m_pivot, vals2.m_pivot);
    OCIO_CHECK_EQUAL(0.1, vals2.m_pivotBlack);
    OCIO_CHECK_EQUAL(defLog.m_pivotWhite, vals2.m_pivotWhite);
    OCIO_CHECK_EQUAL(defLog.m_clampBlack, vals2.m_clampBlack);
    OCIO_CHECK_EQUAL(1., vals2.m_clampWhite);
    OCIO_CHECK_ASSERT(!gradingPrimary2->isDynamic());

    const auto op3 = fileOps[3];
    const auto gradingPrimary3 = std::dynamic_pointer_cast<const OCIO::GradingPrimaryOpData>(op3);
    OCIO_REQUIRE_ASSERT(gradingPrimary3);
    OCIO_CHECK_EQUAL(OCIO::GRADING_LOG, gradingPrimary3->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, gradingPrimary3->getDirection());
    const auto vals3 = gradingPrimary3->getValue();
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(-0.25, 0.5, 7.62939453125e-06, 1.), vals3.m_brightness);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(1.25, 1.5, -1.25, 103125.e-5), vals3.m_contrast);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(0.75, 0.625, 1.25, 0.9e+1), vals3.m_gamma);
    OCIO_CHECK_EQUAL(1.03125, vals3.m_saturation);
    OCIO_CHECK_EQUAL(0.75, vals3.m_pivot);
    OCIO_CHECK_EQUAL(defLog.m_pivotBlack, vals3.m_pivotBlack);
    OCIO_CHECK_EQUAL(defLog.m_pivotWhite, vals3.m_pivotWhite);
    OCIO_CHECK_EQUAL(defLog.m_clampBlack, vals3.m_clampBlack);
    OCIO_CHECK_EQUAL(defLog.m_clampWhite, vals3.m_clampWhite);
    OCIO_CHECK_ASSERT(gradingPrimary3->isDynamic());
}

OCIO_ADD_TEST(FileFormatCTF, load_grading_primary_lin)
{
    std::istringstream ctfLin;
    ctfLin.str(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingPrimary inBitDepth="32f" outBitDepth="32f" style="linear">
        <Offset rgb="0.1 0 0" master="0" />
        <Exposure rgb="0 0 0" master="0.1" />
        <Contrast rgb="1 1.1 1" master="1" />
        <Saturation master="1.1" />
        <Pivot contrast="0.28" />
        <Clamp black="0" white="1" />
    </GradingPrimary>
    <GradingPrimary inBitDepth="32f" outBitDepth="32f" style="linearRev">
        <DynamicParameter param="PRIMARY" />
    </GradingPrimary>
    <GradingPrimary inBitDepth="32f" outBitDepth="16f" style="linear">
        <Offset rgb="0.1 0 0" master="0" />
        <Exposure rgb="0 0 0" master="0.1" />
        <Saturation master="1.1" />
        <Clamp white="1" />
    </GradingPrimary>
    <GradingPrimary inBitDepth="16f" outBitDepth="32f" style="linear">
        <Offset rgb="0.25 -0.015625 +.03125" master="0" />
        <Exposure rgb="-0.25 0.5 7.62939453125e-06" master="1" />
        <Contrast rgb="0.75 0.625 1.25" master=".9e+1" />
        <Pivot contrast="1.25" />
        <Saturation master="1.125" />
        <DynamicParameter param="PRIMARY" />
    </GradingPrimary>
</ProcessList>
)");

    // Load file.
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr file;
    OCIO_CHECK_NO_THROW(file = tester.read(ctfLin, emptyString, OCIO::INTERP_DEFAULT));
    OCIO::LocalCachedFileRcPtr cachedFile = OCIO_DYNAMIC_POINTER_CAST<OCIO::LocalCachedFile>(file);
    OCIO_REQUIRE_ASSERT(cachedFile);
    const auto & fileOps = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(fileOps.size(), 4);
    const auto op0 = fileOps[0];
    const auto gradingPrimary0 = std::dynamic_pointer_cast<const OCIO::GradingPrimaryOpData>(op0);
    OCIO_REQUIRE_ASSERT(gradingPrimary0);
    OCIO_CHECK_EQUAL(OCIO::GRADING_LIN, gradingPrimary0->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, gradingPrimary0->getDirection());
    const auto vals0 = gradingPrimary0->getValue();
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(0.1, 0., 0., 0.), vals0.m_offset);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(0., 0., 0., 0.1), vals0.m_exposure);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(1., 1.1, 1., 1.), vals0.m_contrast);
    OCIO_CHECK_EQUAL(1.1, vals0.m_saturation);
    OCIO_CHECK_EQUAL(0.28, vals0.m_pivot);
    OCIO_CHECK_EQUAL(0., vals0.m_clampBlack);
    OCIO_CHECK_EQUAL(1., vals0.m_clampWhite);
    OCIO_CHECK_ASSERT(!gradingPrimary0->isDynamic());

    const auto op1 = fileOps[1];
    const auto gradingPrimary1 = std::dynamic_pointer_cast<const OCIO::GradingPrimaryOpData>(op1);
    OCIO_REQUIRE_ASSERT(gradingPrimary1);
    OCIO_CHECK_EQUAL(OCIO::GRADING_LIN, gradingPrimary1->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, gradingPrimary1->getDirection());
    const OCIO::GradingPrimary defLin{ OCIO::GRADING_LIN };
    OCIO_CHECK_EQUAL(defLin, gradingPrimary1->getValue());
    OCIO_CHECK_ASSERT(gradingPrimary1->isDynamic());

    const auto op2 = fileOps[2];
    const auto gradingPrimary2 = std::dynamic_pointer_cast<const OCIO::GradingPrimaryOpData>(op2);
    OCIO_REQUIRE_ASSERT(gradingPrimary2);
    OCIO_CHECK_EQUAL(OCIO::GRADING_LIN, gradingPrimary2->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, gradingPrimary2->getDirection());
    const auto vals2 = gradingPrimary2->getValue();
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(0.1, 0., 0., 0.), vals0.m_offset);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(0., 0., 0., 0.1), vals0.m_exposure);
    OCIO_CHECK_EQUAL(defLin.m_contrast, vals2.m_contrast);
    OCIO_CHECK_EQUAL(1.1, vals2.m_saturation);
    OCIO_CHECK_EQUAL(defLin.m_pivot, vals2.m_pivot);
    OCIO_CHECK_EQUAL(defLin.m_clampBlack, vals2.m_clampBlack);
    OCIO_CHECK_EQUAL(1., vals2.m_clampWhite);
    OCIO_CHECK_ASSERT(!gradingPrimary2->isDynamic());

    const auto op3 = fileOps[3];
    const auto gradingPrimary3 = std::dynamic_pointer_cast<const OCIO::GradingPrimaryOpData>(op3);
    OCIO_REQUIRE_ASSERT(gradingPrimary3);
    OCIO_CHECK_EQUAL(OCIO::GRADING_LIN, gradingPrimary3->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, gradingPrimary3->getDirection());
    const auto vals3 = gradingPrimary3->getValue();
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(0.25, -0.015625, +.03125, 0.), vals3.m_offset);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(-0.25, 0.5, 7.62939453125e-06, 1.), vals3.m_exposure);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(0.75, 0.625, 1.25, 0.9e+1), vals3.m_contrast);
    OCIO_CHECK_EQUAL(1.125, vals3.m_saturation);
    OCIO_CHECK_EQUAL(1.25, vals3.m_pivot);
    OCIO_CHECK_EQUAL(defLin.m_clampBlack, vals3.m_clampBlack);
    OCIO_CHECK_EQUAL(defLin.m_clampWhite, vals3.m_clampWhite);
    OCIO_CHECK_ASSERT(gradingPrimary3->isDynamic());
}

OCIO_ADD_TEST(FileFormatCTF, load_grading_primary_video)
{
    std::istringstream ctfVideo;
    ctfVideo.str(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingPrimary inBitDepth="32f" outBitDepth="32f" style="video">
        <Lift rgb="0.1 0 0" master="0" />
        <Gamma rgb="1 1 1" master="1.1" />
        <Gain rgb="1 1.1 1" master="1" />
        <Offset rgb="0 0.1 0" master="0" />
        <Saturation master="1.1" />
        <Pivot black="0.1" white="1.1" />
        <Clamp black="0" white="1" />
    </GradingPrimary>
    <GradingPrimary inBitDepth="32f" outBitDepth="32f" style="videoRev">
        <DynamicParameter param="PRIMARY" />
    </GradingPrimary>
    <GradingPrimary inBitDepth="32f" outBitDepth="16f" style="video">
        <Lift rgb="0.1 0 0" master="0" />
        <Gain rgb="1 1.1 1" master="1" />
        <Pivot black="0.1" />
        <Clamp white="1" />
    </GradingPrimary>
    <GradingPrimary inBitDepth="16f" outBitDepth="32f" style="video">
        <Lift rgb="0.25 -.015625 .03125" master="0" />
        <Gamma rgb=".75 .625 1.25" master=".09e2" />
        <Gain rgb="-0.25 00.500 7.62939453125e-06" master="1" />
        <Offset rgb="02.5 +0.5 -.125" master="1" />
        <Pivot black="-0.25" white="12" />
        <Saturation master="1" />
        <Clamp black="0.5" white="1.5" />
        <DynamicParameter param="PRIMARY" />
    </GradingPrimary>
</ProcessList>
)");

    // Load file.
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr file;
    OCIO_CHECK_NO_THROW(file = tester.read(ctfVideo, emptyString, OCIO::INTERP_DEFAULT));
    OCIO::LocalCachedFileRcPtr cachedFile = OCIO_DYNAMIC_POINTER_CAST<OCIO::LocalCachedFile>(file);
    OCIO_REQUIRE_ASSERT(cachedFile);
    const auto & fileOps = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(fileOps.size(), 4);
    const auto op0 = fileOps[0];
    const auto gradingPrimary0 = std::dynamic_pointer_cast<const OCIO::GradingPrimaryOpData>(op0);
    OCIO_REQUIRE_ASSERT(gradingPrimary0);
    OCIO_CHECK_EQUAL(OCIO::GRADING_VIDEO, gradingPrimary0->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, gradingPrimary0->getDirection());
    const auto vals0 = gradingPrimary0->getValue();
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(0.1, 0., 0., 0.), vals0.m_lift);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(1., 1., 1., 1.1), vals0.m_gamma);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(1., 1.1, 1., 1.), vals0.m_gain);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(0., 0.1, 0., 0.), vals0.m_offset);
    OCIO_CHECK_EQUAL(1.1, vals0.m_saturation);
    OCIO_CHECK_EQUAL(0.1, vals0.m_pivotBlack);
    OCIO_CHECK_EQUAL(1.1, vals0.m_pivotWhite);
    OCIO_CHECK_EQUAL(0., vals0.m_clampBlack);
    OCIO_CHECK_EQUAL(1., vals0.m_clampWhite);
    OCIO_CHECK_ASSERT(!gradingPrimary0->isDynamic());

    const auto op1 = fileOps[1];
    const auto gradingPrimary1 = std::dynamic_pointer_cast<const OCIO::GradingPrimaryOpData>(op1);
    OCIO_REQUIRE_ASSERT(gradingPrimary1);
    OCIO_CHECK_EQUAL(OCIO::GRADING_VIDEO, gradingPrimary1->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, gradingPrimary1->getDirection());
    const OCIO::GradingPrimary defVideo{ OCIO::GRADING_VIDEO };
    OCIO_CHECK_EQUAL(defVideo, gradingPrimary1->getValue());
    OCIO_CHECK_ASSERT(gradingPrimary1->isDynamic());

    const auto op2 = fileOps[2];
    const auto gradingPrimary2 = std::dynamic_pointer_cast<const OCIO::GradingPrimaryOpData>(op2);
    OCIO_REQUIRE_ASSERT(gradingPrimary2);
    OCIO_CHECK_EQUAL(OCIO::GRADING_VIDEO, gradingPrimary2->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, gradingPrimary2->getDirection());
    const auto vals2 = gradingPrimary2->getValue();
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(0.1, 0., 0., 0.), vals2.m_lift);
    OCIO_CHECK_EQUAL(defVideo.m_gamma, vals2.m_gamma);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(1., 1.1, 1., 1.), vals2.m_gain);
    OCIO_CHECK_EQUAL(defVideo.m_offset, vals2.m_offset);
    OCIO_CHECK_EQUAL(defVideo.m_saturation, vals2.m_saturation);
    OCIO_CHECK_EQUAL(0.1, vals2.m_pivotBlack);
    OCIO_CHECK_EQUAL(defVideo.m_pivotWhite, vals2.m_pivotWhite);
    OCIO_CHECK_EQUAL(defVideo.m_clampBlack, vals2.m_clampBlack);
    OCIO_CHECK_EQUAL(1., vals2.m_clampWhite);

    const auto op3 = fileOps[3];
    const auto gradingPrimary3 = std::dynamic_pointer_cast<const OCIO::GradingPrimaryOpData>(op3);
    OCIO_REQUIRE_ASSERT(gradingPrimary3);
    OCIO_CHECK_EQUAL(OCIO::GRADING_VIDEO, gradingPrimary3->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, gradingPrimary3->getDirection());
    const auto vals3 = gradingPrimary3->getValue();
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(0.25, -0.015625, 0.03125, 0.), vals3.m_lift);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(0.75, 0.625, 1.25, 0.9e+1), vals3.m_gamma);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(-0.25, 0.500, 7.62939453125e-06, 1.), vals3.m_gain);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBM(02.5, +0.5, -.125, 1.), vals3.m_offset);
    OCIO_CHECK_EQUAL(1., vals3.m_saturation);
    OCIO_CHECK_EQUAL(-0.25, vals3.m_pivotBlack);
    OCIO_CHECK_EQUAL(12., vals3.m_pivotWhite);
    OCIO_CHECK_EQUAL(0.5, vals3.m_clampBlack);
    OCIO_CHECK_EQUAL(1.5, vals3.m_clampWhite);
    OCIO_CHECK_ASSERT(gradingPrimary3->isDynamic());
}

OCIO_ADD_TEST(FileFormatCTF, load_grading_primary_errors)
{
    // Wrong version.
    OCIO_CHECK_THROW_WHAT(ParseString(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.8" id="empty">
  <GradingPrimary inBitDepth="16f" outBitDepth="32f" style="video">
      <DynamicParameter param="PRIMARY" />
   </GradingPrimary>
</ProcessList>
)"), OCIO::Exception, "CTF file version '1.8' does not support operator 'GradingPrimary'");

    // Master attribute for pivot instead of contrast.
    OCIO_CHECK_THROW_WHAT(ParseString(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="empty">
  <GradingPrimary inBitDepth="16f" outBitDepth="32f" style="video">
      <Lift rgb="0.25 0.5 0.25" master="1" />
      <Gamma rgb="1.0 1.0 1.0" master="1" />
      <Gain rgb="0.25 0.5 0.25" master="1" />
      <Offset rgb="0.25 0.5 0.25" master="1" />
      <Saturation master="1" />
      <Pivot master="1">
      <DynamicParameter param="PRIMARY" />
   </GradingPrimary>
</ProcessList>
)"), OCIO::Exception, "Illegal attribute for 'Pivot': 'master'");

    // Brightness does not have 3 rgb values.
    OCIO_CHECK_THROW_WHAT(ParseString(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="empty">
  <GradingPrimary inBitDepth="16f" outBitDepth="32f" style="log">
      <Brightness rgb="0.25 0.5" master="1" />
      <Contrast rgb="0.25 0.5 0.25" master="1" />
      <Gamma rgb="0.25 0.5 0.25" master="1" />
      <Pivot contrast="1" />
      <Saturation master="1" />
      <DynamicParameter param="PRIMARY" />
   </GradingPrimary>
</ProcessList>
)"), OCIO::Exception, "Illegal number of 'rgb' values for 'Brightness'");

    // Gamma values should be above lower bound.
    OCIO_CHECK_THROW_WHAT(ParseString(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="empty">
  <GradingPrimary inBitDepth="16f" outBitDepth="32f" style="video">
      <Lift rgb="0.25 0.5 0.25" master="1" />
      <Gamma rgb="0.0 0.0 0.0" master="1" />
      <Gain rgb="0.25 0.5 0.25" master="1" />
      <Offset rgb="0.25 0.5 0.25" master="1" />
      <Saturation master="1" />
      <DynamicParameter param="PRIMARY" />
   </GradingPrimary>
</ProcessList>
)"), OCIO::Exception, "GradingPrimary gamma '<r=0, g=0, b=0, m=1>' are below lower bound (0.01)");

    // Brightness does not have a master value.
    OCIO_CHECK_THROW_WHAT(ParseString(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="empty">
  <GradingPrimary inBitDepth="16f" outBitDepth="32f" style="log">
      <Brightness rgb="0.25 0.5 0.25" />
      <Contrast rgb="0.25 0.5 0.25" master="1" />
      <Gamma rgb="0.25 0.5 0.25" master="1" />
      <Pivot contrast="1" />
      <Saturation master="1" />
      <DynamicParameter param="PRIMARY" />
   </GradingPrimary>
</ProcessList>
)"), OCIO::Exception, "Missing 'master' attribute for 'Brightness'");

    // Missing style attribute.
    OCIO_CHECK_THROW_WHAT(ParseString(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="empty">
   <GradingPrimary inBitDepth="16f" outBitDepth="32f">
      <Brightness rgb="0.25 0.5 0.25" master=".1" />
      <Contrast rgb="0.25 0.5 0.25" master="1" />
      <Gamma rgb="0.25 0.5 0.25" master="1" />
      <Pivot contrast="1" />
      <Saturation master="1" />
      <DynamicParameter param="PRIMARY" />
   </GradingPrimary>
</ProcessList>
)"), OCIO::Exception, "Required attribute 'style' is missing");

    // Unsupported dynamic parameter for GradingPrimary.
    OCIO_CHECK_THROW_WHAT(ParseString(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="empty">
   <GradingPrimary inBitDepth="16f" outBitDepth="32f" style="video">
      <DynamicParameter param="CONTRAST" />
   </GradingPrimary>
</ProcessList>
)"), OCIO::Exception, "Dynamic parameter 'CONTRAST' is not supported in 'GradingPrimary'");
}

OCIO_ADD_TEST(FileFormatCTF, load_grading_curves_lin)
{
    std::istringstream ctfLin;
    ctfLin.str(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDGradingCurves">
    <GradingRGBCurve inBitDepth="32f" outBitDepth="32f" style="linear">
        <Red>
            <ControlPoints>
                         -7 -6
                          0 0
                          7 7
            </ControlPoints>
        </Red>
        <Master>
            <ControlPoints>
                         -7 -7
                          0 0
                          7 7
                         16 10
            </ControlPoints>
        </Master>
    </GradingRGBCurve>
    <GradingRGBCurve inBitDepth="32f" outBitDepth="32f" style="linear" bypassLinToLog="true">
        <DynamicParameter param="RGB_CURVE" />
    </GradingRGBCurve>
    <GradingRGBCurve inBitDepth="32f" outBitDepth="32f" style="video">
        <Master>
            <ControlPoints>
                          0 0 1 1
                        1.5 1.4
            </ControlPoints>
            <Slopes> 0 0.7 1.1 </Slopes>
        </Master>
    </GradingRGBCurve>
</ProcessList>
)");

    // Load file.
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr file;
    OCIO_CHECK_NO_THROW(file = tester.read(ctfLin, emptyString, OCIO::INTERP_DEFAULT));
    OCIO::LocalCachedFileRcPtr cachedFile = OCIO_DYNAMIC_POINTER_CAST<OCIO::LocalCachedFile>(file);
    OCIO_REQUIRE_ASSERT(cachedFile);
    const auto & fileOps = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(fileOps.size(), 3);
    const auto op0 = fileOps[0];
    const auto gradingCurves0 = std::dynamic_pointer_cast<const OCIO::GradingRGBCurveOpData>(op0);
    OCIO_REQUIRE_ASSERT(gradingCurves0);
    OCIO_CHECK_EQUAL(gradingCurves0->getStyle(), OCIO::GRADING_LIN);
    OCIO_CHECK_ASSERT(!gradingCurves0->getBypassLinToLog());
    OCIO_CHECK_ASSERT(!gradingCurves0->isDynamic());
    auto curves = gradingCurves0->getValue();
    OCIO_CHECK_EQUAL(curves->getCurve(OCIO::RGB_MASTER)->getNumControlPoints(), 4);
    OCIO_CHECK_ASSERT(*curves->getCurve(OCIO::RGB_BLUE) ==
                      OCIO::GradingRGBCurveImpl::DefaultLin);
    OCIO_CHECK_ASSERT(*curves->getCurve(OCIO::RGB_RED) !=
                      OCIO::GradingRGBCurveImpl::DefaultLin);

    const auto op1 = fileOps[1];
    const auto gradingCurves1 = std::dynamic_pointer_cast<const OCIO::GradingRGBCurveOpData>(op1);
    OCIO_REQUIRE_ASSERT(gradingCurves1);
    OCIO_CHECK_EQUAL(gradingCurves1->getStyle(), OCIO::GRADING_LIN);
    OCIO_CHECK_ASSERT(gradingCurves1->getBypassLinToLog());
    OCIO_CHECK_ASSERT(gradingCurves1->isDynamic());
    curves = gradingCurves1->getValue();
    OCIO_CHECK_ASSERT(*curves->getCurve(OCIO::RGB_RED) ==
                      OCIO::GradingRGBCurveImpl::DefaultLin);
    OCIO_CHECK_ASSERT(curves->getCurve(OCIO::RGB_RED)->slopesAreDefault());

    const auto op2 = fileOps[2];
    const auto gradingCurves2 = std::dynamic_pointer_cast<const OCIO::GradingRGBCurveOpData>(op2);
    OCIO_REQUIRE_ASSERT(gradingCurves2);
    OCIO_CHECK_EQUAL(gradingCurves2->getStyle(), OCIO::GRADING_VIDEO);
    OCIO_CHECK_ASSERT(!gradingCurves2->getBypassLinToLog());
    OCIO_CHECK_ASSERT(!gradingCurves2->isDynamic());
    curves = gradingCurves2->getValue();
    auto master = curves->getCurve(OCIO::RGB_MASTER);
    OCIO_REQUIRE_EQUAL(master->getNumControlPoints(), 3);
    OCIO_CHECK_EQUAL(master->getControlPoint(0), OCIO::GradingControlPoint(0.f, 0.f));
    OCIO_CHECK_EQUAL(master->getControlPoint(1), OCIO::GradingControlPoint(1.f, 1.f));
    OCIO_CHECK_EQUAL(master->getControlPoint(2), OCIO::GradingControlPoint(1.5f, 1.4f));
    OCIO_CHECK_ASSERT(!master->slopesAreDefault());
    OCIO_CHECK_EQUAL(master->getSlope(0), 0.0f);
    OCIO_CHECK_EQUAL(master->getSlope(1), 0.7f);
    OCIO_CHECK_EQUAL(master->getSlope(2), 1.1f);
}

OCIO_ADD_TEST(FileFormatCTF, load_grading_curves_log)
{
    std::istringstream ctfLog;
    ctfLog.str(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="empty">
   <GradingRGBCurve inBitDepth="16f" outBitDepth="32f" style="logRev">
      <Red>
         <ControlPoints>
            0.015625 0
            0.5 0.5
            2 2
         </ControlPoints>
      </Red>
      <Green>
         <ControlPoints>
            0.015625 1
            2.5 0.5
            3.5 1.5
         </ControlPoints>
      </Green>
      <Blue>
         <ControlPoints>
            -4 -4
            4.5 0.5
            5 3
         </ControlPoints>
      </Blue>
      <Master>
         <ControlPoints>
            11 11 12.5 10.5 13.5 0.5 26.5 -1.5
         </ControlPoints>
      </Master>
      <DynamicParameter param="RGB_CURVE" />
   </GradingRGBCurve>
</ProcessList>)");

    // Load file.
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr file;
    OCIO_CHECK_NO_THROW(file = tester.read(ctfLog, emptyString, OCIO::INTERP_DEFAULT));
    OCIO::LocalCachedFileRcPtr cachedFile = OCIO_DYNAMIC_POINTER_CAST<OCIO::LocalCachedFile>(file);
    OCIO_REQUIRE_ASSERT(cachedFile);
    const auto & fileOps = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    const auto op0 = fileOps[0];
    const auto gradingCurves0 = std::dynamic_pointer_cast<const OCIO::GradingRGBCurveOpData>(op0);
    OCIO_REQUIRE_ASSERT(gradingCurves0);
    OCIO_CHECK_EQUAL(gradingCurves0->getStyle(), OCIO::GRADING_LOG);
    OCIO_CHECK_EQUAL(gradingCurves0->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(!gradingCurves0->getBypassLinToLog());
    OCIO_CHECK_ASSERT(gradingCurves0->isDynamic());
    auto curves = gradingCurves0->getValue();
    auto red = curves->getCurve(OCIO::RGB_RED);
    OCIO_CHECK_EQUAL(red->getNumControlPoints(), 3);
    OCIO_CHECK_EQUAL(red->getControlPoint(0).m_x, 0.015625f);
    OCIO_CHECK_EQUAL(red->getControlPoint(0).m_y, 0.0f);
    OCIO_CHECK_EQUAL(red->getControlPoint(1).m_x, 0.5f);
    OCIO_CHECK_EQUAL(red->getControlPoint(1).m_y, 0.5f);
    OCIO_CHECK_EQUAL(red->getControlPoint(2).m_x, 2.0f);
    OCIO_CHECK_EQUAL(red->getControlPoint(2).m_y, 2.0f);

    auto master = curves->getCurve(OCIO::RGB_MASTER);
    OCIO_CHECK_EQUAL(master->getNumControlPoints(), 4);
    OCIO_CHECK_EQUAL(master->getControlPoint(0).m_x, 11.0f);
    OCIO_CHECK_EQUAL(master->getControlPoint(0).m_y, 11.0f);
    OCIO_CHECK_EQUAL(master->getControlPoint(1).m_x, 12.5f);
    OCIO_CHECK_EQUAL(master->getControlPoint(1).m_y, 10.5f);
    OCIO_CHECK_EQUAL(master->getControlPoint(2).m_x, 13.5f);
    OCIO_CHECK_EQUAL(master->getControlPoint(2).m_y,  0.5f);
    OCIO_CHECK_EQUAL(master->getControlPoint(3).m_x, 26.5f);
    OCIO_CHECK_EQUAL(master->getControlPoint(3).m_y, -1.5f);
}

OCIO_ADD_TEST(FileFormatCTF, load_grading_curves_errors)
{
    // Wrong version.
    OCIO_CHECK_THROW_WHAT(ParseString(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.8" id="empty">
  <GradingRGBCurve inBitDepth="16f" outBitDepth="32f" style="video">
      <DynamicParameter param="RGB_CURVE" />
   </GradingRGBCurve>
</ProcessList>
)"), OCIO::Exception, "CTF file version '1.8' does not support operator 'GradingRGBCurve'");

    // Missing style attribute.
    OCIO_CHECK_THROW_WHAT(ParseString(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="empty">
  <GradingRGBCurve inBitDepth="16f" outBitDepth="32f">
      <DynamicParameter param="RGB_CURVE" />
   </GradingRGBCurve>
</ProcessList>
)"), OCIO::Exception, "Required attribute 'style' is missing");

    // Wrong dynamic property param.
    OCIO_CHECK_THROW_WHAT(ParseString(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="empty">
  <GradingRGBCurve inBitDepth="16f" outBitDepth="32f">
      <DynamicParameter param="PRIMARY" />
   </GradingRGBCurve>
</ProcessList>
)"), OCIO::Exception, "Required attribute 'style' is missing");

    // Odd number of values for control points.
    OCIO_CHECK_THROW_WHAT(ParseString(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="empty">
    <GradingRGBCurve inBitDepth="32f" outBitDepth="32f" style="linear">
        <Red>
            <ControlPoints>
                         -7 -6 0 0 7
            </ControlPoints>
        </Red>
    </GradingRGBCurve>
</ProcessList>
)"), OCIO::Exception, "Control points element: odd number of values");

    // Not enough control points.
    OCIO_CHECK_THROW_WHAT(ParseString(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="empty">
    <GradingRGBCurve inBitDepth="32f" outBitDepth="32f" style="linear">
        <Red>
            <ControlPoints>
                         0 1
            </ControlPoints>
        </Red>
    </GradingRGBCurve>
</ProcessList>
)"), OCIO::Exception, "There must be at least 2 control points");

    // Control points don't have increasing x.
    OCIO_CHECK_THROW_WHAT(ParseString(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="empty">
    <GradingRGBCurve inBitDepth="32f" outBitDepth="32f" style="linear">
        <Red>
            <ControlPoints>
                 -7 -6 0 0 -1 7
            </ControlPoints>
        </Red>
    </GradingRGBCurve>
</ProcessList>
)"), OCIO::Exception, "Control point at index 2 has a x coordinate '-1' that is less from "
                      "previous control point x cooordinate '0'");

    // Number of slopes matches control points.
    OCIO_CHECK_THROW_WHAT(ParseString(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="empty">
    <GradingRGBCurve inBitDepth="32f" outBitDepth="32f" style="linear">
        <Red>
            <ControlPoints>
                 -7 -6 0 0 1 7
            </ControlPoints>
            <Slopes> 1 1 1 1 </Slopes>
        </Red>
    </GradingRGBCurve>
</ProcessList>
)"), OCIO::Exception, "Number of slopes must match number of control points");

    // Wrong curve: warning is logged.
    OCIO::LogGuard guard;
    OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_WARNING);

    OCIO_CHECK_NO_THROW(ParseString(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="empty">
    <GradingRGBCurve inBitDepth="32f" outBitDepth="32f" style="linear">
        <Grn>
            <ControlPoints>
                         0 1 1 2 2 3
            </ControlPoints>
        </Grn>
    </GradingRGBCurve>
</ProcessList>
)"));

    const StringUtils::StringVec parts = StringUtils::SplitByLines(StringUtils::RightTrim(guard.output()));
    OCIO_REQUIRE_EQUAL(parts.size(), 2);
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(parts[0], "Unrecognized element 'Grn'"));
    OCIO_CHECK_NE(std::string::npos, StringUtils::Find(parts[1], "Unrecognized element 'ControlPoints'"));

}

OCIO_ADD_TEST(CTFTransform, load_grading_tone)
{
    std::istringstream ctfLog;
    ctfLog.str(R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingTone inBitDepth="32f" outBitDepth="32f" style="log">
        <Blacks rgb="1.1 1 1" master="1" start="0" width="0.98765432198" />
        <Shadows rgb="1 1.1 1" master="1" start="1" pivot="0" />
        <Midtones rgb="1 1 1" master="1.1" center="0" width="1" />
        <Highlights rgb="1 1 1" master="1.1" start="0" pivot="1" />
        <Whites rgb="1 1 1.1" master="1" start="0" width="1" />
        <SContrast master="1.1" />
    </GradingTone>
    <GradingTone inBitDepth="32f" outBitDepth="32f" style="logRev">
        <DynamicParameter param="TONE" />
    </GradingTone>
    <GradingTone inBitDepth="32f" outBitDepth="32f" style="linear">
        <Blacks rgb="1.1 1 1" master="1" start="0" width="1" />
        <Shadows rgb="1 1.1 1" master="1" start="1" pivot="0" />
        <Whites rgb="1 1 1.1" master="1" start="0" width="1" />
        <SContrast master="1.1" />
        <DynamicParameter param="TONE" />
    </GradingTone>
    <GradingTone inBitDepth="32f" outBitDepth="32f" style="linearRev">
        <SContrast master="1.12345678912" />
    </GradingTone>
    <GradingTone inBitDepth="32f" outBitDepth="32f" style="video" />
    <GradingTone inBitDepth="32f" outBitDepth="32f" style="videoRev">
        <Shadows rgb="1 1 1.12345678912" master="1" start="0.6" pivot="0" />
    </GradingTone>
</ProcessList>
)");

    // Load file.
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr file;
    OCIO_CHECK_NO_THROW(file = tester.read(ctfLog, emptyString, OCIO::INTERP_DEFAULT));
    OCIO::LocalCachedFileRcPtr cachedFile = OCIO_DYNAMIC_POINTER_CAST<OCIO::LocalCachedFile>(file);
    OCIO_REQUIRE_ASSERT(cachedFile);
    const auto & fileOps = cachedFile->m_transform->getOps();

    OCIO_REQUIRE_EQUAL(fileOps.size(), 6);

    const auto op0 = fileOps[0];
    const auto gradingTone0 = std::dynamic_pointer_cast<const OCIO::GradingToneOpData>(op0);
    OCIO_REQUIRE_ASSERT(gradingTone0);
    OCIO_CHECK_EQUAL(OCIO::GRADING_LOG, gradingTone0->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, gradingTone0->getDirection());
    const auto vals0 = gradingTone0->getValue();
    OCIO_CHECK_EQUAL(OCIO::GradingRGBMSW(1.1, 1., 1., 1., 0., 0.98765432198), vals0.m_blacks);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBMSW(1., 1.1, 1., 1., 1., 0.), vals0.m_shadows);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBMSW(1., 1., 1., 1.1, 0., 1.), vals0.m_midtones);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBMSW(1., 1., 1., 1.1, 0., 1.), vals0.m_highlights);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBMSW(1., 1., 1.1, 1., 0., 1.), vals0.m_whites);
    OCIO_CHECK_EQUAL(1.1, vals0.m_scontrast);
    OCIO_CHECK_ASSERT(!gradingTone0->isDynamic());

    const auto op1 = fileOps[1];
    const auto gradingTone1 = std::dynamic_pointer_cast<const OCIO::GradingToneOpData>(op1);
    OCIO_REQUIRE_ASSERT(gradingTone1);
    OCIO_CHECK_EQUAL(OCIO::GRADING_LOG, gradingTone1->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, gradingTone1->getDirection());
    const auto vals1 = gradingTone1->getValue();
    const auto defValsLog = OCIO::GradingTone(gradingTone1->getStyle());
    OCIO_CHECK_EQUAL(defValsLog, vals1);
    OCIO_CHECK_ASSERT(gradingTone1->isDynamic());

    const auto op2 = fileOps[2];
    const auto gradingTone2 = std::dynamic_pointer_cast<const OCIO::GradingToneOpData>(op2);
    OCIO_REQUIRE_ASSERT(gradingTone2);
    OCIO_CHECK_EQUAL(OCIO::GRADING_LIN, gradingTone2->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, gradingTone2->getDirection());
    const auto vals2 = gradingTone2->getValue();
    OCIO_CHECK_EQUAL(OCIO::GradingRGBMSW(1.1, 1., 1., 1., 0., 1.), vals2.m_blacks);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBMSW(1., 1.1, 1., 1., 1., 0.), vals2.m_shadows);
    const auto defValsLin = OCIO::GradingTone(gradingTone2->getStyle());
    OCIO_CHECK_EQUAL(defValsLin.m_midtones, vals2.m_midtones);
    OCIO_CHECK_EQUAL(defValsLin.m_highlights, vals2.m_highlights);
    OCIO_CHECK_EQUAL(OCIO::GradingRGBMSW(1., 1., 1.1, 1., 0., 1.), vals2.m_whites);
    OCIO_CHECK_EQUAL(1.1, vals2.m_scontrast);
    OCIO_CHECK_ASSERT(gradingTone2->isDynamic());

    const auto op3 = fileOps[3];
    const auto gradingTone3 = std::dynamic_pointer_cast<const OCIO::GradingToneOpData>(op3);
    OCIO_REQUIRE_ASSERT(gradingTone3);
    OCIO_CHECK_EQUAL(OCIO::GRADING_LIN, gradingTone3->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, gradingTone3->getDirection());
    const auto vals3 = gradingTone3->getValue();
    OCIO::GradingTone expected3(gradingTone3->getStyle());
    expected3.m_scontrast = 1.12345678912;
    OCIO_CHECK_EQUAL(expected3, vals3);
    OCIO_CHECK_ASSERT(!gradingTone3->isDynamic());

    const auto op4 = fileOps[4];
    const auto gradingTone4 = std::dynamic_pointer_cast<const OCIO::GradingToneOpData>(op4);
    OCIO_REQUIRE_ASSERT(gradingTone4);
    OCIO_CHECK_EQUAL(OCIO::GRADING_VIDEO, gradingTone4->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, gradingTone4->getDirection());
    const auto vals4 = gradingTone4->getValue();
    OCIO::GradingTone expected4(gradingTone4->getStyle());
    OCIO_CHECK_EQUAL(expected4, vals4);
    OCIO_CHECK_ASSERT(!gradingTone4->isDynamic());

    const auto op5 = fileOps[5];
    const auto gradingTone5 = std::dynamic_pointer_cast<const OCIO::GradingToneOpData>(op5);
    OCIO_REQUIRE_ASSERT(gradingTone5);
    OCIO_CHECK_EQUAL(OCIO::GRADING_VIDEO, gradingTone5->getStyle());
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, gradingTone5->getDirection());
    const auto vals5 = gradingTone5->getValue();
    OCIO::GradingTone expected5(gradingTone5->getStyle());
    expected5.m_shadows.m_blue = 1.12345678912;
    OCIO_CHECK_EQUAL(expected5, vals5);
    OCIO_CHECK_ASSERT(!gradingTone5->isDynamic());
}

OCIO_ADD_TEST(CTFTransform, load_grading_tone_errors)
{
    // TODO
}

///////////////////////////////////////////////////////////////////////////////
//
// WRITER TESTS
//
///////////////////////////////////////////////////////////////////////////////

OCIO_ADD_TEST(CTFTransform, load_edit_save_matrix)
{
    const std::string ctfFile("clf/matrix_example.clf");
    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = OCIO::GetFileTransformProcessor(ctfFile));
    OCIO_REQUIRE_ASSERT(processor);
    OCIO::GroupTransformRcPtr group;
    OCIO_CHECK_NO_THROW(group = processor->createGroupTransform());
    OCIO_REQUIRE_ASSERT(group);

    group->getFormatMetadata().addAttribute(OCIO::ATTR_INVERSE_OF, "added inverseOf");
    group->getFormatMetadata().addAttribute("Unknown", "not saved");
    group->getFormatMetadata().addChildElement("Unknown", "not saved");
    group->getFormatMetadata().addChildElement(OCIO::METADATA_INFO, "Preserved");
    const auto indexInfo = group->getFormatMetadata().getNumChildrenElements() - 1;
    auto & info = group->getFormatMetadata().getChildElement(indexInfo);
    info.addAttribute("attrib", "value");
    info.addChildElement("Child", "Preserved");

    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    auto matTrans = OCIO::DynamicPtrCast<OCIO::MatrixTransform>(transform);
    OCIO_REQUIRE_ASSERT(matTrans);

    // Validate how escape characters are saved.
    const std::string shortName{ R"(A ' short ' " name)" };
    const std::string description1{ R"(A " short " description with a ' inside)" };
    const std::string description2{ R"(<test"'&>)" };
    matTrans->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, description1.c_str());
    const auto indexDesc = matTrans->getFormatMetadata().getNumChildrenElements() - 1;
    auto & desc = matTrans->getFormatMetadata().getChildElement(indexDesc);
    desc.addAttribute("Unknown", "not saved");
    matTrans->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, description2.c_str());

    matTrans->getFormatMetadata().setName(shortName.c_str());

    const double offset[] = { 0.1, 1.2, 2.3456789123456, 0.0 };
    matTrans->setOffset(offset);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    // Output matrix array as '3 4 3'.
    const std::string expectedCTF{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="b5cc7aed-d405-4d8b-b64b-382b2341a378" name="matrix example" inverseOf="added inverseOf">
    <Description>Basic matrix example using CLF v2 dim syntax</Description>
    <InputDescriptor>RGB</InputDescriptor>
    <OutputDescriptor>XYZ</OutputDescriptor>
    <Info attrib="value">
    Preserved
        <Child>Preserved</Child>
    </Info>
    <Matrix id="c61daf06-539f-4254-81fc-9800e6d02a37" name="A &apos; short &apos; &quot; name" inBitDepth="32f" outBitDepth="32f">
        <Description>Legacy matrix</Description>
        <Description>Note that dim=&quot;3 3 3&quot; should be supported for CLF v2 compatibility</Description>
        <Description>A &quot; short &quot; description with a &apos; inside</Description>
        <Description>&lt;test&quot;&apos;&amp;&gt;</Description>
        <Array dim="3 4 3">
          0.4123908          0.35758434          0.18048079                 0.1
         0.21263901          0.71516868          0.07219232                 1.2
         0.01933082          0.01191948          0.95053215     2.3456789123456
        </Array>
    </Matrix>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expectedCTF.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expectedCTF, outputTransform.str());

    // Read the stream back.
    std::istringstream inputTransform;
    inputTransform.str(outputTransform.str());

    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr file = tester.read(inputTransform, emptyString, OCIO::INTERP_DEFAULT);
    OCIO::LocalCachedFileRcPtr cachedFile = OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(file);

    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::ConstOpDataRcPtr op = fileOps[0];
    OCIO::ConstMatrixOpDataRcPtr mat = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op);
    OCIO_REQUIRE_ASSERT(mat);
    const auto & md = mat->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(md.getNumAttributes(), 2);
    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_ID), md.getAttributeName(0));
    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_NAME), md.getAttributeName(1));
    OCIO_CHECK_EQUAL(shortName, md.getAttributeValue(1));
    OCIO_REQUIRE_EQUAL(md.getNumChildrenElements(), 4);
    const auto & desc0 = md.getChildElement(0);
    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_DESCRIPTION), desc0.getElementName());
    OCIO_CHECK_EQUAL(std::string(R"(Legacy matrix)"), desc0.getElementValue());
    const auto & desc1 = md.getChildElement(2);
    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_DESCRIPTION), desc1.getElementName());
    OCIO_CHECK_EQUAL(description1, desc1.getElementValue());
    const auto & desc2 = md.getChildElement(3);
    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_DESCRIPTION), desc2.getElementName());
    OCIO_CHECK_EQUAL(description2, desc2.getElementValue());
}

namespace
{
OCIO::LocalCachedFileRcPtr WriteRead(OCIO::TransformRcPtr transform)
{
    auto group = OCIO::GroupTransform::Create();
    group->appendTransform(transform);

    std::ostringstream outputTransform;
    WriteGroupCTF(group, outputTransform);

    std::istringstream inputTransform;
    inputTransform.str(outputTransform.str());

    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr file = tester.read(inputTransform, emptyString, OCIO::INTERP_DEFAULT);
    return OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(file);
}
}

OCIO_ADD_TEST(CTFTransform, save_matrix)
{
    OCIO::MatrixTransformRcPtr matTransform = OCIO::MatrixTransform::Create();
    double offset4[]{ 0.123456789123, 0.11, 0.111, 0.2 };
    matTransform->setOffset(offset4);
    matTransform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::LocalCachedFileRcPtr cachedFile = WriteRead(matTransform);
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::ConstOpDataRcPtr op = fileOps[0];
    OCIO::ConstMatrixOpDataRcPtr mat = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op);
    OCIO_REQUIRE_ASSERT(mat);
    OCIO_CHECK_EQUAL(mat->getOffsetValue(0), offset4[0]);
    OCIO_CHECK_EQUAL(mat->getOffsetValue(1), offset4[1]);
    OCIO_CHECK_EQUAL(mat->getOffsetValue(2), offset4[2]);
    OCIO_CHECK_EQUAL(mat->getOffsetValue(3), offset4[3]);
}

OCIO_ADD_TEST(CTFTransform, save_cdl)
{
    OCIO::CDLTransformRcPtr cdlTransform = OCIO::CDLTransform::Create();
    cdlTransform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    const double slope[]{ 1.1, 1.2, 1.3 };
    cdlTransform->setSlope(slope);
    const double offset[]{ 2.1, 2.2, 2.3 };
    cdlTransform->setOffset(offset);
    const double power[]{ 3.1, 3.2, 3.3 };
    cdlTransform->setPower(power);
    const double sat = 0.7;
    cdlTransform->setSat(sat);
    cdlTransform->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "test-cdl-1");
    cdlTransform->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "CDL description 1");
    cdlTransform->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "CDL description 2");
    cdlTransform->getFormatMetadata().addChildElement(OCIO::METADATA_INPUT_DESCRIPTION, "Input");
    cdlTransform->getFormatMetadata().addChildElement(OCIO::METADATA_VIEWING_DESCRIPTION, "Viewing");
    cdlTransform->getFormatMetadata().addChildElement(OCIO::METADATA_SOP_DESCRIPTION, "SOP description 1");
    cdlTransform->getFormatMetadata().addChildElement(OCIO::METADATA_SOP_DESCRIPTION, "SOP description 2");
    cdlTransform->getFormatMetadata().addChildElement(OCIO::METADATA_SAT_DESCRIPTION, "Sat description 1");
    cdlTransform->getFormatMetadata().addChildElement(OCIO::METADATA_SAT_DESCRIPTION, "Sat description 2");

    OCIO::LocalCachedFileRcPtr cachedFile = WriteRead(cdlTransform);
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::ConstOpDataRcPtr op = fileOps[0];
    OCIO::ConstCDLOpDataRcPtr cdl = OCIO::DynamicPtrCast<const OCIO::CDLOpData>(op);
    OCIO_REQUIRE_ASSERT(cdl);
    OCIO_CHECK_EQUAL(cdl->getID(), "test-cdl-1");
    const auto & metadata = cdl->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata.getNumChildrenElements(), 8);
    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_DESCRIPTION),
                     metadata.getChildElement(0).getElementName());
    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_DESCRIPTION),
                     metadata.getChildElement(1).getElementName());
    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_INPUT_DESCRIPTION),
                     metadata.getChildElement(2).getElementName());
    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_VIEWING_DESCRIPTION),
                     metadata.getChildElement(3).getElementName());
    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_SOP_DESCRIPTION),
                     metadata.getChildElement(4).getElementName());
    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_SOP_DESCRIPTION),
                     metadata.getChildElement(5).getElementName());
    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_SAT_DESCRIPTION),
                     metadata.getChildElement(6).getElementName());
    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_SAT_DESCRIPTION),
                     metadata.getChildElement(7).getElementName());
    auto params = cdl->getSlopeParams();
    OCIO_CHECK_EQUAL(params[0], slope[0]);
    OCIO_CHECK_EQUAL(params[1], slope[1]);
    OCIO_CHECK_EQUAL(params[2], slope[2]);
    params = cdl->getOffsetParams();
    OCIO_CHECK_EQUAL(params[0], offset[0]);
    OCIO_CHECK_EQUAL(params[1], offset[1]);
    OCIO_CHECK_EQUAL(params[2], offset[2]);
    params = cdl->getPowerParams();
    OCIO_CHECK_EQUAL(params[0], power[0]);
    OCIO_CHECK_EQUAL(params[1], power[1]);
    OCIO_CHECK_EQUAL(params[2], power[2]);
    auto val = cdl->getSaturation();
    OCIO_CHECK_EQUAL(val, sat);
}

namespace
{
void TestSaveLog(double base, unsigned line)
{
    OCIO::LogTransformRcPtr logT = OCIO::LogTransform::Create();
    logT->setBase(base);

    OCIO::LocalCachedFileRcPtr cachedFile = WriteRead(logT);
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL_FROM(fileOps.size(), 1, line);
    OCIO::ConstOpDataRcPtr op = fileOps[0];
    OCIO::ConstLogOpDataRcPtr log = OCIO::DynamicPtrCast<const OCIO::LogOpData>(op);
    OCIO_REQUIRE_ASSERT_FROM(log, line);
    OCIO_CHECK_EQUAL_FROM(log->getBase(), base, line);
}
}

OCIO_ADD_TEST(CTFTransform, save_log)
{
    TestSaveLog(2.0, __LINE__);
    TestSaveLog(10.0, __LINE__);
    TestSaveLog(8.0, __LINE__);
}

OCIO_ADD_TEST(CTFTransform, save_log_affine)
{
    OCIO::LogAffineTransformRcPtr logT = OCIO::LogAffineTransform::Create();
    const double base = 8.0;
    logT->setBase(base);
    const double vals[] = {0.9, 1.1, 1.2};
    logT->setLinSideSlopeValue(vals);

    OCIO::LocalCachedFileRcPtr cachedFile = WriteRead(logT);
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::ConstOpDataRcPtr op = fileOps[0];
    OCIO::ConstLogOpDataRcPtr log = OCIO::DynamicPtrCast<const OCIO::LogOpData>(op);
    OCIO_REQUIRE_ASSERT(log);
    OCIO_CHECK_EQUAL(log->getBase(), base);
    OCIO_CHECK_EQUAL(log->getRedParams()[OCIO::LIN_SIDE_SLOPE], vals[0]);
    OCIO_CHECK_EQUAL(log->getGreenParams()[OCIO::LIN_SIDE_SLOPE], vals[1]);
    OCIO_CHECK_EQUAL(log->getBlueParams()[OCIO::LIN_SIDE_SLOPE], vals[2]);
}

OCIO_ADD_TEST(CTFTransform, save_log_camera)
{
    const double vals_break[] = { 0.4, 0.5, 0.6 };
    OCIO::LogCameraTransformRcPtr logT = OCIO::LogCameraTransform::Create(vals_break);
    const double base = 8.0;
    logT->setBase(base);
    const double vals[] = { 0.9, 1.1, 1.2 };
    logT->setLinSideSlopeValue(vals);
    const double vals_ls[] = { 1.2, 1.3, 1.4 };
    logT->setLinearSlopeValue(vals_ls);

    OCIO::LocalCachedFileRcPtr cachedFile = WriteRead(logT);
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::ConstOpDataRcPtr op = fileOps[0];
    OCIO::ConstLogOpDataRcPtr log = OCIO::DynamicPtrCast<const OCIO::LogOpData>(op);
    OCIO_REQUIRE_ASSERT(log);
    OCIO_CHECK_EQUAL(log->getBase(), base);
    OCIO_CHECK_EQUAL(log->getRedParams()[OCIO::LIN_SIDE_SLOPE], vals[0]);
    OCIO_CHECK_EQUAL(log->getGreenParams()[OCIO::LIN_SIDE_SLOPE], vals[1]);
    OCIO_CHECK_EQUAL(log->getBlueParams()[OCIO::LIN_SIDE_SLOPE], vals[2]);
    OCIO_CHECK_EQUAL(log->getRedParams()[OCIO::LIN_SIDE_BREAK], vals_break[0]);
    OCIO_CHECK_EQUAL(log->getGreenParams()[OCIO::LIN_SIDE_BREAK], vals_break[1]);
    OCIO_CHECK_EQUAL(log->getBlueParams()[OCIO::LIN_SIDE_BREAK], vals_break[2]);
    OCIO_CHECK_EQUAL(log->getRedParams()[OCIO::LINEAR_SLOPE], vals_ls[0]);
    OCIO_CHECK_EQUAL(log->getGreenParams()[OCIO::LINEAR_SLOPE], vals_ls[1]);
    OCIO_CHECK_EQUAL(log->getBlueParams()[OCIO::LINEAR_SLOPE], vals_ls[2]);
}

OCIO_ADD_TEST(CTFTransform, save_lut_1d_1component)
{
    const std::string ctfFile("clf/lut1d_32f_example.clf");
    OCIO::ConstProcessorRcPtr proc = OCIO::GetFileTransformProcessor(ctfFile);
    auto group = proc->createGroupTransform();

    std::ostringstream outputTransform;
    WriteGroupCTF(group, outputTransform);

    const std::string result = outputTransform.str();
    const std::string expected = "<Array dim=\"65 1\">";
    OCIO_CHECK_ASSERT(result.find(expected) != std::string::npos);
}

OCIO_ADD_TEST(CTFTransform, save_lut_1d_3components)
{
    const std::string ctfFile("lut1d_green.ctf");
    OCIO::ConstProcessorRcPtr proc = OCIO::GetFileTransformProcessor(ctfFile);
    auto group = proc->createGroupTransform();

    std::ostringstream outputTransform;
    WriteGroupCTF(group, outputTransform);

    const std::string result = outputTransform.str();
    const std::string expected = "<Array dim=\"32 3\">";
    OCIO_CHECK_ASSERT(result.find(expected) != std::string::npos);
}

OCIO_ADD_TEST(CTFTransform, save_invlut_1d_3components)
{
    const std::string ctfFile("lut1d_inv.ctf");
    OCIO::ConstProcessorRcPtr proc = OCIO::GetFileTransformProcessor(ctfFile);
    auto group = proc->createGroupTransform();

    std::ostringstream outputTransform;
    WriteGroupCTF(group, outputTransform);

    const std::string result = outputTransform.str();
    const std::string expected1 = "</InverseLUT1D>";
    OCIO_CHECK_ASSERT(result.find(expected1) != std::string::npos);
    // Components are equal, so only 1 get saved.
    const std::string expected2 = "<Array dim=\"17 1\">";
    OCIO_CHECK_ASSERT(result.find(expected2) != std::string::npos);
}

OCIO_ADD_TEST(CTFTransform, save_lut1d_halfdomain)
{
    OCIO::Lut1DTransformRcPtr lutT = OCIO::Lut1DTransform::Create();
    lutT->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    const unsigned long size = OCIO::Lut1DOpData::GetLutIdealSize(OCIO::BIT_DEPTH_F16);
    lutT->setLength(size);
    lutT->setInputHalfDomain(true);

    for (unsigned long i = 0; i < size; ++i)
    {
        half temp;
        temp.setBits((unsigned short)i);
        const float val = (float)temp;
        lutT->setValue(i, val, val, val);
    }

    OCIO::LocalCachedFileRcPtr cachedFile = WriteRead(lutT);
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::ConstOpDataRcPtr op = fileOps[0];
    OCIO::ConstLut1DOpDataRcPtr lut = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(op);
    OCIO_REQUIRE_ASSERT(lut);
    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OCIO_CHECK_ASSERT(lut->isInputHalfDomain());

    OCIO_REQUIRE_EQUAL(lut->getArray().getLength(), size);

    for (unsigned long i = 0; i < size; ++i)
    {
        half expected;
        expected.setBits((unsigned short)i);
        const float loadedVal = lut->getArray()[3 * i];
        const half loaded(loadedVal);
        if (expected.isNan())
        {
            OCIO_CHECK_ASSERT(loaded.isNan());
            OCIO_CHECK_ASSERT(OCIO::IsNan(lut->getArray()[3 * i + 1]));
            OCIO_CHECK_ASSERT(OCIO::IsNan(lut->getArray()[3 * i + 2]));
        }
        else
        {
            OCIO_CHECK_EQUAL(loaded.bits(), expected.bits());
            OCIO_CHECK_EQUAL(loadedVal, lut->getArray()[3 * i + 1]);
            OCIO_CHECK_EQUAL(loadedVal, lut->getArray()[3 * i + 2]);
        }
    }
}

OCIO_ADD_TEST(CTFTransform, save_lut1d_f16_raw)
{
    OCIO::Lut1DTransformRcPtr lutT = OCIO::Lut1DTransform::Create();
    lutT->setFileOutputBitDepth(OCIO::BIT_DEPTH_F16);

    lutT->setLength(2);
    lutT->setOutputRawHalfs(true);

    const float values[]{ half(1.0f / 3.0f),
        HALF_MAX,
        HALF_NRM_MIN,
        half(1.0f / 7.0f),
        half::posInf(),
        -half::posInf() };
    lutT->setValue(0, values[0], values[1], values[2]);
    lutT->setValue(1, values[3], values[4], values[5]);

    OCIO::LocalCachedFileRcPtr cachedFile = WriteRead(lutT);
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::ConstOpDataRcPtr op = fileOps[0];
    OCIO::ConstLut1DOpDataRcPtr lut = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(op);
    OCIO_REQUIRE_ASSERT(lut);
    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F16);

    OCIO_REQUIRE_EQUAL(lut->getArray().getLength(), 2);

    OCIO_CHECK_EQUAL(values[0], lut->getArray()[0]);
    OCIO_CHECK_EQUAL(values[1], lut->getArray()[1]);
    OCIO_CHECK_EQUAL(values[2], lut->getArray()[2]);
    OCIO_CHECK_EQUAL(values[3], lut->getArray()[3]);
    OCIO_CHECK_EQUAL(values[4], lut->getArray()[4]);
    OCIO_CHECK_EQUAL(values[5], lut->getArray()[5]);
}

OCIO_ADD_TEST(CTFTransform, save_lut1d_f32)
{
    OCIO::Lut1DTransformRcPtr lutT = OCIO::Lut1DTransform::Create();
    lutT->setFileOutputBitDepth(OCIO::BIT_DEPTH_F32);

    lutT->setLength(8);
    const float values[]{ 1.0f / 3.0f,
                          0.0000000000000001f,
                          0.9999999f,
                          0,
                          std::numeric_limits<float>::max(),
                          -std::numeric_limits<float>::min(),
                          std::numeric_limits<float>::infinity(),
                          -std::numeric_limits<float>::infinity() };
    
    lutT->setValue(0, values[0], values[0], values[0]);
    lutT->setValue(1, values[1], values[1], values[1]);
    lutT->setValue(2, values[2], values[2], values[2]);
    lutT->setValue(3, values[3], values[3], values[3]);
    lutT->setValue(4, values[4], values[4], values[4]);
    lutT->setValue(5, values[5], values[5], values[5]);
    lutT->setValue(6, values[6], values[6], values[6]);
    lutT->setValue(7, values[7], values[7], values[7]);

    OCIO::LocalCachedFileRcPtr cachedFile = WriteRead(lutT);
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::ConstOpDataRcPtr op = fileOps[0];
    OCIO::ConstLut1DOpDataRcPtr lut = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(op);
    OCIO_REQUIRE_ASSERT(lut);
    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OCIO_REQUIRE_EQUAL(lut->getArray().getLength(), 8);

    OCIO_CHECK_EQUAL(lut->getArray()[0], values[0]);
    OCIO_CHECK_EQUAL(lut->getArray()[3], values[1]);
    OCIO_CHECK_EQUAL(lut->getArray()[6], values[2]);
    OCIO_CHECK_EQUAL(lut->getArray()[9], values[3]);
    OCIO_CHECK_EQUAL(lut->getArray()[12], values[4]);
    OCIO_CHECK_EQUAL(lut->getArray()[15], values[5]);
    OCIO_CHECK_EQUAL(lut->getArray()[18], values[6]);
    OCIO_CHECK_EQUAL(lut->getArray()[21], values[7]);
}

OCIO_ADD_TEST(CTFTransform, save_lut1d_interpolation)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    OCIO::Lut1DTransformRcPtr lut = OCIO::Lut1DTransform::Create();
    lut->setInterpolation(OCIO::INTERP_DEFAULT);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->appendTransform(lut);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCLF(group, outputTransform));

    {
        const std::string result = outputTransform.str();
        const std::string expected = R"(<LUT1D inBitDepth="32f" outBitDepth="32f">)";
        OCIO_CHECK_ASSERT(result.find(expected) != std::string::npos);
    }

    outputTransform.str("");
    outputTransform.clear();
    group = OCIO::GroupTransform::Create();

    lut->setInterpolation(OCIO::INTERP_BEST);

    group->appendTransform(lut);
    OCIO_CHECK_NO_THROW(WriteGroupCLF(group, outputTransform));

    {
        const std::string result = outputTransform.str();
        const std::string expected = R"(<LUT1D inBitDepth="32f" outBitDepth="32f" interpolation="linear">)";
        OCIO_CHECK_ASSERT(result.find(expected) != std::string::npos);
    }

    outputTransform.str("");
    outputTransform.clear();
    group = OCIO::GroupTransform::Create();

    lut->setInterpolation(OCIO::INTERP_LINEAR);

    group->appendTransform(lut);
    OCIO_CHECK_NO_THROW(WriteGroupCLF(group, outputTransform));

    {
        const std::string result = outputTransform.str();
        const std::string expected = R"(<LUT1D inBitDepth="32f" outBitDepth="32f" interpolation="linear">)";
        OCIO_CHECK_ASSERT(result.find(expected) != std::string::npos);
    }

    outputTransform.str("");
    outputTransform.clear();
    group = OCIO::GroupTransform::Create();

    lut->setInterpolation(OCIO::INTERP_CUBIC);
    group->appendTransform(lut);
    OCIO_CHECK_THROW_WHAT(WriteGroupCLF(group, outputTransform), OCIO::Exception,
                          "1D LUT does not support interpolation algorithm: cubic");
}

OCIO_ADD_TEST(CTFTransform, save_invalid_lut_1d)
{
    OCIO::Lut1DTransformRcPtr lutT = OCIO::Lut1DTransform::Create();
    lutT->setFileOutputBitDepth(OCIO::BIT_DEPTH_F32);

    lutT->setLength(8);
    lutT->setInputHalfDomain(true);

    OCIO_CHECK_THROW_WHAT(WriteRead(lutT), OCIO::Exception,
                          "65536 required for halfDomain 1D LUT");
}

OCIO_ADD_TEST(CTFTransform, save_lut_3d)
{
    const std::string ctfFile("clf/lut3d_identity_12i_16f.clf");
    OCIO::ConstProcessorRcPtr proc = OCIO::GetFileTransformProcessor(ctfFile);
    auto group = proc->createGroupTransform();

    std::ostringstream outputTransform;
    WriteGroupCTF(group, outputTransform);

    const std::string result = outputTransform.str();
    const std::string expected = "<Array dim=\"2 2 2 3\">";
    OCIO_CHECK_ASSERT(result.find(expected) != std::string::npos);
}

OCIO_ADD_TEST(CTFTransform, save_range)
{
    OCIO::RangeTransformRcPtr rangeT = OCIO::RangeTransform::Create();
    rangeT->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    rangeT->setMinInValue(0.0);
    rangeT->setMaxInValue(0.5);
    rangeT->setMinOutValue(0.5);
    rangeT->setMaxOutValue(1.5);

    OCIO::LocalCachedFileRcPtr cachedFile = WriteRead(rangeT);
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::ConstOpDataRcPtr op = fileOps[0];
    OCIO::ConstRangeOpDataRcPtr range = OCIO::DynamicPtrCast<const OCIO::RangeOpData>(op);
    OCIO_REQUIRE_ASSERT(range);
    OCIO_CHECK_EQUAL(range->getMinInValue(), rangeT->getMinInValue());
    OCIO_CHECK_EQUAL(range->getMaxInValue(), rangeT->getMaxInValue());
    OCIO_CHECK_EQUAL(range->getMinOutValue(), rangeT->getMinOutValue());
    OCIO_CHECK_EQUAL(range->getMaxOutValue(), rangeT->getMaxOutValue());
}

OCIO_ADD_TEST(CTFTransform, save_group)
{
    OCIO::RangeTransformRcPtr rangeT = OCIO::RangeTransform::Create();
    rangeT->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    rangeT->setMinInValue(0.0f);
    rangeT->setMaxInValue(0.5f);
    rangeT->setMinOutValue(0.5f);
    rangeT->setMaxOutValue(1.5f);

    OCIO::MatrixTransformRcPtr matT = OCIO::MatrixTransform::Create();
    double offset4[]{ 0.123456789123, 0.11, 0.111, 0.2 };
    matT->setOffset(offset4);
    matT->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::GroupTransformRcPtr groupT = OCIO::GroupTransform::Create();
    groupT->appendTransform(rangeT);
    groupT->appendTransform(matT);

    OCIO::LocalCachedFileRcPtr cachedFile = WriteRead(groupT);
    const OCIO::ConstOpDataVec & fileOps = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(fileOps.size(), 2);
    OCIO::ConstOpDataRcPtr op = fileOps[0];
    OCIO::ConstRangeOpDataRcPtr range = OCIO::DynamicPtrCast<const OCIO::RangeOpData>(op);
    OCIO_REQUIRE_ASSERT(range);

    OCIO::ConstOpDataRcPtr op1 = fileOps[1];
    OCIO::ConstMatrixOpDataRcPtr mat = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op1);
    OCIO_REQUIRE_ASSERT(mat);
}

OCIO_ADD_TEST(CTFTransform, load_save_matrix)
{
    const std::string ctfFile("clf/matrix_example.clf");
    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = OCIO::GetFileTransformProcessor(ctfFile));
    OCIO_REQUIRE_ASSERT(processor);
    auto group = processor->createGroupTransform();

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    // Output matrix array as '3 3 3'.
    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="b5cc7aed-d405-4d8b-b64b-382b2341a378" name="matrix example">
    <Description>Basic matrix example using CLF v2 dim syntax</Description>
    <InputDescriptor>RGB</InputDescriptor>
    <OutputDescriptor>XYZ</OutputDescriptor>
    <Matrix id="c61daf06-539f-4254-81fc-9800e6d02a37" inBitDepth="32f" outBitDepth="32f">
        <Description>Legacy matrix</Description>
        <Description>Note that dim=&quot;3 3 3&quot; should be supported for CLF v2 compatibility</Description>
        <Array dim="3 3 3">
          0.4123908          0.35758434          0.18048079
         0.21263901          0.71516868          0.07219232
         0.01933082          0.01191948          0.95053215
        </Array>
    </Matrix>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, save_matrix_444)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::MatrixTransformRcPtr mat = OCIO::MatrixTransform::Create();
    double m[16]{  1.,  0., 0., 0.,
                   0.,  1., 0., 0., 
                   0.,  0., 1., 0., 
                  0.5, 0.5, 0., 1. };
    mat->setMatrix(m);
    OCIO::ConstProcessorRcPtr processor = config->getProcessor(mat);

    OCIO::GroupTransformRcPtr group;
    OCIO_CHECK_NO_THROW(group = processor->createGroupTransform());
    OCIO_REQUIRE_ASSERT(group);

    OCIO_CHECK_EQUAL(group->getNumTransforms(), 1);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    // Output matrix array as '4 4 4'.
    OCIO_CHECK_NE(outputTransform.str().find("\"4 4 4\""), std::string::npos);
}

OCIO_ADD_TEST(CTFTransform, load_edit_save_matrix_clf)
{
    const std::string ctfFile("clf/matrix_example.clf");
    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = OCIO::GetFileTransformProcessor(ctfFile));
    OCIO_REQUIRE_ASSERT(processor);
    OCIO::GroupTransformRcPtr group;
    OCIO_CHECK_NO_THROW(group = processor->createGroupTransform());
    OCIO_REQUIRE_ASSERT(group);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    auto matTrans = OCIO::DynamicPtrCast<OCIO::MatrixTransform>(transform);
    const std::string newDescription{ "Added description" };
    matTrans->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, newDescription.c_str());
    const double offset[] = { 0.1, 1.2, 2.3, 0.0 };
    matTrans->setOffset(offset);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCLF(group, outputTransform));

    const std::string expectedCLF{
R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="b5cc7aed-d405-4d8b-b64b-382b2341a378" name="matrix example">
    <Description>Basic matrix example using CLF v2 dim syntax</Description>
    <InputDescriptor>RGB</InputDescriptor>
    <OutputDescriptor>XYZ</OutputDescriptor>
    <Matrix id="c61daf06-539f-4254-81fc-9800e6d02a37" inBitDepth="32f" outBitDepth="32f">
        <Description>Legacy matrix</Description>
        <Description>Note that dim=&quot;3 3 3&quot; should be supported for CLF v2 compatibility</Description>
        <Description>Added description</Description>
        <Array dim="3 4">
          0.4123908          0.35758434          0.18048079                 0.1
         0.21263901          0.71516868          0.07219232                 1.2
         0.01933082          0.01191948          0.95053215                 2.3
        </Array>
    </Matrix>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expectedCLF.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expectedCLF, outputTransform.str());

    const double offsetAlpha[] = { 0.1, 1.2, 2.3, 0.9 };
    matTrans->setOffset(offsetAlpha);

    std::ostringstream outputTransformCTF;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransformCTF));

    const std::string expectedCTF{
R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="b5cc7aed-d405-4d8b-b64b-382b2341a378" name="matrix example">
    <Description>Basic matrix example using CLF v2 dim syntax</Description>
    <InputDescriptor>RGB</InputDescriptor>
    <OutputDescriptor>XYZ</OutputDescriptor>
    <Matrix id="c61daf06-539f-4254-81fc-9800e6d02a37" inBitDepth="32f" outBitDepth="32f">
        <Description>Legacy matrix</Description>
        <Description>Note that dim=&quot;3 3 3&quot; should be supported for CLF v2 compatibility</Description>
        <Description>Added description</Description>
        <Array dim="4 5 4">
          0.4123908          0.35758434          0.18048079                   0                 0.1
         0.21263901          0.71516868          0.07219232                   0                 1.2
         0.01933082          0.01191948          0.95053215                   0                 2.3
                  0                   0                   0                   1                 0.9
        </Array>
    </Matrix>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expectedCTF.size(), outputTransformCTF.str().size());
    OCIO_CHECK_EQUAL(expectedCTF, outputTransformCTF.str());
}

OCIO_ADD_TEST(CTFTransform, matrix3x3_clf)
{
    OCIO::MatrixTransformRcPtr mat = OCIO::MatrixTransform::Create();
    mat->setFileInputBitDepth(OCIO::BIT_DEPTH_UINT10);
    mat->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    double m[16]{ 1. / 3., 10. / 3., 100. / 3., 0.,
                       3.,       4.,        5., 0.,
                       6.,       7.,        8., 0.,
                       0.,       0.,        0., 1. };
    mat->setMatrix(m);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(mat);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCLF(group, outputTransform));

    // In/out bit-depth equal, matrix not scaled.
    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="UID42">
    <Matrix inBitDepth="10i" outBitDepth="10i">
        <Array dim="3 3">
  0.333333333333333    3.33333333333333    33.3333333333333
                  3                   4                   5
                  6                   7                   8
        </Array>
    </Matrix>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, matrix_offset_alpha_ctf)
{
    OCIO::MatrixTransformRcPtr mat = OCIO::MatrixTransform::Create();
    mat->setFileInputBitDepth(OCIO::BIT_DEPTH_UINT10);
    mat->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    const double m[16]{ 1, 10, 20, 0.5, 3, 4, 5, 0.9, 6, 7, 8, 1.1, 2, 30, 11, 1 };
    mat->setMatrix(m);

    const double o[4] = { 0.1, 0.2, 0.3, 1.0 };
    mat->setOffset(o);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(mat);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    // Note that offset is scale by 1023 (for output bit-depth).
    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UID42">
    <Matrix inBitDepth="10i" outBitDepth="10i">
        <Array dim="4 5 4">
                  1                  10                  20                 0.5               102.3
                  3                   4                   5                 0.9               204.6
                  6                   7                   8                 1.1               306.9
                  2                  30                  11                   1                1023
        </Array>
    </Matrix>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());

    // Alpha not handled by CLF.
    OCIO_CHECK_THROW_WHAT(WriteGroupCLF(group, outputTransform),
                          OCIO::Exception, 
                          "Transform uses the 'Matrix with alpha component' op which cannot be written as CLF");
}

OCIO_ADD_TEST(CTFTransform, matrix_offset_alpha_bitdepth_ctf)
{
    OCIO::MatrixTransformRcPtr mat = OCIO::MatrixTransform::Create();
    mat->setFileInputBitDepth(OCIO::BIT_DEPTH_UINT8);
    mat->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT12);

    const double m[16]{ 255./4095.,          0,       0,       0,
                                 0, 510./4095.,       0,       0,
                                 0,          0, 51./91.,       0,
                                 0,          0,       0, 51./182. };
    mat->setMatrix(m);

    const double o[4] = { 0.01, 0.02, 0.03, 0.001 };
    mat->setOffset(o);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(mat);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    // Matrix scale following input bit-depth.
    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UID42">
    <Matrix inBitDepth="8i" outBitDepth="12i">
        <Array dim="4 5 4">
                  1                   0                   0                   0               40.95
                  0                   2                   0                   0                81.9
                  0                   0                   9                   0              122.85
                  0                   0                   0                 4.5               4.095
        </Array>
    </Matrix>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, matrix_offset_alpha_inverse_ctf)
{
    OCIO::MatrixTransformRcPtr mat = OCIO::MatrixTransform::Create();
    mat->setFileInputBitDepth(OCIO::BIT_DEPTH_F16);
    mat->setFileOutputBitDepth(OCIO::BIT_DEPTH_F32);

    const double m[16]{ 2, 0, 0, 0,
                        0, 4, 0, 0,
                        0, 0, 8, 0,
                        0, 0, 0, 1 };
    mat->setMatrix(m);

    const double o[4] = { 0.1, 0.2, 0.3, 1.0 };
    mat->setOffset(o);

    mat->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(mat);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UID42">
    <Matrix inBitDepth="32f" outBitDepth="16f">
        <Array dim="4 5 4">
                0.5                   0                   0                   0               -0.05
                  0                0.25                   0                   0               -0.05
                  0                   0               0.125                   0             -0.0375
                  0                   0                   0                   1                  -1
        </Array>
    </Matrix>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, legacy_cdl)
{
    // Create empty legacy Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(1);

    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    const double sop[] = { 1.0, 1.1, 1.2,
                           0.2, 0.3, 0.4,
                           3.1, 3.2, 3.3 };
    cdl->setSOP(sop);
    cdl->setSat(2.1);
    
    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    // Need to specify an id so that it does not get generated.
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "cdl0");

    group->appendTransform(cdl);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(group->write(config, OCIO::FILEFORMAT_CTF, outputTransform));

    // For OCIO v1, an ASC CDL was implemented as a Matrix/Gamma/Matrix rather
    // than as a dedicated op as in v2 and onward.
    const std::string expected{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList version=\"1.3\" id=\"cdl0\">\n"
        "    <Matrix inBitDepth=\"32f\" outBitDepth=\"32f\">\n"
        "        <Array dim=\"3 4 3\">\n"
        "                  1                   0                   0                 0.2\n"
        "                  0                 1.1                   0                 0.3\n"
        "                  0                   0                 1.2                 0.4\n"
        "        </Array>\n"
        "    </Matrix>\n"
        "    <Gamma inBitDepth=\"32f\" outBitDepth=\"32f\" style=\"basicFwd\">\n"
        "        <GammaParams channel=\"R\" gamma=\"3.1\" />\n"
        "        <GammaParams channel=\"G\" gamma=\"3.2\" />\n"
        "        <GammaParams channel=\"B\" gamma=\"3.3\" />\n"
        "    </Gamma>\n"
        // Output matrix array as '3 3 3'.
        "    <Matrix inBitDepth=\"32f\" outBitDepth=\"32f\">\n"
        "        <Array dim=\"3 3 3\">\n"
        "            1.86614            -0.78672            -0.07942\n"
        "           -0.23386             1.31328            -0.07942\n"
        "           -0.23386            -0.78672             2.02058\n"
        "        </Array>\n"
        "    </Matrix>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, cdl_clf)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    const double sop[] = { 1.0, 1.1, 1.2,
                           0.2, 0.3, 0.4,
                           3.1, 3.2, 3.3 };
    cdl->setSOP(sop);
    cdl->setSat(2.1);
    cdl->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, "TestCDL");
    cdl->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "CDL42");

    cdl->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "CDL node for unit test");
    cdl->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "Adding another description");
    cdl->getFormatMetadata().addChildElement(OCIO::METADATA_INPUT_DESCRIPTION, "Input");
    cdl->getFormatMetadata().addChildElement(OCIO::METADATA_VIEWING_DESCRIPTION, "Viewing");
    cdl->getFormatMetadata().addChildElement(OCIO::METADATA_SOP_DESCRIPTION, "SOP description 1");
    cdl->getFormatMetadata().addChildElement(OCIO::METADATA_SOP_DESCRIPTION, "SOP description 2");
    cdl->getFormatMetadata().addChildElement(OCIO::METADATA_SAT_DESCRIPTION, "Sat description 1");
    cdl->getFormatMetadata().addChildElement(OCIO::METADATA_SAT_DESCRIPTION, "Sat description 2");


    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    // Need to specify an id so that it does not get generated.
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "cdl1");
    group->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "ProcessList description");
    group->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "=======================");

    group->appendTransform(cdl);

    group->getFormatMetadata().addChildElement(OCIO::METADATA_INFO, "");
    auto & info = group->getFormatMetadata().getChildElement(2);
    info.addChildElement("Release", "2019");
    info.addChildElement("Directors", "");
    auto & sub = info.getChildElement(1);
    sub.addChildElement("Director", "");
    auto & subSub0 = sub.getChildElement(0);
    subSub0.addAttribute("FirstName", "David");
    subSub0.addAttribute("LastName", "Cronenberg");
    sub.addChildElement("Director", "");
    auto & subSub1 = sub.getChildElement(1);
    subSub1.addAttribute("FirstName", "David");
    subSub1.addAttribute("LastName", "Lynch");
    sub.addChildElement("Director", "");
    auto & subSub2 = sub.getChildElement(2);
    subSub2.addAttribute("FirstName", "David");
    subSub2.addAttribute("LastName", "Fincher");
    sub.addChildElement("Director", "");
    auto & subSub3 = sub.getChildElement(3);
    subSub3.addAttribute("FirstName", "David");
    subSub3.addAttribute("LastName", "Lean");

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCLF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="cdl1">
    <Description>ProcessList description</Description>
    <Description>=======================</Description>
    <Info>
        <Release>2019</Release>
        <Directors>
            <Director FirstName="David" LastName="Cronenberg"></Director>
            <Director FirstName="David" LastName="Lynch"></Director>
            <Director FirstName="David" LastName="Fincher"></Director>
            <Director FirstName="David" LastName="Lean"></Director>
        </Directors>
    </Info>
    <ASC_CDL id="CDL42" name="TestCDL" inBitDepth="32f" outBitDepth="32f" style="FwdNoClamp">
        <Description>CDL node for unit test</Description>
        <Description>Adding another description</Description>
        <InputDescription>Input</InputDescription>
        <ViewingDescription>Viewing</ViewingDescription>
        <SOPNode>
            <Description>SOP description 1</Description>
            <Description>SOP description 2</Description>
            <Slope>1 1.1 1.2</Slope>
            <Offset>0.2 0.3 0.4</Offset>
            <Power>3.1 3.2 3.3</Power>
        </SOPNode>
        <SatNode>
            <Description>Sat description 1</Description>
            <Description>Sat description 2</Description>
            <Saturation>2.1</Saturation>
        </SatNode>
    </ASC_CDL>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());

    // Now test if the read is working.

    std::istringstream ctfStream;
    ctfStream.str(expected);

    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr file = tester.read(ctfStream, emptyString, OCIO::INTERP_DEFAULT);
    auto cachedFile = OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(file);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 1);
    auto cdlData = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(cdlData);
    OCIO_CHECK_EQUAL(cdlData->getSlopeParams()[0], 1.);
    OCIO_CHECK_EQUAL(cdlData->getSlopeParams()[1], 1.1);
    OCIO_CHECK_EQUAL(cdlData->getSlopeParams()[2], 1.2);
}

OCIO_ADD_TEST(CTFTransform, cdl_ctf)
{
    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    cdl->setStyle(OCIO::CDL_ASC);

    const double sop[] = { 1.0, 1.1, 1.2,
                           0.2, 0.3, 0.4,
                           3.1, 3.2, 3.3 };
    cdl->setSOP(sop);
    cdl->setSat(2.1);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    // Need to specify an id so that it does not get generated.
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "cdl2");

    group->appendTransform(cdl);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList version=\"1.7\" id=\"cdl2\">\n"
        "    <ASC_CDL inBitDepth=\"32f\" outBitDepth=\"32f\" style=\"Fwd\">\n"
        "        <SOPNode>\n"
        "            <Slope>1 1.1 1.2</Slope>\n"
        "            <Offset>0.2 0.3 0.4</Offset>\n"
        "            <Power>3.1 3.2 3.3</Power>\n"
        "        </SOPNode>\n"
        "        <SatNode>\n"
        "            <Saturation>2.1</Saturation>\n"
        "        </SatNode>\n"
        "    </ASC_CDL>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, range_ctf)
{
    // Non-clamping range are converted to matrix.
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setStyle(OCIO::RANGE_NO_CLAMP);
    range->setMinInValue(0.1);
    range->setMaxInValue(0.9);
    range->setMinOutValue(0.0);
    range->setMaxOutValue(1.2);
    range->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "Range node for unit test");
    range->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, "TestRange");
    range->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "Range42");

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    // Need to specify an id so that it does not get generated.
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "mat0");
    
    group->getFormatMetadata().addChildElement(OCIO::METADATA_INPUT_DESCRIPTOR, "Input descriptor");
    group->getFormatMetadata().addChildElement(OCIO::METADATA_OUTPUT_DESCRIPTOR, "Output descriptor");

    group->appendTransform(range);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList version=\"1.3\" id=\"mat0\">\n"
        "    <InputDescriptor>Input descriptor</InputDescriptor>\n"
        "    <OutputDescriptor>Output descriptor</OutputDescriptor>\n"
        "    <Matrix id=\"Range42\" name=\"TestRange\" inBitDepth=\"32f\" outBitDepth=\"32f\">\n"
        "        <Description>Range node for unit test</Description>\n"
        "        <Array dim=\"3 4 3\">\n"
        "                1.5                   0                   0               -0.15\n"
        "                  0                 1.5                   0               -0.15\n"
        "                  0                   0                 1.5               -0.15\n"
        "        </Array>\n"
        "    </Matrix>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, range1_clf)
{
    // Forward clamping range with all 4 values set and with metadata.
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setFileInputBitDepth(OCIO::BIT_DEPTH_UINT8);
    range->setStyle(OCIO::RANGE_CLAMP);
    range->setMinInValue(16.0/255.0);
    range->setMaxInValue(235/255.0);
    range->setMinOutValue(-0.5);
    range->setMaxOutValue(2.1);
    range->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "Range node for unit test");
    range->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, "TestRange");
    range->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "Range42");

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->getFormatMetadata().addChildElement(OCIO::METADATA_INPUT_DESCRIPTOR, "Input descriptor");
    group->getFormatMetadata().addChildElement(OCIO::METADATA_OUTPUT_DESCRIPTOR, "Output descriptor");
    group->appendTransform(range);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCLF(group, outputTransform));

    const std::string expected{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList compCLFversion=\"3\" id=\"UID42\">\n"
        "    <InputDescriptor>Input descriptor</InputDescriptor>\n"
        "    <OutputDescriptor>Output descriptor</OutputDescriptor>\n"
        "    <Range id=\"Range42\" name=\"TestRange\" inBitDepth=\"8i\" outBitDepth=\"32f\">\n"
        "        <Description>Range node for unit test</Description>\n"
        "        <minInValue> 16 </minInValue>\n"
        "        <maxInValue> 235 </maxInValue>\n"
        "        <minOutValue> -0.5 </minOutValue>\n"
        "        <maxOutValue> 2.1 </maxOutValue>\n"
        "    </Range>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, range2_clf)
{
    // Forward clamping range with just minValues set.
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setFileInputBitDepth(OCIO::BIT_DEPTH_UINT10);
    range->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT8);
    range->setMinInValue(0.1);
    range->setMinOutValue(0.1);
    range->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "Range42");

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(range);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCLF(group, outputTransform));

    const std::string expected{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList compCLFversion=\"3\" id=\"UID42\">\n"
        "    <Range id=\"Range42\" inBitDepth=\"10i\" outBitDepth=\"8i\">\n"
        "        <minInValue> 102.3 </minInValue>\n"
        "        <minOutValue> 25.5 </minOutValue>\n"
        "    </Range>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, range3_clf)
{
    // Forward clamping range with just minValues set.

    // This will only do bit-depth conversion (with a clamp at 0).
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setFileInputBitDepth(OCIO::BIT_DEPTH_F16);
    range->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT12);
    range->setMinInValue(0.);
    range->setMinOutValue(0.);
    range->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "Range42");

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(range);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCLF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="UID42">
    <Range id="Range42" inBitDepth="16f" outBitDepth="12i">
        <minInValue> 0 </minInValue>
        <minOutValue> 0 </minOutValue>
    </Range>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, range4_clf)
{
    // Inverse clamping range with all 4 values set.
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setFileInputBitDepth(OCIO::BIT_DEPTH_F16);
    range->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT12);
    // Set inverse direction.
    range->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    range->setMinInValue(0.);
    range->setMinOutValue(0.5);
    range->setMaxInValue(1.0);
    range->setMaxOutValue(1.0);
    range->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "Range42");

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(range);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCLF(group, outputTransform));

    // Range is saved in the forward direction.
    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="UID42">
    <Range id="Range42" inBitDepth="12i" outBitDepth="16f">
        <minInValue> 2047.5 </minInValue>
        <maxInValue> 4095 </maxInValue>
        <minOutValue> 0 </minOutValue>
        <maxOutValue> 1 </maxOutValue>
    </Range>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, exponent_ctf)
{
    OCIO::ExponentWithLinearTransformRcPtr exp = OCIO::ExponentWithLinearTransform::Create();
    const double gamma[] = { 1.1, 1.2, 1.3, 1.0 };
    exp->setGamma(gamma);
    const double offset[] = { 0.1, 0.2, 0.1, 0.0 };
    exp->setOffset(offset);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(exp);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList version=\"1.3\" id=\"UID42\">\n"
        "    <Gamma inBitDepth=\"32f\" outBitDepth=\"32f\" style=\"monCurveFwd\">\n"
        "        <GammaParams channel=\"R\" gamma=\"1.1\" offset=\"0.1\" />\n"
        "        <GammaParams channel=\"G\" gamma=\"1.2\" offset=\"0.2\" />\n"
        "        <GammaParams channel=\"B\" gamma=\"1.3\" offset=\"0.1\" />\n"
        "    </Gamma>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, gamma1_ctf)
{
    OCIO::ExponentTransformRcPtr exp = OCIO::ExponentTransform::Create();
    exp->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    const double gamma[] = { 2.6, 2.6, 2.6, 1.0 };
    exp->setValue(gamma);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(exp);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    // Identity alpha. Transform written as version 1.3.
    const std::string expected{R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UID42">
    <Gamma inBitDepth="32f" outBitDepth="32f" style="basicRev">
        <GammaParams gamma="2.6" />
    </Gamma>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());

    std::ostringstream outputTransformCLF;
    OCIO_CHECK_NO_THROW(WriteGroupCLF(group, outputTransformCLF));

    const std::string expectedCLF{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="UID42">
    <Exponent inBitDepth="32f" outBitDepth="32f" style="basicRev">
        <ExponentParams exponent="2.6" />
    </Exponent>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expectedCLF.size(), outputTransformCLF.str().size());
    OCIO_CHECK_EQUAL(expectedCLF, outputTransformCLF.str());
}

OCIO_ADD_TEST(CTFTransform, gamma1_mirror_ctf)
{
    OCIO::ExponentTransformRcPtr exp = OCIO::ExponentTransform::Create();
    exp->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    exp->setNegativeStyle(OCIO::NEGATIVE_MIRROR);

    const double gamma[] = { 2.6, 2.6, 2.6, 1.0 };
    exp->setValue(gamma);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(exp);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    // Identity alpha. Transform written as version 2 because of new style.
    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UID42">
    <Exponent inBitDepth="32f" outBitDepth="32f" style="basicMirrorRev">
        <ExponentParams exponent="2.6" />
    </Exponent>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());

    std::ostringstream outputTransformCLF;
    OCIO_CHECK_NO_THROW(WriteGroupCLF(group, outputTransformCLF));

    const std::string expectedCLF{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="UID42">
    <Exponent inBitDepth="32f" outBitDepth="32f" style="basicMirrorRev">
        <ExponentParams exponent="2.6" />
    </Exponent>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expectedCLF.size(), outputTransformCLF.str().size());
    OCIO_CHECK_EQUAL(expectedCLF, outputTransformCLF.str());
}

OCIO_ADD_TEST(CTFTransform, gamma1_pass_thru_ctf)
{
    OCIO::ExponentTransformRcPtr exp = OCIO::ExponentTransform::Create();
    exp->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    exp->setNegativeStyle(OCIO::NEGATIVE_PASS_THRU);

    const double gamma[] = { 2.6, 2.6, 2.6, 1.0 };
    exp->setValue(gamma);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(exp);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    // Identity alpha. Transform written as version 2.0 because of new style.
    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UID42">
    <Exponent inBitDepth="32f" outBitDepth="32f" style="basicPassThruRev">
        <ExponentParams exponent="2.6" />
    </Exponent>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());

    std::ostringstream outputTransformCLF;
    OCIO_CHECK_NO_THROW(WriteGroupCLF(group, outputTransformCLF));

    const std::string expectedCLF{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="UID42">
    <Exponent inBitDepth="32f" outBitDepth="32f" style="basicPassThruRev">
        <ExponentParams exponent="2.6" />
    </Exponent>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expectedCLF.size(), outputTransformCLF.str().size());
    OCIO_CHECK_EQUAL(expectedCLF, outputTransformCLF.str());
}

OCIO_ADD_TEST(CTFTransform, gamma2_ctf)
{
    OCIO::ExponentWithLinearTransformRcPtr exp = OCIO::ExponentWithLinearTransform::Create();
    exp->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    const double gamma[] = { 2.4, 2.2, 2.0, 1.8 };
    exp->setGamma(gamma);

    const double offset[] = { 0.1, 0.2, 0.4, 0.8 };
    exp->setOffset(offset);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(exp);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    // Non-identity alpha. Transform written as version 1.5.
    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.5" id="UID42">
    <Gamma inBitDepth="32f" outBitDepth="32f" style="monCurveRev">
        <GammaParams channel="R" gamma="2.4" offset="0.1" />
        <GammaParams channel="G" gamma="2.2" offset="0.2" />
        <GammaParams channel="B" gamma="2" offset="0.4" />
        <GammaParams channel="A" gamma="1.8" offset="0.8" />
    </Gamma>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());

    // CLF does not allow alpha channel.
    std::ostringstream outputTransformCLF;
    OCIO_CHECK_THROW_WHAT(WriteGroupCLF(group, outputTransformCLF),
                          OCIO::Exception,
                          "Transform uses the 'Gamma with alpha component' op which cannot be written as CLF");
}

OCIO_ADD_TEST(CTFTransform, gamma3_ctf)
{
    OCIO::ExponentWithLinearTransformRcPtr exp = OCIO::ExponentWithLinearTransform::Create();

    const double gamma[] = { 2.42, 2.42, 2.42, 1.0 };
    exp->setGamma(gamma);

    const double offset[] = { 0.099, 0.099, 0.099, 0.0 };
    exp->setOffset(offset);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(exp);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    // Identity alpha.  Transform written as version 1.3.
    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UID42">
    <Gamma inBitDepth="32f" outBitDepth="32f" style="monCurveFwd">
        <GammaParams gamma="2.42" offset="0.099" />
    </Gamma>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());

    // CLF does not allow alpha channel.
    std::ostringstream outputTransformCLF;
    OCIO_CHECK_NO_THROW(WriteGroupCLF(group, outputTransformCLF));

    const std::string expectedCLF{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="UID42">
    <Exponent inBitDepth="32f" outBitDepth="32f" style="monCurveFwd">
        <ExponentParams exponent="2.42" offset="0.099" />
    </Exponent>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expectedCLF.size(), outputTransformCLF.str().size());
    OCIO_CHECK_EQUAL(expectedCLF, outputTransformCLF.str());
}

OCIO_ADD_TEST(CTFTransform, gamma4_ctf)
{
    OCIO::ExponentTransformRcPtr exp = OCIO::ExponentTransform::Create();

    const double gamma[] = { 2.6, 2.5, 2.4, 2.2 };
    exp->setValue(gamma);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(exp);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    // Non-identity alpha.  Transform written as version 1.5.
    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.5" id="UID42">
    <Gamma inBitDepth="32f" outBitDepth="32f" style="basicFwd">
        <GammaParams channel="R" gamma="2.6" />
        <GammaParams channel="G" gamma="2.5" />
        <GammaParams channel="B" gamma="2.4" />
        <GammaParams channel="A" gamma="2.2" />
    </Gamma>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}


OCIO_ADD_TEST(CTFTransform, gamma5_ctf)
{
    OCIO::ExponentWithLinearTransformRcPtr exp = OCIO::ExponentWithLinearTransform::Create();

    const double gamma[] = { 1. / 0.45, 1. / 0.45, 1. / 0.45, 1. / 0.45 };
    exp->setGamma(gamma);

    const double offset[] = { 0.099, 0.099, 0.099, 0.099 };
    exp->setOffset(offset);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(exp);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    // Non-identity alpha.  Transform written as version 1.5.
    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.5" id="UID42">
    <Gamma inBitDepth="32f" outBitDepth="32f" style="monCurveFwd">
        <GammaParams channel="R" gamma="2.22222222222222" offset="0.099" />
        <GammaParams channel="G" gamma="2.22222222222222" offset="0.099" />
        <GammaParams channel="B" gamma="2.22222222222222" offset="0.099" />
        <GammaParams channel="A" gamma="2.22222222222222" offset="0.099" />
    </Gamma>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, gamma6_ctf)
{
    OCIO::ExponentTransformRcPtr exp = OCIO::ExponentTransform::Create();

    const double gamma[] = { 2.4, 2.5, 2.6, 1.0 };
    exp->setValue(gamma);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->appendTransform(exp);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    // R,G,B channels different, but alpha is identity.
    // Transform written as version 1.3.
    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UID42">
    <Gamma inBitDepth="32f" outBitDepth="32f" style="basicFwd">
        <GammaParams channel="R" gamma="2.4" />
        <GammaParams channel="G" gamma="2.5" />
        <GammaParams channel="B" gamma="2.6" />
    </Gamma>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, fixed_function_aces_gamut_comp_13_ctf)
{
    const double data[7] = { 1.147, 1.264, 1.312, 0.815, 0.803, 0.880, 1.2 };
    OCIO::FixedFunctionTransformRcPtr ff =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GAMUT_COMP_13, &data[0], 7);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDFF42");
    group->appendTransform(ff);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2.1" id="UIDFF42">
    <FixedFunction inBitDepth="32f" outBitDepth="32f" style="GamutComp13Fwd" params="1.147 1.264 1.312 0.815 0.803 0.88 1.2">
    </FixedFunction>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, fixed_function_aces_gamut_comp_13_inverse_ctf)
{
    const double data[7] = { 1.147, 1.264, 1.312, 0.815, 0.803, 0.880, 1.2 };
    OCIO::FixedFunctionTransformRcPtr ff =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GAMUT_COMP_13, &data[0], 7);
    ff->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDFF42");
    group->appendTransform(ff);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2.1" id="UIDFF42">
    <FixedFunction inBitDepth="32f" outBitDepth="32f" style="GamutComp13Rev" params="1.147 1.264 1.312 0.815 0.803 0.88 1.2">
    </FixedFunction>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, fixed_function_rec2100_ctf)
{
    const double val = 0.5;
    OCIO::FixedFunctionTransformRcPtr ff =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_REC2100_SURROUND, &val, 1);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDFF42");
    group->appendTransform(ff);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDFF42">
    <FixedFunction inBitDepth="32f" outBitDepth="32f" style="Rec2100SurroundFwd" params="0.5">
    </FixedFunction>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, fixed_function_rec2100_inverse_ctf)
{
    const double val = 0.5;
    OCIO::FixedFunctionTransformRcPtr ff =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_REC2100_SURROUND, &val, 1);
    ff->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDFF42");
    group->appendTransform(ff);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDFF42">
    <FixedFunction inBitDepth="32f" outBitDepth="32f" style="Rec2100SurroundRev" params="0.5">
    </FixedFunction>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, fixed_function_lin_to_gammalog_ctf)
{
    const double vals[10] = {0.0, 0.25, 0.5, 1.0, 0.0,
                             2.718, 0.17883277, 0.807825590164, 1.0, -0.07116723};
    OCIO::FixedFunctionTransformRcPtr ff =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_LIN_TO_GAMMA_LOG, &vals[0], 10);

    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDFF42");
        group->appendTransform(ff);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2.4" id="UIDFF42">
    <FixedFunction inBitDepth="32f" outBitDepth="32f" style="Lin_TO_GammaLog" params="0 0.25 0.5 1 0 2.718 0.17883277 0.807825590164 1 -0.07116723">
    </FixedFunction>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expected, outputTransform.str());
    }
    {
        ff->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDFF42");
        group->appendTransform(ff);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2.4" id="UIDFF42">
    <FixedFunction inBitDepth="32f" outBitDepth="32f" style="GammaLog_TO_Lin" params="0 0.25 0.5 1 0 2.718 0.17883277 0.807825590164 1 -0.07116723">
    </FixedFunction>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expected, outputTransform.str());
    }
}

OCIO_ADD_TEST(CTFTransform, fixed_function_lin_to_doublelog_ctf)
{
    const double vals[13] = { 10.0, 0.25, 0.5, -1.0, 0.0, -1.0, 1.25, 1.0, 1.0, 1.0, 0.5, 1.0, 0.0 };
    OCIO::FixedFunctionTransformRcPtr ff =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_LIN_TO_DOUBLE_LOG, &vals[0], 13);

    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDFF42");
        group->appendTransform(ff);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2.4" id="UIDFF42">
    <FixedFunction inBitDepth="32f" outBitDepth="32f" style="Lin_TO_DoubleLog" params="10 0.25 0.5 -1 0 -1 1.25 1 1 1 0.5 1 0">
    </FixedFunction>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expected, outputTransform.str());
    }
    {
        ff->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDFF42");
        group->appendTransform(ff);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2.4" id="UIDFF42">
    <FixedFunction inBitDepth="32f" outBitDepth="32f" style="DoubleLog_TO_Lin" params="10 0.25 0.5 -1 0 -1 1.25 1 1 1 0.5 1 0">
    </FixedFunction>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expected, outputTransform.str());
    }
}

OCIO_ADD_TEST(CTFTransform, exposure_contrast_video_ctf)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();

    ec->setStyle(OCIO::EXPOSURE_CONTRAST_VIDEO);

    ec->makeExposureDynamic();
    ec->makeGammaDynamic();

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
    group->appendTransform(ec);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UIDEC42">
    <ExposureContrast inBitDepth="32f" outBitDepth="32f" style="video">
        <ECParams exposure="0" contrast="1" gamma="1" pivot="0.18" />
        <DynamicParameter param="EXPOSURE" />
        <DynamicParameter param="GAMMA" />
    </ExposureContrast>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, exposure_contrast_log_ctf)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();

    ec->setStyle(OCIO::EXPOSURE_CONTRAST_LOGARITHMIC);

    ec->setExposure(-1.5);
    ec->setContrast(0.5);
    ec->setGamma(1.5);

    ec->makeExposureDynamic();
    ec->makeContrastDynamic();

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
    group->appendTransform(ec);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UIDEC42">
    <ExposureContrast inBitDepth="32f" outBitDepth="32f" style="log">
        <ECParams exposure="-1.5" contrast="0.5" gamma="1.5" pivot="0.18" />
        <DynamicParameter param="EXPOSURE" />
        <DynamicParameter param="CONTRAST" />
    </ExposureContrast>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, exposure_contrast_linear_ctf)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();

    ec->setStyle(OCIO::EXPOSURE_CONTRAST_LINEAR);

    ec->setExposure(0.65);
    ec->setContrast(1.2);
    ec->setGamma(0.8);
    ec->setPivot(1.0);

    ec->makeExposureDynamic();
    ec->makeContrastDynamic();

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
    group->appendTransform(ec);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UIDEC42">
    <ExposureContrast inBitDepth="32f" outBitDepth="32f" style="linear">
        <ECParams exposure="0.65" contrast="1.2" gamma="0.8" pivot="1" />
        <DynamicParameter param="EXPOSURE" />
        <DynamicParameter param="CONTRAST" />
    </ExposureContrast>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, exposure_contrast_not_dynamic_ctf)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();

    ec->setStyle(OCIO::EXPOSURE_CONTRAST_VIDEO);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
    group->appendTransform(ec);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UIDEC42">
    <ExposureContrast inBitDepth="32f" outBitDepth="32f" style="video">
        <ECParams exposure="0" contrast="1" gamma="1" pivot="0.18" />
    </ExposureContrast>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, exposure_contrast_log_params_ctf)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();

    ec->setStyle(OCIO::EXPOSURE_CONTRAST_LOGARITHMIC);

    ec->setExposure(0.65);
    ec->setContrast(1.2);
    ec->setGamma(0.5);
    ec->setPivot(1.0);
    ec->setLogExposureStep(0.1);
    ec->setLogMidGray(0.5);

    ec->makeExposureDynamic();

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
    group->appendTransform(ec);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList version=\"2\" id=\"UIDEC42\">\n"
        "    <ExposureContrast inBitDepth=\"32f\" outBitDepth=\"32f\" style=\"log\">\n"
        "        <ECParams exposure=\"0.65\" contrast=\"1.2\" gamma=\"0.5\" pivot=\"1\" logExposureStep=\"0.1\" logMidGray=\"0.5\" />\n"
        "        <DynamicParameter param=\"EXPOSURE\" />\n"
        "    </ExposureContrast>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, grading_primary_log_ctf)
{
    auto gradingPrimary = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_LOG);

    // Non-default parameters.
    OCIO::GradingPrimary values(OCIO::GRADING_LOG);
    values.m_brightness.m_red += 0.1;
    values.m_contrast.m_green += 0.1;
    values.m_gamma.m_master += 0.1;
    values.m_saturation += 0.1;
    values.m_pivot += 0.1;
    values.m_pivotBlack += 0.1;
    values.m_pivotWhite += 0.1;
    values.m_clampBlack = 0.0;
    values.m_clampWhite = 1.0;
    gradingPrimary->setValue(values);

    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
        group->appendTransform(gradingPrimary);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expectedDefault{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingPrimary inBitDepth="32f" outBitDepth="32f" style="log">
        <Brightness rgb="0.1 0 0" master="0" />
        <Contrast rgb="1 1.1 1" master="1" />
        <Gamma rgb="1 1 1" master="1.1" />
        <Saturation master="1.1" />
        <Pivot contrast="-0.1" black="0.1" white="1.1" />
        <Clamp black="0" white="1" />
    </GradingPrimary>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expectedDefault.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expectedDefault, outputTransform.str());
    }

    // Use default parameters, make dynamic and change direction.
    OCIO::GradingPrimary valuesDef(OCIO::GRADING_LOG);
    gradingPrimary->setValue(valuesDef);
    gradingPrimary->makeDynamic();
    gradingPrimary->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
        group->appendTransform(gradingPrimary);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expectedDefault{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingPrimary inBitDepth="32f" outBitDepth="32f" style="logRev">
        <DynamicParameter param="PRIMARY" />
    </GradingPrimary>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expectedDefault.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expectedDefault, outputTransform.str());
    }
}

OCIO_ADD_TEST(CTFTransform, grading_primary_lin_ctf)
{
    auto gradingPrimary = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_LIN);

    // Non-default parameters.
    OCIO::GradingPrimary values(OCIO::GRADING_LIN);
    values.m_offset.m_red += 0.1;
    values.m_exposure.m_master += -0.99999;
    values.m_contrast.m_green += 0.1;
    values.m_saturation = 1.00001;
    values.m_pivot += 0.1;
    values.m_pivotBlack += 0.1;
    values.m_pivotWhite += 0.1;
    values.m_clampBlack = 0.0;
    values.m_clampWhite = 1.0;
    gradingPrimary->setValue(values);

    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
        group->appendTransform(gradingPrimary);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expectedDefault{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingPrimary inBitDepth="32f" outBitDepth="32f" style="linear">
        <Offset rgb="0.1 0 0" master="0" />
        <Exposure rgb="0 0 0" master="-0.99999" />
        <Contrast rgb="1 1.1 1" master="1" />
        <Saturation master="1.00001" />
        <Pivot contrast="0.28" />
        <Clamp black="0" white="1" />
    </GradingPrimary>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expectedDefault.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expectedDefault, outputTransform.str());
    }

    // Use default parameters, make dynamic and change direction.
    OCIO::GradingPrimary valuesDef(OCIO::GRADING_LIN);
    gradingPrimary->setValue(valuesDef);
    gradingPrimary->makeDynamic();
    gradingPrimary->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
        group->appendTransform(gradingPrimary);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expectedDefault{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingPrimary inBitDepth="32f" outBitDepth="32f" style="linearRev">
        <DynamicParameter param="PRIMARY" />
    </GradingPrimary>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expectedDefault.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expectedDefault, outputTransform.str());
    }
}

OCIO_ADD_TEST(CTFTransform, grading_primary_video_ctf)
{
    auto gradingPrimary = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_VIDEO);

    // Non-default parameters.
    OCIO::GradingPrimary values(OCIO::GRADING_VIDEO);
    values.m_lift.m_red += 0.1;
    values.m_gamma.m_master += 0.1;
    values.m_gain.m_green += 0.1;
    values.m_offset.m_green += 0.1;
    values.m_saturation += 0.1;
    values.m_pivotBlack += 0.1;
    values.m_pivotWhite += 0.1;
    values.m_clampBlack = 0.0;
    values.m_clampWhite = 1.0;
    gradingPrimary->setValue(values);

    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
        group->appendTransform(gradingPrimary);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expectedDefault{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingPrimary inBitDepth="32f" outBitDepth="32f" style="video">
        <Lift rgb="0.1 0 0" master="0" />
        <Gamma rgb="1 1 1" master="1.1" />
        <Gain rgb="1 1.1 1" master="1" />
        <Offset rgb="0 0.1 0" master="0" />
        <Saturation master="1.1" />
        <Pivot black="0.1" white="1.1" />
        <Clamp black="0" white="1" />
    </GradingPrimary>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expectedDefault.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expectedDefault, outputTransform.str());
    }

    // Use default parameters, make dynamic and change direction.
    OCIO::GradingPrimary valuesDef(OCIO::GRADING_VIDEO);
    gradingPrimary->setValue(valuesDef);
    gradingPrimary->makeDynamic();
    gradingPrimary->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
        group->appendTransform(gradingPrimary);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expectedDefault{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingPrimary inBitDepth="32f" outBitDepth="32f" style="videoRev">
        <DynamicParameter param="PRIMARY" />
    </GradingPrimary>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expectedDefault.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expectedDefault, outputTransform.str());
    }
}

OCIO_ADD_TEST(CTFTransform, grading_rgbcurve_log_ctf)
{
    auto gradingCurves = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LOG);

    // Two of the curves are the default curves, they are not saved.
    OCIO::GradingRGBCurveRcPtr curves = OCIO::GradingRGBCurve::Create(OCIO::GRADING_LOG);
    curves->getCurve(OCIO::RGB_GREEN)->getControlPoint(0).m_y = -0.5f;
    curves->getCurve(OCIO::RGB_MASTER)->setNumControlPoints(4);
    curves->getCurve(OCIO::RGB_MASTER)->getControlPoint(3).m_x = 1.5f;
    curves->getCurve(OCIO::RGB_MASTER)->getControlPoint(3).m_y = 1.4f;
    gradingCurves->setValue(curves);
    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDGradingCurves");
        group->appendTransform(gradingCurves);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDGradingCurves">
    <GradingRGBCurve inBitDepth="32f" outBitDepth="32f" style="log">
        <Green>
            <ControlPoints>
                          0 -0.5
                        0.5 0.5
                          1 1
            </ControlPoints>
        </Green>
        <Master>
            <ControlPoints>
                          0 0
                        0.5 0.5
                          1 1
                        1.5 1.4
            </ControlPoints>
        </Master>
    </GradingRGBCurve>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expected, outputTransform.str());
    }

    // All curves are default curves, no curve is saved.
    curves = OCIO::GradingRGBCurve::Create(OCIO::GRADING_LOG);
    gradingCurves->setValue(curves);
    gradingCurves->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    // Make it dynamic so it is not identity.
    gradingCurves->makeDynamic();
    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDGradingCurves");
        group->appendTransform(gradingCurves);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDGradingCurves">
    <GradingRGBCurve inBitDepth="32f" outBitDepth="32f" style="logRev">
        <DynamicParameter param="RGB_CURVE" />
    </GradingRGBCurve>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expected, outputTransform.str());
    }
}

OCIO_ADD_TEST(CTFTransform, grading_rgbcurve_video_ctf)
{
    auto gradingCurves = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_VIDEO);

    // All curves are default curves, no curve is saved.
    // Make it dynamic so it is not identity.
    gradingCurves->makeDynamic();
    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDGradingCurves");
        group->appendTransform(gradingCurves);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDGradingCurves">
    <GradingRGBCurve inBitDepth="32f" outBitDepth="32f" style="video">
        <DynamicParameter param="RGB_CURVE" />
    </GradingRGBCurve>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expected, outputTransform.str());
    }
}

OCIO_ADD_TEST(CTFTransform, grading_rgbcurve_lin_ctf)
{
    auto gradingCurves = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LIN);

    // Two of the curves are the default curves, they are not saved.
    OCIO::GradingRGBCurveRcPtr curves = OCIO::GradingRGBCurve::Create(OCIO::GRADING_LIN);
    curves->getCurve(OCIO::RGB_RED)->getControlPoint(0).m_y = -6.015625f;
    curves->getCurve(OCIO::RGB_MASTER)->setNumControlPoints(4);
    curves->getCurve(OCIO::RGB_MASTER)->getControlPoint(3).m_x = 16.f;
    curves->getCurve(OCIO::RGB_MASTER)->getControlPoint(3).m_y = 10.f;
    curves->getCurve(OCIO::RGB_MASTER)->setSlope(0, 1.f);
    curves->getCurve(OCIO::RGB_MASTER)->setSlope(1, 0.75f);
    curves->getCurve(OCIO::RGB_MASTER)->setSlope(2, 1.1f);
    curves->getCurve(OCIO::RGB_MASTER)->setSlope(3, 1.f);
    gradingCurves->setValue(curves);
    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDGradingCurves");
        group->appendTransform(gradingCurves);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDGradingCurves">
    <GradingRGBCurve inBitDepth="32f" outBitDepth="32f" style="linear">
        <Red>
            <ControlPoints>
                         -7 -6.015625
                          0 0
                          7 7
            </ControlPoints>
        </Red>
        <Master>
            <ControlPoints>
                         -7 -7
                          0 0
                          7 7
                         16 10
            </ControlPoints>
            <Slopes>
                          1 0.75 1.1 1 
            </Slopes>
        </Master>
    </GradingRGBCurve>
</ProcessList>
)" };

        const StringUtils::StringVec osvec  = StringUtils::SplitByLines(outputTransform.str());
        const StringUtils::StringVec resvec = StringUtils::SplitByLines(expected);
        OCIO_CHECK_EQUAL(osvec.size(), resvec.size());
        for(unsigned int i = 0; i < resvec.size(); ++i)
        {
            if ( (i >= 5 && i <= 7) ||
                 (i >= 12 && i <= 15) ||
                 (i == 18))
            {
                OCIO_CHECK_STR_FLOAT_VEC_CLOSE(osvec[i], resvec[i], 1e-5f);
            }
            else
            {
                OCIO_CHECK_EQUAL(osvec[i], resvec[i]);
            }

        }
    }

    // All curves are default curves, no curve is saved.
    curves = OCIO::GradingRGBCurve::Create(OCIO::GRADING_LIN);
    gradingCurves->setValue(curves);
    gradingCurves->setBypassLinToLog(true);
    // Make it dynamic so it is not identity.
    gradingCurves->makeDynamic();
    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDGradingCurves");
        group->appendTransform(gradingCurves);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDGradingCurves">
    <GradingRGBCurve inBitDepth="32f" outBitDepth="32f" style="linear" bypassLinToLog="true">
        <DynamicParameter param="RGB_CURVE" />
    </GradingRGBCurve>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expected, outputTransform.str());
    }
}

OCIO_ADD_TEST(CTFTransform, grading_tone_log_ctf)
{
    auto gradingTone = OCIO::GradingToneTransform::Create(OCIO::GRADING_LOG);

    // Leave midtones and scontrast as default and verify they aren't saved.
    OCIO::GradingTone values(OCIO::GRADING_LOG);
    values.m_blacks.m_red += 0.12345678912;
    values.m_shadows.m_green += 0.1;
    values.m_highlights.m_start += 0.1;
    values.m_whites.m_width += 0.1;
    gradingTone->setValue(values);

    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
        group->appendTransform(gradingTone);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expectedDefault{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingTone inBitDepth="32f" outBitDepth="32f" style="log">
        <Blacks rgb="1.12345678912 1 1" master="1" start="0.4" width="0.4" />
        <Shadows rgb="1 1.1 1" master="1" start="0.5" pivot="0" />
        <Highlights rgb="1 1 1" master="1" start="0.4" pivot="1" />
        <Whites rgb="1 1 1" master="1" start="0.4" width="0.6" />
    </GradingTone>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expectedDefault.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expectedDefault, outputTransform.str());
    }

    // All are non-default.
    values.m_blacks     = OCIO::GradingRGBMSW(1.1, 0.99999, 1.3, 1.2, 1.1, 1.00001);
    values.m_shadows    = OCIO::GradingRGBMSW(0.99999, 1.00001, 1.3, 1.2, 1.1, 0.1);
    values.m_midtones   = OCIO::GradingRGBMSW(1.1, 1.3, 0.99999, 1.00001, 1.1, 1.2);
    values.m_highlights = OCIO::GradingRGBMSW(1.1, 1.00001, 1.3, 0.99999, 1.1, 1.2);
    values.m_whites     = OCIO::GradingRGBMSW(1.00001, 1.1, 1.3, 1.2, 0.99999, 1.1);
    values.m_scontrast += 0.1111;
    gradingTone->setValue(values);

    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
        group->appendTransform(gradingTone);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expectedDefault{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingTone inBitDepth="32f" outBitDepth="32f" style="log">
        <Blacks rgb="1.1 0.99999 1.3" master="1.2" start="1.1" width="1.00001" />
        <Shadows rgb="0.99999 1.00001 1.3" master="1.2" start="1.1" pivot="0.1" />
        <Midtones rgb="1.1 1.3 0.99999" master="1.00001" center="1.1" width="1.2" />
        <Highlights rgb="1.1 1.00001 1.3" master="0.99999" start="1.1" pivot="1.2" />
        <Whites rgb="1.00001 1.1 1.3" master="1.2" start="0.99999" width="1.1" />
        <SContrast master="1.1111" />
    </GradingTone>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expectedDefault.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expectedDefault, outputTransform.str());
    }

    // Use default parameters, make dynamic and change direction.
    OCIO::GradingTone valuesDef(OCIO::GRADING_LOG);
    gradingTone->setValue(valuesDef);
    gradingTone->makeDynamic();
    gradingTone->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
        group->appendTransform(gradingTone);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expectedDefault{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingTone inBitDepth="32f" outBitDepth="32f" style="logRev">
        <DynamicParameter param="TONE" />
    </GradingTone>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expectedDefault.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expectedDefault, outputTransform.str());
    }
}

OCIO_ADD_TEST(CTFTransform, grading_tone_lin_ctf)
{
    // Only test the style, sub elements tested with GRADING_LOG.
    auto gradingTone = OCIO::GradingToneTransform::Create(OCIO::GRADING_LIN);

    // Make dynamic so it gets saved.
    gradingTone->makeDynamic();

    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
        group->appendTransform(gradingTone);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expectedDefault{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingTone inBitDepth="32f" outBitDepth="32f" style="linear">
        <DynamicParameter param="TONE" />
    </GradingTone>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expectedDefault.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expectedDefault, outputTransform.str());
    }

    gradingTone->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
        group->appendTransform(gradingTone);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expectedDefault{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingTone inBitDepth="32f" outBitDepth="32f" style="linearRev">
        <DynamicParameter param="TONE" />
    </GradingTone>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expectedDefault.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expectedDefault, outputTransform.str());
    }
}

OCIO_ADD_TEST(CTFTransform, grading_tone_video_ctf)
{
    // Only test the style, sub elements tested with GRADING_LOG.
    auto gradingTone = OCIO::GradingToneTransform::Create(OCIO::GRADING_VIDEO);
    // Make dynamic so it gets saved.
    gradingTone->makeDynamic();

    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
        group->appendTransform(gradingTone);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expectedDefault{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingTone inBitDepth="32f" outBitDepth="32f" style="video">
        <DynamicParameter param="TONE" />
    </GradingTone>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expectedDefault.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expectedDefault, outputTransform.str());
    }

    gradingTone->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    {
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
        group->appendTransform(gradingTone);

        std::ostringstream outputTransform;
        OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

        const std::string expectedDefault{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDEC42">
    <GradingTone inBitDepth="32f" outBitDepth="32f" style="videoRev">
        <DynamicParameter param="TONE" />
    </GradingTone>
</ProcessList>
)" };

        OCIO_CHECK_EQUAL(expectedDefault.size(), outputTransform.str().size());
        OCIO_CHECK_EQUAL(expectedDefault, outputTransform.str());
    }
}

OCIO_ADD_TEST(CTFTransform, log_lin_to_log_ctf)
{
    OCIO::LogAffineTransformRcPtr logT = OCIO::LogAffineTransform::Create();
    
    const double base = 2.0;
    logT->setBase(base);
    const double lins[] = { 0.9, 1.1, 1.2 };
    logT->setLinSideSlopeValue(lins);
    const double lino[] = { 0.1, 0.2, 0.3 };
    logT->setLinSideOffsetValue(lino);
    const double logs[] = { 1.3, 1.4, 1.5 };
    logT->setLogSideSlopeValue(logs);
    const double logo[] = { 0.4, 0.5, 0.6 };
    logT->setLogSideOffsetValue(logo);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDLOG42");
    group->appendTransform(logT);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDLOG42">
    <Log inBitDepth="32f" outBitDepth="32f" style="linToLog">
        <LogParams channel="R" base="2" linSideSlope="0.9" linSideOffset="0.1" logSideSlope="1.3" logSideOffset="0.4" />
        <LogParams channel="G" base="2" linSideSlope="1.1" linSideOffset="0.2" logSideSlope="1.4" logSideOffset="0.5" />
        <LogParams channel="B" base="2" linSideSlope="1.2" linSideOffset="0.3" logSideSlope="1.5" logSideOffset="0.6" />
    </Log>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, log_log_to_lin_ctf)
{
    OCIO::LogAffineTransformRcPtr logT = OCIO::LogAffineTransform::Create();
    logT->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    const double base = 2.0;
    logT->setBase(base);
    const double vals[] = { 0.9, 0.9, 0.9 };
    logT->setLinSideSlopeValue(vals);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDLOG42");
    group->appendTransform(logT);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDLOG42">
    <Log inBitDepth="32f" outBitDepth="32f" style="logToLin">
        <LogParams base="2" linSideSlope="0.9" linSideOffset="0" logSideSlope="1" logSideOffset="0" />
    </Log>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, log_antilog2_ctf)
{
    OCIO::LogAffineTransformRcPtr logT = OCIO::LogAffineTransform::Create();
    logT->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    const double base = 2.0;
    logT->setBase(base);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDLOG42");
    group->appendTransform(logT);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UIDLOG42">
    <Log inBitDepth="32f" outBitDepth="32f" style="antiLog2">
    </Log>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, lut1d_clf)
{
    OCIO::Lut1DTransformRcPtr lut = OCIO::Lut1DTransform::Create();
    lut->setInterpolation(OCIO::INTERP_LINEAR);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDLUT42");
    group->appendTransform(lut);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCLF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="UIDLUT42">
    <LUT1D inBitDepth="32f" outBitDepth="32f" interpolation="linear">
        <Array dim="2 1">
          0
          1
        </Array>
    </LUT1D>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, lut1d_inverse_clf)
{
    OCIO::Lut1DTransformRcPtr lut = OCIO::Lut1DTransform::Create();
    lut->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDLUT42");
    group->appendTransform(lut);

    std::ostringstream outputTransform;
    OCIO_CHECK_THROW_WHAT(WriteGroupCLF(group, outputTransform),
                          OCIO::Exception,
                          "Transform uses the 'InverseLUT1D' op which cannot be written as CLF");
}

OCIO_ADD_TEST(CTFTransform, lut1d_ctf)
{
    OCIO::Lut1DTransformRcPtr lut = OCIO::Lut1DTransform::Create();
    lut->setInterpolation(OCIO::INTERP_DEFAULT);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDLUT42");
    group->appendTransform(lut);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UIDLUT42">
    <LUT1D inBitDepth="32f" outBitDepth="32f">
        <Array dim="2 1">
          0
          1
        </Array>
    </LUT1D>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, lut1d_attributes_ctf)
{
    OCIO::Lut1DTransformRcPtr lut = OCIO::Lut1DTransform::Create();
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, "test-lut");
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "lut01");
    lut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);
    lut->setInterpolation(OCIO::INTERP_DEFAULT);
    lut->setInputHalfDomain(true);
    lut->setOutputRawHalfs(true);
    lut->setHueAdjust(OCIO::HUE_DW3);
    lut->setLength(65536);
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    lut->getValue(1000, r, g, b);
    lut->setValue(1000, r*1.001f, g*1.002f, b*1.003f);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDLUT42");
    group->appendTransform(lut);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    std::istringstream inputTransform;
    inputTransform.str(outputTransform.str());

    std::string line;
    OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    OCIO_CHECK_EQUAL(line, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");

    OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    OCIO_CHECK_EQUAL(line, R"(<ProcessList version="1.4" id="UIDLUT42">)");

    OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    OCIO_CHECK_EQUAL(StringUtils::Trim(line),
                     "<LUT1D id=\"lut01\" name=\"test-lut\" inBitDepth=\"32f\""
                     " outBitDepth=\"10i\""
                     " halfDomain=\"true\" rawHalfs=\"true\" hueAdjust=\"dw3\">");

    OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    OCIO_CHECK_EQUAL(StringUtils::Trim(line),
                     R"(<Array dim="65536 3">)");

    for (unsigned int i = 0; i <= 1000; ++i)
    {
        OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    }
    OCIO_CHECK_EQUAL(StringUtils::Trim(line),
                     R"(11216 11218 11220)");
}

OCIO_ADD_TEST(CTFTransform, lut1d_array_16x1_ctf)
{
    OCIO::Lut1DTransformRcPtr lut = OCIO::Lut1DTransform::Create();
    lut->setInterpolation(OCIO::INTERP_DEFAULT);
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, "test-lut");
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "lut01");
    lut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);
    lut->setLength(16);
    float rgb = 0.f;
    for (unsigned long i = 0; i < 16; ++i)
    {
        const float val = rgb / 1023.0f;
        lut->setValue(i, val, val, val);
        rgb += 3.0f;
    }

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDLUT42");
    group->appendTransform(lut);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UIDLUT42">
    <LUT1D id="lut01" name="test-lut" inBitDepth="32f" outBitDepth="10i">
        <Array dim="16 1">
   0
   3
   6
   9
  12
  15
  18
  21
  24
  27
  30
  33
  36
  39
  42
  45
        </Array>
    </LUT1D>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, lut1d_array_16x3_ctf)
{
    OCIO::Lut1DTransformRcPtr lut = OCIO::Lut1DTransform::Create();
    lut->setInterpolation(OCIO::INTERP_DEFAULT);
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, "test-lut");
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "lut01");
    lut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);
    lut->setLength(16);
    float rgb = 0.f;
    for (unsigned long i = 0; i < 16; ++i)
    {
        lut->setValue(i, rgb / 1023.0f, (rgb + 1.0f) / 1023.0f, (rgb + 2.0f) / 1023.0f);
        rgb += 3.0f;
    }

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDLUT42");
    group->appendTransform(lut);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UIDLUT42">
    <LUT1D id="lut01" name="test-lut" inBitDepth="32f" outBitDepth="10i">
        <Array dim="16 3">
   0    1    2
   3    4    5
   6    7    8
   9   10   11
  12   13   14
  15   16   17
  18   19   20
  21   22   23
  24   25   26
  27   28   29
  30   31   32
  33   34   35
  36   37   38
  39   40   41
  42   43   44
  45   46   47
        </Array>
    </LUT1D>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, lut1d_10i_ctf)
{
    OCIO::Lut1DTransformRcPtr lut = OCIO::Lut1DTransform::Create();
    lut->setInterpolation(OCIO::INTERP_DEFAULT);
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, "test-lut");
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "lut01");
    lut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);
    lut->setLength(3);
    lut->setValue(1, 511.0f / 1023.0f, 4011.12345f / 1023.0f, -24.10297f / 1023.0f);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDLUT42");
    group->appendTransform(lut);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UIDLUT42">
    <LUT1D id="lut01" name="test-lut" inBitDepth="32f" outBitDepth="10i">
        <Array dim="3 3">
   0    0    0
 511 4011.12 -24.103
   1023    1023    1023
        </Array>
    </LUT1D>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, lut1d_inverse_ctf)
{
    OCIO::Lut1DTransformRcPtr lut = OCIO::Lut1DTransform::Create();
    lut->setInterpolation(OCIO::INTERP_DEFAULT);
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, "test-lut");
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "lut01");
    lut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);
    lut->setLength(16);
    float rgb = 0.f;
    for (unsigned long i = 0; i < 16; ++i)
    {
        lut->setValue(i, rgb / 1023.0f, (rgb + 1.0f) / 1023.0f, (rgb + 2.0f) / 1023.0f);
        rgb += 3.0f;
    }

    lut->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDLUT42");
    group->appendTransform(lut);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    // Note the type of the node.
    //
    // For an InverseLUT1D, the scaling of array values is based on the inBitDepth.
    //
    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UIDLUT42">
    <InverseLUT1D id="lut01" name="test-lut" inBitDepth="10i" outBitDepth="32f">
        <Array dim="16 3">
   0    1    2
   3    4    5
   6    7    8
   9   10   11
  12   13   14
  15   16   17
  18   19   20
  21   22   23
  24   25   26
  27   28   29
  30   31   32
  33   34   35
  36   37   38
  39   40   41
  42   43   44
  45   46   47
        </Array>
    </InverseLUT1D>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, lut3d_array_ctf)
{
    OCIO::Lut3DTransformRcPtr lut = OCIO::Lut3DTransform::Create();
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, "test-lut3d");
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "lut01");
    lut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);
    lut->setInterpolation(OCIO::INTERP_TETRAHEDRAL);
    const unsigned long gs = 3;
    lut->setGridSize(gs);
    float rgb = 0.f;
    for (unsigned long r = 0; r < gs; ++r)
    {
        for (unsigned long g = 0; g < 3; ++g)
        {
            for (unsigned long b = 0; b < 3; ++b)
            {
                lut->setValue(r, g, b, rgb / 1023.0f,
                                       (rgb + 1.0f) / 1023.0f,
                                       (rgb + 2.0f) / 1023.0f);
                rgb += 3.0f;
            }
        }
    }

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDLUT42");
    group->appendTransform(lut);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UIDLUT42">
    <LUT3D id="lut01" name="test-lut3d" inBitDepth="32f" outBitDepth="10i" interpolation="tetrahedral">
        <Array dim="3 3 3 3">
   0    1    2
   3    4    5
   6    7    8
   9   10   11
  12   13   14
  15   16   17
  18   19   20
  21   22   23
  24   25   26
  27   28   29
  30   31   32
  33   34   35
  36   37   38
  39   40   41
  42   43   44
  45   46   47
  48   49   50
  51   52   53
  54   55   56
  57   58   59
  60   61   62
  63   64   65
  66   67   68
  69   70   71
  72   73   74
  75   76   77
  78   79   80
        </Array>
    </LUT3D>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, lut3d_inverse_clf)
{
    OCIO::Lut3DTransformRcPtr lut = OCIO::Lut3DTransform::Create();
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, "test-lut3d");
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "lut01");
    lut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);
    const unsigned long gs = 3;
    lut->setGridSize(gs);
    float rgb = 0.f;
    for (unsigned long r = 0; r < gs; ++r)
    {
        for (unsigned long g = 0; g < 3; ++g)
        {
            for (unsigned long b = 0; b < 3; ++b)
            {
                lut->setValue(r, g, b, rgb / 1023.0f,
                                       (rgb + 1.0f) / 1023.0f,
                                       (rgb + 2.0f) / 1023.0f);
                rgb += 3.0f;
            }
        }
    }

    lut->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDLUT42");
    group->appendTransform(lut);

    std::ostringstream outputTransform;
    OCIO_CHECK_THROW_WHAT(WriteGroupCLF(group, outputTransform),
                          OCIO::Exception,
                          "Transform uses the 'InverseLUT3D' op which cannot be written as CLF");
}

OCIO_ADD_TEST(CTFTransform, lut3d_inverse_ctf)
{
    OCIO::Lut3DTransformRcPtr lut = OCIO::Lut3DTransform::Create();
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, "test-lut3d");
    lut->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "lut01");
    lut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);
    const unsigned long gs = 3;
    lut->setGridSize(gs);
    float rgb = 0.f;
    for (unsigned long r = 0; r < gs; ++r)
    {
        for (unsigned long g = 0; g < 3; ++g)
        {
            for (unsigned long b = 0; b < 3; ++b)
            {
                lut->setValue(r, g, b, rgb / 1023.0f,
                                       (rgb + 1.0f) / 1023.0f,
                                       (rgb + 2.0f) / 1023.0f);
                rgb += 3.0f;
            }
        }
    }

    lut->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDLUT42");
    group->appendTransform(lut);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    // Note the type of the node.
    //
    // For an InverseLUT3D, the scaling of array values is based on the inBitDepth.
    //
    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.6" id="UIDLUT42">
    <InverseLUT3D id="lut01" name="test-lut3d" inBitDepth="10i" outBitDepth="32f">
        <Array dim="3 3 3 3">
   0    1    2
   3    4    5
   6    7    8
   9   10   11
  12   13   14
  15   16   17
  18   19   20
  21   22   23
  24   25   26
  27   28   29
  30   31   32
  33   34   35
  36   37   38
  39   40   41
  42   43   44
  45   46   47
  48   49   50
  51   52   53
  54   55   56
  57   58   59
  60   61   62
  63   64   65
  66   67   68
  69   70   71
  72   73   74
  75   76   77
  78   79   80
        </Array>
    </InverseLUT3D>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, save_lut3d_interpolation)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    OCIO::Lut3DTransformRcPtr lut = OCIO::Lut3DTransform::Create();
    lut->setInterpolation(OCIO::INTERP_DEFAULT);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->appendTransform(lut);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    {
        const std::string result = outputTransform.str();
        const std::string expected = R"(<LUT3D inBitDepth="32f" outBitDepth="32f">)";
        OCIO_CHECK_ASSERT(result.find(expected) != std::string::npos);
    }

    outputTransform.str("");
    outputTransform.clear();
    group = OCIO::GroupTransform::Create();

    // INTERP_BEST is not a valid CTF/CLF method, so write its concrete value, which is
    // tetrahedral.
    lut->setInterpolation(OCIO::INTERP_BEST);
    group = OCIO::GroupTransform::Create();
    group->appendTransform(lut);
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    {
        const std::string result = outputTransform.str();
        const std::string expected = R"(<LUT3D inBitDepth="32f" outBitDepth="32f" interpolation="tetrahedral">)";
        OCIO_CHECK_ASSERT(result.find(expected) != std::string::npos);
    }

    outputTransform.str("");
    outputTransform.clear();
    group = OCIO::GroupTransform::Create();

    lut->setInterpolation(OCIO::INTERP_LINEAR);
    group = OCIO::GroupTransform::Create();
    group->appendTransform(lut);
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    {
        const std::string result = outputTransform.str();
        const std::string expected = R"(<LUT3D inBitDepth="32f" outBitDepth="32f" interpolation="trilinear">)";
        OCIO_CHECK_ASSERT(result.find(expected) != std::string::npos);
    }

    outputTransform.str("");
    outputTransform.clear();
    group = OCIO::GroupTransform::Create();

    lut->setInterpolation(OCIO::INTERP_TETRAHEDRAL);
    group = OCIO::GroupTransform::Create();
    group->appendTransform(lut);
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    {
        const std::string result = outputTransform.str();
        const std::string expected = R"(<LUT3D inBitDepth="32f" outBitDepth="32f" interpolation="tetrahedral">)";
        OCIO_CHECK_ASSERT(result.find(expected) != std::string::npos);
    }

    outputTransform.str("");
    outputTransform.clear();
    group = OCIO::GroupTransform::Create();

    lut->setInterpolation(OCIO::INTERP_CUBIC);
    group = OCIO::GroupTransform::Create();
    group->appendTransform(lut);
    OCIO_CHECK_THROW_WHAT(WriteGroupCTF(group, outputTransform), OCIO::Exception,
                          "Lut3D does not support interpolation algorithm: cubic");
}

OCIO_ADD_TEST(CTFTransform, bitdepth_ctf)
{
    auto mat = OCIO::MatrixTransform::Create();
    mat->setFileInputBitDepth(OCIO::BIT_DEPTH_UINT8);
    mat->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    auto lut = OCIO::Lut1DTransform::Create();
    lut->setInterpolation(OCIO::INTERP_DEFAULT);
    lut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);
    lut->setLength(3);

    auto exp = OCIO::ExponentTransform::Create();

    auto range = OCIO::RangeTransform::Create();
    range->setFileInputBitDepth(OCIO::BIT_DEPTH_F16);
    range->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT12);
    range->setMinInValue(0.1);
    range->setMinOutValue(-0.1);
    range->setMaxInValue(0.9);
    range->setMaxOutValue(1.1);

    auto log = OCIO::LogTransform::Create();

    auto invlut = OCIO::Lut1DTransform::Create();
    invlut->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    invlut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT16);
    invlut->setLength(3);

    auto mat2 = OCIO::MatrixTransform::Create();
    mat2->setFileInputBitDepth(OCIO::BIT_DEPTH_UINT8);
    mat2->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);
    mat2->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");

    // Transforms are setup as follows:
    //
    // Matrix    fileIn =  8i, fileOut = 10i
    // Lut1D                   fileOut = 10i
    // Exponent
    // Range     fileIn = 16f, fileOut = 12i
    // Matrix    fileIn =  8i, fileOut = 10i
    // Log
    // InvLut1D                fileOut = 16i
    // InvMatrix fileIn =  8i, fileOut = 10i
    // InvLut1D                fileOut = 16i

    // First op keeps its in & out bit-depth.
    // <Matrix inBitDepth="8i" outBitDepth="10i">
    group->appendTransform(mat);

    // Previous op out bit-depth used for in bit-depth.
    // <LUT1D inBitDepth="10i" outBitDepth="10i">
    group->appendTransform(lut);

    // Previous op out bit-depth used for in bit-depth.
    // And next op (range) in bit-depth used for out bit-depth.
    // <Exponent inBitDepth="10i" outBitDepth="16f">
    group->appendTransform(exp);

    // In bit-depth preserved and has been used for out bit-depth of previous op.
    // Next op is a matrix, but current op is range, first op out bit-depth
    // is preserved and overrides the next op's in in bit-depth.
    // <Range inBitDepth="16f" outBitDepth="12i">
    group->appendTransform(range);

    // Previous op out bit-depth used for in bit-depth.
    // <Matrix inBitDepth="12i" outBitDepth="10i">
    group->appendTransform(mat);

    // Previous op out bit-depth used for in bit-depth. Out depth is set by preference
    // of the next op.
    // <Log inBitDepth="10i" outBitDepth="16i">
    group->appendTransform(log);

    // Preferred in bit-depth is preserved. Out depth is set by the next op.
    // <InverseLUT1D inBitDepth="16i" outBitDepth="32f">
    group->appendTransform(invlut);

    // Sets both its preferred in and out depth. Note that because the transform
    // direction is inverse, the mapping of file depths is swapped.
    // <Matrix inBitDepth="10i" outBitDepth="8i">
    group->appendTransform(mat2);

    // This time it doesn't get its preferred in depth, since the previous op has priority.
    // The array values are scaled accordingly.
    // <InverseLUT1D inBitDepth="8i" outBitDepth="32f">
    group->appendTransform(invlut);

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="2" id="UID42">
    <Matrix inBitDepth="8i" outBitDepth="10i">
        <Array dim="3 3">
   4.01176470588235                   0                   0
                  0    4.01176470588235                   0
                  0                   0    4.01176470588235
        </Array>
    </Matrix>
    <LUT1D inBitDepth="10i" outBitDepth="10i">
        <Array dim="3 1">
   0
511.5
 1023
        </Array>
    </LUT1D>
    <Exponent inBitDepth="10i" outBitDepth="16f" style="basicFwd">
        <ExponentParams exponent="1" />
    </Exponent>
    <Range inBitDepth="16f" outBitDepth="12i">
        <minInValue> 0.1 </minInValue>
        <maxInValue> 0.9 </maxInValue>
        <minOutValue> -409.5 </minOutValue>
        <maxOutValue> 4504.5 </maxOutValue>
    </Range>
    <Matrix inBitDepth="12i" outBitDepth="10i">
        <Array dim="3 3">
   0.24981684981685                   0                   0
                  0    0.24981684981685                   0
                  0                   0    0.24981684981685
        </Array>
    </Matrix>
    <Log inBitDepth="10i" outBitDepth="16i" style="log2">
    </Log>
    <InverseLUT1D inBitDepth="16i" outBitDepth="10i">
        <Array dim="3 1">
    0
32767.5
  65535
        </Array>
    </InverseLUT1D>
    <Matrix inBitDepth="10i" outBitDepth="8i">
        <Array dim="3 3">
  0.249266862170088                   0                   0
                  0   0.249266862170088                   0
                  0                   0   0.249266862170088
        </Array>
    </Matrix>
    <InverseLUT1D inBitDepth="8i" outBitDepth="32f">
        <Array dim="3 1">
  0
127.5
  255
        </Array>
    </InverseLUT1D>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, no_ops_ctf)
{
    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");

    std::ostringstream outputTransform;
    OCIO_CHECK_NO_THROW(WriteGroupCTF(group, outputTransform));

    const std::string expected{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UIDEC42">
    <Matrix inBitDepth="32f" outBitDepth="32f">
        <Array dim="3 3 3">
                  1                   0                   0
                  0                   1                   0
                  0                   0                   1
        </Array>
    </Matrix>
</ProcessList>
)" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

///////////////////////////////////////////////////////////////////////////////
//
// BAKER TESTS
//
///////////////////////////////////////////////////////////////////////////////

OCIO_ADD_TEST(FileFormatCTF, bake_1d)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("input");
        cs->setFamily("input");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        config->addColorSpace(cs);
    }

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat(OCIO::FILEFORMAT_CLF);
    baker->setInputSpace("input");
    baker->setTargetSpace("target");
    baker->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    baker->setCubeSize(2);
    std::ostringstream outputCLF;
    baker->bake(outputCLF);

    const std::string expectedCLF{
R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="UID42">
    <LUT1D inBitDepth="32f" outBitDepth="32f">
        <Array dim="2 1">
          0
          1
        </Array>
    </LUT1D>
</ProcessList>
)" };
    OCIO_CHECK_EQUAL(expectedCLF.size(), outputCLF.str().size());
    OCIO_CHECK_EQUAL(expectedCLF, outputCLF.str());

    std::ostringstream outputCTF;
    baker->setFormat(OCIO::FILEFORMAT_CTF);
    baker->bake(outputCTF);
    const std::string expectedCTF{ R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList version="1.3" id="UID42">
    <LUT1D inBitDepth="32f" outBitDepth="32f">
        <Array dim="2 1">
          0
          1
        </Array>
    </LUT1D>
</ProcessList>
)" };
    OCIO_CHECK_EQUAL(expectedCTF.size(), outputCTF.str().size());
    OCIO_CHECK_EQUAL(expectedCTF, outputCTF.str());
}

OCIO_ADD_TEST(FileFormatCTF, bake_1d_shaper)
{
    OCIO::BakerRcPtr bake;
    std::ostringstream os;

    constexpr auto myProfile = R"(
        ocio_profile_version: 1

        colorspaces:
        - !<ColorSpace>
          name : Raw
          isdata : false

        - !<ColorSpace>
          name: Log2
          isdata: false
          from_reference: !<GroupTransform>
            children:
              - !<MatrixTransform> {matrix: [5.55556, 0, 0, 0, 0, 5.55556, 0, 0, 0, 0, 5.55556, 0, 0, 0, 0, 1]}
              - !<LogTransform> {base: 2}
              - !<MatrixTransform> {offset: [6.5, 6.5, 6.5, 0]}
              - !<MatrixTransform> {matrix: [0.076923, 0, 0, 0, 0, 0.076923, 0, 0, 0, 0, 0.076923, 0, 0, 0, 0, 1]}
    )";

    std::istringstream is(myProfile);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_REQUIRE_ASSERT(config);

    {
        // Lin to Log
        OCIO::BakerRcPtr baker = OCIO::Baker::Create();
        baker->setConfig(config);
        baker->setFormat(OCIO::FILEFORMAT_CLF);
        baker->setInputSpace("Raw");
        baker->setTargetSpace("Log2");
        baker->setShaperSpace("Log2");
        baker->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
        baker->setCubeSize(10);
        std::stringstream outputCTF;
        baker->bake(outputCTF);

        std::string emptyString;
        OCIO::LocalFileFormat tester;
        OCIO::CachedFileRcPtr file = tester.read(outputCTF, emptyString, OCIO::INTERP_DEFAULT);
        auto cachedFile = OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(file);

        const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
        OCIO_REQUIRE_EQUAL(opList.size(), 2);

        auto range = OCIO_DYNAMIC_POINTER_CAST<const OCIO::RangeOpData>(opList[0]);
        OCIO_REQUIRE_ASSERT(range);
        OCIO_CHECK_EQUAL(range->getMinInValue(), 0.00198873621411622);
        OCIO_CHECK_EQUAL(range->getMaxInValue(), 16.291877746582);
        OCIO_CHECK_EQUAL(range->getMinOutValue(), 0);
        OCIO_CHECK_EQUAL(range->getMaxOutValue(), 1);

        auto lut = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Lut1DOpData>(opList[1]);
        OCIO_REQUIRE_ASSERT(lut);
        OCIO_REQUIRE_EQUAL(lut->getArray().getLength(), 10);
        OCIO_CHECK_CLOSE(lut->getArray()[0],           0, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[3],   0.7562682, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[6],  0.83313024, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[9],  0.87810701, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[12],  0.9100228, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[15], 0.93478036, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[18],  0.9550097, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[21], 0.97211391, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[24], 0.98693061, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[27],          1, 1.e-5f);
    }

    {
        // Log to Lin
        OCIO::BakerRcPtr baker = OCIO::Baker::Create();
        baker->setConfig(config);
        baker->setFormat(OCIO::FILEFORMAT_CLF);
        baker->setInputSpace("Log2");
        baker->setTargetSpace("Raw");
        baker->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
        baker->setCubeSize(10);
        std::stringstream outputCTF;
        baker->bake(outputCTF);

        std::string emptyString;
        OCIO::LocalFileFormat tester;
        OCIO::CachedFileRcPtr file = tester.read(outputCTF, emptyString, OCIO::INTERP_DEFAULT);
        auto cachedFile = OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(file);

        const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
        OCIO_REQUIRE_EQUAL(opList.size(), 1);

        auto lut = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Lut1DOpData>(opList[0]);
        OCIO_REQUIRE_ASSERT(lut);
        OCIO_REQUIRE_EQUAL(lut->getArray().getLength(), 10);
        OCIO_CHECK_CLOSE(lut->getArray()[0],  0.0019887362, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[3],  0.0054125111, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[6],   0.014730596, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[9],   0.040090535, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[12],   0.10910972, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[15],   0.29695117, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[18],   0.80817699, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[21],    2.1995215, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[24],    5.9861789, 1.e-5f);
        OCIO_CHECK_CLOSE(lut->getArray()[27],    16.291878, 1.e-5f);
    }
}

OCIO_ADD_TEST(FileFormatCTF, bake_3d)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("input");
        cs->setFamily("input");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");

        // Set saturation to cause channel crosstalk, making a 3D LUT
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        transform1->setSat(0.5f);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);

        config->addColorSpace(cs);
    }

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    auto & data = baker->getFormatMetadata();
    data.addAttribute(OCIO::METADATA_ID, "TestID");
    data.addChildElement(OCIO::METADATA_DESCRIPTION,
                         "OpenColorIO Test Line 1");
    data.addChildElement(OCIO::METADATA_DESCRIPTION,
                         "OpenColorIO Test Line 2");
    data.addChildElement("Anything", "Not Saved");
    data.addChildElement(OCIO::METADATA_INPUT_DESCRIPTOR, "Input descriptor");
    data.addChildElement(OCIO::METADATA_INPUT_DESCRIPTOR, "Only first is saved");
    data.addChildElement(OCIO::METADATA_OUTPUT_DESCRIPTOR, "Output descriptor");
    data.addChildElement(OCIO::METADATA_INFO, "");
    auto & info = data.getChildElement(6);
    info.addAttribute("attrib1", "val1");
    info.addAttribute("attrib2", "val2");
    info.addChildElement("anything", "is saved");
    info.addChildElement("anything", "is also saved");

    baker->setFormat(OCIO::FILEFORMAT_CLF);
    baker->setInputSpace("input");
    baker->setTargetSpace("target");
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);

    const std::string expectedCLF{
R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="TestID">
    <Description>OpenColorIO Test Line 1</Description>
    <Description>OpenColorIO Test Line 2</Description>
    <InputDescriptor>Input descriptor</InputDescriptor>
    <OutputDescriptor>Output descriptor</OutputDescriptor>
    <Info attrib1="val1" attrib2="val2">
        <anything>is saved</anything>
        <anything>is also saved</anything>
    </Info>
    <LUT3D inBitDepth="32f" outBitDepth="32f">
        <Array dim="2 2 2 3">
          0           0           0
     0.0361      0.0361  0.53609997
     0.3576  0.85759997      0.3576
     0.3937      0.8937      0.8937
     0.6063      0.1063      0.1063
 0.64240003      0.1424  0.64239997
 0.96389997  0.96389997      0.4639
          1           1           1
        </Array>
    </LUT3D>
</ProcessList>
)" };
    OCIO_CHECK_EQUAL(expectedCLF.size(), output.str().size());
    OCIO_CHECK_EQUAL(expectedCLF, output.str());
}

OCIO_ADD_TEST(FileFormatCTF, bake_1d_3d)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("input");
        cs->setFamily("input");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("shaper");
        cs->setFamily("shaper");
        OCIO::MatrixTransformRcPtr transform1 = OCIO::MatrixTransform::Create();
        double mat[16]{ 0.8,   0,   0, 0,
                          0, 0.8,   0, 0,
                          0,   0, 0.8, 0,
                          0,   0,   0, 1 };
        transform1->setMatrix(mat);
        double offset[4]{ 0.1, 0.1, 0.1, 0 };
        transform1->setOffset(offset);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");

        // Set saturation to cause channel crosstalk, making a 3D LUT
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        transform1->setStyle(OCIO::CDL_ASC);
        transform1->setSat(0.5f);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);

        config->addColorSpace(cs);
    }

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat(OCIO::FILEFORMAT_CLF);
    baker->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    baker->setInputSpace("input");
    baker->setShaperSpace("shaper");
    baker->setTargetSpace("target");
    baker->setCubeSize(2);
    std::stringstream output;
    baker->bake(output);

    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr file = tester.read(output, emptyString, OCIO::INTERP_DEFAULT);
    auto cachedFile = OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(file);

    const OCIO::ConstOpDataVec & opList = cachedFile->m_transform->getOps();
    OCIO_REQUIRE_EQUAL(opList.size(), 2);
    auto shaperLut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[0]);
    OCIO_REQUIRE_ASSERT(shaperLut);
    OCIO_CHECK_ASSERT(shaperLut->isInputHalfDomain());
    const auto & shaperArray = shaperLut->getArray();
    // Calculate the index for 0.5 in a half-domain LUT1D. We'll test the value there.
    const half h05(0.5f);
    const auto h05bits = h05.bits();
    const auto index = h05bits * 3;
    const auto res = 0.5f * 0.8f + 0.1f;

    OCIO_CHECK_CLOSE(shaperArray[index + 0], res, 1e-5f);
    OCIO_CHECK_EQUAL(shaperArray[index + 0], shaperArray[index + 1]);
    OCIO_CHECK_EQUAL(shaperArray[index + 0], shaperArray[index + 2]);

    auto lut = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opList[1]);
    OCIO_REQUIRE_ASSERT(lut);
    OCIO_REQUIRE_EQUAL(lut->getArray().getLength(), 2);
    OCIO_CHECK_EQUAL(lut->getArray()[0], 0.0f);
    OCIO_CHECK_EQUAL(lut->getArray()[1], 0.0f);
    OCIO_CHECK_EQUAL(lut->getArray()[2], 0.0f);
    OCIO_CHECK_CLOSE(lut->getArray()[3], 0.0361f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[4], 0.0361f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[5], 0.5361f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[6], 0.3576f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[7], 0.8576f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[8], 0.3576f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[9], 0.3937f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[10], 0.8937f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[11], 0.8937f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[12], 0.6063f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[13], 0.1063f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[14], 0.1063f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[15], 0.6424f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[16], 0.1424f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[17], 0.6424f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[18], 0.9639f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[19], 0.9639f, 1.e-5f);
    OCIO_CHECK_CLOSE(lut->getArray()[20], 0.4639f, 1.e-5f);
    OCIO_CHECK_EQUAL(lut->getArray()[21], 1.0f);
    OCIO_CHECK_EQUAL(lut->getArray()[22], 1.0f);
    OCIO_CHECK_EQUAL(lut->getArray()[23], 1.0f);

    std::ostringstream output1;
    baker->setShaperSize(10);
    baker->bake(output1);

    const std::string expectedCLF{
R"(<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="UID42">
    <Range inBitDepth="32f" outBitDepth="32f">
        <minInValue> -0.125 </minInValue>
        <maxInValue> 1.125 </maxInValue>
        <minOutValue> 0 </minOutValue>
        <maxOutValue> 1 </maxOutValue>
    </Range>
    <LUT1D inBitDepth="32f" outBitDepth="32f">
        <Array dim="10 1">
          0
 0.11111112
 0.22222224
 0.33333334
 0.44444448
 0.55555558
 0.66666675
 0.77777779
 0.88888896
          1
        </Array>
    </LUT1D>
    <LUT3D inBitDepth="32f" outBitDepth="32f">
        <Array dim="2 2 2 3">
          0           0           0
     0.0361      0.0361  0.53609997
     0.3576  0.85759997      0.3576
     0.3937      0.8937      0.8937
     0.6063      0.1063      0.1063
 0.64240003      0.1424  0.64239997
 0.96389997  0.96389997      0.4639
          1           1           1
        </Array>
    </LUT3D>
</ProcessList>
)" };

    const StringUtils::StringVec osvec  = StringUtils::SplitByLines(output1.str());
    const StringUtils::StringVec resvec = StringUtils::SplitByLines(expectedCLF);
    OCIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < resvec.size(); ++i)
    {
        if ( (i >= 10 && i <= 19) ||
             (i >= 24 && i <= 31))
        {
            OCIO_CHECK_STR_FLOAT_VEC_CLOSE(osvec[i], resvec[i], 1e-5f);
        }
        else
        {
            OCIO_CHECK_EQUAL(osvec[i], resvec[i]);
        }

    }
}

OCIO_ADD_TEST(FileFormatCTF, lut_interpolation_option)
{
    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    // CLF file containing a LUT3D that does not specifiy an interpolation.
    const std::string filePath{ std::string(OCIO::GetTestFilesDir()) +
                                "/clf/lut3d_17x17x17_10i_12i.clf" };

    OCIO::FileTransformRcPtr fileTransform = OCIO::FileTransform::Create();
    fileTransform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    fileTransform->setSrc(filePath.c_str());

    // Get the processor corresponding to the transform with default interpolation.
    // LUT3D is using default interpolation.
    OCIO::ConstProcessorRcPtr proc;
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    auto group = proc->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    auto lut3D = OCIO_DYNAMIC_POINTER_CAST<OCIO::Lut3DTransform>(transform);
    OCIO_REQUIRE_ASSERT(lut3D);
    OCIO_CHECK_EQUAL(lut3D->getInterpolation(), OCIO::INTERP_DEFAULT);

    // For LUTs that do not specify an interpolation, if the FileTransform interpolation may be
    // used, it is.
    fileTransform->setInterpolation(OCIO::INTERP_BEST);
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    group = proc->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    transform = group->getTransform(0);
    lut3D = OCIO_DYNAMIC_POINTER_CAST<OCIO::Lut3DTransform>(transform);
    OCIO_REQUIRE_ASSERT(lut3D);
    OCIO_CHECK_EQUAL(lut3D->getInterpolation(), OCIO::INTERP_BEST);

    // If the FileTranform interpolation is not supported by the LUT, it is ignored and default
    // interpolation is used.
    fileTransform->setInterpolation(OCIO::INTERP_CUBIC);
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    group = proc->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    transform = group->getTransform(0);
    lut3D = OCIO_DYNAMIC_POINTER_CAST<OCIO::Lut3DTransform>(transform);
    OCIO_REQUIRE_ASSERT(lut3D);
    OCIO_CHECK_EQUAL(lut3D->getInterpolation(), OCIO::INTERP_DEFAULT);

    // CTF file containing a LUT3D that specifies tetrahedral interpolation.
    const std::string filePath2{ std::string(OCIO::GetTestFilesDir()) +
                                 "/lut3d_example_Inv.ctf" };

    fileTransform->setSrc(filePath2.c_str());

    // Whatever the file transform interpolation, LUT will keep its interpolation.
    fileTransform->setInterpolation(OCIO::INTERP_DEFAULT);
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    group = proc->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    transform = group->getTransform(0);
    lut3D = OCIO_DYNAMIC_POINTER_CAST<OCIO::Lut3DTransform>(transform);
    OCIO_REQUIRE_ASSERT(lut3D);
    OCIO_CHECK_EQUAL(lut3D->getInterpolation(), OCIO::INTERP_TETRAHEDRAL);

    // (INTERP_LINEAR is trilinear for LUT3D.)
    fileTransform->setInterpolation(OCIO::INTERP_LINEAR);
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    group = proc->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    transform = group->getTransform(0);
    lut3D = OCIO_DYNAMIC_POINTER_CAST<OCIO::Lut3DTransform>(transform);
    OCIO_REQUIRE_ASSERT(lut3D);
    OCIO_CHECK_EQUAL(lut3D->getInterpolation(), OCIO::INTERP_TETRAHEDRAL);
}
