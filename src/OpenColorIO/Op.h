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


#ifndef INCLUDED_OCIO_OP_H
#define INCLUDED_OCIO_OP_H

#include <OpenColorIO/OpenColorIO.h>

#include "Mutex.h"

#include <sstream>
#include <vector>

OCIO_NAMESPACE_ENTER
{
    struct AllocationData
    {
        Allocation allocation;
        std::vector<float> vars;
        
        AllocationData():
            allocation(ALLOCATION_UNIFORM)
            {};
        
        std::string getCacheID() const;
    };
    
    std::ostream& operator<< (std::ostream&, const AllocationData&);
    
    class OpData;
    typedef OCIO_SHARED_PTR<OpData> OpDataRcPtr;


    // The OpData class is a helper class to hold the data part of an Op 
    // with some basic behaviors (i.e. isNoop(), isIdentity() …). The Op class 
    // holds an OpData and offers high-level behaviors such as op's combinations, 
    // CPU processing and GPU code generator.
    // 
    // As the specialized Ops are private classes, operations between different 
    // Op types could not all be done. A 'Read-only' access to the data part of 
    // an Op allows to question its data, to cast to the specialized OpData 
    // (to have finer knowledge) and to apply any optimization rules.
    // 
    // For example, one optimization is to remove from the color transformation 
    // an identity Range (but still clamping) followed by any arbitrary LUT 1D 
    // except if the LUT is a half domain one (i.e. the input domain is 
    // all possible 16-bit floating-point values so there is not clamping). 
    // It means that the methods Range::canCombineWith() and Range::combineWith() 
    // have to question the LUT Op with a ‘not generic’ call (i.e. isInputHalfDomain()).
    // 
    // The design is to have a read-only access to the OpData of the Op, and
    // the ability to cast in any specialized OpData classes.
    // 
    // In contrast to several file format readers which could only read 1D 
    // and/or 3D LUT's, the CLF file format (i.e. Common LUT Format from ACES) 
    // read a list of arbitrary ops. As the specialized Ops are private classes, 
    // data must be stored in intermediate structures. The 1D and 3D LUT's 
    // already having such a structure, all the other Ops need a dedicated one. 
    // 
    // The design is to avoid code duplication by using the OpData for the Op 
    // and the Transform.
    // 
    class OpData
    {
    public:

        // Enumeration of all possible operator types
        enum Type
        {
            Lut1DType,        // A 1D LUT
            Lut3DType,        // A 3D LUT
            MatrixType,       // A matrix
            LogType,          // A log
            ExponentType,     // An exponent

            NoOpType
        };

    public:
        OpData(BitDepth inBitDepth, BitDepth outBitDepth);
        OpData(const OpData& rhs);
        OpData& operator=(const OpData& rhs);
        virtual ~OpData();

        inline BitDepth getInputBitDepth() const { return m_inBitDepth; }
        virtual void setInputBitDepth(BitDepth in);

        inline BitDepth getOutputBitDepth() const { return m_outBitDepth; }
        virtual void setOutputBitDepth(BitDepth out);

        virtual void validate() const;

        virtual Type getType() const = 0;

        // A "no-op" is an op where inBitDepth==outBitDepth 
        // and isIdentity is true, therefore the output pixels
        // will be unchanged.
        virtual bool isNoOp() const;

        // An identity is an op that only does bit-depth conversion
        // and/or clamping.
        // Each op will overload this with the appropriate calculation.
        // An op where isIdentity() is true will typically be removed
        // or replaced during the optimization process.
        virtual bool isIdentity() const = 0;

        // Determine whether the output of the op mixes R, G, B channels.
        // For example, Rout = 5*Rin is channel independent, but Rout = Rin + Gin
        // is not.  Note that the property may depend on the op parameters,
        // so, e.g. MatrixOps may sometimes return true and other times false.
        // returns true if the op's output does not combine input channels
        virtual bool hasChannelCrosstalk() const = 0;

        virtual bool operator==(const OpData& other) const;

        virtual std::string getCacheID() const;

    protected:           
        virtual std::string finalize() const = 0;

        mutable std::string m_cacheID;
        mutable Mutex m_mutex;

    private:
        BitDepth m_inBitDepth;
        BitDepth m_outBitDepth;
    };
    
    class Op;
    typedef OCIO_SHARED_PTR<Op> OpRcPtr;
    typedef std::vector<OpRcPtr> OpRcPtrVec;
    
    std::string SerializeOpVec(const OpRcPtrVec & ops, int indent=0);
    bool IsOpVecNoOp(const OpRcPtrVec & ops);
    
    void FinalizeOpVec(OpRcPtrVec & opVec, bool optimize=true);
    
    void OptimizeOpVec(OpRcPtrVec & result);
    
    class Op
    {
        public:
            virtual ~Op();
            
            virtual OpRcPtr clone() const = 0;
            
            // Something short, and printable.
            // The type of stuff you'd want to see in debugging.
            virtual std::string getInfo() const = 0;
            
            // This should yield a string of not unreasonable length.
            // It can only be called after finalize()
            virtual std::string getCacheID() const = 0;
            
            // Is the processing a noop? I.e, does apply do nothing.
            // (Even no-ops may define Allocation though.)
            // This must be implemented in a manner where its valid to
            // call *prior* to finalize. (Optimizers may make use of it)
            virtual bool isNoOp() const { return m_data->isNoOp(); }

            virtual bool isIdentity() const { return m_data->isIdentity(); }
            
            virtual bool isSameType(const OpRcPtr & op) const = 0;
            
            virtual bool isInverse(const OpRcPtr & op) const = 0;
            
            virtual bool canCombineWith(const OpRcPtr & op) const;
            
            // Return a vector of result ops, which correspond to
            // THIS combinedWith secondOp.
            //
            // If the result is a noOp, it is valid for the resulting opsVec
            // to be empty.
            
            virtual void combineWith(OpRcPtrVec & ops, const OpRcPtr & secondOp) const;
            
            virtual bool hasChannelCrosstalk() const { return m_data->hasChannelCrosstalk(); }
            
            virtual void dumpMetadata(ProcessorMetadataRcPtr & /*metadata*/) const
            { }
            
            // This is called a single time after construction.
            // Final pre-processing and safety checks should happen here,
            // rather than in the constructor.
            
            virtual void finalize() = 0;
            
            // Render the specified pixels.
            //
            // This must be safe to call in a multi-threaded context.
            // Ops that have mutable data internally, or rely on external
            // caching, must thus be appropriately mutexed.
            
            virtual void apply(float* rgbaBuffer, long numPixels) const = 0;
            
            
            // Is this op supported by the legacy shader text generator ?
            virtual bool supportedByLegacyShader() const { return true; }

            // Create & add the gpu shader information needed by the op
            virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const = 0;

            virtual BitDepth getInputBitDepth() const { return m_data->getInputBitDepth(); }
            virtual BitDepth getOutputBitDepth() const { return m_data->getOutputBitDepth(); }

            virtual void setInputBitDepth(BitDepth bitdepth) { m_data->setInputBitDepth(bitdepth); }
            virtual void setOutputBitDepth(BitDepth bitdepth) { m_data->setOutputBitDepth(bitdepth); }

            const OpDataRcPtr & data() const { return m_data; }

        protected:
            Op();
            OpDataRcPtr & data() { return m_data; }

        private:
            Op(const Op &);
            Op& operator= (const Op &);
            OpDataRcPtr m_data;
    };
    
    std::ostream& operator<< (std::ostream&, const Op&);
}
OCIO_NAMESPACE_EXIT

#endif
