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
        class CachedFileCSP : public CachedFile
        {
        public:
            CachedFileCSP ()
            {
                csptype = "unknown";
                metadata = "none";
                //prelut = OCIO_SHARED_PTR<Lut1D>(new Lut1D());
                lut1D = OCIO_SHARED_PTR<Lut1D>(new Lut1D());
                lut3D = OCIO_SHARED_PTR<Lut3D>(new Lut3D());
            };
            ~CachedFileCSP() {};
            
            std::string csptype;
            std::string metadata;
            //OCIO_SHARED_PTR<Lut1D> prelut;
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
            //Lut1DRcPtr prelut_ptr(new Lut1D()); // TODO: add prelut support
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
            // TODO: preluts are ignored atm till new spline op is built
            for (int c = 0; c < 3; ++c)
            {

                // how many points do we have for this channel
                nextline (istream, line);
                int cpoints = atoi (line.c_str());

                // read in / out channel points
                float point;
                std::vector<float> pts[2];
                pts[0].reserve (cpoints);
                pts[1].reserve (cpoints);
                for (int i = 0; i < 2; ++i)
                {
                    for (int j = 0; j < cpoints; ++j)
                    {
                        istream >> point;
                        pts[i].push_back(point);
                    }
                }

                // TODO: put the pts vector somewhere useful
                // DEBUG
                //for ( int i = 0; i < 2; ++i )
                //    std::cout << "points: " << pts[i] << "\n";

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
                lut3d_ptr->lut.resize (lut3d_ptr->size[0]
                                     * lut3d_ptr->size[1]
                                     * lut3d_ptr->size[2] * 3);

                // load the cube
                int entries_remaining = lut3d_ptr->size[0] * lut3d_ptr->size[1] * lut3d_ptr->size[2];
                for (int r = 0; r < lut3d_ptr->size[0]; ++r) {
                    for (int g = 0; g < lut3d_ptr->size[1]; ++g) {
                        for (int b = 0; b < lut3d_ptr->size[2]; ++b) {

                            // store each row
                            int i = GetAutodeskLut3DArrayOffset (r, g, b,
                                                           lut3d_ptr->size[0],
                                                           lut3d_ptr->size[1],
                                                           lut3d_ptr->size[2]);

                            if(i < 0 || i >= (int) lut3d_ptr->lut.size ()) {
                                std::ostringstream os;
                                os << "Cannot load .csp lut, data is invalid. ";
                                os << "A lut entry is specified (";
                                os << r << " " << g << " " << b;
                                os << " that falls outside of the cube.";
                                throw Exception(os.str ().c_str ());
                            }

                            nextline (istream, line);
                            if(sscanf (line.c_str(), "%f %f %f",
                               &lut3d_ptr->lut[i],
                               &lut3d_ptr->lut[i+1],
                               &lut3d_ptr->lut[i+2]) != 3 ) {
                                throw Exception("malformed 3D csp lut, couldn't read cube row");
                            }

                            // reverse count
                            entries_remaining--;
                        }
                    }
                }

                // Have we fully populated the table?
                if (entries_remaining != 0) 
                    throw Exception("malformed 3D csp lut, no cube points don't match cube size");

            }

            //
            CachedFileCSPRcPtr cachedFile = CachedFileCSPRcPtr (new CachedFileCSP ());
            cachedFile->csptype = csptype;
            cachedFile->metadata = metadata;
            if(csptype == "1D") {
                lut1d_ptr->generateCacheID ();
                cachedFile->lut1D = lut1d_ptr;
            } else if (csptype == "3D") {
                lut3d_ptr->generateCacheID ();
                cachedFile->lut3D = lut3d_ptr;
            }
            return cachedFile;
        }

        void
        FileFormatCSP::BuildFileOps(OpRcPtrVec & ops,
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

            if(newDir == TRANSFORM_DIR_FORWARD) {
                // TODO: add prelut support
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
                // TODO: add prelut support
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
    
    // TODO: add the prelut data here
    float red[6]   = { 0.0f, 0.2f, 0.4f, 0.5f, 0.6f, 1.0f };
    float green[6] = { 0.0f, 0.3f, 0.5f, 0.6f, 0.8f, 0.9f };
    float blue[6]  = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f };
    
    std::istringstream simple1D;
    simple1D.str(strebuf.str());
    
    // Load file
    OCIO::FileFormatCSP tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Load(simple1D);
    OCIO::CachedFileCSPRcPtr csplut = OCIO::DynamicPtrCast<OCIO::CachedFileCSP>(cachedFile);
    
    // check prelut data
    // NOT DONE YET
    
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
    
    // TODO: add the prelut data here
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
    // NOT DONE YET
    
    // check cube data
    unsigned int i;
    for(i = 0; i < csplut->lut3D->lut.size(); ++i) {
        BOOST_CHECK_EQUAL (cube[i], csplut->lut3D->lut[i]);
    }
    
}

BOOST_AUTO_TEST_SUITE_END()

#endif // OCIO_UNIT_TEST
