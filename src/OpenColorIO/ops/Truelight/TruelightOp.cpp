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
#include <string.h>

#ifdef OCIO_TRUELIGHT_SUPPORT
#include <truelight.h>
#else
#define TL_INPUT_LOG 0
#define TL_INPUT_LIN 1
#define TL_INPUT_VID 2
#endif // OCIO_TRUELIGHT_SUPPORT

#include <OpenColorIO/OpenColorIO.h>

#include "TruelightOp.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        class TruelightOp : public Op
        {
        public:
            TruelightOp(const char * configroot,
                        const char * profile,
                        const char * camera,
                        const char * inputdisplay,
                        const char * recorder,
                        const char * print,
                        const char * lamp,
                        const char * outputcamera,
                        const char * display,
                        const char * cubeinput,
                        TransformDirection direction);
            virtual ~TruelightOp();

            OpRcPtr clone() const override;
            
            std::string getInfo() const override;
            std::string getCacheID() const override;

            bool isNoOpType() const override { return false; }
            
            bool isNoOp() const override;
            bool isSameType(ConstOpRcPtr & op) const override;
            bool isInverse(ConstOpRcPtr & op) const override;
            bool hasChannelCrosstalk() const override;
            
            void finalize() override;

            void apply(void * rgbaBuffer, long numPixels) const override;
            void apply(const void * inImg, void * outImg, long numPixels) const override;

            bool supportedByLegacyShader() const override;
            void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;

            BitDepth getInputBitDepth() const override { return BIT_DEPTH_F32; }
            void setInputBitDepth(BitDepth /*bitDepth*/) override { }
            BitDepth getOutputBitDepth() const override { return BIT_DEPTH_F32; }
            void setOutputBitDepth(BitDepth /*bitDepth*/) override { }

        private:
            TransformDirection m_direction;
#ifdef OCIO_TRUELIGHT_SUPPORT
            void * m_truelight = nullptr;
#endif
            std::string m_configroot;
            std::string m_profile;
            std::string m_camera;
            std::string m_inputdisplay;
            std::string m_recorder;
            std::string m_print;
            std::string m_lamp;
            std::string m_outputcamera;
            std::string m_display;
            int m_cubeinput;
        };
        
        TruelightOp::TruelightOp(const char * configroot,
                                 const char * profile,
                                 const char * camera,
                                 const char * inputdisplay,
                                 const char * recorder,
                                 const char * print,
                                 const char * lamp,
                                 const char * outputcamera,
                                 const char * display,
                                 const char * cubeinput,
                                 TransformDirection direction):
                                 Op(),
                                 m_direction(direction),
                                 m_configroot(configroot),
                                 m_profile(profile),
                                 m_camera(camera),
                                 m_inputdisplay(inputdisplay),
                                 m_recorder(recorder),
                                 m_print(print),
                                 m_lamp(lamp),
                                 m_outputcamera(outputcamera),
                                 m_display(display)
        {
            
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception("Cannot apply TruelightOp op, unspecified transform direction.");
            }
            
            std::string _tmp = pystring::lower(cubeinput);
            if(_tmp == "log")    m_cubeinput = TL_INPUT_LOG;
            else if(_tmp == "linear") m_cubeinput = TL_INPUT_LIN;
            else if(_tmp == "video")  m_cubeinput = TL_INPUT_VID;
            else
            {
                std::ostringstream err;
                err << "we don't support cubeinput of type " << cubeinput;
                err << " try log, linear or video.";
                throw Exception(err.str().c_str());
            }
            
