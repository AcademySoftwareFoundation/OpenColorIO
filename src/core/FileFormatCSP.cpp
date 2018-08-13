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

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <sstream>
#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>

#include "FileTransform.h"
#include "Lut1DOp.h"
#include "Lut3DOp.h"
#include "MathUtils.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        const int NUM_PRELUT_SAMPLES = 65536; // 2**16 samples
        // Always use linear interpolation for preluts to get the
        // best precision
        const Interpolation PRELUT_INTERPOLATION = INTERP_LINEAR;
        
        typedef struct rsr_Interpolator1D_Raw_
        {
            float * stims;
            float * values;
            unsigned int length;
        } rsr_Interpolator1D_Raw;

        rsr_Interpolator1D_Raw * rsr_Interpolator1D_Raw_create( unsigned int prelutLength );
        void rsr_Interpolator1D_Raw_destroy( rsr_Interpolator1D_Raw * prelut );


        /* An opaque handle to the cineSpace 1D Interpolator object */
        typedef struct rsr_Interpolator1D_ rsr_Interpolator1D;

        rsr_Interpolator1D * rsr_Interpolator1D_createFromRaw( rsr_Interpolator1D_Raw * data );
        void rsr_Interpolator1D_destroy( rsr_Interpolator1D * rawdata );
        float rsr_Interpolator1D_interpolate( float x, rsr_Interpolator1D * data );


        /*
         * =========== INTERNAL HELPER FUNCTIONS ============
         */
        struct rsr_Interpolator1D_
        {
            int nSamplePoints;
            float * stims;

            /* 5 * (nSamplePoints-1) long, holding a sequence of
             * 1.0/delta, a, b, c, d
             * such that the curve in interval i is given by
             * z = (stims[i] - x)* (1.0/delta)
             * y = a + b*z + c*z^2 + d*z^3
             */
            float * parameters;

            float minValue;   /* = f( stims[0] )              */
            float maxValue;   /* = f(stims[nSamplePoints-1] ) */
        };

        static int rsr_internal_I1D_refineSegment( float x, float * data, int low, int high )
        {
            int midPoint;

            // TODO: Change assert to an exception?
            assert( x>= data[low] );
            assert( x<= data[high] );
            assert( (high-low) > 0);
            if( high-low==1 ) return low;

            midPoint = (low+high)/2;
            if( x<data[midPoint] ) return rsr_internal_I1D_refineSegment(x, data, low, midPoint );
            return rsr_internal_I1D_refineSegment(x, data, midPoint, high );
        }

        static int rsr_internal_I1D_findSegmentContaining( float x, float * data, int n )
        {
            return rsr_internal_I1D_refineSegment(x, data, 0, n-1);
        }

        /*
         * =========== USER FUNCTIONS ============
         */


        void rsr_Interpolator1D_destroy( rsr_Interpolator1D * data )
        {
            if(data==NULL) return;
            free(data->stims);
            free(data->parameters);
            free(data);
        }



        float rsr_Interpolator1D_interpolate( float x, rsr_Interpolator1D * data )
        {
            int segId;
            float * segdata;
            float invDelta;
            float a,b,c,d,z;

            assert(data!=NULL);

            /* Is x in range? */
            if( isnan(x) ) return x;

            if( x<data->stims[0] ) return data->minValue;
            if (x>data->stims[ data->nSamplePoints -1] ) return data->maxValue;

            /* Ok so its between the begining and end .. lets find out where... */
            segId = rsr_internal_I1D_findSegmentContaining( x, data->stims, data->nSamplePoints );

            assert(data->parameters !=NULL );

            segdata = data->parameters + 5 * segId;

            invDelta = segdata[0];
            a = segdata[1];
            b = segdata[2];
            c = segdata[3];
            d = segdata[4];

            z = ( x - data->stims[segId] ) * invDelta;

            return a + z * ( b + z * ( c  + d * z ) ) ;

        }

        rsr_Interpolator1D * rsr_Interpolator1D_createFromRaw( rsr_Interpolator1D_Raw * data )
        {
            rsr_Interpolator1D * retval = NULL;
            /* Check the sanity of the data */
            assert(data!=NULL);
            assert(data->length>=2);
            assert(data->stims!=NULL);
            assert(data->values!=NULL);

            /* Create the real data. */
            retval = (rsr_Interpolator1D*)malloc( sizeof(rsr_Interpolator1D) ); // OCIO change: explicit cast
            if(retval==NULL)
            {
                return NULL;
            }

            retval->stims = (float*)malloc( sizeof(float) * data->length ); // OCIO change: explicit cast
            if(retval->stims==NULL)
            {
                free(retval);
                return NULL;
            }
            memcpy( retval->stims, data->stims, sizeof(float) * data->length );

            retval->parameters = (float*)malloc( 5*sizeof(float) * ( data->length - 1 ) ); // OCIO change: explicit cast
            if(retval->parameters==NULL)
            {
                free(retval->stims);
                free(retval);
                return NULL;
            }
            retval->nSamplePoints = data->length;
            retval->minValue = data->values[0];
            retval->maxValue = data->values[ data->length -1];

            /* Now the fun part .. filling in the coeficients. */
            if(data->length==2)
            {
                retval->parameters[0] = 1.0f/(data->stims[1]-data->stims[0]);
                retval->parameters[1] = data->values[0];
                retval->parameters[2] = ( data->values[1] - data->values[0] );
                retval->parameters[3] = 0;
                retval->parameters[4] = 0;
            }
            else
            {
                unsigned int i;
                float * params = retval->parameters;
                for(i=0; i< data->length-1; ++i)
                {
                    float f0 = data->values[i+0];
                    float f1 = data->values[i+1];

                    params[0] = 1.0f/(retval->stims[i+1]-retval->stims[i+0]);

                    if(i==0)
                    {
                        float delta = data->stims[i+1] - data->stims[i];
                        float delta2 = (data->stims[i+2] - data->stims[i+1])/delta;
                        float f2 = data->values[i+2];

                        float dfdx1 = (f2-f0)/(1+delta2);
                        params[1] =  1.0f * f0 + 0.0f * f1 + 0.0f * dfdx1;
                        params[2] = -2.0f * f0 + 2.0f * f1 - 1.0f * dfdx1;
                        params[3] =  1.0f * f0 - 1.0f * f1 + 1.0f * dfdx1;
                        params[4] =  0.0;
                    }
                    else if (i==data->length-2)
                    {
                        float delta = data->stims[i+1] - data->stims[i];
                        float delta1 = (data->stims[i]-data->stims[i-1])/delta;
                        float fn1 = data->values[i-1];
                        float dfdx0 = (f1-fn1)/(1+delta1);
                        params[1] =  1.0f * f0 + 0.0f * f1 + 0.0f * dfdx0;
                        params[2] =  0.0f * f0 + 0.0f * f1 + 1.0f * dfdx0;
                        params[3] = -1.0f * f0 + 1.0f * f1 - 1.0f * dfdx0;
                        params[4] =  0.0;
                    }
                    else
                    {
                        float delta = data->stims[i+1] - data->stims[i];
                        float fn1=data->values[i-1];
                        float delta1 = (data->stims[i] - data->stims[i-1])/delta;

                        float f2=data->values[i+2];
                        float delta2 = (data->stims[i+2] - data->stims[i+1])/delta;

                        float dfdx0 = (f1-fn1)/(1.0f+delta1);
                        float dfdx1 = (f2-f0)/(1.0f+delta2);

                        params[1] = 1.0f * f0 + 0.0f * dfdx0 + 0.0f * f1 + 0.0f * dfdx1;
                        params[2] = 0.0f * f0 + 1.0f * dfdx0 + 0.0f * f1 + 0.0f * dfdx1;
                        params[3] =-3.0f * f0 - 2.0f * dfdx0 + 3.0f * f1 - 1.0f * dfdx1;
                        params[4] = 2.0f * f0 + 1.0f * dfdx0 - 2.0f * f1 + 1.0f * dfdx1;
                    }

                    params+=5;
                }
            }
            return retval;
        }

        rsr_Interpolator1D_Raw * rsr_Interpolator1D_Raw_create( unsigned int prelutLength)
        {
            unsigned int i;
            rsr_Interpolator1D_Raw * prelut = (rsr_Interpolator1D_Raw*)malloc( sizeof(rsr_Interpolator1D_Raw) ); // OCIO change: explicit cast
            if(prelut==NULL) return NULL;

            prelut->stims = (float*)malloc( sizeof(float) * prelutLength ); // OCIO change: explicit cast
            if(prelut->stims==NULL)
            {
                free(prelut);
                return NULL;
            }

            prelut->values = (float*)malloc( sizeof(float) * prelutLength ); // OCIO change: explicit cast
            if(prelut->values == NULL)
            {
                free(prelut->stims);
                free(prelut);
                return NULL;
            }

            prelut->length = prelutLength;

            for( i=0; i<prelutLength; ++i )
            {
                prelut->stims[i] = 0.0;
                prelut->values[i] = 0.0;
            }

            return prelut;
        }

        void rsr_Interpolator1D_Raw_destroy( rsr_Interpolator1D_Raw * prelut )
        {
            if(prelut==NULL) return;
            free( prelut->stims );
            free( prelut->values );
            free( prelut );
        }

    } // End unnamed namespace for Interpolators.c

    namespace
    {
        class CachedFileCSP : public CachedFile
        {
        public:
            CachedFileCSP () :
                hasprelut(false),
                csptype("unknown"),
                metadata("none")
            {
                prelut = Lut1D::Create();
                lut1D = Lut1D::Create();
                lut3D = Lut3D::Create();
            };
            ~CachedFileCSP() {};
            
            bool hasprelut;
            std::string csptype;
            std::string metadata;
            Lut1DRcPtr prelut;
            Lut1DRcPtr lut1D;
            Lut3DRcPtr lut3D;
        };
        typedef OCIO_SHARED_PTR<CachedFileCSP> CachedFileCSPRcPtr;
        
        inline bool
        startswithU(const std::string & str, const std::string & prefix)
        {
            return pystring::startswith(pystring::upper(pystring::strip(str)), prefix);
        }
        
        class LocalFileFormat : public FileFormat
        {
        public:
            
            ~LocalFileFormat() {};
            
            virtual void GetFormatInfo(FormatInfoVec & formatInfoVec) const;
            
            virtual CachedFileRcPtr Read(
                std::istream & istream,
                const std::string & fileName) const;
            
            virtual void Write(const Baker & baker,
                               const std::string & formatName,
                               std::ostream & ostream) const;
            
            virtual void BuildFileOps(OpRcPtrVec & ops,
                                      const Config& config,
                                      const ConstContextRcPtr & context,
                                      CachedFileRcPtr untypedCachedFile,
                                      const FileTransform& fileTransform,
                                      TransformDirection dir) const;
        };


        // TODO: remove this when we don't need to debug
        /*
        template<class T>
        std::ostream& operator<< (std::ostream& os, const std::vector<T>& v)
        {
            copy(v.begin(), v.end(), std::ostream_iterator<T>(std::cout, " ")); 
            return os;
        }
        */
        
        void LocalFileFormat::GetFormatInfo(FormatInfoVec & formatInfoVec) const
        {
            FormatInfo info;
            info.name = "cinespace";
            info.extension = "csp";
            info.capabilities = (FORMAT_CAPABILITY_READ | FORMAT_CAPABILITY_WRITE);
            formatInfoVec.push_back(info);
        }
        
        CachedFileRcPtr
        LocalFileFormat::Read(
            std::istream & istream,
            const std::string & /* fileName unused */) const
        {

            // this shouldn't happen
            if (!istream)
            {
                throw Exception ("file stream empty when trying to read csp LUT");
            }
            
            Lut1DRcPtr prelut_ptr = Lut1D::Create();
            Lut1DRcPtr lut1d_ptr = Lut1D::Create();
            Lut3DRcPtr lut3d_ptr = Lut3D::Create();

            // try and read the LUT header
            std::string line;
            nextline (istream, line);
            if (!startswithU(line, "CSPLUTV100"))
            {
                std::ostringstream os;
                os << "LUT doesn't seem to be a csp file, expected 'CSPLUTV100'.";
                os << "First line: '" << line << "'.";
                throw Exception(os.str().c_str());
            }

            // next line tells us if we are reading a 1D or 3D LUT
            nextline (istream, line);
            if (!startswithU(line, "1D") && !startswithU(line, "3D"))
            {
                std::ostringstream os;
                os << "Unsupported CSP LUT type. Require 1D or 3D. ";
                os << "Found, '" << line << "'.";
                throw Exception(os.str().c_str());
            }
            std::string csptype = line;

            // read meta data block
            std::string metadata;
            bool lineUpdateNeeded = false;
            nextline (istream, line);
            if(startswithU(line, "BEGIN METADATA"))
            {
                while (!startswithU(line, "END METADATA") ||
                       !istream)
                {
                    nextline (istream, line);
                    if (!startswithU(line, "END METADATA"))
                        metadata += line + "\n";
                }
                lineUpdateNeeded = true;
            }// else line update not needed
            
            
            // Make 3 vectors of prelut inputs + output values
            std::vector<float> prelut_in[3];
            std::vector<float> prelut_out[3];
            bool useprelut[3] = { false, false, false };
            
            // Parse the prelut block
            for (int c = 0; c < 3; ++c)
            {
                // how many points do we have for this channel
                if (lineUpdateNeeded)
                    nextline (istream, line);

                int cpoints = 0;
                
                if(!StringToInt(&cpoints, line.c_str()) || cpoints<0)
                {
                    std::ostringstream os;
                    os << "Prelut does not specify valid dimension size on channel '";
                    os << c << ": " << line;
                    throw Exception(os.str().c_str());
                }
                
                if(cpoints>=2)
                {
                    std::vector<std::string> inputparts, outputparts;
                    
                    nextline (istream, line);
                    pystring::split(pystring::strip(line), inputparts);
                    
                    nextline (istream, line);
                    pystring::split(pystring::strip(line), outputparts);
                    
                    if(static_cast<int>(inputparts.size()) != cpoints ||
                       static_cast<int>(outputparts.size()) != cpoints)
                    {
                        std::ostringstream os;
                        os << "Prelut does not specify the expected number of data points. ";
                        os << "Expected: " << cpoints << ".";
                        os << "Found: " << inputparts.size() << ", " << outputparts.size() << ".";
                        throw Exception(os.str().c_str());
                    }
                    
                    if(!StringVecToFloatVec(prelut_in[c], inputparts) ||
                       !StringVecToFloatVec(prelut_out[c], outputparts))
                    {
                        std::ostringstream os;
                        os << "Prelut data is malformed, cannot to float array.";
                        throw Exception(os.str().c_str());
                    }
                    
                    
                    useprelut[c] = (!VecsEqualWithRelError(&(prelut_in[c][0]),  static_cast<int>(prelut_in[c].size()),
                                                           &(prelut_out[c][0]), static_cast<int>(prelut_out[c].size()),
                                                           1e-6f));
                }
                else
                {
                    // Even though it's probably not part of the spec, why not allow for a size 0
                    // in a channel to be specified?  It should be synonymous with identity,
                    // and allows the code lower down to assume all 3 channels exist
                    
                    prelut_in[c].push_back(0.0f);
                    prelut_in[c].push_back(1.0f);
                    prelut_out[c].push_back(0.0f);
                    prelut_out[c].push_back(1.0f);
                    useprelut[c] = false;
                }
                lineUpdateNeeded = true;
            }

            if (csptype == "1D")
            {

                // how many 1D LUT points do we have
                nextline (istream, line);
                int points1D = atoi (line.c_str());

                //
                float from_min = 0.0;
                float from_max = 1.0;
                for(int i=0; i<3; ++i)
                {
                    lut1d_ptr->from_min[i] = from_min;
                    lut1d_ptr->from_max[i] = from_max;
                    lut1d_ptr->luts[i].clear();
                    lut1d_ptr->luts[i].reserve(points1D);
                }

                for(int i = 0; i < points1D; ++i)
                {

                    // scan for the three floats
                    float lp[3];
                    nextline (istream, line);
                    if (sscanf (line.c_str(), "%f %f %f",
                        &lp[0], &lp[1], &lp[2]) != 3) {
                        throw Exception ("malformed 1D csp LUT");
                    }

                    // store each channel
                    lut1d_ptr->luts[0].push_back(lp[0]);
                    lut1d_ptr->luts[1].push_back(lp[1]);
                    lut1d_ptr->luts[2].push_back(lp[2]);

                }

            }
            else if (csptype == "3D")
            {
                // read the cube size
                nextline (istream, line);
                if (sscanf (line.c_str(), "%d %d %d",
                    &lut3d_ptr->size[0],
                    &lut3d_ptr->size[1],
                    &lut3d_ptr->size[2]) != 3 ) {
                    throw Exception("malformed 3D csp LUT, couldn't read cube size");
                }
                
                
                // resize cube
                int num3dentries = lut3d_ptr->size[0] * lut3d_ptr->size[1] * lut3d_ptr->size[2];
                lut3d_ptr->lut.resize(num3dentries * 3);
                
                for(int i=0; i<num3dentries; ++i)
                {
                    // load the cube
                    nextline (istream, line);
                    
                    if(sscanf (line.c_str(), "%f %f %f",
                       &lut3d_ptr->lut[3*i+0],
                       &lut3d_ptr->lut[3*i+1],
                       &lut3d_ptr->lut[3*i+2]) != 3 )
                    {
                        std::ostringstream os;
                        os << "Malformed 3D csp LUT, couldn't read cube row (";
                        os << i << "): " << line << " .";
                        throw Exception(os.str().c_str());
                    }
                }
            }
            
            CachedFileCSPRcPtr cachedFile = CachedFileCSPRcPtr (new CachedFileCSP ());
            cachedFile->csptype = csptype;
            cachedFile->metadata = metadata;
            
            if(useprelut[0] || useprelut[1] || useprelut[2])
            {
                cachedFile->hasprelut = true;
                
                for (int c = 0; c < 3; ++c)
                {
                    size_t prelut_numpts = prelut_in[c].size();
                    float from_min = prelut_in[c][0];
                    float from_max = prelut_in[c][prelut_numpts-1];
                    
                    // Allocate the interpolator
                    rsr_Interpolator1D_Raw * cprelut_raw = 
                        rsr_Interpolator1D_Raw_create(static_cast<unsigned int>(prelut_numpts));
                    
                    // Copy our prelut data into the interpolator
                    for(size_t i=0; i<prelut_numpts; ++i)
                    {
                        cprelut_raw->stims[i] = prelut_in[c][i];
                        cprelut_raw->values[i] = prelut_out[c][i];
                    }
                    
                    // Create interpolater, to resample to simple 1D LUT
                    rsr_Interpolator1D * interpolater =
                        rsr_Interpolator1D_createFromRaw(cprelut_raw);
                    
                    // Resample into 1D LUT
                    // TODO: Fancy spline analysis to determine required number of samples
                    prelut_ptr->from_min[c] = from_min;
                    prelut_ptr->from_max[c] = from_max;
                    prelut_ptr->luts[c].clear();
                    prelut_ptr->luts[c].reserve(NUM_PRELUT_SAMPLES);
                    
                    for (int i = 0; i < NUM_PRELUT_SAMPLES; ++i)
                    {
                        float interpo = float(i) / float(NUM_PRELUT_SAMPLES-1);
                        float srcval = lerpf(from_min, from_max, interpo);
                        float newval = rsr_Interpolator1D_interpolate(srcval, interpolater);
                        prelut_ptr->luts[c].push_back(newval);
                    }

                    rsr_Interpolator1D_Raw_destroy(cprelut_raw);
                    rsr_Interpolator1D_destroy(interpolater);
                }
                
                prelut_ptr->maxerror = 1e-6f;
                prelut_ptr->errortype = ERROR_RELATIVE;
                
                cachedFile->prelut = prelut_ptr;
            }
            
            if(csptype == "1D")
            {
                lut1d_ptr->maxerror = 0.0f;
                lut1d_ptr->errortype = ERROR_RELATIVE;
                cachedFile->lut1D = lut1d_ptr;
            }
            else if (csptype == "3D")
            {
                cachedFile->lut3D = lut3d_ptr;
            }
            
            return cachedFile;
        }
        
        
        void LocalFileFormat::Write(const Baker & baker,
                                    const std::string & /*formatName*/,
                                    std::ostream & ostream) const
        {
            const int DEFAULT_CUBE_SIZE = 32;
            const int DEFAULT_SHAPER_SIZE = 1024;
            
            ConstConfigRcPtr config = baker.getConfig();
            
            // TODO: Add 1d/3d LUT writing switch, using hasChannelCrosstalk
            int cubeSize = baker.getCubeSize();
            if(cubeSize==-1) cubeSize = DEFAULT_CUBE_SIZE;
            cubeSize = std::max(2, cubeSize); // smallest cube is 2x2x2
            std::vector<float> cubeData;
            cubeData.resize(cubeSize*cubeSize*cubeSize*3);
            GenerateIdentityLut3D(&cubeData[0], cubeSize, 3, LUT3DORDER_FAST_RED);
            PackedImageDesc cubeImg(&cubeData[0], cubeSize*cubeSize*cubeSize, 1, 3);

            std::string looks = baker.getLooks();
            
            std::vector<float> shaperInData;
            std::vector<float> shaperOutData;
            
            // Use an explicitly shaper space
            // TODO: Use the optional allocation for the shaper space,
            //       instead of the implied 0-1 uniform allocation
            std::string shaperSpace = baker.getShaperSpace();
            if(!shaperSpace.empty())
            {
                int shaperSize = baker.getShaperSize();
                if(shaperSize<0) shaperSize = DEFAULT_SHAPER_SIZE;
                if(shaperSize<2)
                {
                    std::ostringstream os;
                    os << "When a shaper space has been specified, '";
                    os << baker.getShaperSpace() << "', a shaper size less than 2 is not allowed.";
                    throw Exception(os.str().c_str());
                }
                
                shaperOutData.resize(shaperSize*3);
                shaperInData.resize(shaperSize*3);
                GenerateIdentityLut1D(&shaperOutData[0], shaperSize, 3);
                GenerateIdentityLut1D(&shaperInData[0], shaperSize, 3);
                
                ConstProcessorRcPtr shaperToInput = config->getProcessor(baker.getShaperSpace(), baker.getInputSpace());
                if(shaperToInput->hasChannelCrosstalk())
                {
                    // TODO: Automatically turn shaper into non-crosstalked version?
                    std::ostringstream os;
                    os << "The specified shaperSpace, '";
                    os << baker.getShaperSpace() << "' has channel crosstalk, which is not appropriate for shapers. ";
                    os << "Please select an alternate shaper space or omit this option.";
                    throw Exception(os.str().c_str());
                }
                PackedImageDesc shaperInImg(&shaperInData[0], shaperSize, 1, 3);
                shaperToInput->apply(shaperInImg);

                ConstProcessorRcPtr shaperToTarget;
                if (!looks.empty())
                {
                    LookTransformRcPtr transform = LookTransform::Create();
                    transform->setLooks(looks.c_str());
                    transform->setSrc(baker.getShaperSpace());
                    transform->setDst(baker.getTargetSpace());
                    shaperToTarget = config->getProcessor(transform,
                        TRANSFORM_DIR_FORWARD);
                }
                else
                {
                  shaperToTarget = config->getProcessor(baker.getShaperSpace(),
                      baker.getTargetSpace());
                }
                shaperToTarget->apply(cubeImg);
            }
            else
            {
                // A shaper is not specified, let's fake one, using the input space allocation as
                // our guide
                
                ConstColorSpaceRcPtr inputColorSpace = config->getColorSpace(baker.getInputSpace());

                if(!inputColorSpace)
                {
                    std::ostringstream os;
                    os << "Could not find colorspace '" << baker.getInputSpace() << "'";
                    throw Exception(os.str().c_str());
                }

                // Let's make an allocation transform for this colorspace
                AllocationTransformRcPtr allocationTransform = AllocationTransform::Create();
                allocationTransform->setAllocation(inputColorSpace->getAllocation());
                
                // numVars may be '0'
                int numVars = inputColorSpace->getAllocationNumVars();
                if(numVars>0)
                {
                    std::vector<float> vars(numVars);
                    inputColorSpace->getAllocationVars(&vars[0]);
                    allocationTransform->setVars(numVars, &vars[0]);
                }
                else
                {
                    allocationTransform->setVars(0, NULL);
                }
                
                // What size shaper should we make?
                int shaperSize = baker.getShaperSize();
                if(shaperSize<0) shaperSize = DEFAULT_SHAPER_SIZE;
                shaperSize = std::max(2, shaperSize);
                if(inputColorSpace->getAllocation() == ALLOCATION_UNIFORM)
                {
                    // This is an awesome optimization.
                    // If we know it's a uniform scaling, only 2 points will suffice!
                    shaperSize = 2;
                }
                shaperOutData.resize(shaperSize*3);
                shaperInData.resize(shaperSize*3);
                GenerateIdentityLut1D(&shaperOutData[0], shaperSize, 3);
                GenerateIdentityLut1D(&shaperInData[0], shaperSize, 3);
                
                // Apply the forward to the allocation to the output shaper y axis, and the cube
                ConstProcessorRcPtr shaperToInput = config->getProcessor(allocationTransform, TRANSFORM_DIR_INVERSE);
                PackedImageDesc shaperInImg(&shaperInData[0], shaperSize, 1, 3);
                shaperToInput->apply(shaperInImg);
                shaperToInput->apply(cubeImg);
                
                // Apply the 3d LUT to the remainder (from the input to the output)
                ConstProcessorRcPtr inputToTarget;
                if (!looks.empty())
                {
                    LookTransformRcPtr transform = LookTransform::Create();
                    transform->setLooks(looks.c_str());
                    transform->setSrc(baker.getInputSpace());
                    transform->setDst(baker.getTargetSpace());
                    inputToTarget = config->getProcessor(transform,
                        TRANSFORM_DIR_FORWARD);
                }
                else
                {
                    inputToTarget = config->getProcessor(baker.getInputSpace(), baker.getTargetSpace());
                }
                inputToTarget->apply(cubeImg);
            }
            
            // Write out the file
            ostream << "CSPLUTV100\n";
            ostream << "3D\n";
            ostream << "\n";
            ostream << "BEGIN METADATA\n";
            std::string metadata = baker.getMetadata();
            if(!metadata.empty())
            {
                ostream << metadata << "\n";
            }
            ostream << "END METADATA\n";
            ostream << "\n";
            
            // Write out the 1D Prelut
            ostream.setf(std::ios::fixed, std::ios::floatfield);
            ostream.precision(6);
            
            if(shaperInData.size()<2 || shaperOutData.size() != shaperInData.size())
            {
                throw Exception("Internal shaper size exception.");
            }
            
            if(!shaperInData.empty())
            {
                for(int c=0; c<3; ++c)
                {
                    ostream << shaperInData.size()/3 << "\n";
                    for(unsigned int i = 0; i<shaperInData.size()/3; ++i)
                    {
                        if(i != 0) ostream << " ";
                        ostream << shaperInData[3*i+c];
                    }
                    ostream << "\n";
                    
                    for(unsigned int i = 0; i<shaperInData.size()/3; ++i)
                    {
                        if(i != 0) ostream << " ";
                        ostream << shaperOutData[3*i+c];
                    }
                    ostream << "\n";
                }
            }
            ostream << "\n";
            
            // Write out the 3D Cube
            if(cubeSize < 2)
            {
                throw Exception("Internal cube size exception.");
            }
            ostream << cubeSize << " " << cubeSize << " " << cubeSize << "\n";
            for(int i=0; i<cubeSize*cubeSize*cubeSize; ++i)
            {
                ostream << cubeData[3*i+0] << " " << cubeData[3*i+1] << " " << cubeData[3*i+2] << "\n";
            }
            ostream << "\n";
        }
        
        void
        LocalFileFormat::BuildFileOps(OpRcPtrVec & ops,
                                    const Config& /*config*/,
                                    const ConstContextRcPtr & /*context*/,
                                    CachedFileRcPtr untypedCachedFile,
                                    const FileTransform& fileTransform,
                                    TransformDirection dir) const
        {

            CachedFileCSPRcPtr cachedFile = DynamicPtrCast<CachedFileCSP>(untypedCachedFile);

            // This should never happen.
            if(!cachedFile)
            {
                std::ostringstream os;
                os << "Cannot build CSP Op. Invalid cache type.";
                throw Exception(os.str().c_str());
            }

            TransformDirection newDir = CombineTransformDirections(dir,
                fileTransform.getDirection());

            if(newDir == TRANSFORM_DIR_FORWARD)
            {
                if(cachedFile->hasprelut)
                {
                    CreateLut1DOp(ops, cachedFile->prelut,
                                  PRELUT_INTERPOLATION, newDir);
                }
                if(cachedFile->csptype == "1D")
                    CreateLut1DOp(ops, cachedFile->lut1D,
                                  fileTransform.getInterpolation(), newDir);
                else if(cachedFile->csptype == "3D")
                    CreateLut3DOp(ops, cachedFile->lut3D,
                                  fileTransform.getInterpolation(), newDir);
            }
            else if(newDir == TRANSFORM_DIR_INVERSE)
            {
                if(cachedFile->csptype == "1D")
                    CreateLut1DOp(ops, cachedFile->lut1D,
                                  fileTransform.getInterpolation(), newDir);
                else if(cachedFile->csptype == "3D")
                    CreateLut3DOp(ops, cachedFile->lut3D,
                                  fileTransform.getInterpolation(), newDir);
                if(cachedFile->hasprelut)
                {
                    CreateLut1DOp(ops, cachedFile->prelut,
                                  PRELUT_INTERPOLATION, newDir);
                }
            }

            return;

        }
    }
    
    FileFormat * CreateFileFormatCSP()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

