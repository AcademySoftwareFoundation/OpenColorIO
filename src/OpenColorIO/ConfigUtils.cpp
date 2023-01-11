// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "ConfigUtils.h"
#include "MathUtils.h"
#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
{
namespace ConfigUtils
{
    bool containsSRGB(ConstColorSpaceRcPtr & cs)
    {
        std::string name = StringUtils::Lower(cs->getName());
        if (StringUtils::Find(name, "srgb") != std::string::npos)
        {
            return true;
        }

        size_t nbOfAliases = cs->getNumAliases();
        for (size_t i = 0; i < nbOfAliases; i++)
        {
            if (StringUtils::Find(cs->getAlias(i), "srgb") != std::string::npos)
            {
                return true;
            }
        }

        return false;
    }

    std::string getRefSpace(const Config & cfg)
    {
        // Find a color space where isData is false and it has neither a to_ref or from_ref 
        // transform.
        auto nbCs = cfg.getNumColorSpaces();
        for (int i = 0; i < nbCs; i++)
        {
            auto cs = cfg.getColorSpace(cfg.getColorSpaceNameByIndex(i));
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

    ConstProcessorRcPtr getRefToSRGBTransform(const ConstConfigRcPtr & builtinConfig, 
                                              std::string refColorSpaceName)
    {
        // Build reference space of the given prims to sRGB transform.
        std::string srgbColorSpaceName = "Input - Generic - sRGB - Texture";

        ColorSpaceTransformRcPtr csTransform = ColorSpaceTransform::Create();
        csTransform->setSrc(refColorSpaceName.c_str());
        csTransform->setDst(srgbColorSpaceName.c_str());
        
        ConstProcessorRcPtr proc  = builtinConfig->getProcessor(csTransform, TRANSFORM_DIR_FORWARD);
        return proc;
    }

    std::string getReferenceSpaceFromLinearSpace(const Config & srcConfig,
                                                 const ConstColorSpaceRcPtr & cs,
                                                 const ConstConfigRcPtr & builtinConfig,
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
        std::vector<float> vals = { 0.7f,  0.4f,  0.02f, 0.f,
                                    0.02f, 0.6f,  0.2f,  0.f,
                                    0.3f,  0.02f, 0.5f,  0.f,
                                    0.f,   0.f,   0.f,   0.f,
                                    1.f,   1.f,   1.f,   0.f };

        // Generate matrices between all combinations of the Built-in linear color spaces. 
        // Then combine these with the transform from the current color space to see if the result is 
        // an identity. If so, then it identifies the reference space being used by the source config.
        ConstProcessorRcPtr p1 = srcConfig.getProcessor(srcTransform, TRANSFORM_DIR_FORWARD);
        for (size_t i = 0; i < builtinLinearSpaces.size(); i++)
        {
            for (size_t j = 0; j < builtinLinearSpaces.size(); j++)
            {
                if (i != j)
                {
                    ColorSpaceTransformRcPtr csTransform = ColorSpaceTransform::Create();
                    csTransform->setSrc(builtinLinearSpaces[j].c_str());
                    csTransform->setDst(builtinLinearSpaces[i].c_str());

                    ConstProcessorRcPtr p2 = builtinConfig->getProcessor(csTransform, 
                                                                         TRANSFORM_DIR_FORWARD);   

                    if (Processor::AreProcessorsEquivalent(p1, p2, &vals[0], 5, 1e-3f))
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

    std::string getReferenceSpaceFromSRGBSpace(const Config & config, 
                                               const ConstColorSpaceRcPtr cs,
                                               const ConstConfigRcPtr & builtinConfig,
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
            0.5f,  0.5f,  0.5f, 
            0.03f, 0.03f, 0.03f, 
            0.25f, 0.25f, 0.25f, 
            0.75f, 0.75f, 0.75f, 
            0.f,   0.f,   0.f, 
            1.f,   1.f ,  1.f
        };
        std::vector<float> out = vals;

        PackedImageDesc desc(&vals[0], (long) vals.size()/3, 1, CHANNEL_ORDERING_RGB);
        PackedImageDesc descDst(&out[0], (long) vals.size()/3, 1, CHANNEL_ORDERING_RGB);

        ConstProcessorRcPtr proc = config.getProcessor(toRefTransform, TRANSFORM_DIR_FORWARD);

        ConstCPUProcessorRcPtr cpu  = proc->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
        cpu->apply(desc, descDst);

        for (size_t i = 0; i < out.size(); i++)
        {
            // Apply the sRGB function (linear to non-lin).
            // Please see GammaOpUtils.cpp. This value provides continuity at the breakpoint.
            if (out[i] <= 0.0030399346397784323f)
            {
                out[i] *= 12.923210180787857f;
            }
            else
            {
                out[i] = 1.055f * std::pow(out[i], 1/2.4f) - 0.055f;
            }

            if (!EqualWithAbsError(vals[i], out[i], 1e-3f))
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
        vals = { 0.7f,  0.4f,  0.02f, 0.f,
                 0.02f, 0.6f,  0.2f,  0.f,
                 0.3f,  0.02f, 0.5f,  0.f,
                 0.f,   0.f,   0.f,   0.f,
                 1.f,   1.f,   1.f,   0.f, };
        std::string refSpace = "";
        ConstProcessorRcPtr fromRefProc;
        if (toRefTransform)
        {
            // The color space has the sRGB non-linearity. Now try combining the transform with a 
            // transform from the Built-in config that goes from a variety of reference spaces to an 
            // sRGB texture space. If the result is an identity, then that tells what the source config
            // reference space is.
            for (size_t i = 0; i < builtinLinearSpaces.size(); i++)
            {
                fromRefProc = getRefToSRGBTransform(builtinConfig, builtinLinearSpaces[i]);

                ConstProcessorRcPtr toRefProc = config.getProcessor(toRefTransform, 
                                                                    TRANSFORM_DIR_FORWARD);
                
                if (Processor::AreProcessorsEquivalent(toRefProc, fromRefProc, &vals[0], 5, 1e-3f))
                {
                    refSpace = builtinLinearSpaces[i];
                }
            }
        }

        return refSpace;
    }

    bool isColorSpaceLinear(const char * colorSpace, 
                            ReferenceSpaceType referenceSpaceType, 
                            const Config & cfg)
    {
        auto cs = cfg.getColorSpace(colorSpace);

        if (cs->isData())
        {
            return false;
        }

        // Colorspace is not linear if the types are opposite.
        if (cs->getReferenceSpaceType() != referenceSpaceType)
        {
            return false;
        }

        std::string encoding = cs->getEncoding();
        if (!encoding.empty())
        {
            // Check the encoding value if it is set.        
            if ((StringUtils::Compare(cs->getEncoding(), "scene-linear") && 
                referenceSpaceType == REFERENCE_SPACE_SCENE) || 
                (StringUtils::Compare(cs->getEncoding(), "display-linear") && 
                referenceSpaceType == REFERENCE_SPACE_DISPLAY))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        // We want to assess linearity over at least a reasonable range of values, so use a very dark 
        // value and a very bright value. Test neutral, red, green, and blue points to detect situations 
        // where the neutral may be linear but there is non-linearity off the neutral axis.
        auto evaluate = [](const Config & config, ConstTransformRcPtr &t) -> bool
        {
            std::vector<float> img = 
            { 
                0.0625f, 0.0625f, 0.0625f, 4.f, 4.f, 4.f,
                0.0625f, 0.f, 0.f, 4.f, 0.f, 0.f,
                0.f, 0.0625f, 0.f, 0.f, 4.f, 0.f,
                0.f, 0.f, 0.0625f, 0.f, 0.f, 4.f
            };
            std::vector<float> dst = 
            { 
                0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                0.f, 0.f, 0.f, 0.f, 0.f, 0.f
            };

            PackedImageDesc desc(&img[0], 8, 1, CHANNEL_ORDERING_RGB);
            PackedImageDesc descDst(&dst[0], 8, 1, CHANNEL_ORDERING_RGB);
            
            // Create an editable copy to avoir filling the processor cache.
            ConfigRcPtr eConfig = config.createEditableCopy();
            eConfig->setProcessorCacheFlags(PROCESSOR_CACHE_OFF);

            auto procToReference = eConfig->getProcessor(t, TRANSFORM_DIR_FORWARD);
            auto optCPUProc = procToReference->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
            optCPUProc->apply(desc, descDst);

            float absError      = 1e-5f;
            float multiplier    = 64.f;
            bool ret            = true;

            // Test the first RGB pair.
            ret &= EqualWithAbsError(dst[0]*multiplier, dst[3], absError);
            ret &= EqualWithAbsError(dst[1]*multiplier, dst[4], absError);
            ret &= EqualWithAbsError(dst[2]*multiplier, dst[5], absError);

            // Test the second RGB pair.
            ret &= EqualWithAbsError(dst[6]*multiplier, dst[9], absError);
            ret &= EqualWithAbsError(dst[7]*multiplier, dst[10], absError);
            ret &= EqualWithAbsError(dst[8]*multiplier, dst[11], absError);

            // Test the third RGB pair.
            ret &= EqualWithAbsError(dst[12]*multiplier, dst[15], absError);
            ret &= EqualWithAbsError(dst[13]*multiplier, dst[16], absError);
            ret &= EqualWithAbsError(dst[14]*multiplier, dst[17], absError);

            // Test the fourth RGB pair.
            ret &= EqualWithAbsError(dst[18]*multiplier, dst[21], absError);
            ret &= EqualWithAbsError(dst[19]*multiplier, dst[22], absError);
            ret &= EqualWithAbsError(dst[20]*multiplier, dst[23], absError);

            return ret;
        };
        
        ConstTransformRcPtr transformToReference = cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
        ConstTransformRcPtr transformFromReference = cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if ((transformToReference && transformFromReference) || transformToReference)
        {
            // Color space has a transform for the to-reference direction, or both directions.
            return evaluate(cfg, transformToReference);
        }
        else if (transformFromReference)
        {
            // Color space only has a transform for the from-reference direction.
            return evaluate(cfg, transformFromReference);
        }

        // Color space matches the desired reference space type, is not a data space, and has no 
        // transforms, so it is equivalent to the reference space and hence linear.
        return true;
    }

    void identifyInterchangeSpace(char * srcInterchange, 
                                  char * builtinInterchange, 
                                  const Config & cfg)
    {
        // Use the Default config as the Built-in config.      
        ConstConfigRcPtr builtinConfig = Config::CreateFromFile("ocio://default");

        // Using createEditableCopy to avoid filling the processor cache in the Config object.
        ConfigRcPtr eSrcConfig = cfg.createEditableCopy();
        eSrcConfig->setProcessorCacheFlags(PROCESSOR_CACHE_OFF);

        // Define the set of candidate reference linear color spaces (aka, reference primaries) that 
        // will be used when searching through the source config. If the source config scene-referred 
        // reference space is the equivalent of one of these spaces, it should be possible to identify 
        // it with the following heuristics.
        std::vector<std::string> builtinLinearSpaces = { "ACES - ACES2065-1", 
                                                         "ACES - ACEScg", 
                                                         "Utility - Linear - Rec.709", 
                                                         "Utility - Linear - P3-D65",
                                                         "Utility - Linear - Rec.2020" };

        // Use heuristics to try and find a color space in the source config that matches 
        // a color space in the Built-in config.

        // Get the name of (one of) the reference spaces.
        std::string refColorSpaceName = getRefSpace(*eSrcConfig);
        if (refColorSpaceName.empty())
        {
            std::ostringstream os;
            os  << "The supplied config does not have a color space for the reference.";
            throw Exception(os.str().c_str());
        }

        // Check for an sRGB texture space.
        std::string refColorSpacePrims = "";
        int nbCs = eSrcConfig->getNumColorSpaces();
        for (int i = 0; i < nbCs; i++)
        {
            ConstColorSpaceRcPtr cs = eSrcConfig->getColorSpace(eSrcConfig->getColorSpaceNameByIndex(i));
            if (containsSRGB(cs))
            {
                refColorSpacePrims = getReferenceSpaceFromSRGBSpace(*eSrcConfig, 
                                                                    cs, 
                                                                    builtinConfig, 
                                                                    builtinLinearSpaces);
                // Break out when a match is found.
                if (!refColorSpacePrims.empty()) break; 
            }
        }

        if (refColorSpacePrims.empty())
        {
            // Check for a linear space with known primaries.
            nbCs = eSrcConfig->getNumColorSpaces();
            for (int i = 0; i < nbCs; i++)
            {
                auto cs = eSrcConfig->getColorSpace(eSrcConfig->getColorSpaceNameByIndex(i));
                if (eSrcConfig->isColorSpaceLinear(cs->getName(), REFERENCE_SPACE_SCENE))
                {
                    refColorSpacePrims = getReferenceSpaceFromLinearSpace(*eSrcConfig,
                                                                        cs, 
                                                                        builtinConfig, 
                                                                        builtinLinearSpaces);
                    // Break out when a match is found.
                    if (!refColorSpacePrims.empty()) break; 
                }
            }
        }

        if (!refColorSpaceName.empty() && !refColorSpacePrims.empty())
        {
            // Copy interchange role from source config.
            snprintf(srcInterchange, refColorSpaceName.size()+1, "%s", refColorSpaceName.c_str());
            // Copy interchange role from built-in config.
            snprintf(builtinInterchange, refColorSpacePrims.size()+1, "%s", refColorSpacePrims.c_str());
        }
        else
        {
            std::ostringstream os;
            os  << "Heuristics were not able to find a known color space in the provided config.\n"
                << "Please set the interchange roles.";
            throw Exception(os.str().c_str());
        }
    }

    const char * identifyBuiltinColorSpace(const char * builtinColorSpaceName, const Config & cfg)
    {
        // Use the Default config as the Built-in config.      
        ConstConfigRcPtr builtinConfig = Config::CreateFromFile("ocio://default");
        
        if (builtinConfig->getColorSpace(builtinColorSpaceName) == nullptr)
        {
            std::ostringstream os;
            os  << "Built-in config does not contain the requested color space: " 
                << builtinColorSpaceName << ".";
            throw Exception(os.str().c_str());
        }

        char srcInterchange[255];
        char builtinInterchange[255];

        // Identify interchange space.
        identifyInterchangeSpace(srcInterchange, builtinInterchange, cfg);

        // Get processor from that space to the built-in color space.
        ConstProcessorRcPtr builtinProc;
        if (builtinInterchange && builtinInterchange[0])
        {
            builtinProc = builtinConfig->getProcessor(builtinConfig->getCurrentContext(), 
                                                    builtinInterchange, 
                                                    builtinColorSpaceName);
        }
        
        if (builtinProc && srcInterchange && srcInterchange[0])
        {
            // Iterate over each color space in the source config.
            std::vector<float> vals = { 0.7f,  0.4f,  0.02f, 0.f,
                                        0.02f, 0.6f,  0.2f,  0.f,
                                        0.3f,  0.02f, 0.5f,  0.f,
                                        0.f,   0.f,   0.f,   0.f,
                                        1.f,   1.f,   1.f,   0.f };
            int nbCs = cfg.getNumColorSpaces();
            for (int i = 0; i < nbCs; i++)
            {
                // Get processor from that space to its reference and then use isProcessorEquivalent.
                // If equivalent, return that color space name.

                ConstColorSpaceRcPtr cs = cfg.getColorSpace(cfg.getColorSpaceNameByIndex(i));
                const char * csName = cs->getName();

                ConstProcessorRcPtr proc = cfg.getProcessor(cfg.getCurrentContext(),
                                                            csName,
                                                            srcInterchange);

                if (Processor::AreProcessorsEquivalent(builtinProc, proc, &vals[0], 5, 1e-3f))
                {
                    return csName;
                }
            }
        }

        return "";
    }
}

}