// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "fileformats/amf/AMFParser.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{
// Canonical ACES AMF sample files (copied from the aces-amf library's
// Generated_Samples_AMF/valid_AMFs corpus) that use ACES 2.0 transform IDs and
// therefore resolve against the ACES 2 studio builtin config. See
// tests/data/files/amf/README.md.
static const std::vector<std::string> kValidAces2Samples =
{
    "01_minimal.amf",
    "02_idt_only_vfx_pull.amf",
    "03_idt_odt_viewing_pipeline.amf",
    "04_basic_grading_single_lmt.amf",
    "05_complex_grading_multiple_lmts.amf",
    "06_cdl_in_acescct.amf",
    "07_lut_based_look.amf",
    "08_mixed_cdl_and_lut.amf",
    "09_applied_non_applied_sequence.amf",
    "10_clip_specific_metadata.amf",
    "11_multi_author.amf",
    "12_primary_with_archived_looks.amf",
    "13_url_encoded_filenames.amf",
    "14_complex_metadata.amf",
    "15_hdr_pipeline_p3_st2084.amf",
    "16_gamut_compress_lmt.amf",
    "24_aces_2_0_basic.amf",
    "25_aces_2_0_with_lmt.amf",
    "30_cdl_reference_from_ccc.amf",
    "35_aces_2_0_full_grading.amf",
    "36_aces_2_0_hdr_p3.amf",
    "37_aces_2_0_multiple_outputs.amf",
};
} // anonymous namespace

OCIO_ADD_TEST(AMFParser, CreateFromAMF_canonical_aces2_samples)
{
    for (const auto & sample : kValidAces2Samples)
    {
        const std::string amfFilePath(GetTestFilesDir() + "/amf/" + sample);

        AMFInfoRcPtr amfInfoObject = AMFInfo::Create();
        ConstConfigRcPtr amfConfig;
        OCIO_CHECK_NO_THROW_FROM(
            amfConfig = Config::CreateFromAMF(amfInfoObject, amfFilePath.c_str()), sample);
        OCIO_REQUIRE_ASSERT(amfConfig);

        // The generated config must validate, and the input transform must
        // resolve to a color space.
        OCIO_CHECK_NO_THROW_FROM(amfConfig->validate(), sample);
        OCIO_CHECK_ASSERT_MESSAGE(std::strlen(amfInfoObject->getInputColorSpaceName()) > 0, sample);

        // AMFInfo reports the resolved display/view; the pipeline must build a
        // processor end to end.
        const std::string display = amfInfoObject->getDisplayName();
        const std::string view    = amfInfoObject->getViewName();
        OCIO_CHECK_ASSERT_MESSAGE(!display.empty(), sample);
        OCIO_CHECK_ASSERT_MESSAGE(!view.empty(), sample);

        // Samples 01/02 have no output transform, so their display/view
        // legitimately fall back to the config's default ("None"/"Raw"). Every
        // other sample must resolve a real output display -- assert it is not
        // the fallback, so a regression that drops output-transform resolution
        // (silently falling back) is caught.
        const bool outputLess =
            (sample == "01_minimal.amf" || sample == "02_idt_only_vfx_pull.amf");
        if (!outputLess)
            OCIO_CHECK_ASSERT_MESSAGE(display != "None", sample);

        DisplayViewTransformRcPtr transform = DisplayViewTransform::Create();
        transform->setSrc(amfInfoObject->getInputColorSpaceName());
        transform->setDisplay(display.c_str());
        transform->setView(view.c_str());
        OCIO_CHECK_NO_THROW_FROM(transform->validate(), sample);

        ConstProcessorRcPtr processor;
        OCIO_CHECK_NO_THROW_FROM(processor = amfConfig->getProcessor(transform), sample);
        OCIO_CHECK_ASSERT_MESSAGE(processor.get() != nullptr, sample);
    }
}

OCIO_ADD_TEST(AMFParser, CreateFromAMF_rejects_amf_v1_schema)
{
    // Only AMF schema v2.0+ is supported; a v1.0 document is rejected.
    const std::string amfFilePath(GetTestFilesDir() + "/amf/negative_v1_schema.amf");
    AMFInfoRcPtr amfInfoObject = AMFInfo::Create();
    OCIO_CHECK_THROW(Config::CreateFromAMF(amfInfoObject, amfFilePath.c_str()), Exception);
}

OCIO_ADD_TEST(AMFParser, CreateFromAMF_rejects_aces1_transforms)
{
    // ACES 1.x transform IDs are not present in the ACES 2 builtin config, so a
    // valid v2.0-schema AMF that references them must fail to load.
    const std::string amfFilePath(GetTestFilesDir() + "/amf/26_aces_1_3_explicit.amf");
    AMFInfoRcPtr amfInfoObject = AMFInfo::Create();
    OCIO_CHECK_THROW(Config::CreateFromAMF(amfInfoObject, amfFilePath.c_str()), Exception);
}

} // namespace OCIO_NAMESPACE
