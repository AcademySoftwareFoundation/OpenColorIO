// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "BakingUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{
    ConstCPUProcessorRcPtr GetSrcToTargetProcessor(const Baker & baker, const char * src)
    {
        ConstProcessorRcPtr processor;

        const std::string looks = baker.getLooks();
        const std::string display = baker.getDisplay();
        const std::string view = baker.getView();

        if (!display.empty() && !view.empty())
        {
            processor = baker.getConfig()->getProcessor(
                src,
                display.c_str(),
                view.c_str(),
                TRANSFORM_DIR_FORWARD
            );
        }
        else
        {
            LookTransformRcPtr transform = LookTransform::Create();
            transform->setLooks(!looks.empty() ? looks.c_str() : "");
            transform->setSrc(src);
            transform->setDst(baker.getTargetSpace());
            processor = baker.getConfig()->getProcessor(
                transform, TRANSFORM_DIR_FORWARD
            );
        }

        return processor->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
    }
    
    std::array<float, 2> GetSrcRange(const Baker & baker, const char * src)
    {
        // Calculate min/max value
        ConstCPUProcessorRcPtr shaperToInput =
            baker.getConfig()->getProcessor(src, baker.getInputSpace()
        )->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);

        float minval[3] = {0.0f, 0.0f, 0.0f};
        float maxval[3] = {1.0f, 1.0f, 1.0f};

        shaperToInput->applyRGB(minval);
        shaperToInput->applyRGB(maxval);

        float fromInStart = std::min(std::min(minval[0], minval[1]), minval[2]);
        float fromInEnd = std::max(std::max(maxval[0], maxval[1]), maxval[2]);

        return { fromInStart, fromInEnd };
    }
} // Anonymous namespace


ConstCPUProcessorRcPtr GetInputToShaperProcessor(const Baker & baker)
{
    ConstProcessorRcPtr processor = baker.getConfig()->getProcessor(
        baker.getInputSpace(), baker.getShaperSpace());
    return processor->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
}

ConstCPUProcessorRcPtr GetShaperToInputProcessor(const Baker & baker)
{
    ConstProcessorRcPtr processor = baker.getConfig()->getProcessor(
        baker.getShaperSpace(), baker.getInputSpace());
    return processor->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
}

ConstCPUProcessorRcPtr GetInputToTargetProcessor(const Baker & baker)
{
    return GetSrcToTargetProcessor(baker, baker.getInputSpace());
}

ConstCPUProcessorRcPtr GetShaperToTargetProcessor(const Baker & baker)
{
    return GetSrcToTargetProcessor(baker, baker.getShaperSpace());
}

std::array<float, 2> GetShaperRange(const Baker & baker)
{
    return GetSrcRange(baker, baker.getShaperSpace());
}

std::array<float, 2> GetTargetRange(const Baker & baker)
{
    return GetSrcRange(baker, baker.getTargetSpace());
}

} // namespace OCIO_NAMESPACE
