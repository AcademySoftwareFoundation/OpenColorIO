// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_MUTEX_H
#define INCLUDED_OCIO_MUTEX_H


#include <mutex> 
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
    ~DebugLock() { assert(!m_locked); }

    void lock()     { assert(!m_locked); m_mutex.lock();   m_locked = true;  }
    void unlock()   { assert(m_locked);  m_mutex.unlock(); m_locked = false; }
    void try_lock() { assert(!m_locked); m_locked = m_mutex.try_lock(); }

    bool locked() const { return m_locked; }

private:
    // An exclusive and non-recursive ownership lock.
    std::mutex m_mutex;
    bool       m_locked = false;
};

typedef DebugLock Mutex;

#else

typedef std::mutex Mutex;

#endif

// A non-copyable lock guard i.e. no copy and move semantics.
typedef std::lock_guard<Mutex> AutoMutex;

} // namespace OCIO_NAMESPACE

#endif
