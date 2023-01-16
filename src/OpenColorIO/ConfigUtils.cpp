// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "ConfigUtils.h"
#include "MathUtils.h"
#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
{
    // Define the set of candidate reference linear color spaces (aka, reference primaries) that 
    // will be used when searching through the source config. If the source config scene-referred 
    // reference space is the equivalent of one of these spaces, it should be possible to identify 
    // it with the following heuristics.
    constexpr int numberOfbuiltinLinearSpaces = 5;
    constexpr const char * builtinLinearSpaces[] = { "ACES - ACES2065-1", 
                                                            "ACES - ACEScg", 
                                                            "Utility - Linear - Rec.709", 
                                                            "Utility - Linear - P3-D65",
                                                            "Utility - Linear - Rec.2020" };

namespace ConfigUtils
{
    const char * getBuiltinLinearSpaces(int index)
    {
        return builtinLinearSpaces[index];
    }

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

    int getRefSpace(const Config & cfg)
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

            return i;
        }
        return -1;
    }

    bool isIdentityTransform(const Config & srcConfig, 
                             GroupTransformRcPtr & tf, 
                             std::vector<float> & vals, 
                             float absTolerance)
    {
        std::vector<float> out = vals;
        
        PackedImageDesc desc(&vals[0], (long) vals.size()/4, 1, CHANNEL_ORDERING_RGB);
        PackedImageDesc descDst(&out[0], (long) vals.size()/4, 1, CHANNEL_ORDERING_RGB);

        ConstProcessorRcPtr proc = srcConfig.getProcessor(tf, TRANSFORM_DIR_FORWARD);
        ConstCPUProcessorRcPtr cpu  = proc->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
        cpu->apply(desc, descDst);

        for (size_t i = 0; i < out.size(); i++)
        {
            if (!EqualWithAbsError(vals[i], out[i], absTolerance))
            {
                return false;
            }
        }

        return true;
    }

    TransformRcPtr getTransformToSRGBSpace(const ConstConfigRcPtr & builtinConfig, 
                                           std::string refColorSpaceName)
    {
        // Build reference space of the given prims to sRGB transform.
        std::string srgbColorSpaceName = "Input - Generic - sRGB - Texture";

        ColorSpaceTransformRcPtr csTransform = ColorSpaceTransform::Create();
        csTransform->setSrc(refColorSpaceName.c_str());
        csTransform->setDst(srgbColorSpaceName.c_str());
        
        //ConstProcessorRcPtr proc  = builtinConfig->getProcessor(csTransform, TRANSFORM_DIR_FORWARD);
        //return proc;
        return csTransform;
    }

    int getReferenceSpaceFromLinearSpace(const Config & srcConfig,
                                         const ConstColorSpaceRcPtr & cs,
                                         const ConstConfigRcPtr & builtinConfig,
                                         const char * const * builtinLinearSpaces)
    {
        // If the color space is a recognized linear space, return the reference space used by 
        // the config.
        int refSpaceIndex = -1;
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
                return -1;
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
        TransformRcPtr eSrcTransform = srcTransform->createEditableCopy();
        for (size_t i = 0; i < numberOfbuiltinLinearSpaces; i++)
        {
            for (size_t j = 0; j < numberOfbuiltinLinearSpaces; j++)
            {
                if (i != j)
                {
                    ColorSpaceTransformRcPtr csTransform = ColorSpaceTransform::Create();
                    csTransform->setSrc(builtinLinearSpaces[j]);
                    csTransform->setDst(builtinLinearSpaces[i]);
  
                    GroupTransformRcPtr grptf = GroupTransform::Create();
                    grptf->appendTransform(eSrcTransform);
                    grptf->appendTransform(csTransform);

                    if (isIdentityTransform(*builtinConfig, grptf, vals, 1e-3f))
                    {
                        if (toRefDirection) 
                        {
                            refSpaceIndex = (int) j;
                        }
                        else 
                        {
                            refSpaceIndex = (int) i;
                        }

                        return refSpaceIndex;
                    }
                }
            }
        }

        return -1;
    }

    int getReferenceSpaceFromSRGBSpace(const Config & config, 
                                       const ConstColorSpaceRcPtr cs,
                                       const ConstConfigRcPtr & builtinConfig,
                                       const char * const * builtinLinearSpaces)
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
                return -1;
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
                return -1;
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
        int refSpaceIndex = -1;
        TransformRcPtr fromRefTransform;
        TransformRcPtr eToRefTransform = toRefTransform->createEditableCopy();
        if (toRefTransform)
        {
            // The color space has the sRGB non-linearity. Now try combining the transform with a 
            // transform from the Built-in config that goes from a variety of reference spaces to an 
            // sRGB texture space. If the result is an identity, then that tells what the source config
            // reference space is.
            for (size_t i = 0; i < numberOfbuiltinLinearSpaces; i++)
            {
                fromRefTransform = getTransformToSRGBSpace(builtinConfig, builtinLinearSpaces[i]);

                GroupTransformRcPtr grptf = GroupTransform::Create();
                grptf->appendTransform(eToRefTransform);
                grptf->appendTransform(fromRefTransform);

                if (isIdentityTransform(*builtinConfig, grptf, vals, 1e-3f))
                {
                    refSpaceIndex = (int) i;
                    break;
                }
            }
        }

        return refSpaceIndex;
    }

    void identifyInterchangeSpace(int & srcInterchangeIndex, Config & eSrcConfig)
    {
        if (eSrcConfig.hasRole(ROLE_INTERCHANGE_SCENE))
        {
            std::ostringstream os;
            os << "The role '" << ROLE_INTERCHANGE_SCENE << "' is missing in the source config.";
            throw Exception(os.str().c_str());
        }

        // Get the name of (one of) the reference spaces.
        int refColorSpaceIndex = getRefSpace(eSrcConfig);
        if (refColorSpaceIndex < 0)
        {
            std::ostringstream os;
            os  << "The supplied config does not have a color space for the reference.";
            throw Exception(os.str().c_str());
        }

        if (refColorSpaceIndex > -1)
        {
            srcInterchangeIndex = refColorSpaceIndex;
        }
        else
        {
            std::ostringstream os;
            os  << "Heuristics were not able to find a known color space in the provided config.\n"
                << "Please set the interchange roles.";
            throw Exception(os.str().c_str());
        }
    }

    void identifyBuiltinInterchangeSpace(int & builtinInterchangeIndex, Config & eSrcConfig)
    {
        // Use the Default config as the Built-in config.      
        ConstConfigRcPtr builtinConfig = Config::CreateFromFile("ocio://default");

        if (eSrcConfig.hasRole(ROLE_INTERCHANGE_SCENE))
        {
            std::ostringstream os;
            os << "The role '" << ROLE_INTERCHANGE_SCENE << "' is missing in the source config.";
            throw Exception(os.str().c_str());
        }

        // Use heuristics to try and find a color space in the source config that matches 
        // a color space in the Built-in config.

        // Check for an sRGB texture space.
        int refColorSpacePrimsIndex = -1;
        int nbCs = eSrcConfig.getNumColorSpaces();
        for (int i = 0; i < nbCs; i++)
        {
            ConstColorSpaceRcPtr cs = eSrcConfig.getColorSpace(eSrcConfig.getColorSpaceNameByIndex(i));
            if (containsSRGB(cs))
            {
                refColorSpacePrimsIndex = getReferenceSpaceFromSRGBSpace(eSrcConfig, 
                                                                         cs, 
                                                                         builtinConfig, 
                                                                         builtinLinearSpaces);
                // Break out when a match is found.
                if (refColorSpacePrimsIndex > -1) break; 
            }
        }

        if (refColorSpacePrimsIndex < 0)
        {
            // Check for a linear space with known primaries.
            nbCs = eSrcConfig.getNumColorSpaces();
            for (int i = 0; i < nbCs; i++)
            {
                auto cs = eSrcConfig.getColorSpace(eSrcConfig.getColorSpaceNameByIndex(i));
                if (eSrcConfig.isColorSpaceLinear(cs->getName(), REFERENCE_SPACE_SCENE))
                {
                    refColorSpacePrimsIndex = getReferenceSpaceFromLinearSpace(eSrcConfig,
                                                                               cs, 
                                                                               builtinConfig, 
                                                                               builtinLinearSpaces);
                    // Break out when a match is found.
                    if (refColorSpacePrimsIndex > -1) break; 
                }
            }
        }

        if (refColorSpacePrimsIndex > -1)
        {
            builtinInterchangeIndex = refColorSpacePrimsIndex;
        }
        else
        {
            std::ostringstream os;
            os  << "Heuristics were not able to find a known color space in the provided config.\n"
                << "Please set the interchange roles.";
            throw Exception(os.str().c_str());
        }
    }

    int identifyBuiltinColorSpace(const char * builtinColorSpaceName, Config & eSrcConfig)
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

        int srcInterchangeIndex = -1;
        int builtinInterchangeIndex = -1;
        // Identify interchange space.
        identifyInterchangeSpace(srcInterchangeIndex, eSrcConfig);
        identifyBuiltinInterchangeSpace(builtinInterchangeIndex, eSrcConfig);
        
        // Get processor from that space to the built-in color space.
        ConstProcessorRcPtr builtinProc;
        if (builtinInterchangeIndex > -1)
        {
            builtinProc = builtinConfig->getProcessor(builtinConfig->getCurrentContext(), 
                                                      builtinConfig->getColorSpaceNameByIndex(builtinInterchangeIndex), 
                                                      builtinColorSpaceName);
        }

        if (builtinProc && srcInterchangeIndex > -1)
        {
            // Iterate over each color space in the source config.
            std::vector<float> vals = { 0.7f,  0.4f,  0.02f, 0.f,
                                        0.02f, 0.6f,  0.2f,  0.f,
                                        0.3f,  0.02f, 0.5f,  0.f,
                                        0.f,   0.f,   0.f,   0.f,
                                        1.f,   1.f,   1.f,   0.f };
            int nbCs = eSrcConfig.getNumColorSpaces();
            for (int i = 0; i < nbCs; i++)
            {
                // Get processor from that space to its reference and then use isProcessorEquivalent.
                // If equivalent, return that color space name.

                const char * csName = eSrcConfig.getColorSpaceNameByIndex(i);
                ConstProcessorRcPtr proc = eSrcConfig.getProcessor(eSrcConfig.getCurrentContext(),
                                                                   csName,
                                                                   eSrcConfig.getColorSpaceNameByIndex(srcInterchangeIndex));

                auto grptf = builtinProc->createGroupTransform();
                grptf->appendTransform(proc->createGroupTransform());
                if (isIdentityTransform(eSrcConfig, grptf, vals, 1e-3f))
                {
                    return i;
                }
            }
        }

        return -1; 
    }
}

}