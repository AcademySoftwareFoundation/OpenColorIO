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

/*
    
    Houdini LUTs
    http://www.sidefx.com/docs/hdk11.0/hdk_io_lut.html
    
    Types:
      - 1D Lut (partial support)
      - 3D Lut
      - 3D Lut with 1D Prelut
    
    TODO:
        - cleanup some of the code duplication
        - Add support for other 1D types (R, G, B, A, RGB, RGBA, All)
          we only support type 'C' atm.
        - Relax the ordering of header fields (the order matches what
          houdini creates)
        - Add support for 'Sampling' tag
    
*/

#include <cstdio>
#include <iostream>
#include <iterator>
#include <cmath>
#include <pystring.h>

#include <OpenColorIO/OpenColorIO.h>

#include "FileTransform.h"
#include "Lut1DOp.h"
#include "Lut3DOp.h"
#include "ParseUtils.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        
        static int MAX_LINE_LENGTH = 128;
        
        class CachedFileHDL : public CachedFile
        {
        public:
            CachedFileHDL ()
            {
                hdlversion = "unknown";
                hdlformat = "unknown";
                hdltype = "unknown";
                hdlblack = 0.0;
                hdlwhite = 1.0;
                lut1D = OCIO_SHARED_PTR<Lut1D>(new Lut1D());
                lut3D = OCIO_SHARED_PTR<Lut3D>(new Lut3D());
            };
            ~CachedFileHDL() {};
            std::string hdlversion;
            std::string hdlformat;
            std::string hdltype;
            float to_min; // TODO: maybe add this to Lut1DOp?
            float to_max; // TODO: maybe add this to Lut1DOp?
            float hdlblack;
            float hdlwhite;
            OCIO_SHARED_PTR<Lut1D> lut1D;
            OCIO_SHARED_PTR<Lut3D> lut3D;
        };
        typedef OCIO_SHARED_PTR<CachedFileHDL> CachedFileHDLRcPtr;
        
        class FileFormatHDL : public FileFormat
        {
        public:
            
            ~FileFormatHDL() {};
            
            virtual std::string GetName() const;
            virtual std::string GetExtension () const;
            
            virtual bool Supports(const std::string & feature) const;
            
            virtual CachedFileRcPtr Load (std::istream & istream) const;
            
            virtual bool Write(TransformData & data, std::ostream & ostream) const;
            
            virtual void BuildFileOps(OpRcPtrVec & ops,
                                      const Config& config,
                                      const ConstContextRcPtr & context,
                                      CachedFileRcPtr untypedCachedFile,
                                      const FileTransform& fileTransform,
                                      TransformDirection dir) const;
        };
        
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
        FileFormatHDL::GetName() const
        {
            return "houdini";
        }
        
        std::string
        FileFormatHDL::GetExtension() const
        {
            return "lut";
        }
        
        bool
        FileFormatHDL::Supports(const std::string & feature) const
        {
            if(feature == "load") return true;
            if(feature == "write") return true;
            return false;
        }
        
        CachedFileRcPtr
        FileFormatHDL::Load(std::istream & istream) const
        {
            
            // this shouldn't happen
            if (!istream)
                throw Exception ("file stream empty when trying to read Houdini lut");

            //
            CachedFileHDLRcPtr cachedFile = CachedFileHDLRcPtr (new CachedFileHDL ());
            Lut1DRcPtr lut1d_ptr(new Lut1D());
            Lut3DRcPtr lut3d_ptr(new Lut3D());

            // try and read the lut header
            std::string line;
            nextline (istream, line);
            if (line.substr(0, 7) != "Version" || line.length() <= 9)
                throw Exception("lut doesn't seem to be a houdini lut file");
            cachedFile->hdlversion = line.substr(9, MAX_LINE_LENGTH);
            
            // Format
            nextline (istream, line);
            if (line.substr(0, 6) != "Format" || line.length() <= 8)
                throw Exception("malformed Houdini lut, couldn't read Format line");
            cachedFile->hdlformat = line.substr(8, MAX_LINE_LENGTH);
            
            // Type
            nextline (istream, line);
            if (line.substr(0, 4) != "Type" || line.length() <= 6)
                throw Exception("malformed Houdini lut, couldn't read Type line");
            cachedFile->hdltype = line.substr(6, MAX_LINE_LENGTH);
            if(cachedFile->hdltype != "3D" &&
               cachedFile->hdltype != "C" &&
               cachedFile->hdltype != "3D+1D")
                throw Exception("Unsupported Houdini lut type");
            
            // From
            nextline (istream, line);
            if (line.substr(0, 4) != "From" || line.length() <= 6)
                throw Exception("malformed Houdini lut, couldn't read From line");
            float from_min, from_max;
            if (sscanf (line.c_str(), "From\t\t%f %f",
                &from_min, &from_max) != 2) {
                throw Exception ("malformed Houdini lut, 'From' line incomplete");
            }
            
            // To
            nextline (istream, line);
            if (line.substr(0, 2) != "To" || line.length() <= 4)
                throw Exception("malformed Houdini lut, couldn't read To line");
            if (sscanf (line.c_str(), "To\t\t%f %f",
                &cachedFile->to_min, &cachedFile->to_max) != 2) {
                throw Exception ("malformed Houdini lut, 'To' line incomplete");
            }
            
            // Black
            nextline (istream, line);
            if (line.substr(0, 5) != "Black" || line.length() <= 5)
                throw Exception("malformed Houdini lut, couldn't read Black line");
            if (sscanf (line.c_str(), "Black\t\t%f",
                &cachedFile->hdlblack) != 1) {
                throw Exception ("malformed Houdini lut, 'Black' line incomplete");
            }
            
            // White
            nextline (istream, line);
            if (line.substr(0, 5) != "White" || line.length() <= 5)
                throw Exception("malformed Houdini lut, couldn't read White line");
            if (sscanf (line.c_str(), "White\t\t%f",
                &cachedFile->hdlwhite) != 1) {
                throw Exception ("malformed Houdini lut, 'White' line incomplete");
            }
            
            // Length
            nextline (istream, line);
            if (line.substr(0, 6) != "Length" || line.length() <= 8)
                throw Exception("malformed Houdini lut, couldn't read Length line");
            int length_prelut, length_1d, length_3d = 0;
            if (cachedFile->hdltype == "3D")
            {
                if (sscanf (line.c_str(), "Length\t\t%d",
                    &length_3d) != 1)
                    throw Exception ("malformed Houdini lut, 'Length' line incomplete");
            }
            else if (cachedFile->hdltype == "C")
            {
                if (sscanf (line.c_str(), "Length\t\t%d",
                    &length_1d) != 1)
                    throw Exception ("malformed Houdini lut, 'Length' line incomplete");
            }
            else if(cachedFile->hdltype == "3D+1D")
            {
                if (sscanf (line.c_str(), "Length\t\t%d %d",
                    &length_3d, &length_prelut) != 2)
                    throw Exception ("malformed Houdini lut, 'Length' line incomplete");
            }
            lut3d_ptr->size[0] = length_3d;
            lut3d_ptr->size[1] = length_3d;
            lut3d_ptr->size[2] = length_3d;
            
            // LUT:
            nextline (istream, line);
            if (line.substr(0, 4) != "LUT:")
                throw Exception("malformed Houdini lut, couldn't read 'LUT:' line");
            
            if(cachedFile->hdltype == "3D+1D")
            {
                // Pre {
                nextline (istream, line);
                if (line.substr(0, 5) != "Pre {")
                    throw Exception("malformed Houdini lut, couldn't read 'Pre {' line");
                
                // Pre lut data
                for(int i = 0; i < 3; ++i)
                {
                    lut1d_ptr->from_min[i] = from_min;
                    lut1d_ptr->from_max[i] = from_max;
                    lut1d_ptr->luts[i].clear();
                    lut1d_ptr->luts[i].reserve(length_prelut);
                }
                
                // Read single change into all rgb channels
                for(int i = 0; i < length_prelut; ++i)
                {
                    nextline (istream, line);
                    float p;
                    if (sscanf (line.c_str(), "%f", &p) != 1) {
                        throw Exception ("malformed Houdini lut, prelut incomplete");
                    }
                    lut1d_ptr->luts[0].push_back(p);
                    lut1d_ptr->luts[1].push_back(p);
                    lut1d_ptr->luts[2].push_back(p);
                }
                
                // } - end of Prelut
                nextline (istream, line);
                if (line.substr(0, 1) != "}")
                    throw Exception("malformed Houdini lut, couldn't read '}' end of prelut");
                
            }
            
            if(cachedFile->hdltype == "C")
            {
                // RGB
                nextline (istream, line);
                if (line.substr(0, 5) != "RGB {")
                    throw Exception("malformed Houdini lut, couldn't read 'RGB {' line");
                
                // 1D lut data
                for(int i = 0; i < 3; ++i)
                {
                    lut1d_ptr->from_min[i] = from_min;
                    lut1d_ptr->from_max[i] = from_max;
                    lut1d_ptr->luts[i].clear();
                    lut1d_ptr->luts[i].reserve(length_1d);
                }
                
                // Read single change into all rgb channels
                for(int i = 0; i < length_1d; ++i)
                {
                    nextline (istream, line);
                    float p;
                    if (sscanf (line.c_str(), "%f", &p) != 1) {
                        throw Exception ("malformed Houdini lut, prelut incomplete");
                    }
                    lut1d_ptr->luts[0].push_back(p);
                    lut1d_ptr->luts[1].push_back(p);
                    lut1d_ptr->luts[2].push_back(p);
                    
                    // check we have a } on the last line
                    if(i == (length_1d - 1))
                    {
                        if(line[line.length()-1] != '}')
                            throw Exception("malformed Houdini lut, couldn't read '}' end of 1D lut");
                    }
                }
            }
            else if (cachedFile->hdltype == "3D" ||
                     cachedFile->hdltype == "3D+1D")
            {
                // 3D
                nextline (istream, line);
                if (line.substr(0, 4) != "3D {" &&
                    line.substr(0, 2) != " {")
                    throw Exception("malformed Houdini lut, couldn't read '3D {' or ' {' line");
                
                // resize cube
                lut3d_ptr->lut.resize (lut3d_ptr->size[0]
                                     * lut3d_ptr->size[1]
                                     * lut3d_ptr->size[2] * 3);
                
                // load the cube
                int entries_remaining = lut3d_ptr->size[0] * lut3d_ptr->size[1] * lut3d_ptr->size[2];
                for (int b = 0; b < lut3d_ptr->size[0]; ++b) {
                    for (int g = 0; g < lut3d_ptr->size[1]; ++g) {
                        for (int r = 0; r < lut3d_ptr->size[2]; ++r) {
                            
                            // store each row
                            int i = GetLut3DIndex_B(r, g, b,
                                                    lut3d_ptr->size[0],
                                                    lut3d_ptr->size[1],
                                                    lut3d_ptr->size[2]);
                            
                            if(i < 0 || i >= (int) lut3d_ptr->lut.size ()) {
                                std::ostringstream os;
                                os << "Cannot load Houdini lut, data is invalid. ";
                                os << "A lut entry is specified (";
                                os << r << " " << g << " " << b;
                                os << ") that falls outside of the cube.";
                                throw Exception(os.str ().c_str ());
                            }
                            
                            nextline (istream, line);
                            if(sscanf (line.c_str(), "%f %f %f",
                               &lut3d_ptr->lut[i],
                               &lut3d_ptr->lut[i+1],
                               &lut3d_ptr->lut[i+2]) != 3 ) {
                                   throw Exception("malformed 3D Houdini lut, couldn't read cube row");
                            }
                            
                            // reverse count
                            entries_remaining--;
                        }
                    }
                }
                
                // Have we fully populated the table?
                if (entries_remaining != 0) 
                    throw Exception("malformed Houdini lut, number of cube points don't match cube size");
                nextline (istream, line);
                if (line.substr(0, 1) != "}" && line != " }")
                    throw Exception("malformed Houdini lut, couldn't read '}' line");
            }
            
            //
            if(cachedFile->hdltype == "C" ||
               cachedFile->hdltype == "3D+1D")
            {
                lut1d_ptr->finalize(0.0, ERROR_RELATIVE);
                cachedFile->lut1D = lut1d_ptr;
            }
            if(cachedFile->hdltype == "3D" ||
               cachedFile->hdltype == "3D+1D")
            {
                lut3d_ptr->generateCacheID ();
                cachedFile->lut3D = lut3d_ptr;
            }
            
            if(cachedFile->hdltype != "C" &&
               cachedFile->hdltype != "3D" &&
               cachedFile->hdltype != "3D+1D")
            {
                throw Exception("Unsupported Houdini lut type");
            }
            
            return cachedFile;
        }
        
        bool
        FileFormatHDL::Write(TransformData & data,
                             std::ostream & ostream) const
        {
            
            // setup the floating point precision
            ostream.setf(std::ios::fixed, std::ios::floatfield);
            ostream.precision(6);
            
            std::string tab = "\t";
            
            // Output the lut header
            ostream << "Version\t\t3\n";
            ostream << "Format\t\tany\n";
            if(data.shaper_encode.size() != 0)
            {
                ostream << "Type\t\t3D+1D\n";
            }
            else
            {
                ostream << "Type\t\t3D\n";
                tab = " ";
            }
            ostream << "From\t\t" << data.minlum[0] << " " << data.maxlum[0] << "\n";
            ostream << "To\t\t" << 0.f << " " << 1.f << "\n";
            ostream << "Black\t\t" << 0.f << "\n";
            ostream << "White\t\t" << 1.f << "\n";
            if(data.shaper_encode.size() != 0)
                ostream << "Length\t\t" << data.lookup3DSize << " " << data.shaperSize << "\n";
            else
                ostream << "Length\t\t" << data.lookup3DSize << "\n";
            ostream << "LUT:\n";
            
            // Output the shaper
            if(data.shaper_encode.size() != 0)
            {
                ostream << "Pre" << " {\n";
                for(size_t pnt = 0; pnt < data.shaperSize; pnt++)
                {
                    ostream << "\t" << std::min(1.f, std::max(0.f, data.shaper_encode[3*pnt+0])) << "\n";
                }
                ostream << "}\n";
            }
            
            // Output the 3D lut
            if(data.shaper_encode.size() != 0)
                ostream << "3D {\n";
            else
                ostream << " {\n";
            for (size_t ib = 0; ib < data.lookup3DSize; ++ib) {
                for (size_t ig = 0; ig < data.lookup3DSize; ++ig) {
                    for (size_t ir = 0; ir < data.lookup3DSize; ++ir) {
                        const size_t ii = 3 * (ir + data.lookup3DSize
                                            * ig + data.lookup3DSize
                                            * data.lookup3DSize * ib);
                        const float rv = std::min(1.f, std::max(0.f, data.lookup3D[ii+0]));
                        const float gv = std::min(1.f, std::max(0.f, data.lookup3D[ii+1]));
                        const float bv = std::min(1.f, std::max(0.f, data.lookup3D[ii+2]));
                        ostream << tab << rv << " " << gv << " " << bv << "\n";
                    }
                }
            }
            if(data.shaper_encode.size() != 0)
                ostream << "}\n";
            else
                ostream << " }\n";
            
            return true;
        };
        
        void
        FileFormatHDL::BuildFileOps(OpRcPtrVec & ops,
                                    const Config& /*config*/,
                                    const ConstContextRcPtr & /*context*/,
                                    CachedFileRcPtr untypedCachedFile,
                                    const FileTransform& fileTransform,
                                    TransformDirection dir) const
        {
            
            CachedFileHDLRcPtr cachedFile = DynamicPtrCast<CachedFileHDL>(untypedCachedFile);
            
            // This should never happen.
            if(!cachedFile)
            {
                std::ostringstream os;
                os << "Cannot build Houdini Op. Invalid cache type.";
                throw Exception(os.str().c_str());
            }
            
            TransformDirection newDir = CombineTransformDirections(dir,
                fileTransform.getDirection());
            
            if(newDir == TRANSFORM_DIR_FORWARD) {
                if(cachedFile->hdltype == "C")
                {
                    CreateLut1DOp(ops, cachedFile->lut1D,
                                  fileTransform.getInterpolation(), newDir);
                }
                else if(cachedFile->hdltype == "3D")
                {
                    CreateLut3DOp(ops, cachedFile->lut3D,
                                  fileTransform.getInterpolation(), newDir);
                }
                else if(cachedFile->hdltype == "3D+1D")
                {
                    CreateLut1DOp(ops, cachedFile->lut1D,
                                  fileTransform.getInterpolation(), newDir);
                    CreateLut3DOp(ops, cachedFile->lut3D,
                                  fileTransform.getInterpolation(), newDir);
                }
            } else if(newDir == TRANSFORM_DIR_INVERSE) {
                if(cachedFile->hdltype == "C")
                {
                    CreateLut1DOp(ops, cachedFile->lut1D,
                                  fileTransform.getInterpolation(), newDir);
                }
            }
            return;
        };
        
        struct AutoRegister
        {
            AutoRegister() { RegisterFileFormat(new FileFormatHDL()); }
        };
        static AutoRegister registerIt;
    }
    
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OIIO_ADD_TEST(FileFormatHDL, simple1D)
{
    
    std::ostringstream strebuf;
    strebuf << "Version		1" << "\n";
    strebuf << "Format		any" << "\n";
    strebuf << "Type		C" << "\n";
    strebuf << "From		0.1 3.2" << "\n";
    strebuf << "To		0 1" << "\n";
    strebuf << "Black		0" << "\n";
    strebuf << "White		0.99" << "\n";
    strebuf << "Length		9" << "\n";
    strebuf << "LUT:" << "\n";
    strebuf << "RGB {" << "\n";
    strebuf << "	0" << "\n";
    strebuf << "	0.000977517" << "\n";
    strebuf << "	0.00195503" << "\n";
    strebuf << "	0.00293255" << "\n";
    strebuf << "	0.00391007" << "\n";
    strebuf << "	0.00488759" << "\n";
    strebuf << "	0.0058651" << "\n";
    strebuf << "	0.999022" << "\n";
    strebuf << "	1.67 }" << "\n";
    
    //
    float from_min = 0.1f;
    float from_max = 3.2f;
    float to_min = 0.0f;
    float to_max = 1.0f;
    float black = 0.0f;
    float white = 0.99f;
    float lut1d[10] = { 0.0f, 0.000977517f, 0.00195503f, 0.00293255f,
        0.00391007f, 0.00488759f, 0.0058651f, 0.999022f, 1.67f };
    
    std::istringstream simple3D1D;
    simple3D1D.str(strebuf.str());
    
    // Load file
    OCIO::FileFormatHDL tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Load(simple3D1D);
    OCIO::CachedFileHDLRcPtr lut = OCIO::DynamicPtrCast<OCIO::CachedFileHDL>(cachedFile);
    
    //
    OIIO_CHECK_EQUAL(to_min, lut->to_min);
    OIIO_CHECK_EQUAL(to_max, lut->to_max);
    OIIO_CHECK_EQUAL(black, lut->hdlblack);
    OIIO_CHECK_EQUAL(white, lut->hdlwhite);
    
    // check 1D data (each channel has the same data)
    for(int c = 0; c < 3; ++c) {
        OIIO_CHECK_EQUAL(from_min, lut->lut1D->from_min[c]);
        OIIO_CHECK_EQUAL(from_max, lut->lut1D->from_max[c]);
        for(unsigned int i = 0; i < lut->lut1D->luts[c].size(); ++i) {
            OIIO_CHECK_EQUAL(lut1d[i], lut->lut1D->luts[c][i]);
        }
    }
    
}

