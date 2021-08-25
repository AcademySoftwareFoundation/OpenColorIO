/*
OFX Support Library, a library that skins the OFX plug-in API with C++ classes.
Copyright (C) 2004-2005 The Open Effects Association Ltd
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

#include "ofxsSupportPrivate.h"

namespace OFX {

  namespace MultiThread {

    ////////////////////////////////////////////////////////////////////////////////
    // SMP class

    /** @brief ctor */
    Processor::Processor(void)
    {
    }

    /** @brief dtor */
    Processor::~Processor()
    {
    }

    /** @brief Function to pass to the multi thread suite */
    void Processor::staticMultiThreadFunction(unsigned int threadIndex, unsigned int threadMax, void *customArg)
    {
      // cast the custom arg to one of me
      Processor *me = (Processor *)  customArg;

      // and call my thread function
      me->multiThreadFunction(threadIndex, threadMax);
    }

    /** @brief Function to pass to the multi thread suite */
    void Processor::multiThread(unsigned int nCPUs)
    {
      // if 0, use all the CPUs we can
      if(nCPUs == 0)
        nCPUs = OFX::MultiThread::getNumCPUs();

      // if 1 cpu, don't bother with the threading
      if(nCPUs == 1) {
        multiThreadFunction(0, 1);
      }
      else {
        // OK do it
          OfxStatus stat = kOfxStatFailed;
          if(OFX::Private::gThreadSuite){
              stat = OFX::Private::gThreadSuite->multiThread(staticMultiThreadFunction, nCPUs, (void *)this);
          }

        // did we do it?
        throwSuiteStatusException(stat);
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // futility functions

    /** @brief Has the current thread been spawned from an MP */
    bool isSpawnedThread(void)
    {
      if(OFX::Private::gThreadSuite){
          int v = OFX::Private::gThreadSuite->multiThreadIsSpawnedThread();
          return v != 0;
      }else{
          return false;
      }
    }

    /** @brief The number of CPUs that can be used for MP-ing */
    unsigned  int getNumCPUs(void)
    {
      unsigned int n = 1;
      OfxStatus stat = OFX::Private::gThreadSuite ? OFX::Private::gThreadSuite->multiThreadNumCPUs(&n) : kOfxStatFailed;

      if(stat != kOfxStatOK) n = 1;
      return n;
    }
      
    /** @brief The index of the current thread. From 0 to numCPUs() - 1 */
    unsigned int getThreadIndex(void)
    {
      unsigned int n = 0;
      OfxStatus stat = OFX::Private::gThreadSuite ? OFX::Private::gThreadSuite->multiThreadIndex(&n) : kOfxStatFailed;
      if(stat != kOfxStatOK) n = 0;
      return n;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // MUTEX class

    /** @brief ctor */
    Mutex::Mutex(int lockCount)
      : _handle(0)
    {
      OfxStatus stat = OFX::Private::gThreadSuite ? OFX::Private::gThreadSuite->mutexCreate(&_handle, lockCount) : kOfxStatReplyDefault;
      throwSuiteStatusException(stat);
    }

    /** @brief dtor */
    Mutex::~Mutex(void)
    {
      OfxStatus stat = OFX::Private::gThreadSuite ? OFX::Private::gThreadSuite->mutexDestroy(_handle) : kOfxStatReplyDefault;
      (void)stat;
    }

    /** @brief lock it, blocks until lock is gained */
    void Mutex::lock()
    {
      OfxStatus stat = OFX::Private::gThreadSuite ? OFX::Private::gThreadSuite->mutexLock(_handle) : kOfxStatReplyDefault;
      throwSuiteStatusException(stat);
    }

    /** @brief unlock it */
    void Mutex::unlock()
    {
      OfxStatus stat = OFX::Private::gThreadSuite ? OFX::Private::gThreadSuite->mutexUnLock(_handle) : kOfxStatReplyDefault;
      throwSuiteStatusException(stat);
    }

    /** @brief attempt to lock, non-blocking */
    bool Mutex::tryLock()
    {
      OfxStatus stat = OFX::Private::gThreadSuite ? OFX::Private::gThreadSuite->mutexTryLock(_handle) : kOfxStatReplyDefault;
      return stat == kOfxStatOK;
    }

  };
};
