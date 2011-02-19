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

#include <cstdio>
#include <iostream>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "FileTransform.h"
#include "Lut1DOp.h"
#include "Lut3DOp.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        // Interpolators.h/.c from RSR's cinespacelutlib
        #include <stdlib.h>
        #include <string.h>

        #include <assert.h>
        #include <math.h>

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
            CachedFileCSP ()
            {
                csptype = "unknown";
                metadata = "none";
                prelut = OCIO_SHARED_PTR<Lut1D>(new Lut1D());
                lut1D = OCIO_SHARED_PTR<Lut1D>(new Lut1D());
                lut3D = OCIO_SHARED_PTR<Lut3D>(new Lut3D());
            };
            ~CachedFileCSP() {};
            
            std::string csptype;
            std::string metadata;
            OCIO_SHARED_PTR<Lut1D> prelut;
            OCIO_SHARED_PTR<Lut1D> lut1D;
            OCIO_SHARED_PTR<Lut3D> lut3D;
        };
        typedef OCIO_SHARED_PTR<CachedFileCSP> CachedFileCSPRcPtr;
        
        
        
        class FileFormatCSP : public FileFormat
        {
        public:
            
            ~FileFormatCSP() {};
            
            virtual std::string GetExtension () const;
            
            virtual CachedFileRcPtr Load (std::istream & istream) const;
            
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

        // read the next non empty line
        static void
        nextline (std::istream &istream, std::string &line)
        {
            while ( istream )
            {
                std::getline(istream, line);
                std::string::size_type firstPos = line.find_first_not_of(" \t\r");
                if ( firstPos != std::string::npos )
                {
                    if ( line[line.size()-1] == '\r' )
                        line.erase(line.size()-1);
                    break;
                }
            }
            return;
        }

        std::string
        FileFormatCSP::GetExtension() const { return "csp"; }

        CachedFileRcPtr
        FileFormatCSP::Load(std::istream & istream) const
        {

            // this shouldn't happen
            if (!istream)
            {
                throw Exception ("file stream empty when trying to read csp lut");
            }

            // 
            Lut1DRcPtr prelut_ptr(new Lut1D());
            Lut1DRcPtr lut1d_ptr(new Lut1D());
            Lut3DRcPtr lut3d_ptr(new Lut3D());

            // try and read the lut header
            std::string line;
            nextline (istream, line);
            if (line != "CSPLUTV100")
            {
                throw Exception("lut doesn't seem to be a csp file");
            }

            // next line tells us if we are reading a 1D or 3D lut
            nextline (istream, line);
            if (line != "1D" && line != "3D")
            {
                throw Exception("unsupported lut type");
            }
            std::string csptype = line;

            // read meta data block
            std::string metadata;
            std::streampos curr_pos = istream.tellg ();
            nextline (istream, line);
            if (line == "BEGIN METADATA")
            {
                while (line != "END METADATA" || !istream)
                {
                    nextline (istream, line);
                    if (line != "END METADATA")
                        metadata += line + "\n";
                }
            }
            else
            {
                istream.seekg (curr_pos);
            }

            // read the prelut block
            for (int c = 0; c < 3; ++c)
            {

                // how many points do we have for this channel
                nextline (istream, line);
                int cpoints = atoi (line.c_str());

                // Create interoplator for prelut
                rsr_Interpolator1D_Raw * cprelut_raw;
                cprelut_raw = rsr_Interpolator1D_Raw_create((unsigned int)cpoints);

                if (!cprelut_raw)
                {
                    throw Exception ("Unknown error creating Interpolator1D for prelut");
                }

                // read in / out channel points
                // TODO: should check line contains correct number of values
                float point;
                for (int i = 0; i < cpoints; ++i)
                {
                    istream >> point;
                    cprelut_raw->stims[i] = point;
                }
                for (int i = 0; i < cpoints; ++i)
                {
                    istream >> point;
                    cprelut_raw->values[i] = point;
                }

                // create interpolater, to resample to simple 1D lut
                rsr_Interpolator1D * interpolater;
                interpolater = rsr_Interpolator1D_createFromRaw(cprelut_raw);

                // resample into 1D lut
                // TODO: Fancy spline analysis to determine required number of samples
                const int numsample = 65536; // 2**16 samples

                prelut_ptr->from_min[c] = cprelut_raw->stims[0];
                prelut_ptr->from_max[c] = cprelut_raw->stims[cpoints-1];

                prelut_ptr->luts[c].clear();
                prelut_ptr->luts[c].reserve(numsample);

                for (int i = 0; i < numsample; ++i)
                {
                    float interpo = float(i) / (numsample-1);
                    float srcval = (prelut_ptr->from_min[c] * (1-interpo)) + (prelut_ptr->from_max[c] * interpo);
                    float newval = rsr_Interpolator1D_interpolate(srcval, interpolater);
                    // std::cerr << "lerp(" << prelut_ptr->from_min[c] << ", " << prelut_ptr->from_max[c] << ", " << interpo << ")" << " == " << srcval << std::endl;
                    // std::cerr << "prelut_interpolate(" << srcval << ") == " << newval << std::endl;
                    prelut_ptr->luts[c].push_back(newval);
                }

                rsr_Interpolator1D_Raw_destroy(cprelut_raw);
                rsr_Interpolator1D_destroy(interpolater);

            }

            if (csptype == "1D")
            {

                // how many 1D lut points do we have
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
                        throw Exception ("malformed 1D csp lut");
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
                    throw Exception("malformed 3D csp lut, couldn't read cube size");
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
                        os << "Malformed 3D csp lut, couldn't read cube row (";
                        os << i << "): " << line << " .";
                        throw Exception(os.str().c_str());
                    }
                }
            }
            
            CachedFileCSPRcPtr cachedFile = CachedFileCSPRcPtr (new CachedFileCSP ());
            cachedFile->csptype = csptype;
            cachedFile->metadata = metadata;

            prelut_ptr->finalize(1e-6, ERROR_RELATIVE);
            cachedFile->prelut = prelut_ptr;

            if(csptype == "1D") {
                lut1d_ptr->finalize(0.0, ERROR_RELATIVE);
                cachedFile->lut1D = lut1d_ptr;
            } else if (csptype == "3D") {
                lut3d_ptr->generateCacheID ();
                cachedFile->lut3D = lut3d_ptr;
            }
            return cachedFile;
        }

        void
        FileFormatCSP::BuildFileOps(OpRcPtrVec & ops,
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

            if(newDir == TRANSFORM_DIR_FORWARD){
                CreateLut1DOp(ops, cachedFile->prelut,
                              fileTransform.getInterpolation(), newDir);
                if(cachedFile->csptype == "1D")
                    CreateLut1DOp(ops, cachedFile->lut1D,
                                  fileTransform.getInterpolation(), newDir);
                else if(cachedFile->csptype == "3D")
                    CreateLut3DOp(ops, cachedFile->lut3D,
                                  fileTransform.getInterpolation(), newDir);
            } else if(newDir == TRANSFORM_DIR_INVERSE) {
                if(cachedFile->csptype == "1D")
                    CreateLut1DOp(ops, cachedFile->lut1D,
                                  fileTransform.getInterpolation(), newDir);
                else if(cachedFile->csptype == "3D")
                    CreateLut3DOp(ops, cachedFile->lut3D,
                                  fileTransform.getInterpolation(), newDir);
                CreateLut1DOp(ops, cachedFile->prelut,
                              fileTransform.getInterpolation(), newDir);

            }

            return;

        }
    }
    
    struct AutoRegister
    {
        AutoRegister() { RegisterFileFormat(new FileFormatCSP); }
    };
    static AutoRegister registerIt;
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

