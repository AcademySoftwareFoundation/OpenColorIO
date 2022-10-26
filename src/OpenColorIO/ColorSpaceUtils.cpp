// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "ColorSpaceUtils.h"
#include "MathUtils.h"
#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
{
std::string getRefSpace(ConstConfigRcPtr & cfg)
{
    // Find a color space where isData is false and it has neither a to_ref or from_ref 
    // transform.
    auto nbCs = cfg->getNumColorSpaces();
    for (int i = 0; i < nbCs; i++)
    {
        auto cs = cfg->getColorSpace(cfg->getColorSpaceNameByIndex(i));
        if (cs->isData())
        {
            continue;
        }

        auto t = cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
        if (t != nullptr)
        {
            continue;
        }

        t = cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if (t != nullptr) 
        {
            continue;
        }

        return cs->getName();
    }
    return "";
}

bool containsSRGB(ConstColorSpaceRcPtr & cs)
{
    std::string name = StringUtils::Lower(cs->getName());
    if (StringUtils::Find(name, "srgb") != std::string::npos)
    {
        return true;
    }

    auto nbOfAliases = cs->getNumAliases();
    for (auto i = 0; i < nbOfAliases; i++)
    {
        if (StringUtils::Find(cs->getAlias(i), "srgb") != std::string::npos)
        {
            return true;
        }
    }

    return false;
}


bool rgbWithinAbsTolerance(std::vector<float> & rgb0, 
                           std::vector<float> & rgb1, 
                           float scale0, 
                           float tolerance)
{
    for (size_t i = 0; i < 3; i++)
    {
        if (abs(rgb0[i] * scale0 - rgb1[i]) > tolerance)
        {
            return false;
        }
    }
    return true;
}

GroupTransformRcPtr getRefToSRGBTransform(ConstConfigRcPtr & builtinConfig, 
                                          std::string refColorSpaceName)
{
    // Build reference space of the given prims to sRGB transform.
    std::string srgbColorSpaceName = "Input - Generic - sRGB - Texture";
    ConstProcessorRcPtr proc = builtinConfig->getProcessor( refColorSpaceName.c_str(), 
                                                            srgbColorSpaceName.c_str());
    return proc->createGroupTransform();
}

ConstGroupTransformRcPtr combineGroupTransforms(ConstTransformRcPtr tf1, ConstTransformRcPtr tf2)
{
    GroupTransformRcPtr newGT = GroupTransform::Create();
    newGT->appendTransform(tf1->createEditableCopy());
    newGT->appendTransform(tf2->createEditableCopy());
    return newGT;
}

bool isIdentityTransform(ConstConfigRcPtr & config, 
                         ConstGroupTransformRcPtr & gt, 
                         std::vector<float> & vals)
{
    std::vector<float> out = vals;

    PackedImageDesc desc(&vals[0], (long) vals.size()/3, 1, CHANNEL_ORDERING_RGB);
    PackedImageDesc descDst(&out[0], (long) vals.size()/3, 1, CHANNEL_ORDERING_RGB);

    ConstProcessorRcPtr proc = config->getProcessor(gt);
    ConstCPUProcessorRcPtr cpu  = proc->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
    cpu->apply(desc, descDst);

    for (size_t i = 0; i < out.size(); i++)
    {
        if (!EqualWithAbsError(vals[i] * 1.f, out[i], 1e-3f))
        {
            return false;
        }
    }

    return true;
}

std::string checkForLinearColorSpace(ConstConfigRcPtr & config, 
                                     ConstColorSpaceRcPtr & cs,
                                     ConstConfigRcPtr & builtinConfig,
                                     std::vector<std::string> & builtinLinearSpaces)
{
    // If the color space is a recognized linear space, return the reference space used by 
    // the config.
    std::string refSpace;
    bool toRefDirection = true;
    auto srcTransform = cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
    if (!srcTransform)
    {
        srcTransform = cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if (srcTransform)
        {
            toRefDirection = false;
        }
        else
        {
            return "";
        }
    }

    // Define a set of (somewhat arbitrary) RGB values to test whether the combined transform is 
    // enough of an identity.
    std::vector<float> vals = {  0.7f,  0.4f,  0.02f,
                                0.02f,  0.6f,   0.2f,
                                 0.3f, 0.02f,   0.5f,
                                  0.f,   0.f,    0.f,
                                  1.f,   1.f,    1.f };

    // Generate matrices between all combinations of the Built-in linear color spaces. 
    // Then combine these with the transform from the current color space to see if the result is 
    // an identity. If so, then it identifies the reference space being used by the source config.
    for (size_t i = 0; i < builtinLinearSpaces.size(); i++)
    {
        for (size_t j = 0; j < builtinLinearSpaces.size(); j++)
        {
            if (i != j)
            {
                ConstGroupTransformRcPtr gt;
                ConstTransformRcPtr transform;
                ConstProcessorRcPtr proc  = builtinConfig->getProcessor(
                    builtinLinearSpaces[j].c_str(),
                    builtinLinearSpaces[i].c_str()
                );
                
                transform = proc->createGroupTransform();   
                gt = combineGroupTransforms(srcTransform, transform);

                if (isIdentityTransform(config, gt, vals))
                {
                    if (toRefDirection) 
                    {
                        refSpace = builtinLinearSpaces[j];
                    }
                    else 
                    {
                        refSpace = builtinLinearSpaces[i];
                    }

                    return refSpace;
                }
            }
        }
    }

    return "";
}

std::string checkForSRGBTextureColorSpace(ConstConfigRcPtr & config, 
                                          ConstColorSpaceRcPtr cs,
                                          ConstConfigRcPtr & builtinConfig,
                                          std::vector<std::string> & builtinLinearSpaces)
{
    // If the color space is an sRGB texture space, return the reference space used by the config.

    // Get a transform in the to-reference direction.
    ConstTransformRcPtr toRefTransform;
    ConstTransformRcPtr ctransform = cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
    if (ctransform) 
    {
        toRefTransform = ctransform;
    }
    else
    {
        ctransform = cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if (ctransform)
        {
            TransformRcPtr transform = ctransform->createEditableCopy();
            transform->setDirection(TRANSFORM_DIR_INVERSE);
            toRefTransform = transform;
        }
        else
        {
            // Both directions missing.
            return "";
        }
    }

    // First check if it has the right non-linearity. The objective is to fail quickly on color 
    // spaces that are definitely not sRGB before proceeding to the longer test of guessing the 
    // reference space primaries.

    // Break point is at 0.039286, so include at least one value below this.
    std::vector<float> vals =
    {
        0.5f, 0.5f, 0.5f, 
        0.03f, 0.03f, 0.03f, 
        0.25f, 0.25f, 0.25f, 
        0.75f, 0.75f, 0.75f, 
        0.f, 0.f, 0.f, 
        1.f, 1.f , 1.f
    };
    std::vector<float> out = vals;

    PackedImageDesc desc(&vals[0], (long) vals.size()/3, 1, CHANNEL_ORDERING_RGB);
    PackedImageDesc descDst(&out[0], (long) vals.size()/3, 1, CHANNEL_ORDERING_RGB);

    ConstProcessorRcPtr    proc = config->getProcessor(toRefTransform);
    ConstCPUProcessorRcPtr cpu  = proc->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
    cpu->apply(desc, descDst);

    for (size_t i = 0; i < out.size(); i++)
    {
        // Apply the sRGB function (linear to non-lin).
        if (out[i] <= 0.0030399346397784323f)
        {
            out[i] *= 12.923210180787857f;
        }
        else
        {
            out[i] = 1.055f * pow(out[i], 1/2.4f) - 0.055f;
        }

        if (!EqualWithAbsError(vals[i] * 1.f, out[i], 1e-3f))
        {
            return "";
        }
    } 

    //
    // Then try the various primaries for the reference space.
    //


    // Define a (somewhat arbitrary) set of RGB values to test whether the transform is in fact 
    // converting sRGB texture values to the candidate reference space. It includes 0.02 which is 
    // on the sRGB linear segment, color values, and neutral values.
    vals = {  0.7f,   0.4f,  0.02f,
             0.02f,   0.6f,   0.2f,
              0.3f,  0.02f,   0.5f,
               0.f,    0.f,    0.f,
               1.f,    1.f,    1.f };
    std::string refSpace = "";
    ConstTransformRcPtr fromRefTransform;
    if (toRefTransform)
    {
        // The color space has the sRGB non-linearity. Now try combining the transform with a 
        // transform from the Built-in config that goes from a variety of reference spaces to an 
        // sRGB texture space. If the result is an identity, then that tells what the source config
        // reference space is.
        for (size_t i = 0; i < builtinLinearSpaces.size(); i++)
        {
            fromRefTransform = getRefToSRGBTransform(builtinConfig, builtinLinearSpaces[i]);
            auto gt = combineGroupTransforms(toRefTransform, fromRefTransform);
            if (isIdentityTransform(config, gt, vals))
            {
                refSpace = builtinLinearSpaces[i];
            }
        }
    }

    return refSpace;
}

ConstProcessorRcPtr getProcessorToOrFromBuiltinColorSpace(ConstConfigRcPtr srcConfig,
                                                    const char * srcColorSpaceName, 
                                                    const char * builtinColorSpaceName,
                                                    TransformDirection direction)
{
    // Use the Default config as the Built-in config to interpret the known color space name.        
    ConstConfigRcPtr builtinConfig = Config::CreateFromFile("ocio://default");

    // Define the set of candidate reference linear color spaces (aka, reference primaries) that 
    // will be used when searching through the source config. If the source config scene-referred 
    // reference space is the equivalent of one of these spaces, it should be possible to identify 
    // it with the following heuristics.
    std::vector<std::string> builtinLinearSpaces = {    "ACES - ACES2065-1", 
                                                        "ACES - ACEScg", 
                                                        "Utility - Linear - Rec.709", 
                                                        "Utility - Linear - P3-D65" };

    if (builtinConfig->getColorSpace(builtinColorSpaceName) == nullptr)
    {
        std::ostringstream os;
        os  << "Built-in config does not contain the requested color space: " 
            << builtinColorSpaceName << ".";
        throw Exception(os.str().c_str());
    }

    // If both configs have the interchange roles set, then it's easy.
    try
    {
        ConstProcessorRcPtr proc = Config::GetProcessorFromConfigs(srcConfig, 
                                                                   srcColorSpaceName, 
                                                                   builtinConfig, 
                                                                   builtinColorSpaceName);
        return proc;
    }
    catch(const Exception & e) 
    { 
        std::string str1 = "The role 'aces_interchange' is missing in the source config";
        std::string str2 = "The role 'cie_xyz_d65_interchange' is missing in the source config";

        // Re-throw when the error is not about interchange roles.
        if (!StringUtils::StartsWith(e.what(), str1) && !StringUtils::StartsWith(e.what(), str2))
        {
            throw Exception(e.what());
        }
        // otherwise, do nothing and continue.
    }
    
    // Use heuristics to try and find a color space in the source config that matches 
    // a color space in the Built-in config.

    // Get the name of (one of) the reference spaces.
    std::string refColorSpaceName = getRefSpace(srcConfig);
    if (refColorSpaceName.empty())
    {
        std::ostringstream os;
        os  << "The supplied config does not have a color space for the reference.";
        throw Exception(os.str().c_str());
    }

    // Check for an sRGB texture space.
    std::string refColorSpacePrims = "";
    int nbCs = srcConfig->getNumColorSpaces();
    for (int i = 0; i < nbCs; i++)
    {
        ConstColorSpaceRcPtr cs = srcConfig->getColorSpace(srcConfig->getColorSpaceNameByIndex(i));
        if (containsSRGB(cs))
        {
            refColorSpacePrims = checkForSRGBTextureColorSpace(srcConfig, 
                                                               cs, 
                                                               builtinConfig, 
                                                               builtinLinearSpaces);
            // Break out when a match is found.
            if (!refColorSpacePrims.empty()) break; 
        }
    }
    
    if (!refColorSpacePrims.empty())
    {
        // Use the interchange spaces to get the processor.
        std::string srcInterchange = refColorSpaceName;
        std::string builtinInterchange = refColorSpacePrims;

        ConstProcessorRcPtr proc;
        if (direction == TRANSFORM_DIR_FORWARD)
        {
            proc = Config::GetProcessorFromConfigs( srcConfig,
                                                    srcColorSpaceName,
                                                    srcInterchange.c_str(),
                                                    builtinConfig,
                                                    builtinColorSpaceName,
                                                    builtinInterchange.c_str());
        }
        else if (direction == TRANSFORM_DIR_INVERSE)
        {
            proc = Config::GetProcessorFromConfigs( builtinConfig,
                                                    builtinColorSpaceName,
                                                    builtinInterchange.c_str(),
                                                    srcConfig,
                                                    srcColorSpaceName,
                                                    srcInterchange.c_str());
        }
        return proc;
    }

    // Check for a linear space with known primaries.
    nbCs = srcConfig->getNumColorSpaces();
    for (int i = 0; i < nbCs; i++)
    {
        auto cs = srcConfig->getColorSpace(srcConfig->getColorSpaceNameByIndex(i));
        if (srcConfig->isColorSpaceLinear(cs->getName(), REFERENCE_SPACE_SCENE))
        {
            refColorSpacePrims = checkForLinearColorSpace(srcConfig, 
                                                          cs, 
                                                          builtinConfig, 
                                                          builtinLinearSpaces);
            // Break out when a match is found.
            if (!refColorSpacePrims.empty()) break; 
        }
    }

    if (!refColorSpacePrims.empty())
    {
        // Use the interchange spaces to get the processor.
        std::string srcInterchange = refColorSpaceName;
        std::string builtinInterchange = refColorSpacePrims;

        ConstProcessorRcPtr proc;
        if (direction == TRANSFORM_DIR_FORWARD)
        {
            proc = Config::GetProcessorFromConfigs( srcConfig,
                                                    srcColorSpaceName,
                                                    srcInterchange.c_str(),
                                                    builtinConfig,
                                                    builtinColorSpaceName,
                                                    builtinInterchange.c_str());
        }
        else if (direction == TRANSFORM_DIR_INVERSE)
        {
            proc = Config::GetProcessorFromConfigs( builtinConfig,
                                                    builtinColorSpaceName,
                                                    builtinInterchange.c_str(),
                                                    srcConfig,
                                                    srcColorSpaceName,
                                                    srcInterchange.c_str());
        }
        return proc;
    }

    std::ostringstream os;
    os  << "Heuristics were not able to find a known color space in the provided config.\n"
        << "Please set the interchange roles.";
    throw Exception(os.str().c_str());
}

} // namespace OCIO_NAMESPACE