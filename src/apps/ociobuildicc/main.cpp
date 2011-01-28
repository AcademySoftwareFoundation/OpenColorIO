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

//
// Build a ICC profile for doing soft proofing
//
// N-component LUT-based display profile required tags
// ------------------------------------------------------------------------------
// Tag Name                 General Description
// ------------------------------------------------------------------------------
// profileDescriptionTag    Structure containing invariant and localizable
//                          versions of the profile name for display
// AToB0Tag                 Device to PCS: 8-bit or 16-bit data: intent of 0
// BToA0Tag                 PCS to Device space: 8-bit or 16-bit data: intent of 0
// mediaWhitePointTag       Media XYZ white point
// copyrightTag             Profile copyright information
// chromaticAdaptationTag   Converts XYZ colour from the actual illumination
//                          source to PCS illuminant. Required only if the actual
//                          illumination source is not D50.

#include <iostream>
#include <cmath>
#include <vector>

#include "lcms2.h"
#include "lcms2_plugin.h"

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include <boost/program_options.hpp>
namespace po = boost::program_options;

void ErrorHandler(cmsContext /*ContextID*/, cmsUInt32Number /*ErrorCode*/, const char *Text)
{
    std::cout << "OCIO Error: " << Text << "\n";
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
    cmsFloat32Number pix[3] = {in[0]/65535.f, in[1]/65535.f, in[2]/65535.f};
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

void
print_help(boost::program_options::options_description desc) {
    std::cout << "\n";
    std::cout << "ociobuildicc <options> outputprofile.icc\n";
    std::cout << "\n";
    std::cout << desc << "\n";
    return;
}

int main (int argc, char* argv[])
{
    
    int cubesize;
    int whitepointtemp;
    std::string displayicc;
    std::string description;
    std::string copyright;
    std::string workingspace;
    std::string viewspace;
    std::string outputfile;
    
    try
    {
        
        po::options_description desc("Command Line Options");
        desc.add_options()
            ("help,h", "produce this help message")
            ("cubesize", po::value<int>(&cubesize)->default_value(32),
                "size of the icc CLUT cube")
            ("whitepoint", po::value<int>(&whitepointtemp)->default_value(6505),
                "whitepoint for the profile")
            ("displayicc", po::value<std::string>(&displayicc)->default_value(""),
                "an icc profile which matches the OCIO profiles target display")
            ("description", po::value<std::string>(&description)->default_value(""),
                "a meaningful description, this will show up in UI like photoshop")
            ("copyright", po::value<std::string>(&copyright)->default_value("Sony Imageworks"),
                "a copyright field (this is required to make a vaild profile)")
            ("workingspace", po::value<std::string>(&workingspace)->default_value(""),
                "the workingspace of the file being viewed")
            ("viewspace", po::value<std::string>(&viewspace)->default_value(""),
                "the viewspace of the profile")
            ("outputfile", po::value<std::string>(&outputfile), "output icc profile")
            ;
        
        po::positional_options_description posi;
        posi.add("outputfile", -1);
        
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(posi).run(), vm);
        po::notify(vm);
        
        // print help
        if(vm.count("help"))
        {
            print_help(desc);
            return 1;
        }
        
        if(!vm.count("outputfile"))
        {
            std::cout << "you need to specify a output icc path\n";
            print_help(desc);
            return 1;
        }
        
        if(description == "")
        {
            std::cout << "need to specify a --description to embed in the icc profile\n";
            print_help(desc);
            return 1;
        }
        
        if(copyright == "")
        {
            std::cout << "need to specify a --copyright to embed in the icc profile\n";
            print_help(desc);
            return 1;
        }
        
        if(workingspace == "")
        {
            std::cout << "need to specify a --workingspace of the source that the icc profile will be applied\n";
            print_help(desc);
            return 1;
        }
        
        if(viewspace == "")
        {
            std::cout << "need to specify a --viewspace of the display for the icc profile\n";
            print_help(desc);
            return 1;
        }
        
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
        
        // 16Bit
        data.to_PCS16 = cmsCreateTransform(DisplayProfile, TYPE_RGB_16, labProfile, TYPE_LabV2_16,
                                           INTENT_PERCEPTUAL, cmsFLAGS_NOOPTIMIZE|cmsFLAGS_NOCACHE);
        data.from_PCS16 = cmsCreateTransform(labProfile, TYPE_LabV2_16, DisplayProfile, TYPE_RGB_16,
                                             INTENT_PERCEPTUAL, cmsFLAGS_NOOPTIMIZE|cmsFLAGS_NOCACHE);
        
        //
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        data.processor = config->getProcessor(workingspace.c_str(), viewspace.c_str());
        
        //
        // AToB0Tag - Device to PCS (16-bit) intent of 0 (perceptual)
        //
        // cmsSigCurveSetElemType
        // `- cmsSigCLutElemType
        //  `- cmsSigCurveSetElemType
        //   `- cmsSigMatrixElemType
        //    `- cmsSigCurveSetElemType
        //
        std::cout << "[OpenColorIO INFO]: Adding AToB0Tag\n";
        cmsPipeline* AToB0Tag = cmsPipelineAlloc(NULL, 3, 3);
        
        Add3GammaCurves(AToB0Tag, 1.f); // cmsSigCurveSetElemType
        
        // cmsSigCLutElemType
        cmsStage* AToB0Clut = cmsStageAllocCLut16bit(NULL, cubesize, 3, 3, NULL);
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
        std::cout << "[OpenColorIO INFO]: Adding BToA0Tag\n";
        cmsPipeline* BToA0Tag = cmsPipelineAlloc(NULL, 3, 3);
        
        Add3GammaCurves(BToA0Tag, 1.f); // cmsSigCurveSetElemType
        AddIdentityMatrix(BToA0Tag);    // cmsSigMatrixElemType
        Add3GammaCurves(BToA0Tag, 1.f); // cmsSigCurveSetElemType
        
        // cmsSigCLutElemType
        cmsStage* BToA0Clut = cmsStageAllocCLut16bit(NULL, cubesize, 3, 3, NULL);
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
        std::cout << "[OpenColorIO INFO]: Writing Profile\n";
        cmsSaveProfileToFile(hProfile, outputfile.c_str());
        cmsCloseProfile(hProfile);
        
        std::cout << "[OpenColorIO INFO]: Finished\n";
    }
    catch(OCIO::Exception & exception)
    {
        std::cerr << "OCIO Error: " << exception.what() << std::endl;
        exit(1);
    } catch (std::exception& exception) {
        std::cerr << "Error: " << exception.what() << "\n";
        return 1;
    }
    catch(...)
    {
        std::cerr << "Unknown OCIO error encountered." << std::endl;
        exit(1);
    }
    
    return 0;
}
