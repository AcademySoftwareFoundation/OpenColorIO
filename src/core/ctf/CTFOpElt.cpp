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

#include "CTFOpElt.h"
#include "CTFReaderUtils.h"
#include "CTFReaderVersion.h"
#include "CTFMatrixElt.h"
#include "CTFLut1DElt.h"
#include "CTFInvLut1DElt.h"
#include "CTFLut3DElt.h"
#include "CTFInvLut3DElt.h"
#include "CTFRangeElt.h"
#include "CTFCDLElt.h"
#include "../Platform.h"
#include "../pystring/pystring.h"
#include <sstream>
#include <algorithm>

OCIO_NAMESPACE_ENTER
{

// Private namespace to the OpData sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{

OpElt::OpElt() : ContainerElt("", 0, "")
{
}

OpElt::~OpElt()
{
}

void OpElt::setContext(const std::string& name,
                       const TransformPtr& pTransform,
                       unsigned xmlLineNumber,
                       const std::string& xmlFile)
{
    Element::setContext(name, xmlLineNumber, xmlFile);

    m_transform = pTransform;

    if (!pTransform.get())
    {
        Throw("ProcessList tag missing.");
    }
}

const std::string& OpElt::getIdentifier() const
{
    return getOp()->getId();
}

void OpElt::appendDescription(const std::string& desc)
{
    getOp()->getDescriptions() += desc;
}

void OpElt::start(const char **atts)
{
    // Append by default
    m_transform->getOps().append(getOp());

    enum BitDepthFlags
    {
        NO_BIT_DEPTH = 0x00,
        INPUT_BIT_DEPTH = 0x01,
        OUTPUT_BIT_DEPTH = 0x02,
        ALL_BIT_DEPTH = (INPUT_BIT_DEPTH | OUTPUT_BIT_DEPTH)
    };

    unsigned bitDepthFound = NO_BIT_DEPTH;

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_ID, atts[i]))
        {
            getOp()->setId(atts[i + 1]);
        }
        else if (0 == Platform::Strcasecmp(ATTR_NAME, atts[i]))
        {
            getOp()->setName(atts[i + 1]);
        }
        else if (0 == Platform::Strcasecmp(ATTR_IN_BIT_DEPTH, atts[i]))
        {
            BitDepth bitdepth = getBitDepth(atts[i + 1]);
            if (bitdepth == BIT_DEPTH_UNKNOWN)
            {
                std::ostringstream os;
                os << "inBitDepth unknown value (";
                os << atts[i + 1] << ")";
                Throw(os.str());
            }
            getOp()->setInputBitDepth(bitdepth);
            bitDepthFound |= INPUT_BIT_DEPTH;
        }
        else if (0 == Platform::Strcasecmp(ATTR_OUT_BIT_DEPTH, atts[i]))
        {
            BitDepth bitdepth = getBitDepth(atts[i + 1]);
            if (bitdepth == BIT_DEPTH_UNKNOWN)
            {
                std::ostringstream os;
                os << "outBitDepth unknown value (";
                os << atts[i + 1] << ")";
                Throw(os.str());
            }
            getOp()->setOutputBitDepth(bitdepth);
            bitDepthFound |= OUTPUT_BIT_DEPTH;
        }
        /* TODO: CTF
        else if (0 == Platform::Strcasecmp(ATTR_BYPASS, atts[i]))
        {
            std::string s(atts[i + 1]);
            boost::algorithm::Trim(s);
            bool bypass = false;
            if (0 == Foundation::Utils::Strcasecmp(s.c_str(), "true"))
            {
                bypass = true;
            }
            else
            {
                XML_THROW1(ILLEGAL_BYPASS_VALUE, atts[i + 1]);
            }
            getOp()->getBypass()->setValue(bypass);
        }*/

        i += 2;
    }

    // Check mandatory attributes
    if ((bitDepthFound&INPUT_BIT_DEPTH) == NO_BIT_DEPTH)
    {
        Throw("inBitDepth is missing.");
    }
    else if ((bitDepthFound&OUTPUT_BIT_DEPTH) == NO_BIT_DEPTH)
    {
        Throw("outBitDepth is missing.");
    }
}

