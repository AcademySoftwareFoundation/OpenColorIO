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

#include <iostream>

#include <lcms2.h>
#include <lcms2_plugin.h>

#include <OpenColorIO/OpenColorIO.h>

#include "ICCOp.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        
        void ErrorHandler(cmsContext /*ContextID*/, cmsUInt32Number ErrorCode, const char *Text)
        {
            std::ostringstream err;
            err << "Error: (" << ErrorCode << ") '" << Text << "'";
            throw Exception(err.str().c_str());
        }
        
        class ICCOp : public Op
        {
        public:
            ICCOp(const char * input,
                  const char * output,
                  const char * proof,
                  IccIntent intent,
                  bool blackpointCompensation,
                  bool softProofing,
                  bool gamutCheck,
                  TransformDirection direction);
            virtual ~ICCOp();
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const;
            virtual std::string getCacheID() const;
            
            virtual bool isNoOp() const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;
            
            virtual bool supportsGpuShader() const;
            virtual void writeGpuShader(std::ostream & shader,
                                        const std::string & pixelName,
                                        const GpuShaderDesc & shaderDesc) const;
            
            virtual bool definesAllocation() const;
            virtual AllocationData getAllocation() const;
        
        private:
            TransformDirection m_direction;
            std::string m_input;
            std::string m_output;
            std::string m_proof;
            IccIntent m_intent;
            bool m_blackpointCompensation;
            bool m_softProofing;
            bool m_gamutCheck;
            cmsHTRANSFORM m_transform;
            std::string m_cacheID;
        };
        
        ICCOp::ICCOp(const char * input,
                     const char * output,
                     const char * proof,
                     IccIntent intent,
                     bool blackpointCompensation,
                     bool softProofing,
                     bool gamutCheck,
                     TransformDirection direction):
                     Op(),
                     m_direction(direction),
                     m_input(input),
                     m_output(output),
                     m_proof(proof),
                     m_intent(intent),
                     m_blackpointCompensation(blackpointCompensation),
                     m_softProofing(softProofing),
                     m_gamutCheck(gamutCheck),
                     m_transform(NULL)
        {
            // Setup the Error Handler
            cmsSetLogErrorHandler(ErrorHandler);
        }
        
        OpRcPtr ICCOp::clone() const
        {
            OpRcPtr op = OpRcPtr(new ICCOp(m_input.c_str(),
                                           m_output.c_str(),
                                           m_proof.c_str(),
                                           m_intent,
                                           m_blackpointCompensation,
                                           m_softProofing,
                                           m_gamutCheck,
                                           m_direction));
            return op;
        }
        
        ICCOp::~ICCOp()
        {
            if(m_transform) cmsDeleteTransform(m_transform);
        }
        
        std::string ICCOp::getInfo() const
        {
            return "<ICCOp>";
        }
        
        std::string ICCOp::getCacheID() const
        {
            return m_cacheID;
        }
        
        bool ICCOp::isNoOp() const
        {
            return false;
        }
        
        void ICCOp::finalize()
        {
            
            // TODO: should this be in the ICCTransform rather than here?
            //std::string inputpath = context->resolveFileLocation(m_input.c_str());
            //std::string outputpath = context->resolveFileLocation(m_output.c_str());
            
            cmsHPROFILE inputIcc = cmsOpenProfileFromFile(m_input.c_str(), "r");
            cmsHPROFILE outputIcc = cmsOpenProfileFromFile(m_output.c_str(), "r");
            
            // invert the transform depending on direction
            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                m_transform = cmsCreateTransform(inputIcc,  TYPE_RGBA_FLT,
                                                 outputIcc, TYPE_RGBA_FLT,
                                                 INTENT_PERCEPTUAL,
                                                 0);
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                m_transform = cmsCreateTransform(outputIcc, TYPE_RGBA_FLT,
                                                 inputIcc,  TYPE_RGBA_FLT,
                                                 INTENT_PERCEPTUAL,
                                                 0);
            }
            
            cmsCloseProfile(inputIcc);
            cmsCloseProfile(outputIcc);
            
            // build cache id
            std::ostringstream cacheIDStream;
            cacheIDStream << "<ICCOp ";
            cacheIDStream << m_input << " ";
            cacheIDStream << m_output << " ";
            cacheIDStream << m_proof << " ";
            cacheIDStream << m_intent << " ";
            cacheIDStream << m_blackpointCompensation << " ";
            cacheIDStream << m_softProofing << " ";
            cacheIDStream << m_gamutCheck << " ";
            cacheIDStream << TransformDirectionToString(m_direction) << " ";
            cacheIDStream << ">";
            m_cacheID = cacheIDStream.str();
        }
        
        void ICCOp::apply(float* rgbaBuffer, long numPixels) const
        {
            if(m_transform != NULL) cmsDoTransform(m_transform, rgbaBuffer, rgbaBuffer, numPixels);
        }
        
        bool ICCOp::supportsGpuShader() const
        {
            return false;
        }
        
        void ICCOp::writeGpuShader(std::ostream & /*shader*/,
                                   const std::string & /*pixelName*/,
                                   const GpuShaderDesc & /*shaderDesc*/) const
        {
            throw Exception("ICCOp does not define an gpu shader.");
        }
        
        bool ICCOp::definesAllocation() const
        {
            return false;
        }
        
        AllocationData ICCOp::getAllocation() const
        {
            throw Exception("ICCOp does not define an allocation.");
        }
        
    }  // anonymous namespace
    
    void CreateICCOps(OpRcPtrVec & ops,
                      const ICCTransform & data,
                      TransformDirection direction)
    {
        ops.push_back(OpRcPtr(new ICCOp(data.getIntput(),
                                        data.getOutput(),
                                        data.getProof(),
                                        data.getIntent(),
                                        data.getBlackpointCompensation(),
                                        data.getSoftProofing(),
                                        data.getGamutCheck(),
                                        direction)));
    }
}
OCIO_NAMESPACE_EXIT
