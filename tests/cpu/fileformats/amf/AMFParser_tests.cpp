// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "fileformats/amf/AMFParser.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO_NAMESPACE
{
    OCIO_ADD_TEST(AMFParser, CreateFromAMF_slogtopq)
    {
        AMFInfoRcPtr amfInfoObject = AMFInfo::Create();
        std::string amfFilePath(GetTestFilesDir() + "/amf/slogtopq.amf");
        ConstConfigRcPtr amfConfig = CreateFromAMF(amfInfoObject, amfFilePath.c_str());

        int numRoles = amfConfig->getNumRoles();
        OCIO_REQUIRE_ASSERT(numRoles > 0);

        for (int i = 0; i < numRoles; i++)
        {
            std::string roleName = amfConfig->getRoleName(i);
            if (roleName.find("amf_clip_") == 0)
            {
                std::string srcColorSpace = amfConfig->getRoleColorSpace(i);
                std::string activeDisplays = amfConfig->getActiveDisplays();
                std::vector<std::string> listActiveDisplays;
                std::stringstream ss(activeDisplays);
                std::string token;
                while (std::getline(ss, token, ','))
                {
                    listActiveDisplays.push_back(token);
                }
                OCIO_REQUIRE_ASSERT(listActiveDisplays.size() == 1);

                std::string activeDisplay = listActiveDisplays.at(0);
                std::string activeViews = amfConfig->getActiveViews();
                std::vector<std::string> listActiveViews;
                ss.clear();
                ss.str(activeViews);
                while (std::getline(ss, token, ','))
                {
                    listActiveViews.push_back(token);
                }
                OCIO_REQUIRE_ASSERT(listActiveViews.size() == 1);

                std::string activeView = listActiveViews.at(0);
                OCIO_REQUIRE_ASSERT(activeDisplay.compare(srcColorSpace) != 0);

                DisplayViewTransformRcPtr transform = DisplayViewTransform::Create();
                transform->setSrc(srcColorSpace.c_str());
                transform->setDisplay(activeDisplay.c_str());
                transform->setView(activeView.c_str());
                transform->validate();

                ConstProcessorRcPtr processor = amfConfig->getProcessor(transform);
                OCIO_CHECK_ASSERT(processor);

                return;
            }

        }
        OCIO_REQUIRE_ASSERT(true);
    }

    OCIO_ADD_TEST(AMFParser, CreateFromAMF_slogtopq_wlook)
    {
        AMFInfoRcPtr amfInfoObject = AMFInfo::Create();
        std::string amfFilePath(GetTestFilesDir() + "/amf/slogtopq_wlook.amf");
        ConstConfigRcPtr amfConfig = CreateFromAMF(amfInfoObject, amfFilePath.c_str());

        int numRoles = amfConfig->getNumRoles();
        OCIO_REQUIRE_ASSERT(numRoles > 0);

        for (int i = 0; i < numRoles; i++)
        {
            std::string roleName = amfConfig->getRoleName(i);
            if (roleName.find("amf_clip_") == 0)
            {
                std::string srcColorSpace = amfConfig->getRoleColorSpace(i);
                std::string activeDisplays = amfConfig->getActiveDisplays();
                std::vector<std::string> listActiveDisplays;
                std::stringstream ss(activeDisplays);
                std::string token;
                while (std::getline(ss, token, ','))
                {
                    listActiveDisplays.push_back(token);
                }
                OCIO_REQUIRE_ASSERT(listActiveDisplays.size() == 1);

                std::string activeDisplay = listActiveDisplays.at(0);
                std::string activeViews = amfConfig->getActiveViews();
                std::vector<std::string> listActiveViews;
                ss.clear();
                ss.str(activeViews);
                while (std::getline(ss, token, ','))
                {
                    listActiveViews.push_back(token);
                }
                OCIO_REQUIRE_ASSERT(listActiveViews.size() == 1);

                std::string activeView = listActiveViews.at(0);
                OCIO_REQUIRE_ASSERT(activeDisplay.compare(srcColorSpace) != 0);

                DisplayViewTransformRcPtr transform = DisplayViewTransform::Create();
                transform->setSrc(srcColorSpace.c_str());
                transform->setDisplay(activeDisplay.c_str());
                transform->setView(activeView.c_str());
                transform->validate();

                ConstProcessorRcPtr processor = amfConfig->getProcessor(transform);
                OCIO_CHECK_ASSERT(processor);

                return;
            }

        }
        OCIO_REQUIRE_ASSERT(true);
    }

    OCIO_ADD_TEST(AMFParser, CreateFromAMF_aces2_slog3_to_p3d65)
    {
        // An AMF that references ACES 2.0 (v2.0) transform IDs must auto-select
        // an ACES 2 reference config and resolve every transform through the
        // "amf_transform_ids" interchange attribute.
        AMFInfoRcPtr amfInfoObject = AMFInfo::Create();
        std::string amfFilePath(GetTestFilesDir() + "/amf/aces2_slog3_to_p3d65.amf");
        // Exercise the idiomatic Config::CreateFromAMF factory (Phase 4 API).
        ConstConfigRcPtr amfConfig = Config::CreateFromAMF(amfInfoObject, amfFilePath.c_str());

        OCIO_REQUIRE_ASSERT(amfConfig);

        // The auto-selected ACES 2 studio config is profile version 2.5+.
        OCIO_CHECK_ASSERT(amfConfig->getMajorVersion() > 2 ||
                          (amfConfig->getMajorVersion() == 2 && amfConfig->getMinorVersion() >= 5));

        // The input transform (S-Log3) must have resolved to a color space.
        OCIO_REQUIRE_ASSERT(std::strlen(amfInfoObject->getInputColorSpaceName()) > 0);

        // The output transform must have produced an active display and view.
        std::string activeDisplay = amfConfig->getActiveDisplays();
        std::string activeView    = amfConfig->getActiveViews();
        OCIO_CHECK_ASSERT(!activeDisplay.empty());
        OCIO_CHECK_ASSERT(!activeView.empty());

        // Both look transforms (Reference Gamut Compress + a CDL) resolved.
        OCIO_CHECK_ASSERT(amfConfig->getNumLooks() >= 2);

        // The resolved pipeline must be usable end to end.
        DisplayViewTransformRcPtr transform = DisplayViewTransform::Create();
        transform->setSrc(amfInfoObject->getInputColorSpaceName());
        transform->setDisplay(activeDisplay.c_str());
        transform->setView(activeView.c_str());
        transform->validate();

        ConstProcessorRcPtr processor = amfConfig->getProcessor(transform);
        OCIO_CHECK_ASSERT(processor);
    }

} // namespace OCIO_NAMESPACE
