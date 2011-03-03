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
            if( std::isnan(x) ) return x;

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
                prelut = OCIO_SHARED_PTR<Lut1D>(new Lut1D());
                lut1D = OCIO_SHARED_PTR<Lut1D>(new Lut1D());
                lut3D = OCIO_SHARED_PTR<Lut3D>(new Lut3D());
            };
            ~CachedFileCSP() {};
            
            bool hasprelut;
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
            
            virtual std::string GetName() const;
            virtual std::string GetExtension () const;
            
            virtual bool Supports(const FileFormatFeature & feature) const;
            
            virtual CachedFileRcPtr Load (std::istream & istream) const;
            
            virtual bool Write(TransformData & /*data*/, std::ostream & /*ostream*/) const;
            
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
        FileFormatCSP::GetName() const { return "CineSpace"; }
        
        std::string
        FileFormatCSP::GetExtension() const { return "csp"; }
        
        bool
        FileFormatCSP::Supports(const FileFormatFeature & feature) const
        {
            if(feature == FILE_FORMAT_READ) return true;
            if(feature == FILE_FORMAT_WRITE) return true;
            return false;
        }
        
        CachedFileRcPtr
        FileFormatCSP::Load(std::istream & istream) const
        {

            // this shouldn't happen
            if (!istream)
            {
                throw Exception ("file stream empty when trying to read csp lut");
            }

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
            
            // Make 3 vectors of prelut inputs + output values
            std::vector<float> prelut_in[3];
            std::vector<float> prelut_out[3];
            bool useprelut[3] = { false, false, false };
            
            // Parse the prelut block
            for (int c = 0; c < 3; ++c)
            {
                // how many points do we have for this channel
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
                    
                    // Create interpolater, to resample to simple 1D lut
                    rsr_Interpolator1D * interpolater =
                        rsr_Interpolator1D_createFromRaw(cprelut_raw);
                    
                    // Resample into 1D lut
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
                
                prelut_ptr->finalize(1e-6f, ERROR_RELATIVE);
                cachedFile->prelut = prelut_ptr;
            }
            
            if(csptype == "1D")
            {
                lut1d_ptr->finalize(0.0, ERROR_RELATIVE);
                cachedFile->lut1D = lut1d_ptr;
            }
            else if (csptype == "3D")
            {
                lut3d_ptr->generateCacheID ();
                cachedFile->lut3D = lut3d_ptr;
            }
            
            return cachedFile;
        }
        
        bool FileFormatCSP::Write(TransformData & data, std::ostream & ostream) const
        {
            
            // setup the floating point precision
            ostream.setf(std::ios::fixed, std::ios::floatfield);
            ostream.precision(6);
            
            // Output the 1D LUT
            ostream << "CSPLUTV100\n";
            ostream << "3D\n";
            ostream << "\n";
            
            // Output metadata
            ostream << "BEGIN METADATA" << std::endl;
            // TODO: add other metadata here
            char str[20];
            time_t curTime = time( NULL );
            struct tm *tm = localtime( &curTime );
            sprintf(str, "%4d:%02d:%02d %02d:%02d:%02d",
                    tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
                    tm->tm_hour, tm->tm_min, tm->tm_sec);
            ostream << "date: " << str << std::endl;
            ostream << "END METADATA" << std::endl << std::endl;
            
            // Output the prelut for each channel
            if(data.shaper_encode.size() != 0 && data.shaper_decode.size() != 0)
            {
                for(size_t i = 0; i < 3; i++) {
                    ostream << data.shaperSize << "\n";
                    for(size_t pnt = 0; pnt < data.shaperSize; pnt++)
                    {
                        ostream << data.shaper_ident[3*pnt+i];
                        ostream << ((pnt < data.shaperSize-1) ? " " : "");
                    }
                    ostream << "\n";
                    for(size_t pnt = 0; pnt < data.shaperSize; pnt++)
                    {
                        ostream << data.shaper_encode[3*pnt+i];
                        ostream << ((pnt < data.shaperSize-1) ? " " : "");
                    }
                    ostream << "\n";
                }
            }
            else
            {
                ostream << "2\n";
                ostream << "0.0 1.0\n";
                ostream << "0.0 1.0\n";
                ostream << "2\n";
                ostream << "0.0 1.0\n";
                ostream << "0.0 1.0\n";
                ostream << "2\n";
                ostream << "0.0 1.0\n";
                ostream << "0.0 1.0\n";
            }
            
            // Cube
            ostream << "\n";
            ostream << data.lookup3DSize << " " << data.lookup3DSize << " " << data.lookup3DSize << "\n";
            for (size_t ib = 0; ib < data.lookup3DSize; ++ib) {
                for (size_t ig = 0; ig < data.lookup3DSize; ++ig) {
                    for (size_t ir = 0; ir < data.lookup3DSize; ++ir) {
                        const size_t ii = 3 * (ir + data.lookup3DSize
                                             * ig + data.lookup3DSize
                                             * data.lookup3DSize * ib);
                        const float rv = std::min(1.f, std::max(0.f, data.lookup3D[ii+0]));
                        const float gv = std::min(1.f, std::max(0.f, data.lookup3D[ii+1]));
                        const float bv = std::min(1.f, std::max(0.f, data.lookup3D[ii+2]));
                        ostream << rv << " " << gv << " " << bv << "\n";
                    }
                }
            }
            ostream << "\n";
            
            return true;
        };
        
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
    
    struct AutoRegister
    {
        AutoRegister() { FormatRegistry::GetInstance().registerFileFormat(new FileFormatCSP); }
    };
    static AutoRegister registerIt;
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

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
    strebuf << "0.0 0.1 1.0"            << "\n";
    strebuf << "0.0 0.2 2.0"            << "\n";
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
    OCIO::FileFormatCSP tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Load(simple3D);
    OCIO::CachedFileCSPRcPtr csplut = OCIO::DynamicPtrCast<OCIO::CachedFileCSP>(cachedFile);
    
    // check prelut data
    OIIO_CHECK_ASSERT(csplut->hasprelut == false);
    
    // check cube data
    for(unsigned int i = 0; i < csplut->lut3D->lut.size(); ++i) {
        OIIO_CHECK_EQUAL(cube[i], csplut->lut3D->lut[i]);
    }
    
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
    
    std::string bout =
    "CSPLUTV100\n"
    "3D\n"
    "\n"
    "BEGIN METADATA\n"
    "date: 2011:02:21 15:22:55\n"
    "END METADATA\n"
    "\n"
    "10\n"
    "0.000000 0.111111 0.222222 0.333333 0.444444 0.555556 0.666667 0.777778 0.888889 1.000000\n"
    "0.000000 0.429520 0.560744 0.655378 0.732058 0.797661 0.855604 0.907865 0.955710 1.000000\n"
    "10\n"
    "0.000000 0.111111 0.222222 0.333333 0.444444 0.555556 0.666667 0.777778 0.888889 1.000000\n"
    "0.000000 0.429520 0.560744 0.655378 0.732058 0.797661 0.855604 0.907865 0.955710 1.000000\n"
    "10\n"
    "0.000000 0.111111 0.222222 0.333333 0.444444 0.555556 0.666667 0.777778 0.888889 1.000000\n"
    "0.000000 0.429520 0.560744 0.655378 0.732058 0.797661 0.855604 0.907865 0.955710 1.000000\n"
    "\n"
    "2 2 2\n"
    "0.100000 0.100000 0.100000\n"
    "1.000000 0.100000 0.100000\n"
    "0.100000 1.000000 0.100000\n"
    "1.000000 1.000000 0.100000\n"
    "0.100000 0.100000 1.000000\n"
    "1.000000 0.100000 1.000000\n"
    "0.100000 1.000000 1.000000\n"
    "1.000000 1.000000 1.000000\n"
    "\n";
    
    //
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
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
    OCIO::pystring::splitlines(bout, resvec);
    OIIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < resvec.size(); ++i)
    {
        // skip timestamp line
        if(i != 4) OIIO_CHECK_EQUAL(osvec[i], resvec[i]);
    }
    
}

// TODO: More strenuous tests of prelut resampling (non-noop preluts)

#endif // OCIO_UNIT_TEST
