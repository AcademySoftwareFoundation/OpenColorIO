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

#include <OpenColorSpace/OpenColorSpace.h>
#include "ImageDesc.h"
#include "ScanlineHelper.h"

#include <algorithm>
#include <cmath>
#include <iostream>

OCS_NAMESPACE_ENTER
{
    namespace
    {
        // Convert rgb -> hsv
        // rgb, s, v are unconstrained
        // h [0,1]
        // Supports in-place operation
        
        void rgb_to_hsv(float* hsv, const float* rgb)
        {
            float r = rgb[0];
            float g = rgb[1];
            float b = rgb[2];
            
            float minComponent = std::min(std::min(r,g),b);
            float maxComponent = std::max(std::max(r,g),b);
            
            hsv[2] = maxComponent;
            
            float delta = maxComponent - minComponent;
            
            // gray-scale
            if (delta==0.0)
            {
                hsv[0] = 0.0f;
                hsv[1] = 0.0f;
                return;
            }
            
            // Compute saturation
            hsv[1] = delta / maxComponent;
            
            // Compute hue angle
            if (r==maxComponent)
                hsv[0] = (g - b) / (6*delta); // Between yellow & magenta
            else if (g==maxComponent)
                hsv[0] = 1/3.0f + (b - r) / (6.0f*delta); // Between cyan & yellow
            else
                hsv[0] = 2/3.0f + (r - g) / (6.0f*delta); // Between magenta & cyan
            
            // Normalize hue
            while (hsv[0] < 0.0f)
                hsv[0] += 1.0f;
        }
        
        // Convert hsv -> rgb
        // rgb, s, v are unconstrained
        // h [0,1]
        // Supports in-place operation
        void hsv_to_rgb(float* rgb, const float* hsv)
        {
            float h = hsv[0];
            float s = hsv[1];
            float v = hsv[2];
            
            // gray-scale
            if (h==0.0)
            {
                rgb[0] = v;
                rgb[1] = v;
                rgb[2] = v;
                return;
            }
            
            // Sector h from [0,5]
            while (h<0.0)
            {
                h += 1.0f;
            }
            while (h>=1.0)
            {
                h -= 1.0f;
            }
            
            h *= 6.0f;
            
            int i = (int)(floor(h));
            
            // Compute intermediates
            float f = h - (float)i;
            float p = v * ( 1.0f - s );
            float q = v * ( 1.0f - s * f );
            float t = v * ( 1.0f - s * ( 1 - f ) );
            
            // Create final color based on hue angle
            switch(i)
            {
            case 0:
                rgb[0] = v;
                rgb[1] = t;
                rgb[2] = p;
                break;
            case 1:
                rgb[0] = q;
                rgb[1] = v;
                rgb[2] = p;
                break;
            case 2:
                rgb[0] = p;
                rgb[1] = v;
                rgb[2] = t;
                break;
            case 3:
                rgb[0] = p;
                rgb[1] = q;
                rgb[2] = v;
                break;
            case 4:	
                rgb[0] = t;
                rgb[1] = p;
                rgb[2] = v;
                break;
            default: // Case 5
                rgb[0] = v;
                rgb[1] = p;
                rgb[2] = q;
                break;
            }
        }
    } // namespace
    
    // "STANDARD" HSL IMPLEMENTATION
    
    namespace
    {
        inline float Hue_2_RGB(float v1, float v2, float vH )
        {
           if ( vH < 0.0f ) vH += 1.0f;
           if ( vH > 1.0f ) vH -= 1.0f;
           if ( ( 6.0f * vH ) < 1.0f )
               return ( v1 + ( v2 - v1 ) * 6.0f * vH );
           if ( ( 2.0f * vH ) < 1.0f )
               return ( v2 );
           if ( ( 3.0f * vH ) < 2.0f )
               return ( v1 + ( v2 - v1 ) * ( ( 2.0f / 3.0f ) - vH ) * 6.0f );
           return ( v1 );
        }
        
        // Convert rgb -> hls
        // rgb, s, v are unconstrained
        // h [0,1]
        // Supports in-place operation
        
        void rgb_to_hsl(float* hsl, const float* rgb)
        {
            float r = rgb[0];
            float g = rgb[1];
            float b = rgb[2];
            
            hsl[0] = 0.0;
            hsl[1] = 0.0;
            hsl[2] = 0.0;
            
            float minComponent = std::min( r, std::min(g, b ));
            float maxComponent = std::max( r, std::max(g, b ));
            float delta = maxComponent - minComponent;
            
            hsl[2] = (maxComponent + minComponent) / 2.0f;
            
            if ( delta == 0.0f )
            {
                hsl[0] = 0.0f;
                hsl[1] = 0.0f;
                return;
            }
            
            if(hsl[2] < 0.5f)
            {
                hsl[1] = delta / ( maxComponent + minComponent );
            }
            else
            {
                hsl[1] = delta / ( 2 - maxComponent - minComponent );
            }
            
            float del_R = ( ( ( maxComponent - r ) / 6.0f ) + 
                ( delta / 2.0f ) ) / delta;
            float del_G = ( ( ( maxComponent - g ) / 6.0f ) + 
                ( delta / 2.0f ) ) / delta;
            float del_B = ( ( ( maxComponent - b ) / 6.0f ) + 
                ( delta / 2.0f ) ) / delta;
            
            if ( r == maxComponent ) hsl[0] = del_B - del_G;
            else if ( g == maxComponent ) hsl[0] = ( 1.0f / 3.0f ) + del_R - del_B;
            else hsl[0] = ( 2.0f / 3.0f ) + del_G - del_R;
            
            if ( hsl[0] < 0.0f ) hsl[0] += 1;
            if ( hsl[0] > 1.0f ) hsl[0] -= 1;
        }
        
        // Convert hsv -> rgb
        // rgb, s, v are unconstrained
        // h [0,1]
        // Supports in-place operation
        
        void hsl_to_rgb(float* rgb, const float* hsl)
        {
            float h = hsl[0];
            float s = hsl[1];
            float l = hsl[2];
            
            if(s==0.0)
            {
                rgb[0] = l;
                rgb[1] = l;
                rgb[2] = l;
                return;
            }
            
            float var2;
            
            if(l < 0.5)
            {
                var2 = l * (1 + s);
            }
            else
            {
                var2 = (l + s) - (s * l);
            }
            
            float var1 = 2*l - var2;
            
            rgb[0] = Hue_2_RGB( var1, var2, h + ( 1.0f / 3.0f ) );
            rgb[1] = Hue_2_RGB( var1, var2, h );
            rgb[2] = Hue_2_RGB( var1, var2, h - ( 1.0f / 3.0f ) );
        }
    } // namespace
    
    namespace
    {
        const float TML_GREEN_MIN = 1.0e-06f;
        
        // Convert rgb -> tml
        // Supports in-place operation
        void rgb_to_tml(float* tml, const float* rgb)
        {
            float greenNorm[3];
            
            // Normalize green component to 1.0
            if(rgb[1] < TML_GREEN_MIN)
            {
                greenNorm[0] = rgb[0];
                greenNorm[1] = rgb[1];
                greenNorm[2] = rgb[2];
            }
            else
            {
                greenNorm[0] = rgb[0]/rgb[1];
                greenNorm[1] = 1.0f;
                greenNorm[2] = rgb[2]/rgb[1];
            }
            
            float redblueAve = (greenNorm[0] + greenNorm[2]) / 2.0f;
            float colorTemp = greenNorm[2] - greenNorm[0]; // [-1,1]
            float colorTempNorm = colorTemp / 2.0f + 0.5f;
            
            float lumaCoef[] = { 0.2126f, 0.7152f, 0.0722f };
            float luminance = rgb[0]*lumaCoef[0] + \
                              rgb[1]*lumaCoef[1] + \
                              rgb[2]*lumaCoef[2];
            
            tml[0] = colorTempNorm;
            tml[1] = redblueAve;
            tml[2] = luminance;
        }
        
        // Supports in-place operation
        void tml_to_rgb(float* rgb, const float* tml)
        {
            float colorTempNorm = tml[0];
            float redblueAve = tml[1];
            float luminance = tml[2];
            
             // color temp is red / blue difference
            float colorTemp = colorTempNorm * 2.0f - 1.0f;
            
            float greenNorm[3];
            greenNorm[0] = redblueAve-colorTemp/2.0f;
            greenNorm[1] = 1.0;
            greenNorm[2] = redblueAve+colorTemp/2.0f;
            
            
            float lumaCoef[] = { 0.2126f, 0.7152f, 0.0722f };
            float previousLuminance = greenNorm[0]*lumaCoef[0] + 
                                      greenNorm[1]*lumaCoef[1] + 
                                      greenNorm[2]*lumaCoef[2];
            
            if(previousLuminance>TML_GREEN_MIN)
            {
                rgb[0] = greenNorm[0] / previousLuminance * luminance;
                rgb[1] = greenNorm[1] / previousLuminance * luminance;
                rgb[2] = greenNorm[2] / previousLuminance * luminance;
            }
            else
            {
                rgb[0] = greenNorm[0];
                rgb[1] = greenNorm[1];
                rgb[2] = greenNorm[2];
            }
        }
    } // namespace
    
    
    
    namespace
    {
        typedef void (*funcptr)(float*, const float*);
    }
    
    // 87.2 ms normal
    
    /*
    void ApplyTransform(ImageDesc& img,
                        CoordinateSpace srcCoordinateSpace,
                        CoordinateSpace dstCoordinateSpace)
    {
        if(srcCoordinateSpace == dstCoordinateSpace) return;
        
        ScanlineHelper scanlineHelper(img);
        float * rgbaBuffer = 0;
        long numPixels = 0;
        
        funcptr transformToRGBFunc = 0;
        if(srcCoordinateSpace == COORDINATE_SPACE_HSV)
            transformToRGBFunc = &hsv_to_rgb;
        else if(srcCoordinateSpace == COORDINATE_SPACE_HSL)
            transformToRGBFunc = &hsl_to_rgb;
        else if(srcCoordinateSpace == COORDINATE_SPACE_TML)
            transformToRGBFunc = &tml_to_rgb;
        
        funcptr transformFromRGBFunc = 0;
        if(dstCoordinateSpace == COORDINATE_SPACE_HSV)
            transformFromRGBFunc = &rgb_to_hsv;
        else if(dstCoordinateSpace == COORDINATE_SPACE_HSL)
            transformFromRGBFunc = &rgb_to_hsl;
        else if(dstCoordinateSpace == COORDINATE_SPACE_TML)
            transformFromRGBFunc = &rgb_to_tml;
        
        while(true)
        {
            scanlineHelper.prepRGBAScanline(&rgbaBuffer, &numPixels);
            if(numPixels == 0) break;
            
            if(transformToRGBFunc)
            {
                float * bufLoc = rgbaBuffer;
                for(int pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                {
                    transformToRGBFunc(bufLoc, bufLoc);
                    bufLoc += 4;
                }
            }
            
            if(transformFromRGBFunc)
            {
                float * bufLoc = rgbaBuffer;
                for(int pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
                {
                    transformFromRGBFunc(bufLoc, bufLoc);
                    bufLoc += 4;
                }
            }
            
            scanlineHelper.finishRGBAScanline();
        }
    }
    */
}
OCS_NAMESPACE_EXIT