#ifdef OCIO_TRUELIGHT_SUPPORT
            
            if((TruelightBegin("")) == 0)
            {
                std::ostringstream err;
                err << "Error: " << TruelightGetErrorString();
                throw Exception(err.str().c_str());
            }
            
            m_truelight = TruelightCreateInstance();
            if(!m_truelight)
            {
                std::ostringstream err;
                err << "Error: '" << TruelightGetErrorString();
                throw Exception(err.str().c_str());
            }
            
            // floating point
            TruelightInstanceSetMax(m_truelight, 1);
            
            // where too look for the profiles, prints etc
            TruelightSetRoot(m_configroot.c_str());
            
            // invert the transform depending on direction
            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                TruelightInstanceSetInvertFlag(m_truelight, 0);
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                TruelightInstanceSetInvertFlag(m_truelight, 1);
            }
        
#endif // OCIO_TRUELIGHT_SUPPORT
        
        }
        
        OpRcPtr TruelightOp::clone() const
        {
            std::string _cubeinput = "unknown";
            if(m_cubeinput == TL_INPUT_LOG) _cubeinput = "log";
            else if(m_cubeinput == TL_INPUT_LIN) _cubeinput = "linear";
            else if(m_cubeinput == TL_INPUT_VID) _cubeinput = "video";
            return std::make_shared<TruelightOp>(m_configroot.c_str(),
                                                 m_profile.c_str(),
                                                 m_camera.c_str(),
                                                 m_inputdisplay.c_str(),
                                                 m_recorder.c_str(),
                                                 m_print.c_str(),
                                                 m_lamp.c_str(),
                                                 m_outputcamera.c_str(),
                                                 m_display.c_str(),
                                                 _cubeinput.c_str(),
                                                 m_direction);
        }
        
        TruelightOp::~TruelightOp()
        {
#ifdef OCIO_TRUELIGHT_SUPPORT
            if(m_truelight) TruelightDestroyInstance(m_truelight);
#endif // OCIO_TRUELIGHT_SUPPORT
        }
        
        std::string TruelightOp::getInfo() const
        {
            return "<TruelightOp>";
        }
        
        std::string TruelightOp::getCacheID() const
        {
            return m_cacheID;
        }
        
        bool TruelightOp::isNoOp() const
        {
            return false;
        }
        bool TruelightOp::isSameType(ConstOpRcPtr & /*op*/) const
        {
            // TODO: TruelightOp::isSameType
            return false;
        }
        
        bool TruelightOp::isInverse(ConstOpRcPtr & /*op*/) const
        {
            // TODO: TruelightOp::isInverse
            return false;
        }
        
        bool TruelightOp::hasChannelCrosstalk() const
        {
            return true;
        }
        
        void TruelightOp::finalize()
        {
#ifndef OCIO_TRUELIGHT_SUPPORT
            std::ostringstream err;
            err << "OCIO has been built without Truelight support";
            throw Exception(err.str().c_str());
#else
            if(m_profile != "")
            {
                if(TruelightInstanceSetProfile(m_truelight, m_profile.c_str()) == 0)
                {
                    std::ostringstream err;
                    err << "Error: " << TruelightGetErrorString();
                    throw Exception(err.str().c_str());
                }
            }
            
            if(m_camera != "")
            {
                if(TruelightInstanceSetCamera(m_truelight, m_camera.c_str()) == 0)
                {
                    std::ostringstream err;
                    err << "Error: " << TruelightGetErrorString();
                    throw Exception(err.str().c_str());
                }
            }
            
            if(m_inputdisplay != "")
            {
                if(TruelightInstanceSetInputDisplay(m_truelight, m_inputdisplay.c_str()) == 0)
                {
                    std::ostringstream err;
                    err << "Error: " << TruelightGetErrorString();
                    throw Exception(err.str().c_str());
                }
            }
            
            if(m_recorder != "")
            {
                if(TruelightInstanceSetRecorder(m_truelight, m_recorder.c_str()) == 0)
                {
                    std::ostringstream err;
                    err << "Error: " << TruelightGetErrorString();
                    throw Exception(err.str().c_str());
                }
            }
            
            if(m_print != "")
            {
                if(TruelightInstanceSetPrint(m_truelight, m_print.c_str()) == 0)
                {
                    std::ostringstream err;
                    err << "Error: " << TruelightGetErrorString();
                    throw Exception(err.str().c_str());
                }
            }
            
            if(m_lamp != "")
            {
                if(TruelightInstanceSetLamp(m_truelight, m_lamp.c_str()) == 0)
                {
                    std::ostringstream err;
                    err << "Error: " << TruelightGetErrorString();
                    throw Exception(err.str().c_str());
                }
            }
            
            if(m_outputcamera != "")
            {
                if(TruelightInstanceSetOutputCamera(m_truelight, m_outputcamera.c_str()) == 0)
                {
                    std::ostringstream err;
                    err << "Error: " << TruelightGetErrorString();
                    throw Exception(err.str().c_str());
                }
            }
            
            if(m_display != "")
            {
                if(TruelightInstanceSetDisplay(m_truelight, m_display.c_str()) == 0)
                {
                    std::ostringstream err;
                    err << "Error: " << TruelightGetErrorString();
                    throw Exception(err.str().c_str());
                }
            }
            
            if(TruelightInstanceSetCubeInput(m_truelight, m_cubeinput) == 0)
            {
                std::ostringstream err;
                err << "Error: " << TruelightGetErrorString();
                throw Exception(err.str().c_str());
            }
            
            if(TruelightInstanceSetUp(m_truelight) == 0)
            {
                std::ostringstream err;
                err << "Error: " << TruelightGetErrorString();
                throw Exception(err.str().c_str());
            }
#endif // OCIO_TRUELIGHT_SUPPORT
            
            // build cache id
            std::ostringstream cacheIDStream;
            cacheIDStream << "<TruelightOp ";
            cacheIDStream << m_profile << " ";
            cacheIDStream << m_camera << " ";
            cacheIDStream << m_inputdisplay << " ";
            cacheIDStream << m_recorder << " ";
            cacheIDStream << m_print << " ";
            cacheIDStream << m_lamp << " ";
            cacheIDStream << m_outputcamera << " ";
            cacheIDStream << m_display << " ";
            cacheIDStream << m_cubeinput << " ";
            cacheIDStream << TransformDirectionToString(m_direction) << " ";
            cacheIDStream << ">";
            m_cacheID = cacheIDStream.str();
        }
        
        void TruelightOp::apply(void * rgbaBuffer, long numPixels) const
        {
            apply(rgbaBuffer, rgbaBuffer, numPixels);
        }
        
        void TruelightOp::apply(const void * inImg, void * outImg, long numPixels) const
        {
            const float * in = reinterpret_cast<const float *>(inImg);
            float * out = reinterpret_cast<float *>(outImg);

            for(long pixelIndex = 0; pixelIndex < numPixels; ++pixelIndex)
            {
                memcpy(out, in, 4 * sizeof(float));

#ifdef OCIO_TRUELIGHT_SUPPORT
                TruelightInstanceTransformF(m_truelight, out);
#endif // OCIO_TRUELIGHT_SUPPORT

                in  += 4; // skip alpha
                out += 4;
            }
        }

        bool TruelightOp::supportedByLegacyShader() const
        {
            return false; 
        }

        void TruelightOp::extractGpuShaderInfo(GpuShaderDescRcPtr & /*shaderDesc*/) const
        {
            throw Exception("TruelightOp does not define an gpu shader.");
        }
        
    }  // anonymous namespace
    
    void CreateTruelightOps(OpRcPtrVec & ops,
                            const TruelightTransform & data,
                            TransformDirection direction)
    {
        ops.push_back(std::make_shared<TruelightOp>(data.getConfigRoot(),
                                                    data.getProfile(),
                                                    data.getCamera(),
                                                    data.getInputDisplay(),
                                                    data.getRecorder(),
                                                    data.getPrint(),
                                                    data.getLamp(),
                                                    data.getOutputCamera(),
                                                    data.getDisplay(),
                                                    data.getCubeInput(),
                                                    direction));
    }
}
OCIO_NAMESPACE_EXIT