BOOST_AUTO_TEST_SUITE( FileFormatCSP_Unit_Tests )

BOOST_AUTO_TEST_CASE ( test_simple1D )
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
    strebuf << "0.0 1.0"                 << "\n";
    strebuf << "6"                       << "\n";
    strebuf << "0.0 0.2 0.4 0.6 0.8 1.0" << "\n";
    strebuf << "0.0 0.2 0.4 0.6 0.8 1.0" << "\n";
    strebuf << "3"                       << "\n";
    strebuf << "0.0 0.25 1.0"            << "\n";
    strebuf << "0.0 0.25 1.0"            << "\n";
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
    
    // Load file
    OCIO::FileFormatCSP tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Load(simple1D);
    OCIO::CachedFileCSPRcPtr csplut = OCIO::DynamicPtrCast<OCIO::CachedFileCSP>(cachedFile);
    
    // check prelut data (note: the spline is resampled into a 1D LUT)
    for (int i = 0; i < 65536; ++i) {
		float curvalue = float(i) / (65536-1);
		BOOST_CHECK_CLOSE (curvalue, csplut->prelut->luts[0][i], 1e-4);
		BOOST_CHECK_CLOSE (curvalue, csplut->prelut->luts[1][i], 1e-4);
		BOOST_CHECK_CLOSE (curvalue, csplut->prelut->luts[2][i], 1e-4);
	}
    
    // check 1D data
    // red
    unsigned int i;
    for(i = 0; i < csplut->lut1D->luts[0].size(); ++i)
        BOOST_CHECK_EQUAL (red[i], csplut->lut1D->luts[0][i]);
    // green
    for(i = 0; i < csplut->lut1D->luts[1].size(); ++i)
        BOOST_CHECK_EQUAL (green[i], csplut->lut1D->luts[1][i]);
    // blue
    for(i = 0; i < csplut->lut1D->luts[2].size(); ++i)
        BOOST_CHECK_EQUAL (blue[i], csplut->lut1D->luts[2][i]);
    
}


BOOST_AUTO_TEST_CASE ( test_simple3D )
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
    strebuf << "0.0 0.2 0.4 0.6 0.8 1.0"                     << "\n";
    strebuf << "0.0 0.2 0.4 0.6 0.8 1.0"                     << "\n";
    strebuf << "5"                                           << "\n";
    strebuf << "0.0 0.25 0.5 0.75 1.0"                       << "\n";
    strebuf << "0.0 0.25 0.5 0.75 1.0"                       << "\n";
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
    OCIO::FileFormatCSP tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Load(simple3D);
    OCIO::CachedFileCSPRcPtr csplut = OCIO::DynamicPtrCast<OCIO::CachedFileCSP>(cachedFile);
    
    // check prelut data (note: the spline is resampled into a 1D LUT)
    for (int i = 0; i < 65536; ++i) {
		float curvalue = float(i) / (65536-1);
		BOOST_CHECK_CLOSE (curvalue, csplut->prelut->luts[0][i], 1e-4);
		BOOST_CHECK_CLOSE (curvalue, csplut->prelut->luts[1][i], 1e-4);
		BOOST_CHECK_CLOSE (curvalue, csplut->prelut->luts[2][i], 1e-4);
	}

    // check cube data
    unsigned int i;
    for(i = 0; i < csplut->lut3D->lut.size(); ++i) {
        BOOST_CHECK_EQUAL (cube[i], csplut->lut3D->lut[i]);
    }
    
}

// TODO: More strenuous tests of prelut resampling (non-noop preluts)

BOOST_AUTO_TEST_SUITE_END()

#endif // OCIO_UNIT_TEST
