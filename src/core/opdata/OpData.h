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


#ifndef INCLUDED_OCIO_OPDATAOPDATA_H
#define INCLUDED_OCIO_OPDATAOPDATA_H

#include <OpenColorIO/OpenColorIO.h>

#include <string>
#include "OpDataDescriptions.h"
#include "../ctf/CTFReaderVersion.h"

OCIO_NAMESPACE_ENTER
{
    // Private namespace to the OpData sub-directory
    namespace OpData
    {
        class OpData;
        typedef OCIO_SHARED_PTR<OpData> OpDataRcPtr;

        class OpDataVec;


        // Base class for all types of operation nodes.
        // It refers to the ProcessNode in the AMPAS common LUT format
        class OpData
        {
        public:

            // Enumeration of all possible operation types
            enum OpType
            {
                // TODO: enable other types for CTF support
                Lut1DType,         // A 1D LUT
                InvLut1DType,      // A 1D LUT Inverse
                Lut3DType,         // A 3D LUT
                InvLut3DType,      // A 3D LUT Inverse
                MatrixType,        // A matrix
                RangeType,         // A range
                //Reference,       // A reference
                //Gamma,           // A parametric gamma curve
                //ExposureContrast,// Dynamic E/C controls
                //Log,             // A parametric logarithm curve
                CDLType,           // An ASC Color Decision List
                //Dither,          // An ordered dither
                //GamutMap,        // A gamut mapping algorithm
                //HueVector,       // A hue-based selective color adjustment
                //ACES_Type,       // Algorithm components specific to ACES implementation
                //Function         // Miscellaneous color functions
            };

            // Enumerator of different clone behavior
            enum CloneType
            {
                DO_SHALLOW_COPY = 0, // Preserve existing Ops and share dynamic properties    
                DO_DEEP_COPY         // Preserve existing Ops while cloning dynamic properties
            };


        public:
            // Constructor
            OpData(BitDepth inBitDepth, BitDepth outBitDepth);

            // Constructor
            // - inBitDepth Requested bit depth for the input
            // - outBitDepth Requested bit depth for the output
            // - id Identifier
            // - name Human readable identifier
            // - description Human readable descriptions
            OpData(BitDepth inBitDepth,            
                   BitDepth outBitDepth,           
                   const std::string& id,          
                   const std::string& name,        
                   const Descriptions& descriptions
            );

            // Copy constructor
            // - rhs is the Op to copy
            OpData(const OpData& rhs);

            // Assignation operator
            OpData& operator=(const OpData& rhs);

            // Destructor
            virtual ~OpData();

            // Clone the instance
            // - type is the type of clone to be used
            // returns the new instance
            virtual OpData* clone(CloneType type) const = 0;

            // Get the identifier
            // returns the identifier
            inline const std::string& getId() const { return m_id; }

            // Set the identifier
            void setId(const std::string& id);

            // Get the name
            inline const std::string& getName() const { return m_name; }

            // Set the name
            void setName(const std::string& name);

            // Get the bit depth of the input
            inline BitDepth getInputBitDepth() const { return m_inBitDepth; }

            // Set the input bit depth
            virtual void setInputBitDepth(BitDepth in);

            // Get the bit depth of the output
            inline BitDepth getOutputBitDepth() const { return m_outBitDepth; }

            // Set the output bit depth
            virtual void setOutputBitDepth(BitDepth out);

            // Get the descriptions
            inline const Descriptions& getDescriptions() const { return m_descriptions; }

            // Get the descriptions
            inline Descriptions& getDescriptions() { return m_descriptions; }

            // Validate the state of the instance
            virtual void validate() const;

            // Get the operation type
            virtual OpType getOpType() const = 0;

            // Get the operation type name
            virtual const std::string& getOpTypeName() const = 0;

            // A "no-op" is an op where inBitDepth==outBitDepth and isIdentity
            // is true, and isClamping is false, therefore the output pixels
            // will be unchanged.
            virtual bool isNoOp() const;

            // An identity is an op that only does bit-depth conversion
            // and/or clamping.
            // Each op will overload this with the appropriate calculation.
            // An op where isIdentity() is true will typically be removed
            // or replaced during the optimization process.
            virtual bool isIdentity() const = 0;

            // TODO: isClamping has been added to avoid clamping ops to be optimized.
            //       to rework when doing optimization.
            // Values outside a range are clamped.
            virtual bool isClamping() const = 0;

            // Make an op to replace an identity (or pair identity) of this op type.
            // (Note: For a pair identity, call this on the first half and then set
            // the result's output bit-depth to match the second half.)
            // returns the opData (to be managed by caller)
            virtual std::auto_ptr<OpData> getIdentityReplacement() const = 0;

            // Determine whether the output of the op mixes R, G, B channels.
            // For example, Rout = 5*Rin is channel independent, but Rout = Rin + Gin
            // is not.  Note that the property may depend on the op parameters,
            // so, e.g. MatrixOps may sometimes return true and other times false.
            // returns true if the op's output does not combine input channels
            virtual bool hasChannelCrosstalk() const = 0;

            // Get an identifier based
            // As the name and the identifier attributes are not mandatory,
            // the identification could be null. To avoid this issue,
            // this method will get:
            // 1/ the name if not null
            // 2/ the identifier if not null
            // 3/ the op type (which is never null)
            // return a meaningful identifier
            const std::string& getMeaningfullIdentifier() const;

            /* TODO: DynamicProperty and BypassProperty will be added later
            // Returns whether the op dynamic.
            // returns true if the op contains dynamic properties.
            virtual bool isDynamic() const;

            // Checks whether a dynamic property is handled by this op.
            // - id is the identifier of the dynamic property to validate
            // returns true if the op supports the dynamic property
            virtual bool hasDynamicProperty(DynamicProperty::Ids id) const;

            // Set the value of a dynamic property.
            // - prop is the Dynamic Property value to set
            // throws if the property is not used by the op.
            // The caller should check whether the property is available
            // by calling hasDynamicProperty().
            virtual void setDynamicProperty(const DynamicProperty& prop);

            // Get the dynamic property.
            // - prop is the dynamic property to get.
            // throws if the property is not used by the op.
            // The caller should check whether the property is available
            // by calling hasDynamicProperty().
            virtual void getDynamicProperty(SYNCOLOR::DynamicProperty& prop) const;

            // Compare the bypass (on/off) and dynamic (yes/no) state of two ops.
            // - other is the other Op to compare with.
            // returns true if they are equal, false otherwise
            virtual bool hasMatchingBypass(const Op& other) const;

            // Set the bypass dynamic property
            // - bypass is the instance to set
            void setBypass(const BypassPropertyPtr& bypass);

            // Get the Bypass Dynamic Property
            const BypassPropertyPtr& getBypass() const { return _bypass; }

            // Check if this op is currently bypassed.
            bool isBypassed() const { return _bypass->getValue(); }
            */

            // Check if an Op is equal to this instance.
            // - other is the other Op to compare with.
            // returns true if they are equal, false otherwise
            virtual bool operator==(const OpData& other) const;

            // Get the corresponding inverse operations
            // - ops an opList to which are appended the ops (may be >1) 
            //   needed to invert the op instance
            virtual void inverse(OpDataVec& ops) const = 0;

            // TODO: Could lower this further once we overload this in all ops.
            // Get the minimum XML version required for this op.
            // returns the version number
            virtual const CTF::Version & getMinimumVersion() const
            {
                return CTF::CTF_PROCESS_LIST_VERSION_1_3; 
            }

        protected:
            std::string  m_id;            // Human readable Identifier
            std::string  m_name;          // Name
            BitDepth     m_inBitDepth;    // Bit depth of the input
            BitDepth     m_outBitDepth;   // Bit depth of the output
            Descriptions m_descriptions;  // Descriptions
            
            // TODO: BypassProperty will be added later
            // BypassPropertyPtr _bypass;   // The Bypass Dynamic Property


        };
    } // exit OpData namespace
}
OCIO_NAMESPACE_EXIT

#endif