void compareFloats(const std::string& floats1, const std::string& floats2)
{
    // number comparison
    std::vector<std::string> strings1;
    OCIO::pystring::split(OCIO::pystring::strip(floats1), strings1);
    std::vector<float> numbers1;
    OCIO::StringVecToFloatVec(numbers1, strings1);

    std::vector<std::string> strings2;
    OCIO::pystring::split(OCIO::pystring::strip(floats2), strings2);
    std::vector<float> numbers2;
    OCIO::StringVecToFloatVec(numbers2, strings2);

    OIIO_CHECK_EQUAL(numbers1.size(), numbers2.size());
    for(unsigned int j=0; j<numbers1.size(); ++j)
    {
        OIIO_CHECK_CLOSE(numbers1[j], numbers2[j], 1e-5f);
    }
}

OIIO_ADD_TEST(FileFormatCSP, simple1D)
{
    std::ostringstream strebuf;
    strebuf << "CSPLUTV100"              << "\n";
    strebuf << "1D"                      << "\n";
    strebuf << ""                        << "\n";
    strebuf << "BEGIN METADATA"          << "\n";
    strebuf << "foobar"                  << "\n";
    strebuf << "END METADATA"            << "\n";
    strebuf << ""                        << "\n";
    strebuf << "2"                       << "\n";
    strebuf << "0.0 1.0"                 << "\n";
    strebuf << "0.0 2.0"                 << "\n";
    strebuf << "6"                       << "\n";
    strebuf << "0.0 0.2 0.4 0.6 0.8 1.0" << "\n";
    strebuf << "0.0 0.4 0.8 1.2 1.6 2.0" << "\n";
    strebuf << "3"                       << "\n";
    strebuf << "0.0 0.1 1.0"             << "\n";
    strebuf << "0.0 0.2 2.0"             << "\n";
    strebuf << ""                        << "\n";
    strebuf << "6"                       << "\n";
    strebuf << "0.0 0.0 0.0"             << "\n";
    strebuf << "0.2 0.3 0.1"             << "\n";
    strebuf << "0.4 0.5 0.2"             << "\n";
    strebuf << "0.5 0.6 0.3"             << "\n";
    strebuf << "0.6 0.8 0.4"             << "\n";
    strebuf << "1.0 0.9 0.5"             << "\n";
    
    float red[6]   = { 0.0f, 0.2f, 0.4f, 0.5f, 0.6f, 1.0f };
    float green[6] = { 0.0f, 0.3f, 0.5f, 0.6f, 0.8f, 0.9f };
    float blue[6]  = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f };
    
    std::istringstream simple1D;
    simple1D.str(strebuf.str());
    
    // Read file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Read(simple1D, emptyString);
    OCIO::CachedFileCSPRcPtr csplut = OCIO::DynamicPtrCast<OCIO::CachedFileCSP>(cachedFile);

    // check metadata
    OIIO_CHECK_EQUAL(csplut->metadata, std::string("foobar\n"));

    // check prelut data
    OIIO_CHECK_ASSERT(csplut->hasprelut);

    // check prelut data (note: the spline is resampled into a 1D LUT)
    for(int c=0; c<3; ++c)
    {
        for (unsigned int i = 0; i < csplut->prelut->luts[c].size(); i += 128)
        {
            float input = float(i) / float(csplut->prelut->luts[c].size()-1);
            float output = csplut->prelut->luts[c][i];
            OIIO_CHECK_CLOSE(input*2.0f, output, 1e-4);
        }
    }

    // check 1D data
    // red
    unsigned int i;
    for(i = 0; i < csplut->lut1D->luts[0].size(); ++i)
        OIIO_CHECK_EQUAL(red[i], csplut->lut1D->luts[0][i]);
    // green
    for(i = 0; i < csplut->lut1D->luts[1].size(); ++i)
        OIIO_CHECK_EQUAL(green[i], csplut->lut1D->luts[1][i]);
    // blue
    for(i = 0; i < csplut->lut1D->luts[2].size(); ++i)
        OIIO_CHECK_EQUAL(blue[i], csplut->lut1D->luts[2][i]);
    
    // check 3D data
    OIIO_CHECK_EQUAL(csplut->lut3D->lut.size(), 0);
}

