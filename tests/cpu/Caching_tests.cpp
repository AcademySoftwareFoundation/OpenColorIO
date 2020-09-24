// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "Caching.cpp"

#include "testutils/UnitTest.h"

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
}

