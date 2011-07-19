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

#include <OpenColorIO/OpenColorIO.h>
#include "ScanlineHelper.h"
#include "ImagePacking.h"

#include <cassert>
#include <cstdlib>
#include <sstream>

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        bool IsPackedRGBA(const ImageDesc& img)
        {
            char* rPtr = reinterpret_cast<char*>(img.getRData());
            char* gPtr = reinterpret_cast<char*>(img.getGData());
            char* bPtr = reinterpret_cast<char*>(img.getBData());
            char* aPtr = reinterpret_cast<char*>(ImageDesc_GetAData(img));
            
            if(gPtr-rPtr != sizeof(float)) return false;
            if(bPtr-gPtr != sizeof(float)) return false;
            if(aPtr && (aPtr-bPtr != sizeof(float))) return false;
            
            ptrdiff_t xStrideBytes = img.getXStrideBytes();
            
            // Confirm xStrideBytes is a pure float-sized packing
            // (I.e., it will divide evenly)
            if(xStrideBytes <= 0) return false;
            div_t result = div((int) xStrideBytes, (int)sizeof(float));
            if(result.rem != 0) return false;
            
            int numChannels = result.quot;
            if(numChannels != 4) return false;
            
            return true;
        }
        
        const int PIXELS_PER_LINE = 4096;
    }
    
    ////////////////////////////////////////////////////////////////////////////
    
    ScanlineHelper::ScanlineHelper(ImageDesc& img):
                                   m_img(&img),
                                   m_buffer(0),
                                   m_imagePixelIndex(0),
                                   m_numPixelsCopied(0),
                                   m_yIndex(0),
                                   m_inPlaceMode(false)
        {
            if((img.getWidth() <= 0) || (img.getHeight() <= 0))
            {
                std::ostringstream os;
                os << "Cannot process scanline request,";
                os << " img has invalid size: ";
                os << img.getWidth() << " x " << img.getHeight();
                throw Exception(os.str().c_str()); 
            }
            
            if(IsPackedRGBA(img))
            {
                m_inPlaceMode = true;
            }
            else
            {
                // TODO: Re-use memory from thread-safe memory pool, rather
                // than doing a new allocation each time.
                
                m_buffer = (float*)malloc(sizeof(float)*PIXELS_PER_LINE*4);
            }
        }
        
        ScanlineHelper::~ScanlineHelper()
        {
            free(m_buffer);
        }
        
        // Copy from the src image to our scanline, in our preferred
        // pixel layout.
        
        void ScanlineHelper::prepRGBAScanline(float** buffer, long* numPixels)
        {
            if(m_inPlaceMode)
            {
                // TODO: what if scanline is too short, or too long?
                if(m_yIndex >= m_img->getHeight())
                {
                    *numPixels = 0;
                    return;
                }
                
                char* rowPtr = reinterpret_cast<char*>(m_img->getRData());
                rowPtr += m_img->getYStrideBytes()*m_yIndex;
                
                *buffer = reinterpret_cast<float*>(rowPtr);
                *numPixels = m_img->getWidth();
            }
            else
            {
                PackRGBAFromImageDesc(*m_img, m_buffer,
                                      &m_numPixelsCopied,
                                      PIXELS_PER_LINE,
                                      m_imagePixelIndex);
                *buffer = m_buffer;
                *numPixels = m_numPixelsCopied;
            }
        }
        
        // Write back the result of our work, from the scanline to our
        // destination image.
        
        void ScanlineHelper::finishRGBAScanline()
        {
            if(m_inPlaceMode)
            {
                m_yIndex += 1;
            }
            else
            {
                UnpackRGBAToImageDesc(*m_img,
                                      m_buffer,
                                      m_numPixelsCopied,
                                      m_imagePixelIndex);
                m_imagePixelIndex += m_numPixelsCopied;
            }
        }

}
OCIO_NAMESPACE_EXIT
