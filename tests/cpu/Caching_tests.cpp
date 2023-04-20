// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "Caching.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{

// A dummy struct to test the cache classes.
struct Data
{
    bool status = false;
};

using DataRcPtr = std::shared_ptr<Data>;

// A guard to set & unset an envvar.
struct Guard
{
    Guard()
        :    m_envvar(OCIO::OCIO_DISABLE_ALL_CACHES)
    {
        OCIO::Platform::Setenv(m_envvar.c_str(), "1");
    }

    explicit Guard(const char * envvar)
        :    m_envvar(envvar ? envvar : "")
    {
        OCIO::Platform::Setenv(m_envvar.c_str(), "1");
    }

    ~Guard()
    {
        OCIO::Platform::Unsetenv(m_envvar.c_str());
    }
    
    const std::string m_envvar;
};
    
}


OCIO_ADD_TEST(Caching, generic_cache)
{
    // A unit test to check the GenericCache class.

    {
        OCIO::GenericCache<std::string, DataRcPtr> cache;
        OCIO_CHECK_ASSERT(cache.isEnabled());

        {
            OCIO::AutoMutex m(cache.lock());

            DataRcPtr entry1 = std::make_shared<Data>();
            cache["entry1"] = entry1;

            OCIO_CHECK_ASSERT(cache.exists("entry1"));
            OCIO_CHECK_EQUAL(cache["entry1"], entry1);
        }

        // Some faulty checks.
        OCIO_CHECK_ASSERT(!cache.exists("entry2"));

        // Flush the cache and check the content.
        OCIO_CHECK_NO_THROW(cache.clear());
        OCIO_CHECK_ASSERT(!cache.exists("entry1"));
        OCIO_CHECK_ASSERT(!cache.exists("entry2"));
    }

    {
        // Disable all the caches.
        Guard guard;

        OCIO::GenericCache<std::string, DataRcPtr> cache;
        OCIO_CHECK_ASSERT(!cache.isEnabled());

        // The lock control is useless here but useful as a good example to copy & paste.
        OCIO::AutoMutex m(cache.lock());

        DataRcPtr entry1 = std::make_shared<Data>();
        cache["entry1"] = entry1;

        OCIO_CHECK_ASSERT(!cache.exists("entry1"));
    }

    {
        // Disable processor caches i.e. no impact on the generic cache.
        Guard guard(OCIO::OCIO_DISABLE_PROCESSOR_CACHES);

        OCIO::GenericCache<std::string, DataRcPtr> cache;
        OCIO_CHECK_ASSERT(cache.isEnabled());

        // The lock control is useless here but useful as a good example to copy & paste.
        OCIO::AutoMutex m(cache.lock());

        DataRcPtr entry1 = std::make_shared<Data>();
        cache["entry1"] = entry1;

        OCIO_CHECK_ASSERT(cache.exists("entry1"));
    }
}

OCIO_ADD_TEST(Caching, processor_cache)
{
    // A unit test to check the ProcessorCache class.

    {
        OCIO::ProcessorCache<std::string, DataRcPtr> cache;
        OCIO_CHECK_ASSERT(cache.isEnabled());
    }

    // Test that the content of the cache stays the same after disabling and enabling the cache.
    {
        OCIO::ProcessorCache<std::string, DataRcPtr> cache;
        OCIO_CHECK_ASSERT(cache.isEnabled());

        DataRcPtr entry1 = std::make_shared<Data>();
        cache["entry1"] = entry1;

        cache.enable(false);

        // Expecting failure because the cache is disabled.
        OCIO_CHECK_ASSERT(!cache.exists("entry1"));

        cache.enable(true);

        // The data with the key "entry1" still exists after enabling the cache.
        OCIO_CHECK_ASSERT(cache.exists("entry1"));
    }

    {
        // Disable all the caches.
        Guard guard;

        OCIO::ProcessorCache<std::string, DataRcPtr> cache1;
        OCIO_CHECK_ASSERT(!cache1.isEnabled());

        OCIO::GenericCache<std::string, DataRcPtr> cache2;
        OCIO_CHECK_ASSERT(!cache2.isEnabled());
    }

    {
        // Only disable the processor caches so the other caches are still enabled.
        Guard guard(OCIO::OCIO_DISABLE_PROCESSOR_CACHES);

        OCIO::ProcessorCache<std::string, DataRcPtr> cache1;
        OCIO_CHECK_ASSERT(!cache1.isEnabled());

        // But the generic cache is still enabled.
        OCIO::GenericCache<std::string, DataRcPtr> cache2;
        OCIO_CHECK_ASSERT(cache2.isEnabled());
    }

    // Test the processor cache reset.
    {
    static const std::string CONFIG = 
        "ocio_profile_version: 2\n"
        "\n"
        "search_path: " + OCIO::GetTestFilesDir() + "\n"
        "\n"
        "environment: {CS3: lut1d_green.ctf}\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "\n"
        "displays:\n"
        "  disp1:\n"
        "    - !<View> {name: view1, colorspace: cs3}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n"
        "    from_scene_reference: !<MatrixTransform> {offset: [0.11, 0.12, 0.13, 0]}\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs3\n"
        "    from_scene_reference: !<FileTransform> {src: $CS3}\n";

        std::istringstream iss;
        iss.str(CONFIG);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss));

        // Creating a editable config to clear the processor cache later in the test.
        OCIO::ConfigRcPtr cfg = config->createEditableCopy();

        {
            // Test that clearProcessorCache clears the Processor cache.

            // Create two processors and confirm that it is the same object as expected.
            OCIO::ConstProcessorRcPtr procA = cfg->getProcessor("cs3", 
                                                                "disp1", 
                                                                "view1", 
                                                                OCIO::TRANSFORM_DIR_FORWARD);
            OCIO::ConstProcessorRcPtr procB = cfg->getProcessor("cs3", 
                                                                "disp1", 
                                                                "view1", 
                                                                OCIO::TRANSFORM_DIR_FORWARD);

            // Comparing the address of both Processor objects to confirm if they are the same or not.
            OCIO_CHECK_EQUAL(procA, procB);                                                          

            cfg->clearProcessorCache();

            // Create a third processor and confirm that it is different from the previous two as the
            // the processor cache was cleared.
            OCIO::ConstProcessorRcPtr procC = cfg->getProcessor("cs3", 
                                                                "disp1", 
                                                                "view1", 
                                                                OCIO::TRANSFORM_DIR_FORWARD);

            OCIO_CHECK_NE(procC, procA); 
        }

        {
            // Test that disable and re-enable the cache, using setProcessorCacheFlags, does not
            // clear the Processor cache.

            OCIO::ConstProcessorRcPtr procA = cfg->getProcessor("cs3", 
                                                                "disp1", 
                                                                "view1", 
                                                                OCIO::TRANSFORM_DIR_FORWARD);                                                      
            
            // Disable and re-enable the processor cache.
            cfg->setProcessorCacheFlags(OCIO::PROCESSOR_CACHE_OFF);
            cfg->setProcessorCacheFlags(OCIO::PROCESSOR_CACHE_ENABLED);
            
            // Confirm that the processor is the same.
            OCIO::ConstProcessorRcPtr procB = cfg->getProcessor("cs3", 
                                                                "disp1", 
                                                                "view1", 
                                                                OCIO::TRANSFORM_DIR_FORWARD);
            OCIO_CHECK_EQUAL(procA, procB); 
        }
    }
}