void OpElt::end()
{
}

const std::string& OpElt::getTypeName() const
{
    return getOp()->getOpTypeName();
}


//------------------------------------------------------------------------------
//
// These macros are used to define which Op implementation to use
// depending of the selected version. For now, only the Op reader
// could be versionned.
//


//
// Versioning of file formats is a topic that needs careful consideration.
// Any format will have one day to change some part of its structure in order
// to support new features. In our case, the Color Transform XML format
// will evolve to support new Ops and potentially extend some existing Ops.
//
// The two design decisions related to the versioning are that first,
// the XML Reader has to be fully backward compatible (it means to read
// any existing versions) and second, only the lastest version will be written
// to simplify the xml layer (e.g. refer to xmlTransformWriter files)
//
// The macros below provides a mechanism to support versioning at the Op level.
//
// At the design point of view, any new Op's version should be located
// in this file only; factory method already exists to handle the creation
// of the right Op reader instance based on the requested version.
// Please refer to OpElt::getReader() to see that the element reader
// is instanciated following the requested version.
// For example:
//  - ADD_READER_FOR_VERSIONS_UP_TO(MatrixElt, 1_2) imposes that
//    any xml versions lower or equal to 1.2 shall use the MatrixElt reader
//  - ADD_DEFAULT_READER(MatrixElt_1_3) imposes that any xml versions shall
//    use the MatrixElt_1_3 reader
//  - As these two macros are in the same scope, the 1.2 check will be
//    performed before the 1.3 one; hence, MatrixElt_1_3 is only used for
//    versions greater than 1.2
//


