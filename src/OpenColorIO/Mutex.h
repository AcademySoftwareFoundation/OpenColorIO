// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_MUTEX_H
#define INCLUDED_OCIO_MUTEX_H


#include <mutex> 
#include <assert.h>


/** For internal use only */

OCIO_NAMESPACE_ENTER
{

#ifndef NDEBUG
    // In debug mode, try to trap recursive cases and lock/unlock debalancing cases
    template <class T>
    class DebugLock : public T {
        public:
            DebugLock() : _locked(0) {}
            ~DebugLock() { assert(!_locked); }

            void lock()   { assert(!_locked); _locked = 1; T::lock(); }
            void unlock() { assert(_locked); _locked = 0; T::unlock(); }

            bool locked() { return _locked != 0; }
        private:
            int _locked;
    };
#endif

    /** Automatically acquire and release lock within enclosing scope. */
    template <class T>
    class AutoLock
    {
    public:
        AutoLock() = delete;
        AutoLock & operator=(const AutoLock &) = delete;
        AutoLock & operator=(AutoLock &&) = delete;

        AutoLock(T & m) : _m(m) { _m.lock();   }
        ~AutoLock()             { _m.unlock(); }

    private:
	   T & _m;
    };

#ifndef NDEBUG
    // add debug wrappers to mutex
    typedef DebugLock<std::mutex> Mutex;
#else
    typedef std::mutex Mutex;
#endif

    typedef AutoLock<std::mutex> AutoMutex;

}
OCIO_NAMESPACE_EXIT

#endif
