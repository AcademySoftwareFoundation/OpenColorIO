// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "ConfigUtils.h"
#include "MathUtils.h"
#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
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
        for (size_t i = 0; i < builtinLinearSpaces.size(); i++)
        {
            for (size_t j = 0; j < builtinLinearSpaces.size(); j++)
            {
                if (i != j)
                {
                    ConstProcessorRcPtr p1 = srcConfig.getProcessor(srcTransform, 
                                                                    TRANSFORM_DIR_FORWARD);

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
}