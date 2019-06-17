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

#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "DynamicProperty.h"
#include "Mutex.h"

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


    class OpCPU;
    typedef OCIO_SHARED_PTR<OpCPU> OpCPURcPtr;
    typedef OCIO_SHARED_PTR<const OpCPU> ConstOpCPURcPtr;
    typedef std::vector<OpCPURcPtr> OpCPURcPtrVec;
    typedef std::vector<ConstOpCPURcPtr> ConstOpCPURcPtrVec;


    // OpCPU is a helper class to define the CPU pixel processing method signature.
    // Ops may define several optimized renderers tailored to the needs of a given set 
    // of op parameters.
    // For example, in the Range op, if the parameters do not require clamping 
    // at the high end, a renderer that skips that clamp may be called.
    // The CPU renderer to use for a given op instance is decided during finalization.
    // 
    class OpCPU
    {
    public:
        OpCPU() {}
        virtual ~OpCPU() {}

        // All the Ops assume float pointers (i.e. always float bit depths) except 
        // the 1D LUT CPU Op where the finalization depends on input and output bit depths.
        virtual void apply(const void * inImg, void * outImg, long numPixels) const = 0;
    };

    class OpData;
    typedef OCIO_SHARED_PTR<OpData> OpDataRcPtr;
    typedef OCIO_SHARED_PTR<const OpData> ConstOpDataRcPtr;
    typedef std::vector<OpDataRcPtr> OpDataVec;

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

        // Enumeration of all possible operator types.
        enum Type
        {
            CDLType,           // A Color Decision List (aka CDL)
            ExponentType,      // An exponent
            ExposureContrastType, // An op for making interactive viewport adjustments
            FixedFunctionType, // A fixed function (i.e. where the style defines the behavior)
            GammaType,         // A gamma (i.e. enhancement of the Exponent)
            LogType,           // A log
            Lut1DType,         // A 1D LUT
            Lut3DType,         // A 3D LUT
            MatrixType,        // A matrix
            RangeType,         // A range
            ReferenceType,     // A reference to an external file

            // Note: Keep at end of list.
            NoOpType
        };

        class Descriptions
        {
            typedef std::vector<std::string> List;
            List m_descriptions;

        public:
            typedef typename List::size_type size_type;

            typedef typename List::reference reference;
            typedef typename List::const_reference const_reference;

            typedef typename List::iterator iterator;
            typedef typename List::const_iterator const_iterator;

            Descriptions() = default;

            Descriptions(const_reference str)
            {
                m_descriptions.push_back(str);
            }

            inline bool empty() const noexcept { return m_descriptions.empty(); }
            size_type size() const noexcept { return m_descriptions.size(); }

            Descriptions & operator+=(const_reference str)
            {
                m_descriptions.push_back(str);
                return *this;
            }

            Descriptions& operator +=(const Descriptions& rhs)
            {
                if (this != &rhs)
                {
                    m_descriptions.insert(m_descriptions.end(),
                                          rhs.m_descriptions.begin(),
                                          rhs.m_descriptions.end());
                }
                return *this;
            }

            bool operator==(const Descriptions & other) const noexcept
            {
                if (this == &other) return true;
                return (m_descriptions == other.m_descriptions);
            }

            bool operator!=(const Descriptions & other) const noexcept
            {
                return !(*this==other);
            }

            iterator begin() noexcept { return m_descriptions.begin(); }
            iterator end() noexcept { return m_descriptions.end(); }
            const_iterator begin() const noexcept { return m_descriptions.begin(); }
            const_iterator end() const noexcept { return m_descriptions.end(); }

            reference operator[] (size_type n) { return m_descriptions[n]; }
            const_reference operator[] (size_type n) const { return m_descriptions[n]; }
        };

    public:
        OpData(BitDepth inBitDepth, BitDepth outBitDepth);
        OpData(BitDepth inBitDepth, BitDepth outBitDepth,
               const std::string & id, 
               const Descriptions & desc);
        OpData(const OpData& rhs);
        OpData& operator=(const OpData& rhs);
        virtual ~OpData();

        inline const std::string & getID() const { return m_id; }
        void setID(const std::string & id) { m_id = id; }

        inline const std::string& getName() const { return m_name; }
        void setName(const std::string& name) { m_name = name; }

        inline const Descriptions & getDescriptions() const { return m_descriptions; }
        inline Descriptions & getDescriptions() { return m_descriptions; }
        void setDescriptions(const Descriptions & desc) { m_descriptions = desc; }

        inline BitDepth getInputBitDepth() const { return m_inBitDepth; }
        virtual void setInputBitDepth(BitDepth in) { m_inBitDepth = in; }

        inline BitDepth getOutputBitDepth() const { return m_outBitDepth; }
        virtual void setOutputBitDepth(BitDepth out) { m_outBitDepth = out; }

        virtual void validate() const;

        virtual Type getType() const = 0;

        // A "no-op" is an op where inBitDepth==outBitDepth 
        // and isIdentity is true, therefore the output pixels
        // will be unchanged.
        virtual bool isNoOp() const = 0;

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

        virtual bool operator==(const OpData & other) const;
        bool operator!=(const OpData & other) const = delete;

        virtual void finalize() = 0;

        // OpData::finalize() updates the cache identitifer,
        // and Op::finalize() consumes it to compute the Op cache identifier.
        virtual std::string getCacheID() const { return m_cacheID; }

    protected:
        mutable Mutex m_mutex;
        mutable std::string m_cacheID;

    private:
        std::string  m_id;
        std::string  m_name;
        Descriptions m_descriptions;
        BitDepth     m_inBitDepth;
        BitDepth     m_outBitDepth;
    };
    
    class Op;
    typedef OCIO_SHARED_PTR<Op> OpRcPtr;
    typedef OCIO_SHARED_PTR<const Op> ConstOpRcPtr;
    class OpRcPtrVec;
    
    std::string SerializeOpVec(const OpRcPtrVec & ops, int indent=0);
    bool IsOpVecNoOp(const OpRcPtrVec & ops);
    
    void FinalizeOpVec(OpRcPtrVec & opVec, bool optimize=true);

    void OptimizeOpVec(OpRcPtrVec & result);
    
    void CreateOpVecFromOpData(OpRcPtrVec & ops,
                               const OpDataRcPtr & opData,
                               TransformDirection dir);

    void CreateOpVecFromOpDataVec(OpRcPtrVec & ops,
                                  const OpDataVec & opDataVec,
                                  TransformDirection dir);

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
            virtual std::string getCacheID() const { return m_cacheID; }

            virtual bool isNoOpType() const { return m_data->getType() == OpData::NoOpType; }

            // Is the processing a noop? I.e, does apply do nothing.
            // (Even no-ops may define Allocation though.)
            // This must be implemented in a manner where its valid to
            // call *prior* to finalize. (Optimizers may make use of it)
            virtual bool isNoOp() const { return m_data->isNoOp(); }

            virtual bool isIdentity() const { return m_data->isIdentity(); }
            
            virtual bool isSameType(ConstOpRcPtr & op) const = 0;
            
            virtual bool isInverse(ConstOpRcPtr & op) const = 0;
            
            virtual bool canCombineWith(ConstOpRcPtr & op) const;
            
            // Return a vector of result ops, which correspond to
            // THIS combinedWith secondOp.
            //
            // If the result is a noOp, it is valid for the resulting opsVec
            // to be empty.
            
            virtual void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const;
            
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

            virtual void apply(void * img, long numPixels) const
            { m_cpuOp->apply(img, img, numPixels); }

            virtual void apply(const void * inImg, void * outImg, long numPixels) const
            { m_cpuOp->apply(inImg, outImg, numPixels); }

            
            // Is this op supported by the legacy shader text generator ?
            virtual bool supportedByLegacyShader() const { return true; }

            // Create & add the gpu shader information needed by the op
            virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const = 0;

            virtual BitDepth getInputBitDepth() const { return m_data->getInputBitDepth(); }
            virtual BitDepth getOutputBitDepth() const { return m_data->getOutputBitDepth(); }

            virtual void setInputBitDepth(BitDepth bitdepth) { m_data->setInputBitDepth(bitdepth); }
            virtual void setOutputBitDepth(BitDepth bitdepth) { m_data->setOutputBitDepth(bitdepth); }

            virtual bool hasDynamicProperty(DynamicPropertyType type) const;
            virtual DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;
            virtual void replaceDynamicProperty(DynamicPropertyType type,
                                                DynamicPropertyImplRcPtr prop);

            ConstOpCPURcPtr getCPUOp() const { return m_cpuOp; }

            ConstOpDataRcPtr data() const { return std::const_pointer_cast<const OpData>(m_data); }

        protected:
            Op();
            OpDataRcPtr & data() { return m_data; }

            std::string m_cacheID;

            // This holds a CPU renderer for the specific op that is specialized 
            // to the actual parameters being used.
            OpCPURcPtr  m_cpuOp;

        private:
            Op(const Op &) = delete;
            Op& operator= (const Op &) = delete;

            // The OpData instance holds the parameters (LUT values, matrix coefs, etc.) being used.
            OpDataRcPtr m_data;
    };
    
    std::ostream& operator<< (std::ostream&, const Op&);

    // The class handles a list of ops and enforces bit depth consistency between ops.
    //
    // Note: List only manages shared pointers i.e. it never clones ops.
    class OpRcPtrVec
    {
        typedef std::vector<OpRcPtr> Type;
        Type m_ops;

    public:
        typedef Type::value_type value_type;
        typedef Type::size_type size_type;

        typedef Type::iterator iterator;
        typedef Type::const_iterator const_iterator;

        typedef Type::reference reference;
        typedef Type::const_reference const_reference;

        OpRcPtrVec() {}
        OpRcPtrVec(const OpRcPtrVec & v);
        OpRcPtrVec & operator=(const OpRcPtrVec & v);
        // Note: It copies elements i.e. no clone.
        OpRcPtrVec & operator+=(const OpRcPtrVec & v);

        size_type size() const { return m_ops.size(); }

        iterator begin() noexcept { return m_ops.begin(); }
        const_iterator begin() const noexcept { return m_ops.begin(); }
        iterator end() noexcept { return m_ops.end(); }
        const_iterator end() const noexcept { return m_ops.end(); }

        const OpRcPtr & operator[](size_type idx) const { return m_ops[idx]; }

        iterator erase(const_iterator position);       
        iterator erase(const_iterator first, const_iterator last);

        // Insert at the 'position' the elements from the range ['first', 'last'[ 
        // respecting the element's order. Inserting elements at a given position 
        // shifts elements starting at 'position' to the right.
        // 
        // Note: Inserting an empty range will do nothing and inserting 
        // in an empty list appends elements from the range ['first', 'last'[.
        //
        // Note: It copies elements i.e. no clone.
        void insert(const_iterator position, const_iterator first, const_iterator last);

        void clear() noexcept { m_ops.clear(); }
        bool empty() const noexcept { return m_ops.empty(); }

        void push_back(const value_type & val);
        const_reference back() const;

        // Validate the bit depth consistency between Ops.
        void validate() const;

        OpRcPtrVec clone() const;

    protected:
        void adjustBitDepths();
    };

}
OCIO_NAMESPACE_EXIT

#endif