OIIO_ADD_TEST(FileFormatCSP, simple3D)
{
    std::ostringstream strebuf;
    strebuf << "CSPLUTV100"                                  << "\n";
    strebuf << "3D"                                          << "\n";
    strebuf << ""                                            << "\n";
    strebuf << "BEGIN METADATA"                              << "\n";
    strebuf << "foobar"                                      << "\n";
    strebuf << "END METADATA"                                << "\n";
    strebuf << ""                                            << "\n";
    strebuf << "11"                                          << "\n";
    strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
    strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
    strebuf << "6"                                           << "\n";
    strebuf << "0.0 0.2       0.4 0.6 0.8 1.0"               << "\n";
    strebuf << "0.0 0.2000000 0.4 0.6 0.8 1.0"               << "\n";
    strebuf << "5"                                           << "\n";
    strebuf << "0.0 0.25       0.5 0.6 0.7"                  << "\n";
    strebuf << "0.0 0.25000001 0.5 0.6 0.7"                  << "\n";
    strebuf << ""                                            << "\n";
    strebuf << "1 2 3"                                       << "\n";
    strebuf << "0.0 0.0 0.0"                                 << "\n";
    strebuf << "1.0 0.0 0.0"                                 << "\n";
    strebuf << "0.0 0.5 0.0"                                 << "\n";
    strebuf << "1.0 0.5 0.0"                                 << "\n";
    strebuf << "0.0 1.0 0.0"                                 << "\n";
    strebuf << "1.0 1.0 0.0"                                 << "\n";
    
    float cube[1 * 2 * 3 * 3] = { 0.0, 0.0, 0.0,
                                  1.0, 0.0, 0.0,
                                  0.0, 0.5, 0.0,
                                  1.0, 0.5, 0.0,
                                  0.0, 1.0, 0.0,
                                  1.0, 1.0, 0.0 };
    
    std::istringstream simple3D;
    simple3D.str(strebuf.str());
    
    // Load file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Read(simple3D, emptyString);
    OCIO::CachedFileCSPRcPtr csplut = OCIO::DynamicPtrCast<OCIO::CachedFileCSP>(cachedFile);
    
    // check metadata
    OIIO_CHECK_EQUAL(csplut->metadata, std::string("foobar\n"));

    // check prelut data
    OIIO_CHECK_ASSERT(!csplut->hasprelut); // as in & out preLut values are the same
                                           // there is nothing to do.
    
    // check cube data
    for(unsigned int i = 0; i < csplut->lut3D->lut.size(); ++i) {
        OIIO_CHECK_EQUAL(cube[i], csplut->lut3D->lut[i]);
    }

    // check 1D data
    OIIO_CHECK_EQUAL(csplut->lut1D->luts[0].size(), 0);
    OIIO_CHECK_EQUAL(csplut->lut1D->luts[1].size(), 0);
    OIIO_CHECK_EQUAL(csplut->lut1D->luts[2].size(), 0);
}

