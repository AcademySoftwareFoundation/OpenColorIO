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

#include "fileformats/FileFormat3DL.cpp"

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"
#include "UnitTestUtils.h"

OCIO::LocalCachedFileRcPtr LoadLutFile(const std::string & fileName)
{
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(
        fileName, std::ios_base::in);
}

OCIO_ADD_TEST(FileFormat3DL, FormatInfo)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.GetFormatInfo(formatInfoVec);

    OCIO_CHECK_EQUAL(2, formatInfoVec.size());
    OCIO_CHECK_EQUAL("flame", formatInfoVec[0].name);
    OCIO_CHECK_EQUAL("lustre", formatInfoVec[1].name);
    OCIO_CHECK_EQUAL("3dl", formatInfoVec[0].extension);
    OCIO_CHECK_EQUAL("3dl", formatInfoVec[1].extension);
    OCIO_CHECK_EQUAL((OCIO::FORMAT_CAPABILITY_READ
        | OCIO::FORMAT_CAPABILITY_WRITE), formatInfoVec[0].capabilities);
    OCIO_CHECK_EQUAL((OCIO::FORMAT_CAPABILITY_READ
        | OCIO::FORMAT_CAPABILITY_WRITE), formatInfoVec[1].capabilities);
}

OCIO_ADD_TEST(FileFormat3DL, Bake)
{

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        float rgb[3] = { 0.0f, 0.1f, 0.2f };
        transform1->setOffset(rgb);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setMetadata("MetaData not written");
    baker->setFormat("flame");
    baker->setInputSpace("lnf");
    baker->setTargetSpace("target");
    baker->setShaperSize(10);
    baker->setCubeSize(2);
    std::ostringstream outputFlame;
    baker->bake(outputFlame);

    std::vector<std::string> osvecFlame;
    pystring::splitlines(outputFlame.str(), osvecFlame);

    std::ostringstream outputLustre;
    baker->setFormat("lustre");
    baker->bake(outputLustre);
    std::vector<std::string> osvecLustre;
    pystring::splitlines(outputLustre.str(), osvecLustre);

    std::ostringstream bout;
    bout << "3DMESH" << "\n";
    bout << "Mesh 0 12" << "\n";
    bout << "0 114 227 341 455 568 682 796 909 1023" << "\n";
    bout << "0 410 819" << "\n";
    bout << "0 410 4095" << "\n";
    bout << "0 4095 819" << "\n";
    bout << "0 4095 4095" << "\n";
    bout << "4095 410 819" << "\n";
    bout << "4095 410 4095" << "\n";
    bout << "4095 4095 819" << "\n";
    bout << "4095 4095 4095" << "\n";
    bout << "" << "\n";
    bout << "LUT8" << "\n";
    bout << "gamma 1.0" << "\n";

    std::vector<std::string> resvec;
    pystring::splitlines(bout.str(), resvec);
    OCIO_CHECK_EQUAL(resvec.size(), osvecLustre.size());
    OCIO_CHECK_EQUAL(resvec.size() - 4, osvecFlame.size());

    OCIO_CHECK_EQUAL(resvec[0], osvecLustre[0]);
    OCIO_CHECK_EQUAL(resvec[1], osvecLustre[1]);
    for (unsigned int i = 0; i < osvecFlame.size(); ++i)
    {
        OCIO_CHECK_EQUAL(resvec[i+2], osvecFlame[i]);
        OCIO_CHECK_EQUAL(resvec[i+2], osvecLustre[i+2]);
    }
    size_t last = resvec.size() - 2;
    OCIO_CHECK_EQUAL(resvec[last], osvecLustre[last]);
    OCIO_CHECK_EQUAL(resvec[last+1], osvecLustre[last+1]);


}

// FILE      EXPECTED MAX    CORRECTLY DECODED IF MAX IN THIS RANGE 
// 8-bit     255             [0, 511]      
// 10-bit    1023            [512, 2047]
// 12-bit    4095            [2048, 8191]
// 14-bit    16383           [8192, 32767]
// 16-bit    65535           [32768, 131071]

OCIO_ADD_TEST(FileFormat3DL, GetLikelyLutBitDepth)
{
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(-1), -1);
    
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(0), 8);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(1), 8);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(255), 8);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(256), 8);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(511), 8);
    
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(512), 10);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(1023), 10);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(1024), 10);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(2047), 10);
    
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(2048), 12);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(4095), 12);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(4096), 12);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(8191), 12);
    
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(16383), 14);
    
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(65535), 16);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(65536), 16);
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(131071), 16);
    
    OCIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(131072), 16);
}


OCIO_ADD_TEST(FileFormat3DL, TestLoad)
{
    // Discreet 3D LUT file
    const std::string discree3DtLut("discreet-3d-lut.3dl");

    OCIO::LocalCachedFileRcPtr lutFile;
    OCIO_CHECK_NO_THROW(lutFile = LoadLutFile(discree3DtLut));

    OCIO_CHECK_ASSERT(lutFile->has1D);
    OCIO_CHECK_ASSERT(lutFile->has3D);
    OCIO_CHECK_EQUAL(OCIO::Lut1D::ERROR_ABSOLUTE, lutFile->lut1D->errortype);
    OCIO_CHECK_EQUAL(0.00195503421f, lutFile->lut1D->maxerror);
    OCIO_CHECK_EQUAL(0.0f, lutFile->lut1D->from_min[1]);
    OCIO_CHECK_EQUAL(1.0f, lutFile->lut1D->from_max[1]);
    OCIO_CHECK_EQUAL(17, lutFile->lut1D->luts[0].size());
    OCIO_CHECK_EQUAL(0.0f, lutFile->lut1D->luts[0][0]);
    OCIO_CHECK_EQUAL(0.563049853f, lutFile->lut1D->luts[0][9]);
    OCIO_CHECK_EQUAL(1.0f, lutFile->lut1D->luts[0][16]);
    OCIO_CHECK_EQUAL(17, lutFile->lut3D->size[0]);
    OCIO_CHECK_EQUAL(17, lutFile->lut3D->size[1]);
    OCIO_CHECK_EQUAL(17, lutFile->lut3D->size[2]);
    OCIO_CHECK_EQUAL(17*17*17*3, lutFile->lut3D->lut.size());
    // LUT is R fast, file is B fast ([3..5] is [867..869] in file)
    OCIO_CHECK_EQUAL(0.00854700897f, lutFile->lut3D->lut[3]);
    OCIO_CHECK_EQUAL(0.00244200253f, lutFile->lut3D->lut[4]);
    OCIO_CHECK_EQUAL(0.00708180759f, lutFile->lut3D->lut[5]);
    OCIO_CHECK_EQUAL(0.0f, lutFile->lut3D->lut[4335]);
    OCIO_CHECK_EQUAL(0.0368742384f, lutFile->lut3D->lut[4336]);
    OCIO_CHECK_EQUAL(0.0705738738f, lutFile->lut3D->lut[4337]);

    const std::string discree3DtLutFail("error_truncated_file.3dl");

    OCIO_CHECK_THROW_WHAT(LoadLutFile(discree3DtLutFail),
                          OCIO::Exception,
                          "Cannot infer 3D LUT size");

}