OIIO_ADD_TEST(FileFormatHDL, simple3D)
{
    
    std::ostringstream strebuf;
    strebuf << "Version		2" << "\n";
    strebuf << "Format		any" << "\n";
    strebuf << "Type		3D" << "\n";
    strebuf << "From		0.2 0.9" << "\n";
    strebuf << "To		0.001 0.999" << "\n";
    strebuf << "Black		0.002" << "\n";
    strebuf << "White		0.98" << "\n";
    strebuf << "Length		2" << "\n";
    strebuf << "LUT:" << "\n";
    strebuf << " {" << "\n";
    strebuf << " 0 0 0" << "\n";
    strebuf << " 0 0 0" << "\n";
    strebuf << " 0 0.390735 2.68116e-28" << "\n";
    strebuf << " 0 0.390735 0" << "\n";
    strebuf << " 0 0 0" << "\n";
    strebuf << " 0 0 0.599397" << "\n";
    strebuf << " 0 0.601016 0" << "\n";
    strebuf << " 0 0.601016 0.917034" << "\n";
    strebuf << " }" << "\n";
    
    std::istringstream simple3D1D;
    simple3D1D.str(strebuf.str());
    
    // Load file
    OCIO::FileFormatHDL tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Load(simple3D1D);
    OCIO::CachedFileHDLRcPtr lut = OCIO::DynamicPtrCast<OCIO::CachedFileHDL>(cachedFile);
    
    //
    //float from_min = 0.2;
    //float from_max = 0.9;
    float to_min = 0.001f;
    float to_max = 0.999f;
    float black = 0.002f;
    float white = 0.98f;
    float cube[2 * 2 * 2 * 3 ] = {
        0.f, 0.f, 0.f,
        0.f, 0.f, 0.f,
        0.f, 0.390735f, 2.68116e-28f,
        0.f, 0.390735f, 0.f,
        0.f, 0.f, 0.f,
        0.f, 0.f, 0.599397f,
        0.f, 0.601016f, 0.f,
        0.f, 0.601016f, 0.917034f };
    
    //
    OIIO_CHECK_EQUAL(to_min, lut->to_min);
    OIIO_CHECK_EQUAL(to_max, lut->to_max);
    OIIO_CHECK_EQUAL(black, lut->hdlblack);
    OIIO_CHECK_EQUAL(white, lut->hdlwhite);
    
    // check cube data
    for(unsigned int i = 0; i < lut->lut3D->lut.size(); ++i) {
        OIIO_CHECK_EQUAL(cube[i], lut->lut3D->lut[i]);
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
        cs->setName("target");
        cs->setFamily("target");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        float rgb[3] = {0.1f, 0.1f, 0.1f};
        transform1->setOffset(rgb);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }
    
    std::string bout = 
    "Version\t\t3\n"
    "Format\t\tany\n"
    "Type\t\t3D\n"
    "From\t\t0.000000 1.000000\n"
    "To\t\t0.000000 1.000000\n"
    "Black\t\t0.000000\n"
    "White\t\t1.000000\n"
    "Length\t\t2\n"
    "LUT:\n"
    " {\n"
    " 0.100000 0.100000 0.100000\n"
    " 1.000000 0.100000 0.100000\n"
    " 0.100000 1.000000 0.100000\n"
    " 1.000000 1.000000 0.100000\n"
    " 0.100000 0.100000 1.000000\n"
    " 1.000000 0.100000 1.000000\n"
    " 0.100000 1.000000 1.000000\n"
    " 1.000000 1.000000 1.000000\n"
    " }\n";
    
    //
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat("houdini");
    baker->setInputSpace("lnf");
    baker->setTargetSpace("target");
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);
    
    //
    std::vector<std::string> osvec;
    pystring::splitlines(output.str(), osvec);
    std::vector<std::string> resvec;
    pystring::splitlines(bout, resvec);
    OIIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < resvec.size(); ++i)
        OIIO_CHECK_EQUAL(osvec[i], resvec[i]);
    
}