OIIO_ADD_TEST(FileFormatCSP, complete3D)
{
    // check baker output
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("shaper");
        cs->setFamily("shaper");
        OCIO::ExponentTransformRcPtr transform1 = OCIO::ExponentTransform::Create();
        float test[4] = {2.6f, 2.6f, 2.6f, 1.0f};
        transform1->setValue(test);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_TO_REFERENCE);
        config->addColorSpace(cs);
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        float rgb[3] = {0.1f, 0.1f, 0.1f};
        transform1->setOffset(rgb);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }

    std::ostringstream bout;
    bout << "CSPLUTV100"                 << "\n";
    bout << "3D"                         << "\n";
    bout << ""                           << "\n";
    bout << "BEGIN METADATA"             << "\n";
    bout << "date: 2011:02:21 15:22:55"  << "\n";
    bout << "END METADATA"               << "\n";
    bout << ""                           << "\n";
    bout << "10"                         << "\n";
    bout << "0.000000 0.003303 0.020028 0.057476 0.121430 0.216916 0.348468 0.520265 0.736213 1.000000" << "\n";
    bout << "0.000000 0.111111 0.222222 0.333333 0.444444 0.555556 0.666667 0.777778 0.888889 1.000000" << "\n";
    bout << "10"                         << "\n";
    bout << "0.000000 0.003303 0.020028 0.057476 0.121430 0.216916 0.348468 0.520265 0.736213 1.000000" << "\n";
    bout << "0.000000 0.111111 0.222222 0.333333 0.444444 0.555556 0.666667 0.777778 0.888889 1.000000" << "\n";
    bout << "10"                         << "\n";
    bout << "0.000000 0.003303 0.020028 0.057476 0.121430 0.216916 0.348468 0.520265 0.736213 1.000000" << "\n";
    bout << "0.000000 0.111111 0.222222 0.333333 0.444444 0.555556 0.666667 0.777778 0.888889 1.000000" << "\n";
    bout << ""                           << "\n";
    bout << "2 2 2"                      << "\n";
    bout << "0.100000 0.100000 0.100000" << "\n";
    bout << "1.100000 0.100000 0.100000" << "\n";
    bout << "0.100000 1.100000 0.100000" << "\n";
    bout << "1.100000 1.100000 0.100000" << "\n";
    bout << "0.100000 0.100000 1.100000" << "\n";
    bout << "1.100000 0.100000 1.100000" << "\n";
    bout << "0.100000 1.100000 1.100000" << "\n";
    bout << "1.100000 1.100000 1.100000" << "\n";
    bout << ""                           << "\n";
    
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setMetadata("date: 2011:02:21 15:22:55");
    baker->setFormat("cinespace");
    baker->setInputSpace("lnf");
    baker->setShaperSpace("shaper");
    baker->setTargetSpace("target");
    baker->setShaperSize(10);
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);
    
    //
    std::vector<std::string> osvec;
    OCIO::pystring::splitlines(output.str(), osvec);
    std::vector<std::string> resvec;
    OCIO::pystring::splitlines(bout.str(), resvec);
    OIIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < resvec.size(); ++i)
    {
        if(i>6)
        {
            // number comparison
            compareFloats(osvec[i], resvec[i]);
        }
        else
        {
            // text comparison
            OIIO_CHECK_EQUAL(osvec[i], resvec[i]);
        }
    }
}

