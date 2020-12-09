// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_OP_H
#define INCLUDED_OCIO_OP_H

#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "DynamicProperty.h"
#include "fileformats/FormatMetadata.h"
#include "Mutex.h"

namespace OCIO_NAMESPACE
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

    virtual bool hasDynamicProperty(DynamicPropertyType type) const;
    virtual DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;
};

class OpData;
typedef OCIO_SHARED_PTR<OpData> OpDataRcPtr;
typedef OCIO_SHARED_PTR<const OpData> ConstOpDataRcPtr;
typedef std::vector<OpDataRcPtr> OpDataVec;
typedef std::vector<ConstOpDataRcPtr> ConstOpDataVec;

class Op;
typedef OCIO_SHARED_PTR<Op> OpRcPtr;
typedef OCIO_SHARED_PTR<const Op> ConstOpRcPtr;
class OpRcPtrVec;

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
        CDLType,              // A Color Decision List (aka CDL)
        ExponentType,         // An exponent
        ExposureContrastType, // An op for making interactive viewport adjustments
        FixedFunctionType,    // A fixed function (i.e. where the style defines the behavior)
        GammaType,            // A gamma (i.e. enhancement of the Exponent)
        GradingPrimaryType,   // A set of primary grading controls
        GradingRGBCurveType,  // A rgb curve
        GradingToneType,      // A set of grading controls for tonal ranges
        LogType,              // A log
        Lut1DType,            // A 1D LUT
        Lut3DType,            // A 3D LUT
        MatrixType,           // A matrix
        RangeType,            // A range
        ReferenceType,        // A reference to an external file

        // Note: Keep at end of list.
        NoOpType
    };

public:
    OpData();
    OpData(const OpData & rhs);
    OpData & operator=(const OpData & rhs);
    virtual ~OpData();

    const std::string & getID() const;
    void setID(const std::string & id);

    const std::string & getName() const;
    void setName(const std::string & name);

    virtual void validate() const = 0;

    virtual Type getType() const = 0;

    // A "no-op" is an op that will not change the output pixels.
    virtual bool isNoOp() const = 0;

    // An Identity does not modify pixels in its intended domain but may clamp outside that.
    // For example, a Lut1D may be an Identity without being a NoOp.
    virtual bool isIdentity() const = 0;

    virtual OpDataRcPtr getIdentityReplacement() const;

    // At optimization step, ops might get replaced by simpler ops depending on the op parameters.
    virtual void getSimplerReplacement(OpDataVec & ops) const;

    // Determine whether the output of the op mixes R, G, B channels.
    // For example, Rout = 5*Rin is channel independent, but Rout = Rin + Gin
    // is not.  Note that the property may depend on the op parameters,
    // so, e.g. MatrixOps may sometimes return true and other times false.
    // returns true if the op's output does not combine input channels
    virtual bool hasChannelCrosstalk() const = 0;

    virtual bool operator==(const OpData & other) const;
    bool operator!=(const OpData & other) const = delete;

    // This should yield a string of not unreasonable length.
    virtual std::string getCacheID() const = 0;

    // FormatMetadata.
    FormatMetadataImpl & getFormatMetadata() { return m_metadata;  }
    const FormatMetadataImpl & getFormatMetadata() const { return m_metadata; }

protected:
    mutable Mutex m_mutex;

private:
    FormatMetadataImpl m_metadata;
};

const char * GetTypeName(OpData::Type type);

class Op
{
public:
    virtual ~Op();

    virtual OpRcPtr clone() const = 0;

    // Something short, and printable.
    // The type of stuff you'd want to see in debugging.
    virtual std::string getInfo() const = 0;

    virtual bool isNoOpType() const { return m_data->getType() == OpData::NoOpType; }

    // Is the processing a noop? I.e, does apply do nothing.
    // (Even no-ops may define Allocation though.)
    // This must be implemented in a manner where its valid to
    // call *prior* to optimization.
    virtual bool isNoOp() const { return m_data->isNoOp(); }

    virtual bool isIdentity() const { return m_data->isIdentity(); }

    OpRcPtr getIdentityReplacement() const;
    void getSimplerReplacement(OpRcPtrVec & ops) const;

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

    void validate() const;

    // Prepare op for optimization and apply.
    virtual void finalize() { }

    // This should yield a string of not unreasonable length.
    virtual std::string getCacheID() const = 0;

    // Render the specified pixels.
    //
    // This must be safe to call in a multi-threaded context. Ops that have mutable data
    // internally, or rely on external caching, must thus be appropriately mutexed.
    //
    // Note: These apply calls are intended for unit test usage rather than general purpose
    // use and so it is ok to hard-code the fastLogExpPow to false.

    virtual void apply(void * img, long numPixels) const
    { getCPUOp(false)->apply(img, img, numPixels); }

    virtual void apply(const void * inImg, void * outImg, long numPixels) const
    { getCPUOp(false)->apply(inImg, outImg, numPixels); }


    // Is this op supported by the legacy shader text generator?
    virtual bool supportedByLegacyShader() const { return true; }

    // Create & add the gpu shader information needed by the op. Op has to be finalized.
    virtual void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const = 0;

