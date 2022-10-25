// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

// Remove once std::cout are removed.
#include <iostream>

#include "ColorSpaceUtils.h"
#include <OpenColorIO/OpenColorIO.h>
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
        if (cs->isData()) continue;

        auto t = cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
        if (t != nullptr) continue;

        t = cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if (t != nullptr) continue;

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
                         std::vector<std::vector<float>> & vals)
{
    ConstProcessorRcPtr proc = config->getProcessor(gt);
    ConstCPUProcessorRcPtr cpu  = proc->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
    std::vector<std::vector<float>> out = vals;
    for (size_t i = 0; i < vals.size(); i++)
    {
        cpu->applyRGB(&out[i][0]);
        if (!rgbWithinAbsTolerance(vals[i], out[i], 1.f, 1e-3f))
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

    std::vector<std::vector<float>> vals = { {  0.7f,  0.4f,  0.02f },
                                             { 0.02f,  0.6f,   0.2f },
                                             {  0.3f, 0.02f,   0.5f },
                                             {   0.f,   0.f,    0.f },
                                             {   1.f,   1.f,    1.f } };
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
                    if (toRefDirection) refSpace = builtinLinearSpaces[j];
                    else refSpace = builtinLinearSpaces[i];

                    std::cout << "--> Found reference space: " << refSpace << std::endl;
                    std::cout << "   src: " << builtinLinearSpaces[j] + " dst: " 
                                << builtinLinearSpaces[i] << " to_ref: " 
                                << toRefDirection << std::endl;
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

    // Get TO reference transform.
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

    // First check if it has the right non-linearity.
    ConstProcessorRcPtr    proc = config->getProcessor(toRefTransform);
    ConstCPUProcessorRcPtr cpu  = proc->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
    // Break point is at 0.039286, so include at least one value below this.
    std::vector<std::vector<float>> vals =
    {
        {0.5f, 0.5f, 0.5f}, {0.03f, 0.03f, 0.03f}, {0.25f, 0.25f, 0.25f}, 
        {0.75f, 0.75f, 0.75f}, {0.f, 0.f, 0.f}, {1.f, 1.f , 1.f}
    };
    std::vector<std::vector<float>> out = vals;

    for (size_t i = 0; i < vals.size(); i++)
    {
        cpu->applyRGB(&out[i][0]);
        // Apply the sRGB function (linear to non-lin).
        for (int k = 0; k < 3; k++)
        {
            if (out[i][k] <= 0.0030399346397784323f)
            {
                out[i][k] *= 12.923210180787857f;
            }
            else
            {
                out[i][k] = 1.055f * pow(out[i][k], 1/2.4f) - 0.055f;
            }
        }

        //std::cout << "RGB0 = " << vals[i][0] << " OUT0 = " << out[i][0] << std::endl;
        if (!rgbWithinAbsTolerance(vals[i], out[i], 1.f, 1e-4f))
        {
            std::cout << "--> Wrong transfer function" << std::endl;
            return "";
        }
    }

    // Then try the various primaries for the reference space.
    vals = { {  0.7f,  0.4f,  0.02f },
             { 0.02f,  0.6f,   0.2f },
             {  0.3f, 0.02f,   0.5f },
             {   0.f,   0.f,    0.f },
             {   1.f,   1.f,    1.f } };
    std::string refSpace = "";
    ConstTransformRcPtr fromRefTransform;
    if (toRefTransform)
    {
        for (size_t i = 0; i < builtinLinearSpaces.size(); i++)
        {
            fromRefTransform = getRefToSRGBTransform(builtinConfig, builtinLinearSpaces[i]);
            auto gt = combineGroupTransforms(toRefTransform, fromRefTransform);
            if (isIdentityTransform(config, gt, vals))
            {
                std::cout << "--> Found sRGB to reference space: " << builtinLinearSpaces[i] << std::endl;
                refSpace = builtinLinearSpaces[i];
            }
        }
    }

    return refSpace;
}
} // namespace OCIO_NAMESPACE