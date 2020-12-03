// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_MIXING_HELPERS_H
#define INCLUDED_OCIO_MIXING_HELPERS_H


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

class MixingSliderImpl : public MixingSlider
{
public:
    MixingSliderImpl() = delete;
    MixingSliderImpl & operator=(const MixingSliderImpl &) = delete;
    explicit MixingSliderImpl(MixingColorSpaceManager & mixing);
    ~MixingSliderImpl() override = default;

    float getSliderMinEdge() const noexcept override;
    void setSliderMinEdge(float sliderMixingMinEdge) noexcept override;

    float getSliderMaxEdge() const noexcept override;
    void setSliderMaxEdge(float sliderMixingMaxEdge) noexcept override;

    float sliderToMixing(float sliderUnits) const noexcept override;

    float mixingToSlider(float mixingUnits) const noexcept override;

    static void Deleter(MixingSlider * slider);

private:
    MixingColorSpaceManager & m_mixing;

    float m_sliderMinEdge = 0.0f;
    float m_sliderMaxEdge = 1.0f;
};

class MixingColorSpaceManagerImpl : public MixingColorSpaceManager
{
public:
    MixingColorSpaceManagerImpl() = delete;
    MixingColorSpaceManagerImpl & operator=(const MixingColorSpaceManagerImpl &) = delete;
    explicit MixingColorSpaceManagerImpl(ConstConfigRcPtr & config);
    MixingColorSpaceManagerImpl(const MixingColorSpaceManagerImpl &) = delete;
    ~MixingColorSpaceManagerImpl() override = default;

    size_t getNumMixingSpaces() const noexcept override;
    const char * getMixingSpaceUIName(size_t idx) const override;
    size_t getSelectedMixingSpaceIdx() const noexcept override;
    void setSelectedMixingSpaceIdx(size_t idx) override;
    void setSelectedMixingSpace(const char * mixingSpace) override;

    bool isPerceptuallyUniform() const noexcept override;

    size_t getNumMixingEncodings() const noexcept override;
    const char * getMixingEncodingName(size_t idx) const override;
    size_t getSelectedMixingEncodingIdx() const noexcept override;
    void setSelectedMixingEncodingIdx(size_t idx) override;
    void setSelectedMixingEncoding(const char * mixingEncoding) override;

    void refresh(ConstConfigRcPtr config) override;

    ConstProcessorRcPtr getProcessor(const char * workingName,
                                     const char * displayName,
                                     const char * viewName,
                                     TransformDirection direction) const override;

    MixingSlider & getSlider() noexcept override;
    MixingSlider & getSlider(float sliderMixingMinEdge, float sliderMixingMaxEdge) noexcept override;

    static void Deleter(MixingColorSpaceManager * incs);

    std::ostream & serialize(std::ostream & os) const;

protected:
    void refresh();
    ConstProcessorRcPtr getProcessorWithoutEncoding(const char * workingName,
                                                    const char * displayName,
                                                    const char * viewName) const;

private:
    ConstConfigRcPtr m_config;

    MixingSliderImpl m_slider;

    ColorSpaceNames m_mixingSpaces;
    const ColorSpaceNames m_mixingEncodings{ "RGB", "HSV" };

    size_t m_selectedMixingSpaceIdx = 0;
    size_t m_selectedMixingEncodingIdx = 0;

    ConstColorSpaceInfoRcPtr m_colorPicker;
};

}  // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_MIXING_HELPERS_H
