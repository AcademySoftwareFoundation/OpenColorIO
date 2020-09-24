// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CACHING_H
#define INCLUDED_OCIO_CACHING_H


#include <map>

#include <OpenColorIO/OpenColorIO.h>

#include "Mutex.h"
#include "Platform.h"


namespace OCIO_NAMESPACE
{

// Generic cache mechanism where EntryType is the instance type to cache and KeyType is the
// instance type of the key. Note that having efficient key generation & comparison are critical.
// For example integer comparison is efficent but string one could be far less efficient depending
// of its length & where changes occur (e.g. absolute filepaths are inefficient). 
template<typename KeyType, typename EntryType>
class GenericCache
{
public:

    using Entries = std::map<KeyType, EntryType>;
    using Iterator = typename Entries::iterator;

    // Forbid copy & move semantics. 
    GenericCache(const GenericCache &)  = delete;
    GenericCache(GenericCache && other) = delete;
    GenericCache & operator=(const GenericCache &)  = delete;
    GenericCache & operator=(GenericCache && other) = delete;

    GenericCache()
        :   m_envDisableAllCaches(Platform::isEnvPresent(OCIO_DISABLE_ALL_CACHES))
    {
    }

    virtual ~GenericCache() = default;

    void clear() noexcept
    {
        AutoMutex lock(m_mutex);

        m_entries.clear();
    }

    inline void enable(bool enable) noexcept
    {
        AutoMutex lock(m_mutex);

        m_enabled = enable;
        
        if (!isEnabled())
        {
            m_entries.clear();
        }
    }

    inline bool isEnabled() const noexcept { return !m_envDisableAllCaches && m_enabled; }

    // Get and lock the mutex before accessing to a cache entry.
    Mutex & lock() noexcept { return m_mutex; }

    // Check existence of an entry the cache.
    // To only use when lock is on to protect the cache access.
    bool exists(const KeyType & key) const noexcept
    {
        return isEnabled() && m_entries.end() != m_entries.find(key);
    }

    // Get a cache entry. It creates the cache entry if not existing.
    // To only use when lock is on to protect the cache access.
    EntryType & operator[](const KeyType & key) noexcept
    {
        static EntryType dummy;
        return isEnabled() ? m_entries[key] : dummy;
    }

    Iterator begin() noexcept { return m_entries.begin(); }
    Iterator end()   noexcept { return m_entries.end();   }

protected:
    explicit GenericCache(bool disableCaches)
        :   m_envDisableAllCaches(Platform::isEnvPresent(OCIO_DISABLE_ALL_CACHES) || disableCaches)
    {
    }

    const bool m_envDisableAllCaches = false;
    bool m_enabled = true;

private:
    Mutex m_mutex;
    Entries m_entries;
};

// A Processor instance uses this class to cache its derived optimized, CPU, and GPU Processors.
// These caches may be disabled using either of two environment variables. The env. variables allow
// either disabling all caches (including the FileTransform cache), or just the Processor caches.
template<typename KeyType, typename EntryType>
class ProcessorCache : public GenericCache<KeyType, EntryType>
{
public:
    ProcessorCache()
        :   GenericCache<KeyType, EntryType>(Platform::isEnvPresent(OCIO_DISABLE_PROCESSOR_CACHES))
    {
    }

    ~ProcessorCache() = default;
};


} // namespace OCIO_NAMESPACE


#endif // INCLUDED_OCIO_CACHING_H
