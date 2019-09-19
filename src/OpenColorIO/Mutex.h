/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef INCLUDED_OCIO_MUTEX_H
#define INCLUDED_OCIO_MUTEX_H


#include <mutex> 
#include <assert.h>


/** For internal use only */

OCIO_NAMESPACE_ENTER
{

#ifndef NDEBUG
    // In debug mode, try to trap recursive cases and lock/unlock debalancing cases
    class DebugLock
    {
    public:
        DebugLock() = default;
        DebugLock(const DebugLock &) = delete;
        DebugLock& operator=(const DebugLock &) = delete;
        ~DebugLock() { assert(!m_locked); }

        void lock()   { assert(!m_locked); m_mutex.lock(); m_locked = true; }
        void unlock() { assert(m_locked); m_mutex.unlock(); m_locked = false; }
        void try_lock() { assert(!m_locked); m_locked = m_mutex.try_lock(); }

        bool locked() const { return m_locked; }

    private:
        std::mutex m_mutex;
        bool       m_locked = false;
    };
#endif

#ifndef NDEBUG
    // add debug wrappers to mutex
    typedef DebugLock Mutex;
#else
    typedef std::mutex Mutex;
#endif

    typedef std::lock_guard<Mutex> AutoMutex;

}
OCIO_NAMESPACE_EXIT

#endif
