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


#ifndef INCLUDED_OCIO_IMAGEDESC_H
#define INCLUDED_OCIO_IMAGEDESC_H

#include <OpenColorIO/OpenColorIO.h>

OCIO_NAMESPACE_ENTER
{
    class PackedImageDesc::Impl
    {
    public:
        Impl();
        Impl(float * data,
             long width, long height,
             long numChannels,
             ptrdiff_t chanStrideBytes,
             ptrdiff_t xStrideBytes,
             ptrdiff_t yStrideBytes );
        ~Impl();
        
        long getWidth() const;
        long getHeight() const;
        
        ptrdiff_t getXStrideBytes() const;
        ptrdiff_t getYStrideBytes() const;
        
        float* getRData() const;
        float* getGData() const;
        float* getBData() const;
        
    private:
        Impl& operator= (const Impl &);
        Impl(const Impl &);
        
        float * m_data;
        long m_width;
        long m_height;
        ptrdiff_t m_chanStrideBytes;
        ptrdiff_t m_xStrideBytes;
        ptrdiff_t m_yStrideBytes;
    };
    
    
    
    class PlanarImageDesc::Impl
    {
    public:
        Impl();
        Impl(float * rData, float * gData, float * bData,
             long width, long height,
             ptrdiff_t yStrideBytes );
        ~Impl();
        
        long getWidth() const;
        long getHeight() const;
        
        ptrdiff_t getXStrideBytes() const;
        ptrdiff_t getYStrideBytes() const;
        
        float* getRData() const;
        float* getGData() const;
        float* getBData() const;
        
    private:
        Impl& operator= (const Impl &);
        Impl(const Impl &);
        
        float * m_rData;
        float * m_gData;
        float * m_bData;
        
        long m_width;
        long m_height;
        ptrdiff_t m_yStrideBytes;
    };



    /*
    class ImageDesc::Impl
    {
        public:
        
        Impl();
        ~Impl();
        
        void initRGBA(float * data,
                      long width, long height,
                      ptrdiff_t yStrideBytes);
        
        void initSingleBuffer(float * data,
                              long width, long height, long numChannels,
                              ptrdiff_t chanStrideBytes,
                              ptrdiff_t xStrideBytes,
                              ptrdiff_t yStrideBytes);
        
        void initMultiBuffer(float * rData, float * gData, float * bData,
                             long width, long height,
                             ptrdiff_t yStrideBytes);
        
        long getWidth() const;
        long getHeight() const;
        long getNumPixels() const;
        bool isPackedRGBA() const;
        
        float* getRData() const;
        float* getGData() const;
        float* getBData() const;
        
        ptrdiff_t getXStrideBytes() const;
        ptrdiff_t getYStrideBytes() const;
        
        private:
        
        void computePackedRGBA();
        
        long m_width;
        long m_height;
        long m_numPixels;
        bool m_isPackedRGBA;
        
        float * m_rData;
        float * m_gData;
        float * m_bData;
        
        ptrdiff_t m_xStrideBytes;
        ptrdiff_t m_yStrideBytes;
        
        Impl(const Impl &);
        Impl& operator= (const Impl &);
    };
    */
}
OCIO_NAMESPACE_EXIT

#endif
