// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_MUTEX_H
#define INCLUDED_OCIO_MUTEX_H


#include <mutex> 
#include <thread>
#include <assert.h>


/** For internal use only */

namespace OCIO_NAMESPACE
{

#ifndef NDEBUG

// In debug mode, try to trap recursive cases and lock/unlock debalancing cases.
class DebugLock
{
public:
    DebugLock() = default;
    DebugLock(const DebugLock &) = delete;
    DebugLock& operator=(const DebugLock &) = delete;
    ~DebugLock() { assert(m_owner == std::thread::id()); }

    void lock()
    {
        assert(m_owner != std::this_thread::get_id());
        m_mutex.lock();
        m_owner = std::this_thread::get_id();
    }
    void unlock()
    {
        assert(m_owner == std::this_thread::get_id());
        m_owner = std::thread::id();
        m_mutex.unlock();
    }
    bool try_lock()
    {
      assert(m_owner != std::this_thread::get_id());
      return m_mutex.try_lock();
    }

private:
    // An exclusive and non-recursive ownership lock.
    std::mutex      m_mutex;
    std::thread::id m_owner;
};

typedef DebugLock Mutex;

#else

typedef std::mutex Mutex;

#endif

// A non-copyable lock guard i.e. no copy and move semantics.
typedef std::lock_guard<Mutex> AutoMutex;

} // namespace OCIO_NAMESPACE

#endif