OIIO_ADD_TEST(FileFormatCSP, shaper_hdr)
{
    // check baker output
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf_tweak");
        cs->setFamily("lnf_tweak");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        float rgb[3] = {2.0f, -2.0f, 0.9f};
        transform1->setOffset(rgb);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        float rgb[3] = {0.1f, 0.1f, 0.1f};
        transform1->setOffset(rgb);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }

    std::ostringstream bout;
    bout << "CSPLUTV100"                 << "\n";
    bout << "3D"                         << "\n";
    bout << ""                           << "\n";
    bout << "BEGIN METADATA"             << "\n";
    bout << "date: 2011:02:21 15:22:55"  << "\n";
    bout << "END METADATA"               << "\n";
    bout << ""                           << "\n";
    bout << "10"                         << "\n";
    bout << "2.000000 2.111111 2.222222 2.333333 2.444444 2.555556 2.666667 2.777778 2.888889 3.000000" << "\n";
    bout << "0.000000 0.111111 0.222222 0.333333 0.444444 0.555556 0.666667 0.777778 0.888889 1.000000" << "\n";
    bout << "10"                         << "\n";
    bout << "-2.000000 -1.888889 -1.777778 -1.666667 -1.555556 -1.444444 -1.333333 -1.222222 -1.111111 -1.000000" << "\n";
    bout << "0.000000 0.111111 0.222222 0.333333 0.444444 0.555556 0.666667 0.777778 0.888889 1.000000" << "\n";
    bout << "10"                         << "\n";
    bout << "0.900000 1.011111 1.122222 1.233333 1.344444 1.455556 1.566667 1.677778 1.788889 1.900000" << "\n";
    bout << "0.000000 0.111111 0.222222 0.333333 0.444444 0.555556 0.666667 0.777778 0.888889 1.000000" << "\n";
    bout << ""                           << "\n";
    bout << "2 2 2"                      << "\n";
    bout << "0.100000 0.100000 0.100000" << "\n";
    bout << "1.100000 0.100000 0.100000" << "\n";
    bout << "0.100000 1.100000 0.100000" << "\n";
    bout << "1.100000 1.100000 0.100000" << "\n";
    bout << "0.100000 0.100000 1.100000" << "\n";
    bout << "1.100000 0.100000 1.100000" << "\n";
    bout << "0.100000 1.100000 1.100000" << "\n";
    bout << "1.100000 1.100000 1.100000" << "\n";
    bout << ""                           << "\n";

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setMetadata("date: 2011:02:21 15:22:55");
    baker->setFormat("cinespace");
    baker->setInputSpace("lnf_tweak");
    baker->setShaperSpace("lnf");
    baker->setTargetSpace("target");
    baker->setShaperSize(10);
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);
    
    //
    std::vector<std::string> osvec;
    OCIO::pystring::splitlines(output.str(), osvec);
    std::vector<std::string> resvec;
    OCIO::pystring::splitlines(bout.str(), resvec);
    OIIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < resvec.size(); ++i)
    {
        if(i>6)
        {
            // number comparison
            compareFloats(osvec[i], resvec[i]);
        }
        else
        {
            // text comparison
            OIIO_CHECK_EQUAL(osvec[i], resvec[i]);
        }
    }
}

