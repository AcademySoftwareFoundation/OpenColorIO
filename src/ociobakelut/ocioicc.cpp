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

#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>

#include "ocioicc.h"

#include "lcms2.h"
#include "lcms2_plugin.h"

OCIO_NAMESPACE_ENTER
{


namespace
{
void ErrorHandler(cmsContext /*ContextID*/, cmsUInt32Number /*ErrorCode*/, const char *Text)
{
    std::cerr << "OCIO Error: " << Text << "\n";
    return;
}

typedef struct
{
    cmsHTRANSFORM to_PCS16;
    cmsHTRANSFORM from_PCS16;
    //OCIO::ConstProcessorRcPtr shaper_processor;
    OCIO::ConstProcessorRcPtr processor;
} SamplerData;

static void Add3GammaCurves(cmsPipeline* lut, cmsFloat64Number Curve)
{
    cmsToneCurve* id = cmsBuildGamma(NULL, Curve);
    cmsToneCurve* id3[3];
    id3[0] = id;
    id3[1] = id;
    id3[2] = id;
    cmsPipelineInsertStage(lut, cmsAT_END, cmsStageAllocToneCurves(NULL, 3, id3));
    cmsFreeToneCurve(id);
}

static void AddIdentityMatrix(cmsPipeline* lut)
{
    const cmsFloat64Number Identity[] = {
        1, 0, 0,
        0, 1, 0, 
        0, 0, 1, 
        0, 0, 0 };
    cmsPipelineInsertStage(lut, cmsAT_END, cmsStageAllocMatrix(NULL, 3, 3, Identity, NULL));
}

static cmsInt32Number Display2PCS_Sampler16(const cmsUInt16Number in[], cmsUInt16Number out[], void* userdata)
{
    //std::cout << "r" << in[0] << " g" << in[1] << " b" << in[2] << "\n";
    SamplerData* data = (SamplerData*) userdata;
    cmsFloat32Number pix[3] = { static_cast<float>(in[0])/65535.f,
                                static_cast<float>(in[1])/65535.f,
                                static_cast<float>(in[2])/65535.f};
    data->processor->applyRGB(pix);
    out[0] = (cmsUInt16Number)std::max(std::min(pix[0] * 65535.f, 65535.f), 0.f);
    out[1] = (cmsUInt16Number)std::max(std::min(pix[1] * 65535.f, 65535.f), 0.f);
    out[2] = (cmsUInt16Number)std::max(std::min(pix[2] * 65535.f, 65535.f), 0.f);
    cmsDoTransform(data->to_PCS16, out, out, 1);
    return 1;
}

static cmsInt32Number PCS2Display_Sampler16(const cmsUInt16Number in[], cmsUInt16Number out[], void* userdata)
{
    //std::cout << "r" << in[0] << " g" << in[1] << " b" << in[2] << "\n";
    SamplerData* data = (SamplerData*) userdata;
    cmsDoTransform(data->from_PCS16, in, out, 1);
    // we don't have a reverse Lab -> Display transform
    return 1;
}
}  // anon namespace


void SaveICCProfileToFile(const std::string & outputfile,
                          ConstProcessorRcPtr & processor,
                          int cubesize,
                          int whitepointtemp,
                          const std::string & displayicc,
                          const std::string & description,
                          const std::string & copyright,
                          bool verbose)
{

    // Create the ICC Profile

    // Setup the Error Handler
    cmsSetLogErrorHandler(ErrorHandler);

    // D65 white point
    cmsCIExyY whitePoint;
    cmsWhitePointFromTemp(&whitePoint, whitepointtemp);

    // LAB PCS
    cmsHPROFILE labProfile = cmsCreateLab4ProfileTHR(NULL, &whitePoint);

    // Display (OCIO sRGB cube -> LAB)
    cmsHPROFILE DisplayProfile;
    if(displayicc != "") DisplayProfile = cmsOpenProfileFromFile(displayicc.c_str(), "r");
    else DisplayProfile = cmsCreate_sRGBProfileTHR(NULL);

    // Create an empty RGB Profile
    cmsHPROFILE hProfile = cmsCreateRGBProfileTHR(NULL, &whitePoint, NULL, NULL);

    if(verbose)
        std::cout << "[OpenColorIO INFO]: Setting up Profile: " << outputfile << "\n";

    // Added Header fields
    cmsSetProfileVersion(hProfile, 4.2);
    cmsSetDeviceClass(hProfile, cmsSigDisplayClass);
    cmsSetColorSpace(hProfile, cmsSigRgbData);
    cmsSetPCS(hProfile, cmsSigLabData);
    cmsSetHeaderRenderingIntent(hProfile, INTENT_PERCEPTUAL);

    //
    cmsMLU* DescriptionMLU = cmsMLUalloc(NULL, 1);
    cmsMLU* CopyrightMLU = cmsMLUalloc(NULL, 1);
    cmsMLUsetASCII(DescriptionMLU, "en", "US", description.c_str());
    cmsMLUsetASCII(CopyrightMLU, "en", "US", copyright.c_str());
    cmsWriteTag(hProfile, cmsSigProfileDescriptionTag, DescriptionMLU);
    cmsWriteTag(hProfile, cmsSigCopyrightTag, CopyrightMLU);

    //
    SamplerData data;
    data.processor = processor;

    // 16Bit
    data.to_PCS16 = cmsCreateTransform(DisplayProfile, TYPE_RGB_16, labProfile, TYPE_LabV2_16,
                                       INTENT_PERCEPTUAL, cmsFLAGS_NOOPTIMIZE|cmsFLAGS_NOCACHE);
    data.from_PCS16 = cmsCreateTransform(labProfile, TYPE_LabV2_16, DisplayProfile, TYPE_RGB_16,
                                         INTENT_PERCEPTUAL, cmsFLAGS_NOOPTIMIZE|cmsFLAGS_NOCACHE);

    //
    // AToB0Tag - Device to PCS (16-bit) intent of 0 (perceptual)
    //
    // cmsSigCurveSetElemType
    // `- cmsSigCLutElemType
    //  `- cmsSigCurveSetElemType
    //   `- cmsSigMatrixElemType
    //    `- cmsSigCurveSetElemType
    //
    
    if(verbose)
        std::cout << "[OpenColorIO INFO]: Adding AToB0Tag\n";
    cmsPipeline* AToB0Tag = cmsPipelineAlloc(NULL, 3, 3);

    Add3GammaCurves(AToB0Tag, 1.f); // cmsSigCurveSetElemType

    // cmsSigCLutElemType
    cmsStage* AToB0Clut = cmsStageAllocCLut16bit(NULL, cubesize, 3, 3, NULL);
    
    if(verbose)
        std::cout << "[OpenColorIO INFO]: Sampling AToB0 CLUT from Display to Lab\n";
    cmsStageSampleCLut16bit(AToB0Clut, Display2PCS_Sampler16, &data, 0);
    cmsPipelineInsertStage(AToB0Tag, cmsAT_END, AToB0Clut);

    Add3GammaCurves(AToB0Tag, 1.f); // cmsSigCurveSetElemType
    AddIdentityMatrix(AToB0Tag);    // cmsSigMatrixElemType
    Add3GammaCurves(AToB0Tag, 1.f); // cmsSigCurveSetElemType

    // Add AToB0Tag
    cmsWriteTag(hProfile, cmsSigAToB0Tag, AToB0Tag);
    cmsPipelineFree(AToB0Tag);

    //
    // BToA0Tag - PCS to Device space (16-bit) intent of 0 (perceptual)
    //
    // cmsSigCurveSetElemType
    // `- cmsSigMatrixElemType
    //  `- cmsSigCurveSetElemType
    //   `- cmsSigCLutElemType 
    //    `- cmsSigCurveSetElemType
    //
    if(verbose)
        std::cout << "[OpenColorIO INFO]: Adding BToA0Tag\n";
    cmsPipeline* BToA0Tag = cmsPipelineAlloc(NULL, 3, 3);

    Add3GammaCurves(BToA0Tag, 1.f); // cmsSigCurveSetElemType
    AddIdentityMatrix(BToA0Tag);    // cmsSigMatrixElemType
    Add3GammaCurves(BToA0Tag, 1.f); // cmsSigCurveSetElemType

    // cmsSigCLutElemType
    cmsStage* BToA0Clut = cmsStageAllocCLut16bit(NULL, cubesize, 3, 3, NULL);
    if(verbose)
        std::cout << "[OpenColorIO INFO]: Sampling BToA0 CLUT from Lab to Display\n";
    cmsStageSampleCLut16bit(BToA0Clut, PCS2Display_Sampler16, &data, 0);
    cmsPipelineInsertStage(BToA0Tag, cmsAT_END, BToA0Clut);

    Add3GammaCurves(BToA0Tag, 1.f); // cmsSigCurveSetElemType

    // Add BToA0Tag
    cmsWriteTag(hProfile, cmsSigBToA0Tag, BToA0Tag);
    cmsPipelineFree(BToA0Tag);

    //
    // D2Bx - Device to PCS (float) (Not Yet Impl)
    //

    //
    // B2Dx - PCS to Device (float) (Not Yet Impl)
    //

    //
    // Write
    //
    if(verbose)
        std::cout << "[OpenColorIO INFO]: Writing " << outputfile << std::endl;
    cmsSaveProfileToFile(hProfile, outputfile.c_str());
    cmsCloseProfile(hProfile);
    
    if(verbose)
        std::cout << "[OpenColorIO INFO]: Finished\n";
}

}
OCIO_NAMESPACE_EXIT
