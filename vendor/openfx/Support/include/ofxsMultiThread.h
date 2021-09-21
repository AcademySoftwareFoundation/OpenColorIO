#ifndef _ofxsMultiThread_H_
#define _ofxsMultiThread_H_
/*
OFX Support Library, a library that skins the OFX plug-in API with C++ classes.
Copyright (C) 2004-2007 The Open Effects Association Ltd
Author Bruno Nicoletti bruno@thefoundry.co.uk

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
* Neither the name The Open Effects Association Ltd, nor the names of its 
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The Open Effects Association Ltd
1 Wardour St
London W1D 6PA
England



*/

/** @file This file contains core code that wraps OFX 'objects' with C++ classes.

This file only holds code that is visible to a plugin implementation, and so hides much
of the direct OFX objects and any library side only functions.
*/

#include "ofxsCore.h"

typedef struct OfxMutex* OfxMutexHandle;

namespace OFX {

  /** @brief Multi thread namespace */
  namespace MultiThread {

    /** @brief Class that wraps up SMP multi-processing */
    class Processor {
    private :
      /** @brief Function to pass to the multi thread suite */
      static void staticMultiThreadFunction(unsigned int threadIndex, unsigned int threadMax, void *customArg);

    public  :
      /** @brief ctor */
      Processor();

      /** @brief dtor */
      virtual ~Processor();            

      /** @brief function that will be called in each thread. ID is from 0..nThreads-1 nThreads are the number of threads it is being run over */
      virtual void multiThreadFunction(unsigned int threadID, unsigned int nThreads) = 0;

      /** @brief call this to kick off multi threading

      The nCPUs is 0, the maximum allowable number of CPUs will be used.
      */
      virtual void multiThread(unsigned int nCPUs = 0);
    };

    /** @brief Has the current thread been spawned from an MP */
    bool isSpawnedThread(void);

    /** @brief The number of CPUs that can be used for MP-ing */
    unsigned int getNumCPUs(void);

    /** @brief The index of the current thread. From 0 to numCPUs() - 1 */
    unsigned int getThreadIndex(void);

    /** @brief An OFX mutex */
    class Mutex {
    protected :
      OfxMutexHandle _handle; /**< @brief The handle */

    public :
      /** @brief ctor */
      Mutex(int lockCount = 0);

      /** @brief dtor */
      virtual ~Mutex(void);

      /** @brief lock it, blocks until lock is gained */
      void lock();

      /** @brief unlock it */
      void unlock();

      /** @brief attempt to lock, non-blocking

      \brief returns
      - true if the lock was achieved
      - false if it could not
      */
      bool tryLock();
    };

    /// a class to wrap around a mutex which is exception safe
    /// it locks the mutex on construction and unlocks it on destruction
    class AutoMutex {
    protected :
      Mutex &_mutex;

    public :
      /// ctor, acquires the lock
      explicit AutoMutex(Mutex &m)
        : _mutex(m)
      {
        _mutex.lock();
      }

      /// dtor, releases the lock
      virtual ~AutoMutex()
      {
        _mutex.unlock();
      }

    };
  };
};

#endif