OIIO_ADD_TEST(FileFormatHDL, simple3D1D)
{
    std::ostringstream strebuf;
    strebuf << "Version		3" << "\n";
    strebuf << "Format		any" << "\n";
    strebuf << "Type		3D+1D" << "\n";
    strebuf << "From		0.005478 14.080103" << "\n";
    strebuf << "To		0 1" << "\n";
    strebuf << "Black		0" << "\n";
    strebuf << "White		1" << "\n";
    strebuf << "Length		2 10" << "\n";
    strebuf << "LUT:" << "\n";
    strebuf << "Pre {" << "\n";
    strebuf << "	0.994922" << "\n";
    strebuf << "	0.995052" << "\n";
    strebuf << "	0.995181" << "\n";
    strebuf << "	0.995310" << "\n";
    strebuf << "	0.995439" << "\n";
    strebuf << "	0.995568" << "\n";
    strebuf << "	0.995697" << "\n";
    strebuf << "	0.995826" << "\n";
    strebuf << "	0.995954" << "\n";
    strebuf << "	0.996082" << "\n";
    strebuf << "}" << "\n";
    strebuf << "3D {" << "\n";
    strebuf << "	0.093776 0.093776 0.093776" << "\n";
    strebuf << "	0.105219 0.093776 0.093776" << "\n";
    strebuf << "	0.118058 0.093776 0.093776" << "\n";
    strebuf << "	0.132463 0.093776 0.093776" << "\n";
    strebuf << "	0.148626 0.093776 0.093776" << "\n";
    strebuf << "	0.166761 0.093776 0.093776" << "\n";
    strebuf << "	0.187109 0.093776 0.093776" << "\n";
    strebuf << "	0.209939 0.093776 0.093776" << "\n";
    strebuf << "}" << "\n";
    
    //
    float from_min = 0.005478f;
    float from_max = 14.080103f;
    float to_min = 0.0f;
    float to_max = 1.0f;
    float black = 0.0f;
    float white = 1.0f;
    float prelut[10] = { 0.994922f, 0.995052f, 0.995181f, 0.995310f, 0.995439f,
        0.995568f, 0.995697f, 0.995826f, 0.995954f, 0.996082f };
    float cube[2 * 2 * 2 * 3 ] = {
        0.093776f, 0.093776f, 0.093776f,
        0.105219f, 0.093776f, 0.093776f,
        0.118058f, 0.093776f, 0.093776f,
        0.132463f, 0.093776f, 0.093776f,
        0.148626f, 0.093776f, 0.093776f,
        0.166761f, 0.093776f, 0.093776f,
        0.187109f, 0.093776f, 0.093776f,
        0.209939f, 0.093776f, 0.093776f };
    
    std::istringstream simple3D1D;
    simple3D1D.str(strebuf.str());
    
    // Load file
    OCIO::FileFormatHDL tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Load(simple3D1D);
    OCIO::CachedFileHDLRcPtr lut = OCIO::DynamicPtrCast<OCIO::CachedFileHDL>(cachedFile);
    
    //
    OIIO_CHECK_EQUAL(to_min, lut->to_min);
    OIIO_CHECK_EQUAL(to_max, lut->to_max);
    OIIO_CHECK_EQUAL(black, lut->hdlblack);
    OIIO_CHECK_EQUAL(white, lut->hdlwhite);
    
    // check prelut data (each channel has the same data)
    for(int c = 0; c < 3; ++c) {
        OIIO_CHECK_EQUAL(from_min, lut->lut1D->from_min[c]);
        OIIO_CHECK_EQUAL(from_max, lut->lut1D->from_max[c]);
        for(unsigned int i = 0; i < lut->lut1D->luts[c].size(); ++i) {
            OIIO_CHECK_EQUAL(prelut[i], lut->lut1D->luts[c][i]);
        }
    }
    
    // check cube data
    for(unsigned int i = 0; i < lut->lut3D->lut.size(); ++i) {
        OIIO_CHECK_EQUAL(cube[i], lut->lut3D->lut[i]);
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
    "Version\t\t3\n"
    "Format\t\tany\n"
    "Type\t\t3D+1D\n"
    "From\t\t0.000000 1.000000\n"
    "To\t\t0.000000 1.000000\n"
    "Black\t\t0.000000\n"
    "White\t\t1.000000\n"
    "Length\t\t2 10\n"
    "LUT:\n"
    "Pre {\n"
    "\t0.000000\n"
    "\t0.429520\n"
    "\t0.560744\n"
    "\t0.655378\n"
    "\t0.732058\n"
    "\t0.797661\n"
    "\t0.855604\n"
    "\t0.907865\n"
    "\t0.955710\n"
    "\t1.000000\n"
    "}\n"
    "3D {\n"
    "\t0.100000 0.100000 0.100000\n"
    "\t1.000000 0.100000 0.100000\n"
    "\t0.100000 1.000000 0.100000\n"
    "\t1.000000 1.000000 0.100000\n"
    "\t0.100000 0.100000 1.000000\n"
    "\t1.000000 0.100000 1.000000\n"
    "\t0.100000 1.000000 1.000000\n"
    "\t1.000000 1.000000 1.000000\n"
    "}\n";
    
    //
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat("houdini");
    baker->setInputSpace("lnf");
    baker->setShaperSpace("shaper");
    baker->setTargetSpace("target");
    baker->setShaperSize(10);
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);
    
    //
    std::vector<std::string> osvec;
    pystring::splitlines(output.str(), osvec);
    std::vector<std::string> resvec;
    pystring::splitlines(bout, resvec);
    OIIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < resvec.size(); ++i)
        OIIO_CHECK_EQUAL(osvec[i], resvec[i]);
    
}

#endif // OCIO_BUILD_TESTS
