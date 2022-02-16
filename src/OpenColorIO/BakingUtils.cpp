// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "BakingUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{
    ConstCPUProcessorRcPtr GetSrcToTargetProcessor(const Baker & baker,
                                                   const char * src)
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
    
    void GetSrcRange(const Baker & baker,
                     const char * src,
                     float & start,
                     float & end)
    {
        // Calculate min/max value
        ConstCPUProcessorRcPtr shaperToInput =
            baker.getConfig()->getProcessor(src, baker.getInputSpace()
        )->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);

        float minval[3] = {0.0f, 0.0f, 0.0f};
        float maxval[3] = {1.0f, 1.0f, 1.0f};

        shaperToInput->applyRGB(minval);
        shaperToInput->applyRGB(maxval);

        start = std::min(std::min(minval[0], minval[1]), minval[2]);
        end = std::max(std::max(maxval[0], maxval[1]), maxval[2]);
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

void GetShaperRange(const Baker & baker, float& start, float& end)
{
    return GetSrcRange(baker, baker.getShaperSpace(), start, end);
}

void GetTargetRange(const Baker & baker, float& start, float& end)
{
    return GetSrcRange(baker, baker.getTargetSpace(), start, end);
}

} // namespace OCIO_NAMESPACE