OIIO_ADD_TEST(FileFormatCSP, no_shaper)
{
    // check baker output
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        float rgb[3] = {0.1f, 0.1f, 0.1f};
        transform1->setOffset(rgb);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }

    std::ostringstream bout;
    bout << "CSPLUTV100"                 << "\n";
    bout << "3D"                         << "\n";
    bout << ""                           << "\n";
    bout << "BEGIN METADATA"             << "\n";
    bout << "date: 2011:02:21 15:22:55"  << "\n";
    bout << "END METADATA"               << "\n";
    bout << ""                           << "\n";
    bout << "2"                          << "\n";
    bout << "0.000000 1.000000"          << "\n";
    bout << "0.000000 1.000000"          << "\n";
    bout << "2"                          << "\n";
    bout << "0.000000 1.000000"          << "\n";
    bout << "0.000000 1.000000"          << "\n";
    bout << "2"                          << "\n";
    bout << "0.000000 1.000000"          << "\n";
    bout << "0.000000 1.000000"          << "\n";
    bout << ""                           << "\n";
    bout << "2 2 2"                      << "\n";
    bout << "0.100000 0.100000 0.100000" << "\n";
    bout << "1.100000 0.100000 0.100000" << "\n";
    bout << "0.100000 1.100000 0.100000" << "\n";
    bout << "1.100000 1.100000 0.100000" << "\n";
    bout << "0.100000 0.100000 1.100000" << "\n";
    bout << "1.100000 0.100000 1.100000" << "\n";
    bout << "0.100000 1.100000 1.100000" << "\n";
    bout << "1.100000 1.100000 1.100000" << "\n";
    bout << ""                           << "\n";
    
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setMetadata("date: 2011:02:21 15:22:55");
    baker->setFormat("cinespace");
    baker->setInputSpace("lnf");
    baker->setTargetSpace("target");
    baker->setShaperSize(10);
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);

    //
    std::vector<std::string> osvec;
    OCIO::pystring::splitlines(output.str(), osvec);
    std::vector<std::string> resvec;
    OCIO::pystring::splitlines(bout.str(), resvec);
    OIIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < resvec.size(); ++i)
    {
        OIIO_CHECK_EQUAL(osvec[i], resvec[i]);
    }
}

