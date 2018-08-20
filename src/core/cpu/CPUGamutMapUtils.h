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


#ifndef INCLUDED_OCIO_CPU_GAMUTMAPUTILS
#define INCLUDED_OCIO_CPU_GAMUTMAPUTILS


#include <OpenColorIO/OpenColorIO.h>


OCIO_NAMESPACE_ENTER
{
    namespace GamutMapUtils
    {
        // Compute the indices for the smallest, middle, and largest elements of RGB[3].
        // (Trying to be clever and do this without branching.)
        //
        inline void Order3(const float* RGB, int& min, int& mid, int& max)
        {
            //                             0  1  2  3  4  5  6  7  8  (typical val - 3)
            const int table[] = { 2, 1, 0, 2, 1, 0, 2, 1, 2, 0, 1, 2 };

            int val = ( int(RGB[0] > RGB[1]) * 5 + int(RGB[1] > RGB[2]) * 4 )
                      - int(RGB[0] > RGB[2]) * 3 + 3;

            // A NaN in a logical comparison always results in False.
            // So the case to be careful of here is { A, NaN, B } with A > B.
            // In that case, the first two compares are false but the third is true
            // (something that would never happen with regular numbers).
            // So that is the reason for the +3, to make val 0 in that case.

            max = table[  val];
            mid = table[++val];
            min = table[++val];
        }
    };
}
OCIO_NAMESPACE_EXIT

#endif
