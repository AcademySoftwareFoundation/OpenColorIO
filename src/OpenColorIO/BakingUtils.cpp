// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
            DisplayViewTransformRcPtr transform = DisplayViewTransform::Create();
            transform->setSrc(src);
            transform->setDisplay(display.c_str());
            transform->setView(view.c_str());

            LegacyViewingPipelineRcPtr vp = LegacyViewingPipeline::Create();
            vp->setDisplayViewTransform(transform);
            vp->setLooksOverrideEnabled(!looks.empty());
            vp->setLooksOverride(looks.c_str());

            processor = vp->getProcessor(baker.getConfig());
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
        ConstProcessorRcPtr proc = baker.getConfig()->getProcessor(
            src, baker.getInputSpace());
        ConstCPUProcessorRcPtr cpu = proc->getOptimizedCPUProcessor(
            OPTIMIZATION_LOSSLESS);

        float minval[3] = {0.0f, 0.0f, 0.0f};
        float maxval[3] = {1.0f, 1.0f, 1.0f};

        cpu->applyRGB(minval);
        cpu->applyRGB(maxval);

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
    if (baker.getInputSpace() && *baker.getInputSpace())
    {
        return GetSrcToTargetProcessor(baker, baker.getInputSpace());
    }

    throw Exception("Input space is empty.");
}

ConstCPUProcessorRcPtr GetShaperToTargetProcessor(const Baker & baker)
{
    if (baker.getShaperSpace() && *baker.getShaperSpace())
    {
        return GetSrcToTargetProcessor(baker, baker.getShaperSpace());
    }

    throw Exception("Shaper space is empty.");
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