OIIO_ADD_TEST(FileFormatCSP, lessStrictParse)
{
    std::ostringstream strebuf;
    strebuf << " CspluTV100 malformed"                       << "\n";
    strebuf << "3D"                                          << "\n";
    strebuf << ""                                            << "\n";
    strebuf << " BegIN MEtadATA malformed malformed malfo"   << "\n";
    strebuf << "foobar"                                      << "\n";
    strebuf << "   end metadata malformed malformed m a l"   << "\n";
    strebuf << ""                                            << "\n";
    strebuf << "11"                                          << "\n";
    strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
    strebuf << "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" << "\n";
    strebuf << "6"                                           << "\n";
    strebuf << "0.0 0.2       0.4 0.6 0.8 1.0"               << "\n";
    strebuf << "0.0 0.2000000 0.4 0.6 0.8 1.0"               << "\n";
    strebuf << "5"                                           << "\n";
    strebuf << "0.0 0.25       0.5 0.6 0.7"                  << "\n";
    strebuf << "0.0 0.25000001 0.5 0.6 0.7"                  << "\n";
    strebuf << ""                                            << "\n";
    strebuf << "1 2 3"                                       << "\n";
    strebuf << "0.0 0.0 0.0"                                 << "\n";
    strebuf << "1.0 0.0 0.0"                                 << "\n";
    strebuf << "0.0 0.5 0.0"                                 << "\n";
    strebuf << "1.0 0.5 0.0"                                 << "\n";
    strebuf << "0.0 1.0 0.0"                                 << "\n";
    strebuf << "1.0 1.0 0.0"                                 << "\n";
    
    std::istringstream simple3D;
    simple3D.str(strebuf.str());
    
    // Load file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile;
    OIIO_CHECK_NO_THROW(cachedFile = tester.Read(simple3D, emptyString));
    OCIO::CachedFileCSPRcPtr csplut = OCIO::DynamicPtrCast<OCIO::CachedFileCSP>(cachedFile);   
    
    // check metadata
    OIIO_CHECK_EQUAL(csplut->metadata, std::string("foobar\n"));

    // check prelut data
    OIIO_CHECK_ASSERT(!csplut->hasprelut); // as in & out from the preLut are the same,
                                           //  there is nothing to do.
}

// TODO: More strenuous tests of prelut resampling (non-noop preluts)

#endif // OCIO_UNIT_TEST
