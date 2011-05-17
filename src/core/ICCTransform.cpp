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

//#include <iostream>

#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"
#include "ICCOp.h"

OCIO_NAMESPACE_ENTER
{
    
    ICCTransformRcPtr ICCTransform::Create()
    {
        return ICCTransformRcPtr(new ICCTransform(), &deleter);
    }
    
    void ICCTransform::deleter(ICCTransform* t)
    {
        delete t;
    }
    
    class ICCTransform::Impl
    {
    public:
        TransformDirection dir_;
        std::string input_;
        std::string output_;
        std::string proof_;
        IccIntent intent_;
        bool blackpointCompensation_;
        bool softProofing_;
        bool gamutCheck_;
        
        Impl() : dir_(TRANSFORM_DIR_FORWARD)
        { }
        
        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            dir_ = rhs.dir_;
            input_ = rhs.input_;
            output_ = rhs.output_;
            proof_ = rhs.proof_;
            intent_ = rhs.intent_;
            blackpointCompensation_ = rhs.blackpointCompensation_;
            softProofing_ = rhs.softProofing_;
            gamutCheck_ = rhs.gamutCheck_;
            return *this;
        }
    };
    
    ///////////////////////////////////////////////////////////////////////////
    
    ICCTransform::ICCTransform()
        : m_impl(new ICCTransform::Impl)
    {
        getImpl()->input_ = "";
        getImpl()->output_ = "";
        getImpl()->proof_ = "";
        getImpl()->intent_ = ICC_INTENT_UNKNOWN;
        getImpl()->blackpointCompensation_ = false;
        getImpl()->softProofing_ = false;
        getImpl()->gamutCheck_ = false;
    }
    
    TransformRcPtr ICCTransform::createEditableCopy() const
    {
        ICCTransformRcPtr transform = ICCTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    ICCTransform::~ICCTransform()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    ICCTransform& ICCTransform::operator= (const ICCTransform & rhs)
    {
        *m_impl = *rhs.m_impl;
        return *this;
    }
    
    TransformDirection ICCTransform::getDirection() const
    {
        return getImpl()->dir_;
    }
    
    void ICCTransform::setDirection(TransformDirection dir)
    {
        getImpl()->dir_ = dir;
    }
    
    void ICCTransform::setInput(const char * input)
    {
        getImpl()->input_ = input;
    }
    
    void ICCTransform::setInputMem(const void * /*inputPtr*/, int /*size*/)
    {
        // not yet impl
    }
    
    const char * ICCTransform::getIntput() const
    {
        return getImpl()->input_.c_str();
    }
    
    void ICCTransform::setOutput(const char * output)
    {
        getImpl()->output_ = output;
    }
    
    void ICCTransform::setOutputMem(const void * /*outputPtr*/, int /*size*/)
    {
        // not yet impl
    }
    
    const char * ICCTransform::getOutput() const
    {
        return getImpl()->output_.c_str();
    }
    
    void ICCTransform::setProof(const char * proof)
    {
        getImpl()->proof_ = proof;
    }
    
    void ICCTransform::setProofMem(const void * /*proofPtr*/, int /*size*/)
    {
        // not yet impl
    }
    
    const char * ICCTransform::getProof() const
    {
        return getImpl()->proof_.c_str();
    }
    
    void ICCTransform::setIntent(IccIntent intent)
    {
        getImpl()->intent_ = intent;
    }
    
    IccIntent ICCTransform::getIntent() const
    {
        return getImpl()->intent_;
    }
    
    void ICCTransform::setBlackpointCompensation(bool blackpointCompensation)
    {
        getImpl()->blackpointCompensation_ = blackpointCompensation;
    }
    
    bool ICCTransform::getBlackpointCompensation() const
    {
        return getImpl()->blackpointCompensation_;
    }
    
    void ICCTransform::setSoftProofing(bool softProofing)
    {
        getImpl()->softProofing_ = softProofing;
    }
    
    bool ICCTransform::getSoftProofing() const
    {
        return getImpl()->softProofing_;
    }
    
    void ICCTransform::setGamutCheck(bool gamutCheck)
    {
        getImpl()->gamutCheck_ = gamutCheck;
    }
    
    bool ICCTransform::getGamutCheck() const
    {
        return getImpl()->gamutCheck_;
    }
    
    std::ostream& operator<< (std::ostream& os, const ICCTransform& t)
    {
        os << "<ICCTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        os << ">\n";
        return os;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    void BuildICCOps(OpRcPtrVec & ops,
                     const Config& /*config*/,
                     const ICCTransform & transform,
                     TransformDirection dir)
    {
        TransformDirection combinedDir = CombineTransformDirections(dir,
            transform.getDirection());
        CreateICCOps(ops, transform, combinedDir);
    }
    
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"
#include "pystring/pystring.h"

OIIO_ADD_TEST(ICCTransform, simpletest)
{
    
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("iccinput");
        cs->setFamily("foo1");
        config->addColorSpace(cs);
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("iccoutput");
        cs->setFamily("foo2");
        OCIO::ICCTransformRcPtr transform1 = OCIO::ICCTransform::Create();
        // TODO: move these into the project, once we have the paths resolv correctly
        transform1->setInput("/tmp/test1.icc");
        transform1->setOutput("/tmp/test2.icc");
        transform1->setIntent(OCIO::ICC_INTENT_ABSOLUTE_COLORIMETRIC);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }
    
    // check the transform round trip
    OCIO::ConstProcessorRcPtr fwdproc;
    OCIO::ConstProcessorRcPtr revproc;
    
    OIIO_CHECK_NO_THOW(fwdproc = config->getProcessor("iccinput", "iccoutput"));
    OIIO_CHECK_NO_THOW(revproc = config->getProcessor("iccoutput", "iccinput"));
    
    float input[3] = {0.5f, 0.5f, 0.5f};
    float output[3] = {0.51046f, 0.495933f, 0.517784f};
    
    if(fwdproc) OIIO_CHECK_NO_THOW(fwdproc->applyRGB(input));
    if(revproc) OIIO_CHECK_NO_THOW(revproc->applyRGB(input));
    
    OIIO_CHECK_CLOSE(input[0], output[0], 1e-4);
    OIIO_CHECK_CLOSE(input[1], output[1], 1e-4);
    OIIO_CHECK_CLOSE(input[2], output[2], 1e-4);
    
    //
    std::ostringstream os;
    OIIO_CHECK_NO_THOW(config->serialize(os));
    
    std::string testconfig =
    "---\n"
    "ocio_profile_version: 1\n"
    "\n"
    "search_path: \"\"\n"
    "strictparsing: true\n"
    "luma: [0.2126, 0.7152, 0.0722]\n"
    "\n"
    "roles:\n"
    "  {}\n"
    "\n"
    "displays:\n"
    "  {}\n"
    "active_displays: []\n"
    "active_views: []\n"
    "\n"
    "colorspaces:\n"
    "  - !<ColorSpace>\n"
    "    name: iccinput\n"
    "    family: foo1\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n"
    "\n"
    "  - !<ColorSpace>\n"
    "    name: iccoutput\n"
    "    family: foo2\n"
    "    bitdepth: unknown\n"
    "    isdata: false\n"
    "    allocation: uniform\n"
    "    from_reference: !<ICCTransform> {input: /tmp/test1.icc, output: /tmp/test2.icc, intent: absolute_colorimetric}\n";
    
    std::vector<std::string> osvec;
    OCIO::pystring::splitlines(os.str(), osvec);
    std::vector<std::string> testconfigvec;
    OCIO::pystring::splitlines(testconfig, testconfigvec);
    
    OIIO_CHECK_EQUAL(osvec.size(), testconfigvec.size());
    for(unsigned int i = 0; i < testconfigvec.size(); ++i)
        OIIO_CHECK_EQUAL(osvec[i], testconfigvec[i]);
    
    std::istringstream is;
    is.str(testconfig);
    OCIO::ConstConfigRcPtr rtconfig;
    OIIO_CHECK_NO_THOW(rtconfig = OCIO::Config::CreateFromStream(is));
    
}

#endif // OCIO_UNIT_TEST
