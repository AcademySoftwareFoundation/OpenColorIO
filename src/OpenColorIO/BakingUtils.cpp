// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "BakingUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{
    GroupTransformRcPtr GetInputToTargetTransform(const Baker & baker)
    {
        const std::string input = baker.getInputSpace();
        const std::string looks = baker.getLooks();
        const std::string display = baker.getDisplay();
        const std::string view = baker.getView();

        GroupTransformRcPtr group = GroupTransform::Create();

        if (!display.empty() && !view.empty())
        {
            if (!looks.empty())
            {
                LookTransformRcPtr look = LookTransform::Create();
                look->setLooks(looks.c_str());
                look->setSrc(input.c_str());
                look->setDst(input.c_str());

                group->appendTransform(look);
            }

            DisplayViewTransformRcPtr disp = DisplayViewTransform::Create();
            disp->setSrc(input.c_str());
            disp->setDisplay(display.c_str());
            disp->setView(view.c_str());
            disp->setLooksBypass(!looks.empty());

            group->appendTransform(disp);
        }
        else
        {
            LookTransformRcPtr look = LookTransform::Create();
            look->setLooks(!looks.empty() ? looks.c_str() : "");
            look->setSrc(input.c_str());
            look->setDst(baker.getTargetSpace());

            group->appendTransform(look);
        }

        return group;
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
        ConstProcessorRcPtr processor = baker.getConfig()->getProcessor(
            GetInputToTargetTransform(baker), TRANSFORM_DIR_FORWARD
        );
        return processor->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
    }

    throw Exception("Input space is empty.");
}

ConstCPUProcessorRcPtr GetShaperToTargetProcessor(const Baker & baker)
{
    if (baker.getShaperSpace() && *baker.getShaperSpace())
    {
        ColorSpaceTransformRcPtr csc = ColorSpaceTransform::Create();
        csc->setSrc(baker.getShaperSpace());
        csc->setDst(baker.getInputSpace());

        GroupTransformRcPtr group = GetInputToTargetTransform(baker);
        group->prependTransform(csc);

        ConstProcessorRcPtr processor = baker.getConfig()->getProcessor(
            group, TRANSFORM_DIR_FORWARD
        );
        return processor->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
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
