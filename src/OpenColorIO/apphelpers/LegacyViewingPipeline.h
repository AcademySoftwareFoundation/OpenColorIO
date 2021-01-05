// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LEGACYVIEWINGPIPELINE_H
#define INCLUDED_OCIO_LEGACYVIEWINGPIPELINE_H


#include <string>

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

class LegacyViewingPipelineImpl : public LegacyViewingPipeline
{
public:
    LegacyViewingPipelineImpl() = default;
    ~LegacyViewingPipelineImpl() = default;

    LegacyViewingPipelineImpl(const LegacyViewingPipelineImpl &) = delete;
    LegacyViewingPipelineImpl & operator=(const LegacyViewingPipelineImpl &) = delete;

    ConstDisplayViewTransformRcPtr getDisplayViewTransform() const noexcept override;
    void setDisplayViewTransform(const ConstDisplayViewTransformRcPtr & dt) noexcept override;

    ConstTransformRcPtr getLinearCC() const noexcept override;
    void setLinearCC(const ConstTransformRcPtr & cc) noexcept override;

    ConstTransformRcPtr getColorTimingCC() const noexcept override;
    void setColorTimingCC(const ConstTransformRcPtr & cc) noexcept override;

    ConstTransformRcPtr getChannelView() const noexcept override;
    void setChannelView(const ConstTransformRcPtr & transform) noexcept override;

    ConstTransformRcPtr getDisplayCC() const noexcept override;
    void setDisplayCC(const ConstTransformRcPtr & cc) noexcept override;

    void setLooksOverrideEnabled(bool enable) override;
    bool getLooksOverrideEnabled() const override;

    void setLooksOverride(const char * looks) override;
    const char * getLooksOverride() const override;

    ConstProcessorRcPtr getProcessor(const ConstConfigRcPtr & config,
                                     const ConstContextRcPtr & context) const override;

    ConstProcessorRcPtr getProcessor(const ConstConfigRcPtr & config) const override;

    static void Deleter(LegacyViewingPipeline * vp);

protected:
    void validate() const;

private:
    TransformRcPtr m_linearCC;
    TransformRcPtr m_colorTimingCC;
    TransformRcPtr m_channelView;
    TransformRcPtr m_displayCC;
    DisplayViewTransformRcPtr m_displayViewTransform;
    // Looks from DisplayViewTransform are applied separately.
    bool m_dtOriginalLooksBypass{ false };

    bool m_looksOverrideEnabled{ false };
    std::string m_looksOverride;
};

} // namespace OCIO_NAMESPACE


#endif // INCLUDED_OCIO_LEGACYVIEWINGPIPELINE_H