    virtual bool isDynamic() const;
    virtual bool hasDynamicProperty(DynamicPropertyType type) const;
    virtual DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;
    virtual void replaceDynamicProperty(DynamicPropertyType type,
                                        DynamicPropertyDoubleImplRcPtr & prop)
    {
        throw Exception("Op does not implement double dynamic property.");
    }
    virtual void replaceDynamicProperty(DynamicPropertyType type,
                                        DynamicPropertyGradingPrimaryImplRcPtr & prop)
    {
        throw Exception("Op does not implement grading primary dynamic property.");
    }
    virtual void replaceDynamicProperty(DynamicPropertyType type,
                                        DynamicPropertyGradingRGBCurveImplRcPtr & prop)
    {
        throw Exception("Op does not implement grading rgb curve dynamic property.");
    }
    virtual void replaceDynamicProperty(DynamicPropertyType type,
                                        DynamicPropertyGradingToneImplRcPtr & prop)
    {
        throw Exception("Op does not implement grading tone dynamic property.");
    }

    // Make dynamic properties non-dynamic.
    virtual void removeDynamicProperties() {}

    // On-demand creation of the OpCPU instance. Op has to be finalized.
    virtual ConstOpCPURcPtr getCPUOp(bool fastLogExpPow) const = 0;

    ConstOpDataRcPtr data() const { return std::const_pointer_cast<const OpData>(m_data); }

protected:
    Op();
    OpDataRcPtr & data() { return m_data; }

private:
    Op(const Op &) = delete;
    Op& operator= (const Op &) = delete;

    // The OpData instance holds the parameters (LUT values, matrix coefs, etc.) being used.
    OpDataRcPtr m_data;
};

std::ostream& operator<< (std::ostream&, const Op&);

// The class handles a list of ops.
//
// Note: The class follows the std::vector API so it can be used
// by any STL algorithms.
//
// Note: List only manages shared pointers i.e. it never clones ops.
class OpRcPtrVec
{
    typedef std::vector<OpRcPtr> Type;
    Type m_ops;
    FormatMetadataImpl m_metadata;

public:
    typedef Type::value_type value_type;
    typedef Type::size_type size_type;

    typedef Type::iterator iterator;
    typedef Type::const_iterator const_iterator;

    typedef Type::reverse_iterator reverse_iterator;
    typedef Type::const_reverse_iterator const_reverse_iterator;

    typedef Type::reference reference;
    typedef Type::const_reference const_reference;

    OpRcPtrVec();
    ~OpRcPtrVec() {}

    OpRcPtrVec(const OpRcPtrVec & v);
    OpRcPtrVec & operator=(const OpRcPtrVec & v);
    // Note: It copies elements i.e. no clone.
    OpRcPtrVec & operator+=(const OpRcPtrVec & v);

    size_type size() const { return m_ops.size(); }

    iterator begin() noexcept { return m_ops.begin(); }
    const_iterator begin() const noexcept { return m_ops.begin(); }
    iterator end() noexcept { return m_ops.end(); }
    const_iterator end() const noexcept { return m_ops.end(); }

    reverse_iterator rbegin() noexcept { return m_ops.rbegin(); }
    const_reverse_iterator rbegin() const noexcept { return m_ops.rbegin(); }
    reverse_iterator rend() noexcept { return m_ops.rend(); }
    const_reverse_iterator rend() const noexcept { return m_ops.rend(); }

    const OpRcPtr & operator[](size_type idx) const { return m_ops[idx]; }
    OpRcPtr & operator[](size_type idx) { return m_ops[idx]; }

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
    const_reference front() const;

    // The following methods provide helpers for basic Op behaviors.

    FormatMetadataImpl & getFormatMetadata() { return m_metadata; }
    const FormatMetadataImpl & getFormatMetadata() const { return m_metadata; }

    bool isNoOp() const noexcept;
    bool hasChannelCrosstalk() const noexcept;

    bool isDynamic() const noexcept;
    bool hasDynamicProperty(DynamicPropertyType type) const noexcept;
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const;
    void validateDynamicProperties();

    OpRcPtrVec clone() const;

    // Note: The elements are cloned.
    OpRcPtrVec invert() const;

    void validate() const;

    // This calls validate and finalize for each op, then performs optimization. Ops resulting
    // from the optimization are finalized. The optimization step in the finalization could create
    // new Ops but they are finalized by default. For instance combining two matrices will only create
    // a fwd matrix as the inv matrices were already inverted (i.e. no inv matrices are present in the
    // OpVec when reaching the optimization step).
    void finalize(OptimizationFlags oFlags);

    // Only OptimizationFlags related to bitdepth optimization are used.
    void optimizeForBitdepth(const BitDepth & inBitDepth,
                             const BitDepth & outBitDepth,
                             OptimizationFlags oFlags);

};

std::string SerializeOpVec(const OpRcPtrVec & ops, int indent=0);

void CreateOpVecFromOpData(OpRcPtrVec & ops,
                            const ConstOpDataRcPtr & opData,
                            TransformDirection dir);

inline bool HasFlag(OptimizationFlags flags, OptimizationFlags queryFlag)
{
    return (flags & queryFlag) == queryFlag;
}

} // namespace OCIO_NAMESPACE

#endif
