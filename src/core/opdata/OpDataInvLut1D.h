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

// This class facilitates inversion of a 1d LUT.
//
// The class design inherits from the Lut1DOp class. The LUT data stored is the
// same as the forward LUT however it may be adjusted if necessary to be either
// strictly non-decreasing or non-increasing (i.e. flat spots are allowed but
// not reversals).
//
// Please be aware that although the in/out bit-depths are reversed from the
// forward LUT, the scaling of the LUT entries is not changed.

#ifndef INCLUDED_OCIO_OPDATA_INVLUT1D_H
#define INCLUDED_OCIO_OPDATA_INVLUT1D_H

#include <OpenColorIO/OpenColorIO.h>

#include "OpDataLut1D.h"


OCIO_NAMESPACE_ENTER
{
    // Private namespace to the OpData sub-directory
    namespace OpData
    {
        class InvLut1D;
        typedef OCIO_SHARED_PTR<InvLut1D> OpDataInvLut1DRcPtr;

        // The class represents an invert 1D LUT process node
        class InvLut1D : public Lut1D
        {
        public:
            // Enumeration of the inverse lut1d styles.
            enum InvStyle
            {
                EXACT = 0,  // Exact, but slow, inverse processing
                FAST        // Fast, but approximate, inverse processing
            };

            // Holds the properties of a color component
            struct ComponentProperties
            {
                ComponentProperties() 
                    : isIncreasing(false)
                    , startDomain(0)
                    , endDomain(0)
                    , negStartDomain(0)
                    , negEndDomain(0) {}

                bool isIncreasing;       // represents the overall increasing state
                unsigned startDomain;    // is the lowest index such that LUT[start] != LUT[start+1].
                unsigned endDomain;      // is the highest index such that LUT[end-1] != LUT[end].
                unsigned negStartDomain; // startDomain for half-domain negative values
                unsigned negEndDomain;   // endDomain for half-domain negative values
            };

            // Get InvStyle from its name
            // - str is the string representation of the style name
            // Return the enumeration value.
            static InvStyle GetInvStyle(const char * str);

            // Get the string name for the enumeration
            // - invStyle is the enum value
            // Return the string representation
            static const char * GetInvStyleName(InvStyle invStyle);

            // Constructor
            InvLut1D();

            InvLut1D(BitDepth inBitDepth, BitDepth outBitDepth, HalfFlags halfFlags);

            // Constructor
            //  - fwdLut1DOp is the original 1D LUT forward operation
            InvLut1D(const Lut1D & fwdLut1D);

            // Used by XML reader.  After initializing members as a Lut1D,
            // this method is used to finish creating an InvLut1D.
            void initializeFromLut1D();

            // Destructor
            virtual ~InvLut1D();

            inline OpType getOpType() const { return OpData::InvLut1DType; }

            const std::string& getOpTypeName() const;

            OpData * clone(CloneType type) const;

            virtual void inverse(OpDataVec & ops) const;

            inline InvStyle getInvStyle() const { return m_invStyle; }

            void setInvStyle(InvStyle style);

            void setInputBitDepth(BitDepth in);

            void setOutputBitDepth(BitDepth out);

            // Get properties of the red component
            inline const ComponentProperties & getRedProperties() const
            {
                return m_componentProperties[0];
            }

            // Get properties of the green component
            inline const ComponentProperties & getGreenProperties() const
            {
                return m_componentProperties[1];
            }

            // Get properties of the blue component
            inline const ComponentProperties & getBlueProperties() const
            {
                return m_componentProperties[2];
            }

            // Determine if the inverse LUT needs to handle values outside
            // the normal domain: e.g. [0,1023] for 10i or [0.,1.] for 16f.
            // (This is true if the forward LUT had an extended range.)
            // Return true if the Op has an extended domain
            bool hasExtendedDomain() const;

            inline BitDepth getOriginalInDepth() const { return m_origInDepth; }

        private:
            // Make the array monotonic and prepare params for the renderer.
            void prepareArray();

        private:
            // Inverse computation style
            InvStyle m_invStyle;

            // Properties of each color components
            ComponentProperties m_componentProperties[3];

            // The original input bit-depth when the object was created.
            // Note: This is hopefully only needed temporarily.
            //       Used in InvLutUtil::makeFastLut1D.
            BitDepth m_origInDepth;
        };
    }
}
OCIO_NAMESPACE_EXIT


#endif