// Add a reader to all versions lower or equal to the specified one
// - READER is the reader to add
// - VERS is the reference version
// Note: refer to OpElt::getReader() to have a usage and
//       implicit variables used by the macro
#define ADD_READER_FOR_VERSIONS_UP_TO(READER, VERS)  \
  if(version <= CTF_PROCESS_LIST_VERSION_##VERS)    \
  { pOp = new READER; break; }


// Add a default reader for all existing versions
// - READER is the reader to add
// Note: refer to OpElt::getReader() to have a usage and
//       implicit variables used by the macro
//
#define ADD_DEFAULT_READER(READER)                  \
  if(version <= CTF_PROCESS_LIST_VERSION)           \
  { pOp = new CTF::Reader::READER; break; }

// Add a reader to all versions higher or equal to the specified one
//
// - READER is the reader to add
// - VERS is the reference version
//
// Note: refer to OpElt::getReader() to have a usage and
//       implicit variables used by the macro
//
#define ADD_READER_FOR_VERSIONS_STARTING_AT(READER, VERS) \
  if(CTF_PROCESS_LIST_VERSION_##VERS <= version &&        \
     version <= CTF_PROCESS_LIST_VERSION)                 \
  { pOp = new CTF::Reader::READER; break; }


// Add a reader for a range of versions
//
// - READER is the reader to add
// - VERS1 is the earliest version to add this reader for
// - VERS2 is the last version to add this reader for
//
// Note: refer to OpElt::getReader() to have a usage and
//       implicit variables used by the macro
//
#define ADD_READER_FOR_VERSIONS_BETWEEN(READER, VERS1, VERS2) \
  if(CTF_PROCESS_LIST_VERSION_##VERS1 <= version&&           \
     version <= CTF_PROCESS_LIST_VERSION_##VERS2)             \
  { pOp = new CTF::Reader::READER; break; }



OpElt* OpElt::getReader(OpData::OpData::OpType type, const Version & version)
{
    OpElt* pOp = 0x0;

    switch (type)
    {
    case OpData::OpData::MatrixType:
    {
        // If the version is 1.2 or less, then use MatrixElt
        ADD_READER_FOR_VERSIONS_UP_TO(MatrixElt, 1_2);
        // If the version is 1.3 or more, then use MatrixElt_1_3
        ADD_DEFAULT_READER(MatrixElt_1_3);
        break;
    }
    case OpData::OpData::Lut1DType:
    {
        ADD_READER_FOR_VERSIONS_UP_TO(Lut1DElt, 1_3);
        // Adding hue_adjust attribute
        ADD_READER_FOR_VERSIONS_UP_TO(Lut1DElt_1_4, 1_4);
        // Adding basic IndexMap element
        ADD_DEFAULT_READER(Lut1DElt_1_7);
        break;
    }
    case OpData::OpData::InvLut1DType:
    {
        ADD_READER_FOR_VERSIONS_STARTING_AT(InvLut1DElt, 1_3);
        break;
    }
    case OpData::OpData::Lut3DType:
    {
        ADD_READER_FOR_VERSIONS_UP_TO(Lut3DElt, 1_6);
        // Adding basic IndexMap element
        ADD_DEFAULT_READER(Lut3DElt_1_7);
        break;
    }
    case OpData::OpData::InvLut3DType:
    {
        ADD_READER_FOR_VERSIONS_STARTING_AT(InvLut3DElt, 1_6);
        break;
    }
    case OpData::OpData::RangeType:
    {
        ADD_READER_FOR_VERSIONS_UP_TO(RangeElt, 1_6);
        // Adding noClamp style
        ADD_DEFAULT_READER(RangeElt_1_7);
        break;
    }
    /* TODO: CTF
    case Color::Op::Reference:
    {
        ADD_READER_FOR_VERSIONS_UP_TO(ReferenceElt, 1_6);
        // Adding inverted attribute
        ADD_DEFAULT_READER(ReferenceElt_1_7);
        break;
    }
    case Color::Op::Gamma:
    {
        // If the version is 1.4 or less, then use GammaElt
        // This reader forces the alpha transformation to be the identity.
        ADD_READER_FOR_VERSIONS_UP_TO(GammaElt, 1_4);
        // If the version is 1.5 or more, then use GammaElt_1_5
        ADD_DEFAULT_READER(GammaElt_1_5);
        break;
    }
    case Color::Op::ExposureContrast:
    {
        ADD_DEFAULT_READER(ExposureContrastElt);
        break;
    }
    case Color::Op::Log:
    {
        ADD_READER_FOR_VERSIONS_STARTING_AT(LogElt, 1_3);
        break;
    }*/
    case OpData::OpData::CDLType:
    {
        ADD_READER_FOR_VERSIONS_BETWEEN(CDLElt, 1_3, 1_6);
        // Allowing CLF style names
        ADD_DEFAULT_READER(CDLElt_1_7);
        break;
    }
    /* TODO: CTF
    case Color::Op::Dither:
    {
        // The dither op will be added in version 1.5 of the transform format. However, we are 
        // setting 1.3 here to simplify Professional Service's delivery to Dreamworks based on
        // an older release of the software.
        ADD_READER_FOR_VERSIONS_STARTING_AT(DitherElt, 1_3);
        break;
    }
    case Color::Op::GamutMap:
    {
        ADD_READER_FOR_VERSIONS_STARTING_AT(GamutMapElt, 1_4);
        break;
    }
    case Color::Op::HueVector:
    {
        ADD_READER_FOR_VERSIONS_STARTING_AT(HueVectorElt, 1_4);
        break;
    }
    case OpData::OpData::ACESType:
    {
        ADD_READER_FOR_VERSIONS_STARTING_AT(ACESElt, 1_5);
        break;
    }
    case Color::Op::Function:
    {
        ADD_READER_FOR_VERSIONS_STARTING_AT(FunctionElt, 1_8);
        break;
    }*/
    }

    return pOp;
}

BitDepth OpElt::getBitDepth(const std::string & strBD)
{
    const std::string str = pystring::lower(strBD);
    if (str == "8i") return BIT_DEPTH_UINT8;
    else if (str == "10i") return BIT_DEPTH_UINT10;
    else if (str == "12i") return BIT_DEPTH_UINT12;
    else if (str == "16i") return BIT_DEPTH_UINT16;
    else if (str == "16f") return BIT_DEPTH_F16;
    else if (str == "32f") return BIT_DEPTH_F32;
    return BIT_DEPTH_UNKNOWN;
}

} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

